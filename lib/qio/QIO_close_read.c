/* QIO_close_read.c */

#include <qio.h>
#include <lrl.h>

int QIO_close_read(QIO_Reader *in){
  int status;

  status = LRL_close_read_file(in->lrl_file_in);
  free(in);
  return status;
}
