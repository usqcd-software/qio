OBJECTS = \
  QIO_close_read.o \
  QIO_close_write.o \
  QIO_open_read.o \
  QIO_open_write.o \
  QIO_read.o \
  QIO_utils.o \
  QIO_write.o

INCLUDE = ../include
LIB = ../lib

.c.o: ; gcc -g -c $*.c -I${INCLUDE}

HEADERS = ${INCLUDE}/xml.h ${INCLUDE}/lrl.h ${INCLUDE}/dml.h ${INCLUDE}/qio.h

${OBJECTS}:  ${HEADERS}
libqio.a: ${OBJECTS} Makefile
	ar rcs $@ ${OBJECTS}
	mv $@ ${LIB}
