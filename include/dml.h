#ifndef DML_H
#define DML_H

#include <lrl.h>
#include <type32.h>
#include <config.h>

#define DML_MASTER_NODE 0

/* Modes for file reading and writing */
#define DML_SERIAL       0
#define DML_PARALLEL     1

/* Order of sites in the file */
#define DML_LEX_ORDER    0
#define DML_NAT_ORDER    1
#define DML_LIST_ORDER   2

/* File fragmentation */
#define DML_SINGLEFILE   0
#define DML_MULTIFILE    1

/* Limits size of read and write buffers (bytes) 2^18 for now */
#define DML_BUF_BYTES 262144
/* Limit the number of messages that may pile up on a given node */
#define DML_COMM_BLOCK    128

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct {
  u_int32 suma;
  u_int32 sumb;
} DML_Checksum;

/* Type for sitelist values */
/* Change only if creating new file format */
typedef u_int32 DML_SiteRank;

/* For collecting and passing layout information */
/* See qio.h for QIO_Layout */
typedef struct {
  int (*node_number)(const int coords[]);
  int (*node_index)(const int coords[]);
  void (*get_coords)(int coords[], int node, const int index);
  int *latsize;
  int latdim;
  size_t volume;
  size_t sites_on_node;
  int this_node;
  int number_of_nodes;
} DML_Layout;

size_t DML_stream_out(LRL_RecordWriter *lrl_record_out, 
	   void (*get)(char *buf, size_t index, size_t count, void *arg),
	   size_t size, int word_size, void *arg, DML_Layout *layout,
	   int serpar, int volfmt, DML_Checksum *checksum);

size_t DML_stream_in(LRL_RecordReader *lrl_record_in, 
	     void (*put)(char *buf, size_t index, size_t count, void *arg),
	     size_t size, int word_size, void *arg, DML_Layout *layout,
	     int serpar, int siteorder, DML_SiteRank sitelist[],
		     int volfmt, DML_Checksum *checksum);

/* DML internal utilities */

void DML_lex_init(int *dim, int coords[], int latdim, int latsize[]);
int DML_lex_next(int *dim, int coords[], int latdim, int latsize[]);
void DML_lex_coords(int coords[], int latdim, int latsize[], 
		    DML_SiteRank rcv_coords);
DML_SiteRank DML_lex_rank(int coords[], int latdim, int latsize[]);
int *DML_allocate_coords(int latdim, char *myname, int this_node);
char *DML_allocate_msg(size_t size, char *myname, int this_node);
size_t DML_msg_sizeof(size_t size);
char *DML_msg_datum(char *msg, size_t size);
DML_SiteRank *DML_msg_rank(char *msg, size_t size);
int DML_create_sitelist(DML_Layout *layout,DML_SiteRank sitelist[]);
int DML_is_native_sitelist(DML_Layout *layout, DML_SiteRank sitelist[]);
void DML_checksum_init(DML_Checksum *checksum);
void DML_checksum_accum(DML_Checksum *checksum, DML_SiteRank rank, 
			char *buf, size_t size);
void DML_checksum_combine(DML_Checksum *checksum);
void DML_global_xor(u_int32 *x);
void DML_byterevn(char *buf, size_t size, int word_size);
size_t DML_max_buf_sites(size_t size, int factor);
char *DML_allocate_buf(size_t size, size_t max_buf_sites, int this_node);
size_t DML_write_buf_next(LRL_RecordWriter *lrl_record_out, size_t size,
			  char *buf, size_t buf_sites, size_t max_buf_sites, 
			  size_t isite, size_t max_dest_sites, size_t *nbytes,
			  char *myname, int this_node);
size_t DML_read_buf_next(LRL_RecordReader *lrl_record_in, int size,
			 char *buf, size_t *buf_extract, size_t buf_sites, 
			 size_t max_buf_sites, size_t isite, 
			 size_t max_send_sites, 
			 size_t *nbytes, char *myname, int this_node);
int DML_serial_out(LRL_RecordWriter *lrl_record_out, 
	   void (*get)(char *buf, size_t index, size_t count, void *arg),
	   size_t size, int word_size, void *arg, DML_Layout *layout,
	   DML_Checksum *checksum);
int DML_multifile_out(LRL_RecordWriter *lrl_record_out, 
	      void (*get)(char *buf, size_t index, size_t count, void *arg),
	      size_t size, int word_size, void *arg, 
	      DML_Layout *layout, DML_Checksum *checksum);
int DML_parallel_out(LRL_RecordWriter *lrl_record_out, 
	     void (*get)(char *buf, size_t index, size_t count, void *arg),
	     size_t size, int word_size, void *arg, 
	     DML_Layout *layout, DML_Checksum *checksum);
int DML_multifile_in(LRL_RecordReader *lrl_record_in, 
	     DML_SiteRank sitelist[],
	     void (*put)(char *buf, size_t index, size_t count, void *arg),
	     size_t size, int word_size, void *arg, 
	     DML_Layout *layout, DML_Checksum *checksum);
int DML_serial_in(LRL_RecordReader *lrl_record_in, 
	  void (*put)(char *buf, size_t index, size_t count, void *arg),
	  size_t size, int word_size, void *arg, DML_Layout* layout,
	  DML_Checksum *checksum);
int DML_parallel_in(LRL_RecordReader *lrl_record_in, 
	    void (*put)(char *buf, size_t index, size_t count, void *arg),
	    size_t size, int word_size, void *arg, DML_Layout *layout,
	    DML_Checksum *checksum);

void DML_broadcast_bytes(char *buf, size_t size);
void DML_sum_size_t(size_t *ipt);
void DML_sum_int(int *ipt);
int DML_send_bytes(char *buf, size_t size, int tonode);
int DML_get_bytes(char *buf, size_t size, int fromnode);
void DML_sync(void);

u_int32 DML_crc32(u_int32 crc, const unsigned char *buf, u_int32 len);

#ifdef __cplusplus
}
#endif

#endif /* DML_H */

