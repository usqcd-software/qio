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

  fr = (LRL_FileReader *)malloc(sizeof(LRL_FileReader));
  if (fr == NULL)
    return NULL;

  /*** Ignore mode for now ***/
  fr->file = fopen(filename,"r");
  if (fr->file == NULL)return NULL;

  fr->dr = limeCreateReader(fr->file);
  if (fr->dr == (LimeReader *)NULL)
    return NULL;

  return fr;
}

/** 
 * Open a file for writing 
 *
 * \param filename   file for writing  ( Read )
 * \param mode       mode of file - not sure what to do with this ( Read )
 *
 * \return null if failure
 */
LRL_FileWriter *LRL_open_write_file(const char *filename, int mode)
{
  LRL_FileWriter *fr;

  fr = (LRL_FileWriter *)malloc(sizeof(LRL_FileWriter));
  if (fr == NULL)
    return NULL;

  /*** Ignore mode for now ***/
  /*** (RGE) - what do we do with mode ??? */
  /*** (CD)  mode = write (from beginning with truncation) or append to end */
  fr->file = fopen(filename,"w");
  if (fr->file == NULL)
    return NULL;

  fr->dg = limeCreateWriter(fr->file);
  if (fr->dg == (LimeWriter *)NULL)
    return NULL;

  return fr;
}

/** 
 * Open a record for reading
 *
 * \param fr         LRL file reader  ( Read )
 * \param rec_size   record size ( Modify )
 * \param tag        tag for the record header ( Read )
 *
 * \return null if failure
 */
LRL_RecordReader *LRL_open_read_record(LRL_FileReader *fr, size_t *rec_size, 
				       LIME_type lime_type)
{
  LRL_RecordReader *rr;
  int status;
  char myname[] = "LRL_open_read_record";

  if (fr == NULL)
    return NULL;

  rr = (LRL_RecordReader *)malloc(sizeof(LRL_RecordReader));
  if (rr == NULL){
    printf("LRL_open_read_record: Can't malloc reader\n");
    return NULL;
  }
  rr->fr = fr;
  
  /* Get next record header */
  status = limeReaderNextRecord(rr->fr->dr);
  if (status != LIME_SUCCESS){
    printf("%s error getting next record header\n",myname);
    return NULL;
  }

  /* Extract record information */
  *rec_size = limeReaderBytes(rr->fr->dr);
  lime_type = limeReaderType(rr->fr->dr);

  return rr;
}


/** 
 * Open a record for writing 
 *
 * \param fr         LRL file writer  ( Read )
 * \param rec_size   record size ( Modify )
 * \param tag        tag for the record header ( Read )
 *
 * \return null if failure
 */
LRL_RecordWriter *LRL_open_write_record(LRL_FileWriter *fr, int do_write,
					int msg_begin, int msg_end, 
					size_t *rec_size, 
					LIME_type lime_type)
{
  LRL_RecordWriter *rr;
  LimeRecordHeader *h;
  int status;
  char myname[] = "LRL_open_write_record";
  
  if (fr == NULL)
    return NULL;


  rr = (LRL_RecordWriter *)malloc(sizeof(LRL_RecordWriter));
  if (rr == NULL){
    printf("%s: Can't malloc writer\n",myname);
    return NULL;
  }
  rr->fr = fr;

  /* Write record */
  h = limeCreateHeader(msg_begin, msg_end, lime_type, *rec_size);
  status = limeWriteRecordHeader(h, do_write, rr->fr->dg);

  if (status < 0)
  { 
    fprintf(stderr, "%s: fatal error. status is: %d\n", myname, status);
    exit(EXIT_FAILURE);
  }

  limeDestroyHeader(h);

  return rr;
}


/** 
 * Write bytes
 *
 * \param rr         LRL record writer  ( Read )
 * \param buf        buffer for writing ( Read )
 * \param nbytes     number of bytes to write ( Read )
 *
 * \return number of bytes written
 */
size_t LRL_write_bytes(LRL_RecordWriter *rr, char *buf, size_t nbytes)
{
  int status;
  size_t nbyt = nbytes;

  if (rr == NULL)
    return 0;

  status = limeWriteRecordData(buf, &nbyt, rr->fr->dg);

  if( status != LIME_SUCCESS ) 
  { 
    fprintf(stderr, "LRL_write_bytes: some error has occurred. status is: %d\n", status);
    exit(EXIT_FAILURE);
  }

  return nbyt;
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
size_t LRL_read_bytes(LRL_RecordReader *rr, char *buf, size_t nbytes)
{
  int status;
  size_t nbyt = nbytes;
  char myname[] = "LRL_read_bytes";

  if (rr == NULL)
    return 0;

  status = limeReaderReadData((void *)buf, &nbyt, rr->fr->dr);
  if( status != LIME_SUCCESS ) 
  { 
    fprintf(stderr, "%s: LIME error %d has occurred\n", myname, status);
    exit(EXIT_FAILURE);
  }

  return nbyt;
}


/* For skipping ahead offset bytes from the current position in the record
   payload.  We are not allowed to go beyond the end of the
   payload. */

int LRL_seek_write_record(LRL_RecordWriter *rr, off_t offset)
{
  int status;

  if (rr == NULL || rr->fr == NULL)return LRL_ERR_SEEK;
  status = limeWriterSeek(rr->fr->dg, offset, SEEK_CUR);

  if( status != LIME_SUCCESS ) 
  { 
    fprintf(stderr, "LRL_seek_write_record: LIME error %d\n", status);
    return LRL_ERR_SEEK;
  }
  return LRL_SUCCESS;
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
    fprintf(stderr, "LRL_seek_read_record: some error has occurred. status is: %d\n", status);
    return LRL_ERR_SEEK;
  }
  return LRL_SUCCESS;
}

/* For skipping to the beginning of the next message */

int LRL_next_message(LRL_FileReader *fr)
{
  int status;
  int msg_begin = 0;

  if(fr == NULL)return LRL_ERR_SKIP;
  while(msg_begin == 0){
    status = limeReaderNextRecord(fr->dr);
    msg_begin = limeReaderMBFlag(fr->dr);
    if( status != LIME_SUCCESS ) 
      { 
	fprintf(stderr, "LRL_next_message: LIME error %d\n", status);
	return LRL_ERR_SKIP;
      }
    return LRL_SUCCESS;
  }
}

/* For skipping to the beginning of the next record - DO WE NEED THIS? */

int LRL_next_record(LRL_FileReader *fr)
{
  int status;

  if(fr == NULL)return LRL_ERR_SKIP;
  status = limeReaderNextRecord(fr->dr);

  if( status != LIME_SUCCESS ) 
  { 
    fprintf(stderr, "LRL_next_record: LIME error %d\n", status);
    return LRL_ERR_SKIP;
  }
  return LRL_SUCCESS;
}


/**
 *
 * \param rr     LRL record writer
 *
 */

int LRL_close_write_record(LRL_RecordWriter *wr)
{
  int status;

  if(wr == NULL)return LRL_ERR_CLOSE;
  status = limeWriterCloseRecord(wr->fr->dg);
  free(wr);
  if(status != LIME_SUCCESS)return LRL_ERR_CLOSE;
  else return LRL_SUCCESS;
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
 * \param fr   file buffer  ( Modify )
 *
 * \return status
 */
int LRL_close_write_file(LRL_FileWriter *fr)
{
  if (fr == NULL)
    return LRL_SUCCESS;

  limeDestroyWriter(fr->dg);
  fclose(fr->file);
  free(fr);

  return LRL_SUCCESS;
}
