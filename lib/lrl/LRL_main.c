/* LRL_read_bytes.c */

#include <lrl.h>
#include <stdio.h>
#include <malloc.h>
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
  if (fr->file == NULL)
    return NULL;

  fr->dr = dimeCreateReader(fr->file);
  if (fr->dr == (DimeReader *)NULL)
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
  fr->file = fopen(filename,"w");
  if (fr->file == NULL)
    return NULL;

  fr->dg = dimeWriterInit(fr->file);
  if (fr->dg == (DimeWriter *)NULL)
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
				       DIME_tag tag)
{
  LRL_RecordReader *rr;
  int status;

  if (fr == NULL)
    return NULL;

  rr = (LRL_RecordReader *)malloc(sizeof(LRL_RecordReader));
  if (rr == NULL)
    return NULL;
  rr->fr = fr;
  
  /* Check if last record */
  if (rr->fr->dr->is_last != 0 )
  {
    free(rr);
    return NULL;
  }
   
  /* Get next record header */
  status = dimeReaderNextRecord(rr->fr->dr);
  if (status != DIME_SUCCESS)
    return NULL;

  *rec_size = rr->fr->dr->bytes_total;

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
LRL_RecordWriter *LRL_open_write_record(LRL_FileWriter *fr, size_t *rec_size, 
					DIME_tag tag)
{
  LRL_RecordWriter *rr;
  DimeRecordHeader *h;
  int status;
  
  if (fr == NULL)
    return NULL;

  rr = (LRL_RecordWriter *)malloc(sizeof(LRL_RecordWriter));
  if (rr == NULL)
    return NULL;
  rr->fr = fr;

  /* Write record */
  h = dimeCreateHeader(0,
		       TYPE_MEDIA,
		       "application/text", 
		       "http://www.foobar.com",
		       *rec_size);

  status = dimeWriteRecordHeader(h, 
				 0, /* Not last record */
				 rr->fr->dg);

  if (status < 0)
  { 
    fprintf(stderr, "Oh dear some horrible error has occurred. status is: %d\n", status);
    exit(EXIT_FAILURE);
  }

  dimeDestroyHeader(h);

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

  status = dimeWriteRecordData(buf, &nbyt, rr->fr->dg);
  if( status != DIME_SUCCESS ) 
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

  if (rr == NULL)
    return 0;

  status = dimeReaderReadData((void *)buf, &nbyt, rr->fr->dr);
  if( status != DIME_SUCCESS ) 
  { 
    fprintf(stderr, "LRL_read_bytes: some error has occurred. status is: %d\n", status);
    exit(EXIT_FAILURE);
  }

  return nbyt;
}

int LRL_close_read_record(LRL_RecordReader *rr)
{
  free(rr);
  return 0;
}

int LRL_close_write_record(LRL_RecordWriter *rr)
{
  free(rr);
  return 0;
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
    return 0;

  dimeDestroyReader(fr->dr);
  fclose(fr->file);
  free(fr);

  return 0;
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
    return 0;

  dimeWriterClose(fr->dg);
  fclose(fr->file);
  free(fr);

  return 0;
}

