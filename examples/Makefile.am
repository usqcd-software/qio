## Process this file with automake to produce Makefile.in

AM_CFLAGS  = -I@top_srcdir@/include -I@top_srcdir@/other_libs/c-lime/include @QDP_CFLAGS@ @QMP_CFLAGS@ @QLA_CFLAGS@
AM_LDFLAGS = -L../lib -L../other_libs/c-lime/lib @QDP_LDFLAGS@ @QMP_LDFLAGS@ @QLA_LDFLAGS@
LDADD      = -lqdp_f -lqdp_int -lqdp_common_qmp -lqio -llime -lqmp -lqla_f -lqla_df -lqla_d -lqla_int 

# these currently depend on QLA and QDP so we can't use them here
#check_PROGRAMS = qio-test          \
#                 qio-test2         \
#                 qio-test3         \
#                 qio-test4         \
#                 qio-test5         \
#                 qio-test6         \
#                 qio-test7

EXTRA_DIST = qio-test.c qio-test2.c qio-test3.c qio-test4.c qio-test5.c qio-test6.c qio-test7.c Makefile.am

