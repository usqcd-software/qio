#ifndef LRL_H
#define LRL_H

#include <stdio.h>
#include <sys/types.h>
#include "dime.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* Dummy DIME tag for now */
#define MAX_DIME_TYPE_LEN 32
typedef char* DIME_type;

/* Dummy file XML string */
#define XML_MAX 128

typedef struct {
  FILE *file;
  DimeWriter *dg;
} LRL_FileWriter;

typedef struct {
  LRL_FileWriter *fr;
} LRL_RecordWriter;

typedef struct {
  FILE *file;
  DimeReader *dr;
} LRL_FileReader;

typedef struct {
  LRL_FileReader *fr;
} LRL_RecordReader;

LRL_FileReader *LRL_open_read_file(const char *filename);
LRL_FileWriter *LRL_open_write_file(const char *filename, int mode);
LRL_RecordReader *LRL_open_read_record(LRL_FileReader *fr, size_t *rec_size, 
				       DIME_type dime_type);
LRL_RecordWriter *LRL_open_write_record(LRL_FileWriter *fr, size_t *rec_size, 
					DIME_type dime_type);
size_t LRL_write_bytes(LRL_RecordWriter *rr, char *buf, size_t nbytes);
size_t LRL_read_bytes(LRL_RecordReader *rr, char *buf, size_t nbytes);
int LRL_seek_write_record(LRL_RecordWriter *rr, off_t offset);
int LRL_seek_read_record(LRL_RecordReader *rr, off_t offset);
int LRL_next_record(LRL_FileReader *fr);
int LRL_close_read_record(LRL_RecordReader *rr);
int LRL_close_write_record(LRL_RecordWriter *rr);
int LRL_close_read_file(LRL_FileReader *fr);
int LRL_close_write_file(LRL_FileWriter *fr);

#ifdef __cplusplus
}
#endif

#endif /* LRL_H */
