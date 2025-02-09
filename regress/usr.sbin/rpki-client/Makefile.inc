# $OpenBSD: Makefile.inc,v 1.15 2021/10/24 17:54:28 claudio Exp $

.PATH:		${.CURDIR}/../../../../usr.sbin/rpki-client

PROGS += test-ip
PROGS += test-cert
PROGS += test-gbr
PROGS += test-mft
PROGS += test-roa
PROGS += test-tal

.for p in ${PROGS}
REGRESS_TARGETS += run-regress-$p
.endfor

CFLAGS+=	-I${.CURDIR}/.. -I${.CURDIR}/../../../../usr.sbin/rpki-client
LDADD+=		-lcrypto -lutil
DPADD+=		${LIBCRYPTO} ${LIBUTIL}

SRCS_test-ip=	test-ip.c ip.c io.c encoding.c print.c
run-regress-test-ip: test-ip
	./test-ip

TALARGS += ta/AfriNIC.cer			tal/afrinic.tal
TALARGS += ta/apnic-rpki-root-iana-origin.cer	tal/apnic.tal
TALARGS += ta/ripe-ncc-ta.cer			tal/ripe.tal

SRCS_test-cert+=	test-cert.c cert.c cms.c x509.c ip.c as.c io.c \
			log.c tal.c validate.c encoding.c print.c
run-regress-test-cert: test-cert
	./test-cert -v ${.CURDIR}/../cer/*.cer
	./test-cert -vt ${TALARGS:S,,${.CURDIR}/../&,}

SRCS_test-mft+=	test-mft.c mft.c cms.c x509.c ip.c io.c log.c validate.c \
		encoding.c print.c dummy.c
run-regress-test-mft: test-mft
	./test-mft -v ${.CURDIR}/../mft/*.mft

SRCS_test-roa+=	test-roa.c roa.c cms.c x509.c ip.c as.c io.c log.c \
		encoding.c print.c
run-regress-test-roa: test-roa
	./test-roa -v ${.CURDIR}/../roa/*.roa

SRCS_test-gbr+=	test-gbr.c gbr.c cms.c x509.c ip.c io.c log.c \
		encoding.c print.c
run-regress-test-gbr: test-gbr
	./test-gbr -v ${.CURDIR}/../gbr/*.gbr

SRCS_test-tal+=	test-tal.c tal.c ip.c io.c log.c validate.c \
		encoding.c print.c dummy.c
run-regress-test-tal: test-tal
	./test-tal -v ${.CURDIR}/../tal/*.tal
