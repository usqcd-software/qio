/* QIO_close_read.c */

#include <stdlib.h>
#include <qio.h>
#include <xml_string.h>
#include <lrl.h>

int QIO_close_read(QIO_Reader *in){
  int status = 0;

  if(!in)return status;
  status = LRL_close_read_file(in->lrl_file_in);
  if(in->layout)free(in->layout->latsize);
  free(in->layout);
  if(!in->sitelist)free(in->sitelist);
  XML_string_destroy(in->xml_record);
  free(in->record_info);
  free(in);
  return status;
}
