/* QIO_next_record.c */

/* Skip to beginning of next field (LIME message) in file */

#include <qio.h>
#include <lrl.h>
#include <stdio.h>

#if(0)
int QIO_next_record(QIO_Reader *in){
  if(in->read_state == QIO_RECORD_INFO_PRIVATE_NEXT){

    /* Skip private XML record */
    LRL_next_record(in->lrl_file_in);

    /* Skip user XML record */
    LRL_next_record(in->lrl_file_in);
  }

  /* Skip BinX record */
#ifdef DO_BINX
  LRL_next_record(in->lrl_file_in);
#endif

  /* Skip data record */
  LRL_next_record(in->lrl_file_in);

  /* Skip checksum record */
  LRL_next_record(in->lrl_file_in);

}
#endif

int QIO_next_record(QIO_Reader *in){
  if(LRL_next_message(in->lrl_file_in) != LRL_SUCCESS)
    return QIO_ERR_SKIP;
  in->read_state = QIO_RECORD_INFO_PRIVATE_NEXT;
  return QIO_SUCCESS;
}
