#ifndef QIO_H
#define QIO_H

#include <qio_config.h>
#include <qioxml.h>
#include <qio_string.h>
#include <lrl.h>
#include <dml.h>
#include <lime.h>

#define QIO_SINGLEFILE DML_SINGLEFILE 
#define QIO_MULTIFILE  DML_MULTIFILE  
#define QIO_PARTFILE   DML_PARTFILE

#define QIO_FIELD      DML_FIELD
#define QIO_GLOBAL     DML_GLOBAL

#define QIO_SERIAL     DML_SERIAL
#define QIO_PARALLEL   DML_PARALLEL

#define QIO_TRUNC      DML_TRUNC
#define QIO_APPEND     DML_APPEND

/* Return codes */

#define QIO_SUCCESS               (  0)
#define QIO_EOF                   ( -1)
#define QIO_ERR_BAD_WRITE_BYTES   ( -2)
#define QIO_ERR_OPEN_READ         ( -3)
#define QIO_ERR_OPEN_WRITE        ( -4)
#define QIO_ERR_BAD_READ_BYTES    ( -5)
#define QIO_ERR_ALLOC             ( -6)
#define QIO_ERR_CLOSE             ( -7)
#define QIO_ERR_INFO_MISSED       ( -8)
#define QIO_ERR_BAD_SITELIST      ( -9)
#define QIO_ERR_PRIVATE_FILE_INFO (-10)
#define QIO_ERR_PRIVATE_REC_INFO  (-11)
#define QIO_BAD_XML               (-12)
#define QIO_BAD_ARG               (-13)
#define QIO_CHECKSUM_MISMATCH     (-14)
#define QIO_ERR_FILE_INFO         (-15)
#define QIO_ERR_REC_INFO          (-16)
#define QIO_ERR_CHECKSUM_INFO     (-17)
#define QIO_ERR_SKIP              (-18)
#define QIO_ERR_BAD_TOTAL_BYTES   (-19)
#define QIO_ERR_BAD_GLOBAL_TYPE   (-20)
#define QIO_ERR_BAD_VOLFMT        (-21)
#define QIO_ERR_BAD_IONODE        (-22)

#ifdef __cplusplus
extern "C"
{
#endif
  
/* For collecting and passing layout information */
typedef struct {
  int (*node_number)(const int coords[]);
  int (*node_index)(const int coords[]);
  void (*get_coords)(int coords[], int node, int index);
  int *latsize;
  int latdim;
  size_t volume;
  size_t sites_on_node;
  int this_node;
  int number_of_nodes;
} QIO_Layout;

typedef struct {
  LRL_FileWriter *lrl_file_out;
  int volfmt;
  int serpar;
  DML_Layout *layout;
  DML_SiteList *sites;
  DML_Checksum last_checksum;
} QIO_Writer;

#define QIO_RECORD_INFO_PRIVATE_NEXT 0
#define QIO_RECORD_INFO_USER_NEXT 1
#define QIO_RECORD_DATA_NEXT 2
#define QIO_RECORD_CHECKSUM_NEXT 3

typedef struct {
  LRL_FileReader *lrl_file_in;
  int volfmt;
  int serpar;
  DML_Layout *layout;
  DML_SiteList *sites;
  int read_state;
  QIO_String *xml_record;
  QIO_RecordInfo record_info;
  DML_Checksum last_checksum;
} QIO_Reader;

typedef int QIO_ioflag;

/* Support for host file conversion */

#define QIO_SINGLE_PATH 0
#define QIO_MULTI_PATH 1

typedef struct {
  int number_io_nodes;
  int type;                           /* Is node_path specified? */
  int (*my_io_node)(const int node);  /* Mapping as on compute nodes */
  int (*master_io_node)(void);        /* As on compute nodes */
  int *io_node;                       /* Only if number_io_nodes !=
					 number_of_nodes */
  char **node_path;                   /* Only if type = QIO_MULTI_PATH */
} QIO_Filesystem;

/* Internal host file conversion utilities in QIO_host_utils.c */
QIO_Layout *QIO_create_ionode_layout(QIO_Layout *layout, QIO_Filesystem *fs);
void QIO_delete_ionode_layout(QIO_Layout* layout);
QIO_Layout *QIO_create_scalar_layout(QIO_Layout *layout, QIO_Filesystem *fs);
void QIO_delete_scalar_layout(QIO_Layout *layout);
int QIO_ionode_io_node(int node);
int QIO_get_io_node_rank(int node);
int QIO_ionode_to_scalar_index(int ionode_node, int ionode_index);
int QIO_scalar_to_ionode_index(int scalar_node, int scalar_index);

/* Verbosity */
int QIO_verbose(int level);
int QIO_verbosity();

/* Enumerate in order of increasing verbosity */
#define QIO_VERB_OFF    0
#define QIO_VERB_LOW    1
#define QIO_VERB_REG    2
#define QIO_VERB_DEBUG  3

/* HostAPI */
int QIO_single_to_part( const char filename[], QIO_Filesystem *fs,
			QIO_Layout *layout);
int QIO_part_to_single( const char filename[], QIO_Filesystem *fs,
			QIO_Layout *layout);

/* MPP API */
QIO_Writer *QIO_open_write(QIO_String *xml_file, const char *filename, 
			   int volfmt, QIO_Layout *layout, QIO_ioflag oflag);
QIO_Reader *QIO_open_read(QIO_String *xml_file, const char *filename, 
			   QIO_Layout *layout, QIO_ioflag iflag);
int QIO_get_reader_latdim(QIO_Reader *in);
int *QIO_get_reader_latsize(QIO_Reader *in);
uint32_t QIO_get_reader_last_checksuma(QIO_Reader *in);
uint32_t QIO_get_reader_last_checksumb(QIO_Reader *in);

uint32_t QIO_get_writer_last_checksuma(QIO_Writer *out);
uint32_t QIO_get_writer_last_checksumb(QIO_Writer *out);

int QIO_close_write(QIO_Writer *out);
int QIO_close_read(QIO_Reader *in);

int QIO_write(QIO_Writer *out, QIO_RecordInfo *record_info,
	      QIO_String *xml_record, 
	      void (*get)(char *buf, size_t index, int count, void *arg),
	      size_t datum_size, int word_size, void *arg);
int QIO_read(QIO_Reader *in, QIO_RecordInfo *record_info,
	     QIO_String *xml_record, 
	     void (*put)(char *buf, size_t index, int count, void *arg),
	     size_t datum_size, int word_size, void *arg);
int QIO_read_record_info(QIO_Reader *in, QIO_RecordInfo *record_info,
			 QIO_String *xml_record);
int QIO_read_record_data(QIO_Reader *in, 
		 void (*put)(char *buf, size_t index, int count, void *arg),
		 size_t datum_size, int word_size, void *arg);
int QIO_next_record(QIO_Reader *in);

/* Internal utilities  */
QIO_Reader *QIO_open_read_master(const char *filename, 
			  QIO_Layout *layout, QIO_ioflag iflag,
			 int (*io_node)(int), int (*master_io_node)());
int QIO_open_read_nonmaster(QIO_Reader *qio_in, const char *filename);
int QIO_read_check_sitelist(QIO_Reader *qio_in);
int QIO_read_user_file_xml(QIO_String *xml_file, QIO_Reader *qio_in);
QIO_Writer *QIO_generic_open_write(QIO_String *xml_file, const char *filename, 
				   int volfmt, QIO_Layout *layout, 
				   QIO_ioflag oflag, 
				   int (*io_node)(int), 
				   int (*master_io_node)());
int QIO_read_private_record_info(QIO_Reader *in, QIO_RecordInfo *record_info);
int QIO_read_user_record_info(QIO_Reader *in, QIO_String *xml_record);
int QIO_generic_read_record_data(QIO_Reader *in, 
	     void (*put)(char *buf, size_t index, int count, void *arg),
	     size_t datum_size, int word_size, void *arg,
 	     DML_Checksum *checksum, uint64_t *nbytes);
QIO_ChecksumInfo *QIO_read_checksum(QIO_Reader *in);
int QIO_compare_checksum(int this_node,
	 QIO_ChecksumInfo *checksum_info_expect, DML_Checksum *checksum);

int QIO_generic_write(QIO_Writer *out, QIO_RecordInfo *record_info, 
	      QIO_String *xml_record, 
	      void (*get)(char *buf, size_t index, int count, void *arg),
	      size_t datum_size, int word_size, void *arg,
	      DML_Checksum *checksum, uint64_t *nbytes,
	      int *msg_begin, int *msg_end);

char *QIO_filename_edit(const char *filename, int volfmt, int this_node);
int QIO_write_string(QIO_Writer *out, int msg_begin, int msg_end,
		     QIO_String *xml,
		     const LIME_type lime_type);
int QIO_read_string(QIO_Reader *in,
		    QIO_String *xml, LIME_type *lime_type);
DML_SiteList *QIO_create_sitelist(DML_Layout *layout, int volfmt);
int QIO_read_sitelist(QIO_Reader *in, LIME_type *lime_type);
int QIO_write_sitelist(QIO_Writer *out, int msg_begin, int msg_end, 
		       const LIME_type lime_type);
int QIO_read_field(QIO_Reader *in, int globaldata,
	   void (*put)(char *buf, size_t index, int count, void *arg),
	   int count, size_t datum_size, int word_size, void *arg, 
	   DML_Checksum *checksum, uint64_t* nbytes,
	   LIME_type *lime_type);
int QIO_write_field(QIO_Writer *out, int msg_begin, int msg_end,
	    QIO_String *xml_record, int globaldata,
	    void (*get)(char *buf, size_t index, int count, void *arg),
	    int count, size_t datum_size, int word_size, void *arg, 
	    DML_Checksum *checksum, uint64_t *nbytes,
	    const LIME_type lime_type);
#ifdef __cplusplus
}
#endif

#endif /* QIO_H */
