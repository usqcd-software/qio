#ifndef CONFIG_H
#define CONFIG_H

#include "qio_config.h"

/* THIS NEEDS TO BE AUTOCONFIG'ED */

/* Are 32 bit integers and floats stored in big endian on this
   architecture?  Our files are always big endian. */
#define QIO_BIG_ENDIAN 0

/* Does the architecture support parallel reads and writes? */
#define PARALLEL_READ 0
#define PARALLEL_WRITE 0

/* For BinX record */
#undef DO_BINX

/* These are defined in qio_config.h  now */
/* DEFINITELY WANT THIS AUTOCONF'ed !!!  (RGE) */
/* See the AC_CHECK_FUNCS(QMP_route) in qdp++/configure.ac */
/* #ifndef HAVE_QMP_ROUTE
   #define HAVE_QMP_ROUTE
   #endif
*/

#endif
