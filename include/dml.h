#ifndef DML_H
#define DML_H

#include <lrl.h>

#define DML_MASTER_NODE 0

enum {DML_SERIAL, DML_PARALLEL};
enum {DML_LEX_ORDER, DML_NAT_ORDER};
enum {DML_SINGLEFILE, DML_MULTIFILE};

#ifdef __cplusplus
extern "C"
{
#endif

typedef char DML_Checksum;  /* Dummy for now */

/* For collecting and passing layout information */
/* See qio.h for QIO_Layout */
typedef struct {
  int (*node_number)(const int coords[]);
  int *latsize;
  int latdim;
  size_t volume;
  int this_node;
} DML_Layout;

int DML_stream_out(LRL_RecordWriter *lrl_record_out, 
		   void (*get)(char *buf, int coords[], void *arg),
		   size_t size, void *arg, DML_Layout *layout,
		   int serpar, int siteorder, int volfmt, 
		   DML_Checksum *checksum);

int DML_stream_in(LRL_RecordReader *lrl_record_in, 
		  void (*put)(char *buf, int coords[], void *arg),
		  size_t size, void *arg, DML_Layout *layout,
		  int serpar, int siteorder, size_t sitelist[],
		  int volfmt, DML_Checksum *checksum);

/* DML internal utilities */

void DML_lex_init(int *dim, int coords[], int latdim, int latsize[]);
int DML_lex_next(int *dim, int coords[], int latdim, int latsize[]);
void DML_lex_coords(int coords[], int latdim, int latsize[], int rcv_coords);
int DML_send_bytes(char *buf, int size, int tonode);
int DML_get_bytes(char *buf, int size, int fromnode);
void DML_sync(void);
int DML_serial_out(LRL_RecordWriter *lrl_record_out, 
		   void (*get)(char *buf, int coords[], void *arg),
		   size_t size, void *arg, DML_Layout *layout,
		   DML_Checksum *checksum);
int DML_multidump_out(LRL_RecordWriter *lrl_record_out, 
		      void (*get)(char *buf, int coords[], void *arg),
		      size_t size, void *arg, DML_Layout *layout,
		      DML_Checksum *checksum);
int DML_parallel_out(LRL_RecordWriter *lrl_record_out, 
		     void (*get)(char *buf, int coords[], void *arg),
		     size_t size, void *arg, DML_Layout *layout,
		     DML_Checksum *checksum);
int DML_checkpoint_out(LRL_RecordWriter *lrl_record_out, 
		       void (*get)(char *buf, int coords[], void *arg),
		       size_t size, void *arg, DML_Layout *layout,
		       DML_Checksum *checksum);
int DML_multidump_in(LRL_RecordReader *lrl_record_in, size_t sitelist[],
		     void (*put)(char *buf, int coords[], void *arg),
		     size_t size, void *arg, DML_Layout *layout,
		     DML_Checksum *checksum);
int DML_serial_in(LRL_RecordReader *lrl_record_in, int siteorder, 
		  size_t sitelist[],
		  void (*put)(char *buf, int coords[], void *arg),
		  size_t size, void *arg, DML_Layout *layout,
		  DML_Checksum *checksum);
int DML_parallel_in(LRL_RecordReader *lrl_record_in, int siteorder, 
		    size_t sitelist[],
		    void (*put)(char *buf, int coords[], void *arg),
		    size_t size, void *arg, DML_Layout *layout,
		    DML_Checksum *checksum);

#ifdef __cplusplus
}
#endif

#endif /* DML_H */
