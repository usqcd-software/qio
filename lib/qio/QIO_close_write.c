/* QIO_close_write.c */

#include <qio.h>
#include <lrl.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

int QIO_close_write(QIO_Writer *out){
  int status;

  status = LRL_close_write_file(out->lrl_file_out);
  free(out->layout->latsize);
  free(out->layout);
  free(out);
  return status;
}
