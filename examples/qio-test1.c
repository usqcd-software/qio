/* SINGLEFILE test of QIO */

/* C. DeTar */
/* October 18, 2004 */

#include <qio.h>
#include "qio-test.h"

int main(int argc, char *argv[]){
  return qio_test(QIO_SINGLEFILE, argc, argv);
}
