
INCFLAGS=-I@top_srcdir@/include -I@top_srcdir@/other_libs/c-dime/include

AM_CFLAGS = $(INCFLAGS) @CFLAGS@ @QMP_CFLAGS@
VPATH = @top_srcdir@/lib
VPATH += @top_srcdir@/include
VPATH += @top_srcdir@/other_libs/c-dime/include

HDRS = qio.h dml.h lrl.h xml.h dime.h

lib_LIBRARIES = libqio.a

QIO_SRCS = qio/QIO_close_read.c qio/QIO_close_write.c qio/QIO_open_read.c \
     qio/QIO_open_write.c qio/QIO_read.c qio/QIO_utils.c qio/QIO_write.c

DML_GENERIC = dml/DML_get_sitelist.c dml/DML_stream_in.c dml/DML_stream_out.c dml/DML_utils.c
DML_PARSCALAR = ${OBJECTS} dml/DML_parscalar.c
DML_SCALAR = ${OBJECTS} dml/DML_scalar.c

LRL_SRCS = lrl/LRL_dummy.c

XML_SRCS = xml/XML_dummy.c

GENERIC_SRCS = $(QIO_SRCS) $(DML_GENERIC) $(LRL_SRCS) $(XML_SRCS)

if ARCH_SCALAR
  libqio_a_SOURCES = $(GENERIC_SRCS) $(DML_SCALAR)
  EXTRA_libqio_a_SOURCES = $(DML_PARSCALAR)
endif

if ARCH_PARSCALAR
  libqio_a_SOURCES = $(GENERIC_SRCS) $(DML_PARSCALAR)
  EXTRA_libqio_a_SOURCES = $(DML_SCALAR)
endif

# Need this header as part of libqio_a_sources
nodist_libqio_a_SOURCES = $(HDRS)
