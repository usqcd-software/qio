/* QIO Utilities */

#include <qio.h>
#include <lrl.h>
#include <dml.h>
#include <xml.h>
#include <string.h>
#include <stdio.h>

/* Unused dummy for now */
char dime_XML[] = "XML DIME tag";

int QIO_write_string(QIO_Writer *out, char *buf, size_t strlength){
  LRL_RecordWriter *lrl_record_out;
  size_t check;
  size_t rec_size = strlength + 1;  /* Include terminating null */

  lrl_record_out = LRL_open_write_record(out->lrl_file_out, &rec_size, 
					 dime_XML);
  check = LRL_write_bytes(lrl_record_out, buf, rec_size);
  /* Check byte count */
  if(check != rec_size){
    printf("QIO_write_string: check %d -ne rec_size %d\n",check,rec_size);
    return 1;
  }
  LRL_close_write_record(lrl_record_out);
  return 0;
}

int QIO_write_XML(QIO_Writer *out, XML_MetaData *xml){
  size_t xml_size;
  char *xml_string;

  xml_string = XML_string(xml);
  xml_size = strlen(xml_string);
  return QIO_write_string(out, xml_string, xml_size);
}

int QIO_write_field(QIO_Writer *out, int volfmt, XML_MetaData *xml_record, 
		    void (*get)(char *buf, const int coords[], void *arg),
		    size_t datum_size, void *arg, 
		    DML_Checksum *checksum){
  
  LRL_RecordWriter *lrl_record_out;
  int status;
  size_t rec_size;
  size_t volume = out->layout->volume;

  printf("QIO_write_field\n");

  /* Compute record size */
  rec_size = volume * datum_size;

  lrl_record_out = LRL_open_write_record(out->lrl_file_out, &rec_size, 
					 dime_XML);

  status = DML_stream_out(lrl_record_out, get, datum_size, arg, out->layout, 
			  out->serpar, out->siteorder, 
			  out->volfmt, checksum);

  /* Close record when done and clean up*/
  LRL_close_write_record(lrl_record_out);

  return status;
}


size_t QIO_read_string(QIO_Reader *in, char *buf, size_t buf_size){
  LRL_RecordReader *lrl_record_in;
  size_t check,rec_size;

  lrl_record_in = LRL_open_read_record(in->lrl_file_in, &rec_size, dime_XML);

  /* Guard against buffer overrun */
  if(rec_size > buf_size){
    printf("QIO_read_string: rec_size %d > buf_size %d\n",
	   rec_size, buf_size);
    return 0;
  }

  check = LRL_read_bytes(lrl_record_in, buf, rec_size);
  LRL_close_read_record(lrl_record_in);
  return check;
}

size_t QIO_read_XML(QIO_Reader *in, XML_MetaData *xml){
  size_t xml_size;
  char *xml_string;

  xml_size = XML_bytes(xml);   /* The size allocated for the string */
  xml_string = XML_string(xml);
  return QIO_read_string(in, xml_string, xml_size);
}

size_t QIO_read_field(QIO_Reader *in, int volfmt, XML_MetaData *xml_record, 
		      void (*put)(char *buf, const int coords[], void *arg),
		      int datum_size, void *arg, 
		      DML_Checksum *checksum){

  LRL_RecordReader *lrl_record_in;
  size_t rec_size, check;
  size_t volume = in->layout->volume;

  lrl_record_in = LRL_open_read_record(in->lrl_file_in, &rec_size, dime_XML);

  check = DML_stream_in(lrl_record_in, put, datum_size, arg, in->layout,
			in->serpar, in->siteorder, in->sitelist, 
			in->volfmt,  checksum);
  
  /* Close record when done and clean up*/
  LRL_close_read_record(lrl_record_in);

  return check;
}

