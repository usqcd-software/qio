#ifndef QIO_H
#define QIO_H

#include <xml.h>
#include <lrl.h>
#include <dml.h>

#define MAX_BINX  256 /* dummy */
#define QIO_MASTER_NODE DML_MASTER_NODE

#define QIO_SERIAL     DML_SERIAL     
#define QIO_PARALLEL   DML_PARALLEL   
#define QIO_LEX_ORDER  DML_LEX_ORDER  
#define QIO_NAT_ORDER  DML_NAT_ORDER  
#define QIO_SINGLEFILE DML_SINGLEFILE 
#define QIO_MULTIFILE  DML_MULTIFILE  

/* enum {QIO_CREATE, QIO_TRUNCATE, QIO_APPEND}; */
#define QIO_CREATE     0
#define QIO_TRUNCATE   1
#define QIO_APPEND     2

/* Does the architecture support parallel reads and writes? */
/* Should be obtained from system call or a config.h */
#define PARALLEL_READ 0
#define PARALLEL_WRITE 0

#ifdef __cplusplus
extern "C"
{
#endif

/* For collecting and passing layout information */
typedef struct {
  int (*node_number)(const int coords[]);
  int *latsize;
  int latdim;
  size_t volume;
  int this_node;
} QIO_Layout;

typedef struct {
  LRL_FileWriter *lrl_file_out;
  int serpar;
  int volfmt;
  DML_Layout *layout;
  int siteorder;
  size_t *sitelist;
} QIO_Writer;

typedef struct {
  LRL_FileReader *lrl_file_in;
  int serpar;
  int volfmt;
  DML_Layout *layout;
  int siteorder;
  size_t *sitelist;
} QIO_Reader;

QIO_Writer *QIO_open_write(XML_MetaData *xml_file, const char *filename, 
			   int volfmt, int siteorder, int mode,
			   QIO_Layout *layout);
QIO_Reader *QIO_open_read(XML_MetaData *xml_file, const char *filename, int volfmt,
			  QIO_Layout *layout);

int QIO_close_write(QIO_Writer *out);
int QIO_close_read(QIO_Reader *in);

int QIO_write(QIO_Writer *out, XML_MetaData *xml_record, 
	      void (*get)(char *buf, const int coords[], void *arg),
	      int datum_size, void *arg);
int QIO_read(QIO_Reader *out, XML_MetaData *xml_record, 
	     void (*put)(char *buf, const int coords[], void *arg),
	     int datum_size, void *arg);

/* Internal utilities  */
int QIO_write_string(QIO_Writer *out_file_out, char *buf, size_t strlength);
int QIO_write_XML(QIO_Writer *out_file_out, XML_MetaData *xml);
int QIO_write_field(QIO_Writer *out, int volfmt, XML_MetaData *xml_record, 
		    void (*get)(char *buf, const int coords[], void *arg),
		    size_t datum_size, void *arg, 
		    DML_Checksum *checksum);
size_t QIO_read_string(QIO_Reader *lrl_file_in, char *buf, size_t buf_size);
size_t QIO_read_XML(QIO_Reader *in, XML_MetaData *xml);
size_t QIO_read_field(QIO_Reader *in, int volfmt, XML_MetaData *xml_record, 
		   void (*put)(char *buf, const int coords[], void *arg),
		   int datum_size, void *arg, 
		   DML_Checksum *checksum);

#ifdef __cplusplus
}
#endif

#endif /* QIO_H */
