#ifndef LRL_H
#define LRL_H

#include <stdio.h>
#include <sys/types.h>
#include "lime.h"

/* Return codes */
#define LRL_SUCCESS      ( 0)
#define LRL_ERR_SEEK     (-1)
#define LRL_ERR_SKIP     (-2)
#define LRL_ERR_CLOSE    (-3)

#ifdef __cplusplus
extern "C"
{
#endif

/* Dummy LIME tag for now */
#define MAX_LIME_TYPE_LEN 32
typedef char* LIME_type;

/* Dummy file XML string */
#define XML_MAX 128

typedef struct {
  FILE *file;
  LimeWriter *dg;
} LRL_FileWriter;

typedef struct {
  LRL_FileWriter *fr;
} LRL_RecordWriter;

typedef struct {
  FILE *file;
  LimeReader *dr;
} LRL_FileReader;

typedef struct {
  LRL_FileReader *fr;
} LRL_RecordReader;

LRL_FileReader *LRL_open_read_file(const char *filename);
LRL_FileWriter *LRL_open_write_file(const char *filename, int mode);
LRL_RecordReader *LRL_open_read_record(LRL_FileReader *fr, size_t *rec_size, 
				       LIME_type lime_type);
LRL_RecordWriter *LRL_open_write_record(LRL_FileWriter *fr, int do_write,
					int msg_begin, int msg_end, 
					size_t *rec_size, 
					LIME_type lime_type);
size_t LRL_write_bytes(LRL_RecordWriter *rr, char *buf, size_t nbytes);
size_t LRL_read_bytes(LRL_RecordReader *rr, char *buf, size_t nbytes);
int LRL_seek_write_record(LRL_RecordWriter *rr, off_t offset);
int LRL_seek_read_record(LRL_RecordReader *rr, off_t offset);
int LRL_next_record(LRL_FileReader *fr);
int LRL_next_message(LRL_FileReader *fr);
int LRL_close_read_record(LRL_RecordReader *rr);
int LRL_close_write_record(LRL_RecordWriter *rr);
int LRL_close_read_file(LRL_FileReader *fr);
int LRL_close_write_file(LRL_FileWriter *fr);

#ifdef __cplusplus
}
#endif

#endif /* LRL_H */
