OBJECTS = \
  XML_dummy.o

INCLUDE = ../include
LIB = ../lib

.c.o: ; gcc -g -c $*.c -I${INCLUDE}

HEADERS = ${INCLUDE}/xml.h

${OBJECTS}:  ${HEADERS}
libxml.a: ${OBJECTS} Makefile
	ar rcs $@ ${OBJECTS}
	mv $@ ${LIB}
