#ifndef LRL_H
#define LRL_H

#include <stdio.h>
/* #include "dime.h" */

/* Dummy DIME tag for now */
typedef char* DIME_tag;

/* Dummy file XML string */
#define XML_MAX 128

typedef struct {
  FILE *file;
#if 0
  DimeWriter *dg;
#endif
} LRL_FileWriter;

typedef struct {
  LRL_FileWriter *fr;
#if 0
  DimeRecordHeader *h;
#endif
} LRL_RecordWriter;

typedef struct {
  FILE *file;
#if 0
  DimeReader *dg;
#endif
} LRL_FileReader;

typedef struct {
  LRL_FileReader *fr;
} LRL_RecordReader;

LRL_FileReader *LRL_open_read_file(char *filename);
LRL_FileWriter *LRL_open_write_file(char *filename, int mode);
LRL_RecordReader *LRL_open_read_record(LRL_FileReader *fr, size_t *rec_size, 
				       DIME_tag tag);
LRL_RecordWriter *LRL_open_write_record(LRL_FileWriter *fr, size_t *rec_size, 
					DIME_tag tag);
size_t LRL_write_bytes(LRL_RecordWriter *rr, char *buf, size_t nbytes);
size_t LRL_read_bytes(LRL_RecordReader *rr, char *buf, size_t nbytes);
int LRL_close_read_record(LRL_RecordReader *rr);
int LRL_close_write_record(LRL_RecordWriter *rr);
int LRL_close_read_file(LRL_FileReader *fr);
int LRL_close_write_file(LRL_FileWriter *fr);

#endif /* LRL_H */
