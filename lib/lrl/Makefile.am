OBJECTS = \
  LRL_dummy.o

INCLUDE = ../include
LIB = ../lib

.c.o: ; gcc -g -c $*.c -I${INCLUDE}

HEADERS = ${INCLUDE}/lrl.h

${OBJECTS}:  ${HEADERS}
liblrl.a: ${OBJECTS} Makefile
	ar rcs $@ ${OBJECTS}
	mv $@ ${LIB}
