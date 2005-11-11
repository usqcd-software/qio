/* QIO_close_write.c */

#include <qio_config.h>
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
  DML_free_sitelist(out->sites);
  if( out->ildgLFN != NULL ) { 
    QIO_string_destroy(out->ildgLFN); 
  }

  free(out);
  if(status != LRL_SUCCESS)return QIO_ERR_CLOSE;
  return QIO_SUCCESS;
}
