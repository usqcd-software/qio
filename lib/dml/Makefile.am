
INCFLAGS=-I@top_srcdir@/src/dml -I@top_srcdir@/dml 

AM_CFLAGS = -I@top_srcdir@/src/

HDRS = 


OBJECTS = \
  DML_get_sitelist.o \
  DML_stream_in.o \
  DML_stream_out.o \
  DML_utils.o

OBJECTS_QMP = ${OBJECTS} DML_qmp.o
OBJECTS_VANILLA = ${OBJECTS} DML_vanilla.o

INCLUDE = ../include
LIB = ../lib

QMPINCLUDE = ../../QMP/src

.c.o: ; gcc -g -c $*.c -I${INCLUDE} -I${QMPINCLUDE}

HEADERS = \
   ${INCLUDE}/dml.h \
   ${INCLUDE}/lrl.h \
   ${QMPINCLUDE}/qmp.h

${OBJECTS} DML_qmp.o DML_vanilla.o:  ${HEADERS}

.PHONY: libdml.a
libdml.a: libdml_qmp.a libdml_vanilla.a

libdml_qmp.a: ${OBJECTS_QMP} Makefile
	ar rcs $@ ${OBJECTS_QMP}
	mv $@ ${LIB}

libdml_vanilla.a: ${OBJECTS_VANILLA} Makefile
	ar rcs $@ ${OBJECTS_VANILLA}
	mv $@ ${LIB}

