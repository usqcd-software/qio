#ifndef QIOXML_H
#define QIOXML_H

#define QIO_MAXTAG 64
#define QIO_MAXVALUESTRING 512
#define QIO_MAXINTARRAY 8
#define QIO_STRINGALLOC 1024
/*#define QIO_FILEFORMATVERSION "1.0"*/
#define QIO_FILEFORMATVERSION "1.1"
#define QIO_RECORDFORMATVERSION "1.0"
#define QIO_CHECKSUMFORMATVERSION "1.0"
#define QIO_QUESTXML "?xml"
#define QIO_XMLINFO "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"

#include <qio_string.h>
#include <inttypes.h>

#ifdef __cplusplus
extern "C"
{
#endif
/*******************************************************************/
/* Datatype structures */

typedef struct {
  char tag[QIO_MAXTAG];
  char value[QIO_MAXVALUESTRING];
  short occur;
} QIO_TagCharValue;

typedef struct {
  char tag[QIO_MAXTAG];
  uint32_t value;     /* Must be 32-bit type */
  short occur;
} QIO_TagHex32Value;

typedef struct {
  char tag[QIO_MAXTAG];
  int  value;
  short occur;
} QIO_TagIntValue;

typedef struct {
  char tag[QIO_MAXTAG];
  int  value[QIO_MAXINTARRAY];
  int  n;
  short occur;
} QIO_TagIntArrayValue;


/*******************************************************************/
/* Top level wrapper for private record XML

   tag           member           description          
   ------------------------------------------------------------
   scidacRecord  recordinfo_tags  string of private record tags (see below)

*/

typedef struct {
  QIO_TagCharValue     recordinfo_tags;
} QIO_RecordInfoWrapper;

#define QIO_RECORD_INFO_WRAPPER {\
  {"scidacRecord", "" , 0}       \
}


/*******************************************************************/
/* Contents of private record XML

   tag        member       description                  e.g. gauge config
   ------------------------------------------------------------
   version    version      record format version number    1.0
   date       date         creation date in UT     Wed Oct 22 14:58:08 UTC 2003
   globaldata globaldata   1 if global 0 if field          0 
   datatype   datatype     QLA type                        ColorMatrix 
   precision  precision	   I, F, D, or S (random no state) F
   colors     colors       number of colors	           3
   spins      spins        number of spins	 	   --
   typesize   typesize     byte length of datum	           72           
   datacount  datacount    number of data per site	   4

*/

typedef struct {
  QIO_TagCharValue version    ;
  QIO_TagCharValue date       ;
  QIO_TagIntValue  globaldata ;
  QIO_TagCharValue datatype   ;
  QIO_TagCharValue precision  ;
  QIO_TagIntValue  colors     ;
  QIO_TagIntValue  spins      ;
  QIO_TagIntValue  typesize   ;
  QIO_TagIntValue  datacount  ;
} QIO_RecordInfo;

#define QIO_RECORD_INFO_TEMPLATE { \
  {"version",   "", 0},         \
  {"date",      "", 0},         \
  {"globaldata",0 , 0},         \
  {"datatype",  "", 0},         \
  {"precision", "", 0},         \
  {"colors",    0 , 0},         \
  {"spins",     0 , 0},         \
  {"typesize",  0 , 0},         \
  {"datacount", 0 , 0}          \
}

/*******************************************************************/
/* Top level wrapper for private file XML

   tag           member           description          
   ------------------------------------------------------------
   scidacFile  fileinfo_tags  string of private file tags (see below)

*/

typedef struct {
  QIO_TagCharValue     fileinfo_tags;
} QIO_FileInfoWrapper;

#define QIO_FILE_INFO_WRAPPER {\
  {"scidacFile", "" , 0}       \
}


/*******************************************************************/
/* Contents of private file XML

   tag        member       description                  e.g. gauge config
   ------------------------------------------------------------
   version    version      file format version number      1.0
   spacetime  spacetime    dimensions of space plus time   4
   dims       dims         lattice dimensions              20 20 20 64
   volfmt     volfmt       QIO_SINGLEFILE, QIO_PARTFILE, 
                           QIO_MULTIFILE                   0
*/

typedef struct {
  QIO_TagCharValue      version;
  QIO_TagIntValue       spacetime;
  QIO_TagIntArrayValue  dims;
  QIO_TagIntValue       volfmt;
} QIO_FileInfo;

#define QIO_FILE_INFO_TEMPLATE  {\
  {"version",   "", 0},       \
  {"spacetime", 0,  0},       \
  {"dims",      {0}, 1, 0},   \
  {"volfmt",  0 , 0}       \
}

/* Obsolete version 1.0 format */

typedef struct {
  QIO_TagCharValue      version;
  QIO_TagIntValue       spacetime;
  QIO_TagIntArrayValue  dims;
  QIO_TagIntValue       multifile;
} QIO_FileInfo_v1p0;


#define QIO_FILE_INFO_TEMPLATE_v1p0  {\
  {"version",   "", 0},       \
  {"spacetime", 0,  0},       \
  {"dims",      {0}, 1, 0},   \
  {"multifile",  0 , 0}       \
}


/*******************************************************************/
/* Top level wrapper for private checksum XML

   tag           member           description          
   ------------------------------------------------------------
   scidacChecksum  checksuminfo_tags  string of checksum tags (see below)

*/

typedef struct {
  QIO_TagCharValue     checksuminfo_tags;
} QIO_ChecksumInfoWrapper;

#define QIO_CHECKSUM_INFO_WRAPPER {\
  {"scidacChecksum", "" , 0}       \
}


/*******************************************************************/
/* Contents of record checksum XML

   tag        member       description
   ------------------------------------------------------------
   version    version      checksum version number      1.0
   suma       suma         
   sumb       sumb         
*/

typedef struct {
  QIO_TagCharValue      version;
  QIO_TagHex32Value    suma;
  QIO_TagHex32Value    sumb;
} QIO_ChecksumInfo;

#define QIO_CHECKSUM_INFO_TEMPLATE  {\
  {"version",   "", 0},       \
  {"suma",      0,  0},       \
  {"sumb",      0,  0}        \
}

/*******************************************************************/

int QIO_decode_file_info(QIO_FileInfo *file_info, 
			 QIO_String *file_string);
void QIO_encode_file_info(QIO_String *file_string, 
			   QIO_FileInfo *file_info);
int QIO_decode_record_info(QIO_RecordInfo *record_info, 
			    QIO_String *record_string);
void QIO_encode_record_info(QIO_String *record_string, 
			      QIO_RecordInfo *record_info);
int QIO_decode_checksum_info(QIO_ChecksumInfo *checksum, 
			     QIO_String *file_string);
void QIO_encode_checksum_info(QIO_String *file_string, 
			      QIO_ChecksumInfo *checksum);

int QIO_insert_file_tag_string(QIO_FileInfoWrapper *wrapper, 
			       char *fileinfo_tags);
int QIO_insert_spacetime_dims(QIO_FileInfo *file_info, 
			      int spacetime, int *dims);
int QIO_insert_volfmt(QIO_FileInfo *file_info, int volfmt);

int QIO_insert_record_tag_string(QIO_RecordInfoWrapper *wrapper, 
				 char *recordinfo_tags);
int QIO_insert_record_date(QIO_RecordInfo *record_info, char* date);
int QIO_insert_globaldata(QIO_RecordInfo *record_info, int globaldata);
int QIO_insert_datatype(QIO_RecordInfo *record_info, char* datatype);
int QIO_insert_precision(QIO_RecordInfo *record_info, char* precision);
int QIO_insert_colors(QIO_RecordInfo *record_info, int colors);
int QIO_insert_spins(QIO_RecordInfo *record_info, int spins);
int QIO_insert_typesize(QIO_RecordInfo *record_info, int typesize);
int QIO_insert_datacount(QIO_RecordInfo *record_info, int datacount);

int QIO_insert_checksum_tag_string(QIO_ChecksumInfoWrapper *wrapper, 
				   char *checksuminfo_tags);
int QIO_insert_suma_sumb(QIO_ChecksumInfo *checksum_info, 
			 uint32_t suma, uint32_t sumb);

char *QIO_get_file_info_tag_string(QIO_FileInfoWrapper *wrapper);
char *QIO_get_file_version(QIO_FileInfo *file_info);
int QIO_get_spacetime(QIO_FileInfo *file_info);
int *QIO_get_dims(QIO_FileInfo *file_info);
int QIO_get_volfmt(QIO_FileInfo *file_info);
int QIO_get_multifile(QIO_FileInfo_v1p0 *file_info);
int QIO_defined_spacetime(QIO_FileInfo *file_info);
int QIO_defined_dims(QIO_FileInfo *file_info);
int QIO_defined_volfmt(QIO_FileInfo *file_info);

char *QIO_get_record_info_tag_string(QIO_RecordInfoWrapper *wrapper);
char *QIO_get_record_date(QIO_RecordInfo *record_info);
int QIO_get_globaldata(QIO_RecordInfo *record_info);
char *QIO_get_datatype(QIO_RecordInfo *record_info);
char *QIO_get_precision(QIO_RecordInfo *record_info);
int QIO_get_colors(QIO_RecordInfo *record_info);
int QIO_get_spins(QIO_RecordInfo *record_info);
int QIO_get_typesize(QIO_RecordInfo *record_info);
int QIO_get_datacount(QIO_RecordInfo *record_info);

char *QIO_get_checksum_info_tag_string(QIO_ChecksumInfoWrapper *wrapper);
uint32_t QIO_get_suma(QIO_ChecksumInfo *checksum_info);
uint32_t QIO_get_sumb(QIO_ChecksumInfo *checksum_info);
int QIO_defined_suma(QIO_ChecksumInfo *checksum_info);
int QIO_defined_sumb(QIO_ChecksumInfo *checksum_info);

int QIO_defined_record_date(QIO_RecordInfo *record_info);
int QIO_defined_globaldata(QIO_RecordInfo *record_info);
int QIO_defined_datatype(QIO_RecordInfo *record_info);
int QIO_defined_precision(QIO_RecordInfo *record_info);
int QIO_defined_colors(QIO_RecordInfo *record_info);
int QIO_defined_spins(QIO_RecordInfo *record_info);
int QIO_defined_typesize(QIO_RecordInfo *record_info);
int QIO_defined_datacount(QIO_RecordInfo *record_info);

QIO_FileInfo *QIO_create_file_info(int spacetime, int *dims, int volfmt);
void QIO_destroy_file_info(QIO_FileInfo *file_info);
int QIO_compare_file_info(QIO_FileInfo *found, QIO_FileInfo *expect,
			  char *myname, int this_node);
QIO_RecordInfo *QIO_create_record_info(int globaldata,
				       char *datatype, char *precision, 
				       int colors, int spins, int typesize, 
				       int datacount);
void QIO_destroy_record_info(QIO_RecordInfo *record_info);
int QIO_compare_record_info(QIO_RecordInfo *r, QIO_RecordInfo *s);

QIO_ChecksumInfo *QIO_create_checksum_info(uint32_t suma, uint32_t sumb);
void QIO_destroy_checksum_info(QIO_ChecksumInfo *checksum_info);
int QIO_compare_checksum_info(QIO_ChecksumInfo *found, 
			      QIO_ChecksumInfo *expect, 
			      char *myname, int this_node);

#ifdef __cplusplus
}
#endif

#endif /* QIOXML_H */
