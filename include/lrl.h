#ifndef LRL_H
#define LRL_H

#include <stdio.h>
#include <sys/types.h>
#include <lime.h>

/* Return codes */
#define LRL_SUCCESS      ( 0)
#define LRL_EOF          (-1)
#define LRL_ERR_READ     (-2)
#define LRL_ERR_SEEK     (-3)
#define LRL_ERR_SKIP     (-4)
#define LRL_ERR_CLOSE    (-5)

#ifdef __cplusplus
extern "C"
{
#endif

/* Dummy LIME tag for now */
#define MAX_LIME_TYPE_LEN 32
typedef char* LIME_type;

typedef struct {
  FILE *file;
  LimeWriter *dg;
} LRL_FileWriter;

typedef struct {
  LRL_FileWriter *fw;
} LRL_RecordWriter;

typedef struct {
  FILE *file;
  LimeReader *dr;
} LRL_FileReader;

typedef struct {
  LRL_FileReader *fr;
} LRL_RecordReader;

LRL_FileReader *LRL_open_read_file(const char *filename);
LRL_FileWriter *LRL_open_write_file(const char *filename);
LRL_RecordReader *LRL_open_read_record(LRL_FileReader *fr, off_t *rec_size, 
				       LIME_type *lime_type, int *status);
LRL_RecordWriter *LRL_open_write_record(LRL_FileWriter *fr, 
					int msg_begin, int msg_end, 
					off_t *rec_size, 
					LIME_type lime_type);
off_t LRL_write_bytes(LRL_RecordWriter *rr, char *buf, 
		       off_t nbytes);
off_t LRL_read_bytes(LRL_RecordReader *rr, char *buf, 
		      off_t nbytes);
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
