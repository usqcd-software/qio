#include <qio_config.h>
#include <lrl.h>
#include <stdio.h>
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#include <stdlib.h>

/** 
 * Open a file for reading 
 *
 * \param filename   file for writing  ( Read )
 *
 * \return null if failure
 */
LRL_FileReader *LRL_open_read_file(const char *filename)
{
  LRL_FileReader *fr;
  FILE *fpt;

  /* First check for a readable file */
  fpt = fopen(filename,"r");
  if(fpt == NULL)return NULL;

  /* Set up LRL_FileReader structure */
  fr = (LRL_FileReader *)malloc(sizeof(LRL_FileReader));
  if (fr == NULL)
    return NULL;

  fr->file = fpt;
  fr->dr = limeCreateReader(fr->file);
  if (fr->dr == (LimeReader *)NULL)
    return NULL;

  return fr;
}

/** 
 * Open a file for writing 
 *
 * \param filename   file for writing  ( Read )
 *
 * \return null if failure
 */
LRL_FileWriter *LRL_open_write_file(const char *filename)
{
  LRL_FileWriter *fw;

  fw = (LRL_FileWriter *)malloc(sizeof(LRL_FileWriter));
  if (fw == NULL)
    return NULL;

  fw->file = fopen(filename,"w");
  if (fw->file == NULL){
    printf("LRL_open_write_file: failed to open %s for writing\n",
	   filename);
    return NULL;
  }

  fw->dg = limeCreateWriter(fw->file);
  if (fw->dg == (LimeWriter *)NULL){
    printf("LRL_open_write_file: limeCreateWriter failed\n");
    return NULL;
  }

  return fw;
}

/** 
 * Open a record for reading
 *
 * \param fr         LRL file reader  ( Read )
 * \param rec_size   record size ( Modify )
 * \param tag        tag for the record header ( Read )
 *
 * \return null if failure and set status to error flag
 */
LRL_RecordReader *LRL_open_read_record(LRL_FileReader *fr,
				       size_t *rec_size, 
				       LIME_type *lime_type, int *status)
{
  LRL_RecordReader *rr;
  int lime_status;
  char myname[] = "LRL_open_read_record";

  if (fr == NULL)
    return NULL;

  rr = (LRL_RecordReader *)malloc(sizeof(LRL_RecordReader));
  if (rr == NULL){
    printf("%s: Can't malloc reader\n",myname);
    return NULL;
  }
  rr->fr = fr;
  
  /* Get next record header */
  lime_status = limeReaderNextRecord(rr->fr->dr);
  if (lime_status != LIME_SUCCESS){
    if(lime_status == LIME_EOF)*status = LRL_EOF;
    else{
      printf("%s lime error %d getting next record header\n",myname,*status);
      *status = LRL_ERR_READ;
    }
    return NULL;
  }

  /* Extract record information */
  *rec_size = limeReaderBytes(rr->fr->dr);
  *lime_type = limeReaderType(rr->fr->dr);

  *status = LIME_SUCCESS;
  return rr;
}


/** 
 * Open a record for writing 
 *
 * \param fw         LRL file writer  ( Read )
 * \param rec_size   record size ( Modify )
 * \param tag        tag for the record header ( Read )
 *
 * \return null if failure
 */
LRL_RecordWriter *LRL_open_write_record(LRL_FileWriter *fw, 
					int msg_begin, int msg_end, 
					size_t *rec_size, 
					LIME_type lime_type)
{
  LRL_RecordWriter *rw;
  LimeRecordHeader *h;
  int status;
  char myname[] = "LRL_open_write_record";
  
  if (fw == NULL)
    return NULL;


  rw = (LRL_RecordWriter *)malloc(sizeof(LRL_RecordWriter));
  if (rw == NULL){
    printf("%s: Can't malloc writer\n",myname);
    return NULL;
  }
  rw->fw = fw;

  /* Write record */
  h = limeCreateHeader(msg_begin, msg_end, lime_type, *rec_size);
  status = limeWriteRecordHeader(h, rw->fw->dg);

  if (status < 0)
  { 
    printf( "%s: fatal error. LIME status is: %d\n", myname, status);
    exit(EXIT_FAILURE);
  }

  limeDestroyHeader(h);

  return rw;
}


/** 
 * Read bytes
 *
 * \param rr         LRL record reader  ( Read )
 * \param buf        buffer for reading ( Write )
 * \param nbytes     number of bytes to read ( Read )
 *
 * \return number of bytes read
 */
size_t LRL_read_bytes(LRL_RecordReader *rr, char *buf, 
		      size_t nbytes)
{
  int status;
  size_t nbyt = nbytes;
  char myname[] = "LRL_read_bytes";

  if (rr == NULL)
    return 0;

  status = limeReaderReadData((void *)buf, &nbyt, rr->fr->dr);
  if( status != LIME_SUCCESS ) 
  { 
    printf( "%s: LIME error %d has occurred\n", myname, status);
    exit(EXIT_FAILURE);
  }

  return nbyt;
}


/** 
 * Write bytes
 *
 * \param rw         LRL record writer  ( Read )
 * \param buf        buffer for writing ( Read )
 * \param nbytes     number of bytes to write ( Read )
 *
 * \return number of bytes written
 */
size_t LRL_write_bytes(LRL_RecordWriter *rw, char *buf, 
		       size_t nbytes)
{
  int status;
  size_t nbyt = nbytes;

  if (rw == NULL)
    return 0;

  status = limeWriteRecordData(buf, &nbyt, rw->fw->dg);

  if( status != LIME_SUCCESS ) 
  { 
    printf( "LRL_write_bytes: some error has occurred. status is: %d\n", status);
    exit(EXIT_FAILURE);
  }

  return nbyt;
}


/* For skipping ahead offset bytes from the current position in the record
   payload.  We are not allowed to go beyond the end of the
   payload. */

int LRL_seek_read_record(LRL_RecordReader *rr, off_t offset)
{
  int status;

  if (rr == NULL || rr->fr == NULL)return LRL_ERR_SEEK;
  status = limeReaderSeek(rr->fr->dr, offset, SEEK_CUR);

  if( status != LIME_SUCCESS ) 
  { 
    printf( "LRL_seek_read_record: some error has occurred. status is: %d\n", status);
    return LRL_ERR_SEEK;
  }
  return LRL_SUCCESS;
}

/* For skipping ahead offset bytes from the current position in the record
   payload.  We are not allowed to go beyond the end of the
   payload. */

int LRL_seek_write_record(LRL_RecordWriter *rw, off_t offset)
{
  int status;

  if (rw == NULL){
    printf("LRL_seek_write_record: null record writer\n");
    return LRL_ERR_SEEK;
  }
  if(rw->fw == NULL){
    printf("LRL_seek_write_record: null file writer\n");
    return LRL_ERR_SEEK;
  }

  status = limeWriterSeek(rw->fw->dg, offset, SEEK_CUR);

  if( status != LIME_SUCCESS ) 
  { 
    printf("LRL_seek_write_record: LIME error %d\n", status);
    return LRL_ERR_SEEK;
  }
  return LRL_SUCCESS;
}

/* For skipping to the end of the current message */

int LRL_next_message(LRL_FileReader *fr)
{
  int status;
  int msg_end = 0;

  if(fr == NULL)return LRL_ERR_SKIP;
  while(msg_end == 0){
    status = limeReaderNextRecord(fr->dr);
    msg_end = limeReaderMEFlag(fr->dr);
    printf("LRL_next_message skipping msg_end %d status %d\n",
	   msg_end, status);
    if( status != LIME_SUCCESS ) 
      { 
	printf( "LRL_next_message: LIME error %d\n", status);
	return LRL_ERR_SKIP;
      }
  }
  return LRL_SUCCESS;
}

/* For skipping to the beginning of the next record? */

int LRL_next_record(LRL_FileReader *fr)
{
  int status;

  if(fr == NULL)return LRL_ERR_SKIP;
  status = limeReaderNextRecord(fr->dr);

  if( status != LIME_SUCCESS ) 
  { 
    printf( "LRL_next_record: LIME error %d\n", status);
    return LRL_ERR_SKIP;
  }
  return LRL_SUCCESS;
}


/**
 *
 * \param rr     LRL record reader
 *
 * return 0 for success and 1 for failure
 */

int LRL_close_read_record(LRL_RecordReader *rr)
{
  int status;

  status = limeReaderCloseRecord(rr->fr->dr);
  free(rr);
  if(status != LIME_SUCCESS)return LRL_ERR_CLOSE;
  else return LRL_SUCCESS;
}

/**
 *
 * \param rw     LRL record writer
 *
 */

int LRL_close_write_record(LRL_RecordWriter *rw)
{
  int status;

  if(rw == NULL)return LRL_ERR_CLOSE;
  status = limeWriterCloseRecord(rw->fw->dg);
  free(rw);
  if(status != LIME_SUCCESS)return LRL_ERR_CLOSE;
  else return LRL_SUCCESS;
}

/** 
 * Close a file for reading
 *
 * \param fr   file buffer  ( Modify )
 *
 * \return status
 */
int LRL_close_read_file(LRL_FileReader *fr)
{
  if (fr == NULL)
    return LRL_SUCCESS;

  limeDestroyReader(fr->dr);
  fclose(fr->file);
  free(fr);

  return LRL_SUCCESS;
}

/** 
 * Close a file for writing
 *
 * \param fw   file buffer  ( Modify )
 *
 * \return status
 */
int LRL_close_write_file(LRL_FileWriter *fw)
{
  if (fw == NULL)
    return LRL_SUCCESS;

  limeDestroyWriter(fw->dg);
  fclose(fw->file);
  free(fw);

  return LRL_SUCCESS;
}
