#ifndef QIOXML_H
#define QIOXML_H

#define QIO_MAXTAG 64
#define QIO_MAXATTR 512
#define QIO_MAXVALUESTRING 1024
#define QIO_MAXINTARRAY 8
#define QIO_STRINGALLOC 1024
/*#define QIO_FILEFORMATVERSION "1.0"*/
#define QIO_ILDGFORMATVERSION "1.0"
#define QIO_FILEFORMATVERSION "1.1"
#define QIO_RECORDFORMATVERSION "1.0"
#define QIO_CHECKSUMFORMATVERSION "1.0"
#define QIO_QUESTXML "?xml"
#define QIO_XMLINFO "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"

#define QIO_NONSCIDAC_FILE "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<NonSciDACFile/>\n"

#define QIO_NONSCIDAC_RECORD "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<NonSciDACRecord/>\n"
#include <qio_string.h>
#include <qio_stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif
/*******************************************************************/
/* Datatype structures */

typedef struct {
  char tag[QIO_MAXTAG];
  char attr[QIO_MAXATTR];
  char value[QIO_MAXVALUESTRING];
  short occur;
} QIO_TagCharValue;

typedef struct {
  char tag[QIO_MAXTAG];
  char attr[QIO_MAXATTR];
  uint32_t value;     /* Must be 32-bit type */
  short occur;
} QIO_TagHex32Value;

typedef struct {
  char tag[QIO_MAXTAG];
  char attr[QIO_MAXATTR];
  int  value;
  short occur;
} QIO_TagIntValue;

typedef struct {
  char tag[QIO_MAXTAG];
  char attr[QIO_MAXATTR];
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
  {"scidacRecord", "", "" , 0}       \
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
  {"version",   "", "", 0},         \
  {"date",      "", "", 0},         \
  {"globaldata","", 0 , 0},         \
  {"datatype",  "", "", 0},         \
  {"precision", "", "", 0},         \
  {"colors",    "", 0 , 0},         \
  {"spins",     "", 0 , 0},         \
  {"typesize",  "", 0 , 0},         \
  {"datacount", "", 0 , 0}          \
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
  {"scidacFile", "", "" , 0}       \
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
  {"version",  "",  "", 0},       \
  {"spacetime","",  0,  0},       \
  {"dims",     "",  {0}, 1, 0},   \
  {"volfmt",   "", 0 , 0}       \
}

/* Obsolete version 1.0 format */

typedef struct {
  QIO_TagCharValue      version;
  QIO_TagIntValue       spacetime;
  QIO_TagIntArrayValue  dims;
  QIO_TagIntValue       multifile;
} QIO_FileInfo_v1p0;


#define QIO_FILE_INFO_TEMPLATE_v1p0  {\
  {"version",   "", "", 0},       \
  {"spacetime", "", 0,  0},       \
  {"dims",      "", {0}, 1, 0},   \
  {"multifile", "",  0 , 0}       \
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
  {"scidacChecksum", "", "" , 0}       \
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
  {"version",   "", "", 0},       \
  {"suma",      "", 0,  0},       \
  {"sumb",      "", 0,  0}        \
}

/*******************************************************************/
/* Top level wrapper for ILDG Lattice XML

   tag           member           description          
   ------------------------------------------------------------
   ildgFormat  ildgformat_tags  string of ILDG format tags (see below)

*/

typedef struct {
  QIO_TagCharValue     ildgformatinfo_tags;
} QIO_ILDGFormatInfoWrapper;

#define QIO_ILDGFORMATSCHEMA "xmlns=\"http://www.lqcd.org/ildg\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://www.lqcd.org/ildg/filefmt.xsd\""

#define QIO_ILDG_FORMAT_INFO_WRAPPER {\
  {"ildgFormat", QIO_ILDGFORMATSCHEMA, "" , 0}       \
}


/*******************************************************************/
/* Contents of ILDG format XML

   tag        member       description                  e.g. gauge config
   ------------------------------------------------------------
   version    version      ILDG format version number      1.0
   precision  precision	   32 or 64
   field      field        "su3_gauge"
   lx         lx           x dimension
   ly         ly           y dimension
   lz         lz           z dimension
   lt         lt           t dimension
*/

typedef struct {
  QIO_TagCharValue version    ;
  QIO_TagCharValue field      ;
  QIO_TagIntValue  precision  ;
  QIO_TagIntValue  lx         ;
  QIO_TagIntValue  ly         ;
  QIO_TagIntValue  lz         ;
  QIO_TagIntValue  lt         ;
} QIO_ILDGFormatInfo;

#define QIO_ILDG_FORMAT_INFO_TEMPLATE { \
  {"version",   "", "", 0},         \
  {"field",     "", "", 0},         \
  {"precision", "", 0 , 0},         \
  {"lx",        "", 0 , 0},         \
  {"ly",        "", 0 , 0},         \
  {"lz",        "", 0 , 0},         \
  {"lt",        "", 0 , 0}          \
}

/*********************************************************************/
/* Top level wrapper for USQCD lattice record XML 

   tag           member           description          
   ------------------------------------------------------------
   
*/

typedef struct {
  QIO_TagCharValue usqcdlatticeinfo_tags;
} QIO_USQCDLatticeInfoWrapper;

#define QIO_USQCD_LATTICE_INFO_WRAPPER {\
  {"usqcdInfo", "", "" , 0}       \
}

/*******************************************************************/
/* Contents of USQCD lattice record XML

   tag           member           description          
   ------------------------------------------------------------
   version       version        lattice record version number   1.0
   plaq          plaq           Re Tr U_P/3    average plaquette 
   linktr        linktr         Re Tr U_mu/3   average trace of all gauge links
   info          info           XML string     collaboration option
*/


typedef struct {
  QIO_TagCharValue version ;
  QIO_TagCharValue plaq;
  QIO_TagCharValue linktr;
  QIO_TagCharValue info;
} QIO_USQCDLatticeInfo;


#define QIO_USQCDLATTICEFORMATVERSION "1.0"

#define QIO_USQCD_LATTICE_INFO_TEMPLATE {\
   {"version", "", "", 0}, \
   {"plaq"   , "", "", 0}, \
   {"linktr" , "", "", 0}, \
   {"info"   , "", "", 0}  \
}

/*********************************************************************/
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
int QIO_decode_ILDG_format_info(QIO_ILDGFormatInfo *ildg_info, 
				QIO_String *ildg_string);
void QIO_encode_ILDG_format_info(QIO_String *ildg_string, 
				 QIO_ILDGFormatInfo *ildg_info);
void QIO_encode_usqcd_lattice_info(QIO_String *record_string, 
				     QIO_USQCDLatticeInfo *record_info);
int QIO_decode_usqcd_lattice_info(QIO_USQCDLatticeInfo *record_info,
				    QIO_String *record_string);


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

int QIO_insert_ildgformat_tag_string(QIO_ILDGFormatInfoWrapper *wrapper, 
				     char *ildgformatinfo_tags);
int QIO_insert_ildgformat_version(QIO_ILDGFormatInfo *ildg_info, 
				  char *version);
int QIO_insert_ildgformat_field(QIO_ILDGFormatInfo *ildg_info, 
				char *field_string);
int QIO_insert_ildgformat_precision(QIO_ILDGFormatInfo *ildg_info, 
				    int precision);
int QIO_insert_ildgformat_lx(QIO_ILDGFormatInfo *ildg_info, int lx);
int QIO_insert_ildgformat_ly(QIO_ILDGFormatInfo *ildg_info, int ly);
int QIO_insert_ildgformat_lz(QIO_ILDGFormatInfo *ildg_info, int lz);
int QIO_insert_ildgformat_lt(QIO_ILDGFormatInfo *ildg_info, int lt);

int QIO_insert_usqcdlattice_version(QIO_USQCDLatticeInfo *record_info, char *version);
int QIO_insert_usqcdlattice_plaq( QIO_USQCDLatticeInfo *record_info, char *plaq);
int QIO_insert_usqcdlattice_linktr( QIO_USQCDLatticeInfo *record_info, char *linktr);
int QIO_insert_usqcdlattice_info( QIO_USQCDLatticeInfo *record_info, char *info);
int QIO_insert_usqcdlattice_tag_string(QIO_USQCDLatticeInfoWrapper *wrapper,
					 char *recordinfo_tags);

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

void QIO_set_globaldata(QIO_RecordInfo *record_info, int globaldata);
void QIO_set_datatype(QIO_RecordInfo *record_info, char *datatype);
void QIO_set_precision(QIO_RecordInfo *record_info, char *precision);
void QIO_set_record_date(QIO_RecordInfo *record_info, char *date);
void QIO_set_colors(QIO_RecordInfo *record_info, int colors);
void QIO_set_spins(QIO_RecordInfo *record_info, int spins);
void QIO_set_typesize(QIO_RecordInfo *record_info, int typesize);
void QIO_set_datacount(QIO_RecordInfo *record_info, int datacount);

char *QIO_get_checksum_info_tag_string(QIO_ChecksumInfoWrapper *wrapper);
uint32_t QIO_get_suma(QIO_ChecksumInfo *checksum_info);
uint32_t QIO_get_sumb(QIO_ChecksumInfo *checksum_info);
int QIO_defined_suma(QIO_ChecksumInfo *checksum_info);
int QIO_defined_sumb(QIO_ChecksumInfo *checksum_info);

char *QIO_get_ildgformat_info_tag_string(QIO_ILDGFormatInfoWrapper *wrapper);
char *QIO_get_ildgformat_field(QIO_ILDGFormatInfo *ildg_info);
int QIO_get_ildgformat_precision(QIO_ILDGFormatInfo *ildg_info);
int QIO_get_ildgformat_lx(QIO_ILDGFormatInfo *ildg_info);
int QIO_get_ildgformat_ly(QIO_ILDGFormatInfo *ildg_info);
int QIO_get_ildgformat_lz(QIO_ILDGFormatInfo *ildg_info);
int QIO_get_ildgformat_lt(QIO_ILDGFormatInfo *ildg_info);

char *QIO_get_usqcd_lattice_info_tag_string(QIO_USQCDLatticeInfoWrapper *wrapper);
char *QIO_get_plaq(QIO_USQCDLatticeInfo *record_info);
char *QIO_get_linktr(QIO_USQCDLatticeInfo *record_info);
char *QIO_get_info(QIO_USQCDLatticeInfo *record_info);
int QIO_defined_plaq(QIO_USQCDLatticeInfo *record_info);
int QIO_defined_linktr(QIO_USQCDLatticeInfo *record_info);
int QIO_defined_info(QIO_USQCDLatticeInfo *record_info);

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
QIO_ILDGFormatInfo *QIO_create_ildg_format_info(int precision, int *dims);
void QIO_destroy_ildg_format_info(QIO_ILDGFormatInfo *ildg_info);

QIO_USQCDLatticeInfo *QIO_create_usqcd_lattice_info(char *plaq, char *linktr, char *info);
void QIO_destroy_usqcd_lattice_info(QIO_USQCDLatticeInfo *record_info);


#ifdef __cplusplus
}
#endif

#endif /* QIOXML_H */
