/* LRL_read_bytes.c */
/* Dummy */
/* DIME ignored */

#include <lrl.h>
#include <stdio.h>

/** 
 * Open a file for reading 
 *
 * \param filename   file for writing  ( Read )
 *
 * \return null if failure
 */
LRL_FileReader *LRL_open_read_file(char *filename)
{
  LRL_FileReader *fr;

  fr = (LRL_FileReader *)malloc(sizeof(LRL_FileReader));
  if (fr == NULL)
    return NULL;

  /*** Ignore mode for now ***/
  fr->file = fopen(filename,"r");
  if (fr->file == NULL)
    return NULL;

  fr->dr = dimeCreateReader(fr);
  if (fr->dr == (DimeReader *)NULL)
    return NULL

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
LRL_FileWriter *LRL_open_write_file(char *filename, int mode)
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

LRL_RecordReader *LRL_open_read_record(LRL_FileReader *fr, size_t *rec_size, 
				       DIME_tag tag){
  LRL_RecordReader *rr;

  if(fr == NULL)return NULL;

  rr = (LRL_RecordReader *)malloc(sizeof(LRL_RecordReader));
  if(rr == NULL)return NULL;
  rr->fr = fr;
  
  /* Read and get byte size of record */
  if(fread(rec_size, 1, sizeof(size_t), fr->file) != sizeof(size_t))
    return NULL;  

  return rr;
}


/** 
 * Open a record for writing 
 *
 * \param fr         LRL file writer  ( Read )
 * \param rec_size   record size ( Read )
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

  /* Write byte size of record */
  if(fwrite(rec_size, 1, sizeof(size_t), fr->file) != sizeof(size_t))
    return NULL;  

  h = dimeCreateHeader(0,
		       TYPE_MEDIA,
		       "application/text", 
		       "http://www.foobar.com",
		       strlen(message));

  status = dimeWriteRecordHeader(h, 
				 1, /* First and Last record */
				 fr->dg);

  if (status < 0)
  { 
    fprintf(stderr, "Oh dear some horrible error has occurred. status is: %d\n", status);
    exit(EXIT_FAILURE);
  }

  dimeDestroyHeader(h);


  return rr;
}

size_t LRL_write_bytes(LRL_RecordWriter *rr, char *buf, size_t nbytes)
{
  return fwrite(buf, 1, nbytes, rr->fr->file);
}

size_t LRL_read_bytes(LRL_RecordReader *rr, char *buf, size_t nbytes)
{
  return fread(buf, 1, nbytes, rr->fr->file);
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

  dimeDestroyReader(fr->dg);
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

