/* QIO_next_record.c */

/* Skip to beginning of next record */
/* Record must be closed aftwards with QIO_close_read */

#include <qio.h>
#include <lrl.h>
#include <stdio.h>

int QIO_next_record(QIO_Reader *in){
  if(in->read_state == QIO_RECORD_XML_NEXT){

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
