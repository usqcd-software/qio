/* QIO_close_write.c */

#include <stdlib.h>
#include <qio.h>
#include <lrl.h>

int QIO_close_write(QIO_Writer *out){
  int status;

  status = LRL_close_write_file(out->lrl_file_out);
  free(out);
  return status;
}
