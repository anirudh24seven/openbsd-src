/*	$OpenBSD: dt_prov_static.c,v 1.10 2021/09/03 16:45:45 jasper Exp $ */

/*
 * Copyright (c) 2019 Martin Pieuchot <mpi@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>
#include <sys/systm.h>
#include <sys/param.h>
#include <sys/tracepoint.h>

#include <dev/dt/dtvar.h>

int	dt_prov_static_alloc(struct dt_probe *, struct dt_softc *,
	    struct dt_pcb_list *, struct dtioc_req *);
int	dt_prov_static_hook(struct dt_provider *, ...);

struct dt_provider dt_prov_static = {
	.dtpv_name	= "tracepoint",
	.dtpv_alloc	= dt_prov_static_alloc,
	.dtpv_enter	= dt_prov_static_hook,
	.dtpv_dealloc	= NULL,
};

/*
 * Scheduler
 */
DT_STATIC_PROBE2(sched, dequeue, "pid_t", "pid_t");
DT_STATIC_PROBE2(sched, enqueue, "pid_t", "pid_t");
DT_STATIC_PROBE2(sched, off__cpu, "pid_t", "pid_t");
DT_STATIC_PROBE0(sched, on__cpu);
DT_STATIC_PROBE0(sched, remain__cpu);
DT_STATIC_PROBE0(sched, sleep);
DT_STATIC_PROBE0(sched, wakeup);

/*
 * Raw syscalls
 */
DT_STATIC_PROBE1(raw_syscalls, sys_enter, "register_t");
DT_STATIC_PROBE1(raw_syscalls, sys_exit, "register_t");

/*
 * UVM
 */
DT_STATIC_PROBE3(uvm, fault, "vaddr_t", "vm_fault_t", "vm_prot_t");
DT_STATIC_PROBE3(uvm, map_insert, "vaddr_t", "vaddr_t", "vm_prot_t");
DT_STATIC_PROBE3(uvm, map_remove, "vaddr_t", "vaddr_t", "vm_prot_t");
DT_STATIC_PROBE4(uvm, malloc, "int", "void *", "size_t", "int");
DT_STATIC_PROBE3(uvm, free, "int", "void *", "size_t");
DT_STATIC_PROBE3(uvm, pool_get, "void *", "void *", "int");
DT_STATIC_PROBE2(uvm, pool_put, "void *", "void *");

/*
 * VFS
 */
DT_STATIC_PROBE3(vfs, bufcache_rel, "long", "int", "int64_t");
DT_STATIC_PROBE3(vfs, bufcache_take, "long", "int", "int64_t");
DT_STATIC_PROBE4(vfs, cleaner, "long", "int", "long", "long");

/*
 * VMM
 */
DT_STATIC_PROBE2(vmm, guest_enter, "void *", "void *");
DT_STATIC_PROBE3(vmm, guest_exit, "void *", "void *", "uint64_t");

/*
 * List of all static probes
 */
struct dt_probe *dtps_static[] = {
	/* Scheduler */
	&_DT_STATIC_P(sched, dequeue),
	&_DT_STATIC_P(sched, enqueue),
	&_DT_STATIC_P(sched, off__cpu),
	&_DT_STATIC_P(sched, on__cpu),
	&_DT_STATIC_P(sched, remain__cpu),
	&_DT_STATIC_P(sched, sleep),
	&_DT_STATIC_P(sched, wakeup),
	/* Raw syscalls */
	&_DT_STATIC_P(raw_syscalls, sys_enter),
	&_DT_STATIC_P(raw_syscalls, sys_exit),
	/* UVM */
	&_DT_STATIC_P(uvm, fault),
	&_DT_STATIC_P(uvm, map_insert),
	&_DT_STATIC_P(uvm, map_remove),
	&_DT_STATIC_P(uvm, malloc),
	&_DT_STATIC_P(uvm, free),
	&_DT_STATIC_P(uvm, pool_get),
	&_DT_STATIC_P(uvm, pool_put),
	/* VFS */
	&_DT_STATIC_P(vfs, bufcache_rel),
	&_DT_STATIC_P(vfs, bufcache_take),
	&_DT_STATIC_P(vfs, cleaner),
	/* VMM */
	&_DT_STATIC_P(vmm, guest_enter),
	&_DT_STATIC_P(vmm, guest_exit),
};

int
dt_prov_static_init(void)
{
	int i;

	for (i = 0; i < nitems(dtps_static); i++)
		dt_dev_register_probe(dtps_static[i]);

	return i;
}

int
dt_prov_static_alloc(struct dt_probe *dtp, struct dt_softc *sc,
    struct dt_pcb_list *plist, struct dtioc_req *dtrq)
{
	struct dt_pcb *dp;

	KASSERT(dtioc_req_isvalid(dtrq));
	KASSERT(TAILQ_EMPTY(plist));

	dp = dt_pcb_alloc(dtp, sc);
	if (dp == NULL)
		return ENOMEM;

	dp->dp_filter = dtrq->dtrq_filter;
	dp->dp_evtflags = dtrq->dtrq_evtflags;
	TAILQ_INSERT_HEAD(plist, dp, dp_snext);

	return 0;
}

int
dt_prov_static_hook(struct dt_provider *dtpv, ...)
{
	struct dt_probe *dtp;
	struct dt_pcb *dp;
	uintptr_t args[5];
	va_list ap;
	int i;

	va_start(ap, dtpv);
	dtp = va_arg(ap, struct dt_probe *);
	for (i = 0; i < dtp->dtp_nargs; i++) {
		args[i] = va_arg(ap, uintptr_t);
	}
	va_end(ap);

	KASSERT(dtpv == dtp->dtp_prov);

	smr_read_enter();
	SMR_SLIST_FOREACH(dp, &dtp->dtp_pcbs, dp_pnext) {
		struct dt_evt *dtev;

		dtev = dt_pcb_ring_get(dp, 0);
		if (dtev == NULL)
			continue;

		dtev->dtev_args[0] = args[0];
		dtev->dtev_args[1] = args[1];
		dtev->dtev_args[2] = args[2];
		dtev->dtev_args[3] = args[3];
		dtev->dtev_args[4] = args[4];

		dt_pcb_ring_consume(dp, dtev);
	}
	smr_read_leave();
	return 1;
}
