
INCFLAGS=-I@top_srcdir@/include 

AM_CFLAGS = $(INCFLAGS) @CFLAGS@
VPATH = @top_srcdir@/lib
VPATH += @top_srcdir@/include
VPATH += @top_srcdir@/lib/qio
VPATH += @top_srcdir@/lib/dml
VPATH += @top_srcdir@/lib/lrl
VPATH += @top_srcdir@/lib/xml
VPATH += @top_srcdir@/lib/lrl/c-dime/include

HDRS = qio.h dml.h lrl.h xml.h dime.h

lib_LIBRARIES = libqio.a

QIO_SRCS = QIO_close_read.c QIO_close_write.c QIO_open_read.c \
     QIO_open_write.c QIO_read.c QIO_utils.c QIO_write.c

DML_GENERIC = DML_get_sitelist.c DML_stream_in.c DML_stream_out.c DML_utils.c
DML_PARSCALAR = ${OBJECTS} DML_parscalar.c
DML_SCALAR = ${OBJECTS} DML_scalar.c

LRL_SRCS = LRL_dummy.c

XML_SRCS = XML_dummy.c

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
