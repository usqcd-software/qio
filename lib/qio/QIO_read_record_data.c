/* QIO_read_record_data.c */

#include <qio.h>
#include <lrl.h>
#include <dml.h>
#include <xml_string.h>
#include <qioxml.h>
#include <stdio.h>

#undef QIO_DEBUG

/* Read record data */
/* Can be called separately from QIO_read, but then QIO_read_record_info
   must be called first */

/* On failure calling program must signal abort to all nodes. */

int QIO_read_record_data(QIO_Reader *in, 
	     void (*put)(char *buf, size_t index, size_t count, void *arg),
	     size_t datum_size, int word_size, void *arg){


  char myname[] = "QIO_read_record_data";
  XML_String *xml_record_private,*xml_checksum;
  DML_Checksum checksum;
  QIO_ChecksumInfo *checksum_info_expect, *checksum_info_found;
  size_t check;
  int datum_size_info;
  int this_node = in->layout->this_node;
  int status;
  size_t buf_size = datum_size * in->layout->volume;
  LIME_type lime_type=NULL;

  /* It is an error to call for the data before the reading the info
     in a given record */
  if(in->read_state == QIO_RECORD_XML_NEXT){
    printf("%s(%d): Must call QIO_read_record_info first\n",myname,this_node);
    return QIO_ERR_INFO_MISSED;
  }

  /* Require consistency between the byte count specified in the
     private record metadata and the byte count per site to be read */
  if(datum_size != 
     QIO_get_typesize(&(in->record_info)) *
     QIO_get_datacount(&(in->record_info))){
    printf("%s(%d): bytes per site mismatch %d != %d * %d\n",
	   myname,this_node,datum_size,
	   QIO_get_typesize(&(in->record_info)),
	   QIO_get_datacount(&(in->record_info)));
    return QIO_ERR_BAD_SITE_BYTES;
  }

#ifdef DO_BINX
  /* Master node reads the BinX record. This may be dropped. */
  /* We assume the BinX_record was created by the caller */

  if(this_node == QIO_MASTER_NODE){
    if((status=QIO_read_string(in, BinX, lime_type))!=QIO_SUCCESS){
      printf("%s(%d): Bad BinX record\n",myname,this_node);
      return status;
    }
#ifdef QIO_DEBUG
    printf("%s(%d): BinX = %s\n",myname,this_node,XML_string_ptr(BinX));
#endif
  }
#endif
  
  /* Verify byte count per site */
  datum_size_info = QIO_get_typesize(&(in->record_info)) * 
    QIO_get_datacount(&(in->record_info));
  if(datum_size != datum_size_info){
    printf("%s(%d): byte count per site mismatch request %d != actual %d\n",
	   myname, this_node, datum_size, datum_size_info);
    return QIO_ERR_BAD_SITE_BYTES;
  }

  /* Nodes read the field */
  if((status=QIO_read_field(in, put, datum_size, word_size, arg, 
			    &checksum, lime_type))!=QIO_SUCCESS){
    printf("%s(%d): Error reading field\n",myname,this_node);
    return status;
  }

#ifdef QIO_DEBUG
  printf("%s(%d): done reading field\n",myname,this_node);fflush(stdout);
#endif

  /* Master node reads the checksum */
  status = QIO_SUCCESS;  /* Changed if checksum does not match */
  if(this_node == QIO_MASTER_NODE){
    xml_checksum = XML_string_create(QIO_STRINGALLOC);
    if((status=QIO_read_string(in, xml_checksum, lime_type))!=QIO_SUCCESS){
      printf("%s(%d): Error reading checksum\n",myname,this_node);
      return status;
    }
#ifdef QIO_DEBUG
    printf("%s(%d): checksum = %s\n",myname,this_node,
	   XML_string_ptr(xml_checksum));
#endif
    /* Extract checksum */
    checksum_info_expect = QIO_create_checksum_info(0,0);
    if((status=QIO_decode_checksum_info(checksum_info_expect, xml_checksum))
       !=QIO_SUCCESS){
      printf("%s(%d): bad checksum record\n",myname,this_node);
      return status;
    }
    XML_string_destroy(xml_checksum);
    
    /* Compare checksums */
    checksum_info_found = QIO_create_checksum_info(checksum.suma, 
						   checksum.sumb);
    status = QIO_compare_checksum_info(checksum_info_found, 
				       checksum_info_expect,
				       myname,this_node);
  
    QIO_destroy_checksum_info(checksum_info_expect);
    QIO_destroy_checksum_info(checksum_info_found);
  }

  in->read_state = QIO_RECORD_XML_NEXT;
  
  return status;
}

