#ifndef QIO_STDINT_H
#define QIO_STDINT_H

#if HAVE_STDINT_H
#include <stdint.h>
#else
/* QCDOC needs this for now */
#include <sys/types.h>
typedef __uint64_t  uint64_t;
typedef __uint32_t  uint32_t;
typedef __uint16_t  uint16_t;
/* so fac QCDOC is the only excpetion so we will just go with the above */
#if 0
/* best guess for now */
typedef unsigned long   uint64_t;
typedef unsigned int    uint32_t;
typedef unsigned short  uint16_t;
#endif
#endif

#endif
