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

#endif
