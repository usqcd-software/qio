/* QIO_read_record_data.c */

#include <qio.h>
#include <lrl.h>
#include <dml.h>
#include <qio_string.h>
#include <qioxml.h>
#include <stdio.h>

#undef QIO_DEBUG

/* Read record data */
/* Can be called separately from QIO_read, but then QIO_read_record_info
   must be called first */

/* On failure calling program must signal abort to all nodes. */

int QIO_read_record_data(QIO_Reader *in, 
	     void (*put)(char *buf, size_t index, int count, void *arg),
	     size_t datum_size, int word_size, void *arg){


  char myname[] = "QIO_read_record_data";
  QIO_String *xml_checksum;
  DML_Checksum checksum;
  QIO_ChecksumInfo *checksum_info_expect, *checksum_info_found;
  int count = QIO_get_datacount(&(in->record_info));
  size_t datum_size_info;
  int this_node = in->layout->this_node;
  int status;
  int globaldata = QIO_get_globaldata(&(in->record_info));
  LIME_type lime_type=NULL;

  /* It is an error to call for the data before the reading the info
     in a given record */
  if(in->read_state == QIO_RECORD_XML_NEXT){
    printf("%s(%d): Must call QIO_read_record_info first\n",myname,this_node);
    return QIO_ERR_INFO_MISSED;
  }

  /* Require consistency between the byte count obtained from the
     private record metadata and the datum_size and count parameters
     (per site for field data or total for global data) */
  if(datum_size != 
     QIO_get_typesize(&(in->record_info)) * count){
    if(this_node == in->layout->master_io_node){
      printf("%s(%d): requested byte count %lu disagrees with the record %d * %d\n",
	     myname,this_node,(unsigned long)datum_size,
	     QIO_get_typesize(&(in->record_info)), count);
      printf("%s(%d): Record header says datatype %s globaltype %d \n                         precision %s colors %d spins %d count %d\n",
	     myname,this_node,
	     QIO_get_datatype(&(in->record_info)),
	     QIO_get_globaldata(&(in->record_info)),
	     QIO_get_precision(&(in->record_info)),
	     QIO_get_colors(&(in->record_info)),
	     QIO_get_spins(&(in->record_info)),
	     QIO_get_datacount(&(in->record_info)));
    }
    return QIO_ERR_BAD_SITELIST;
  }

#ifdef DO_BINX
  /* Master node reads the BinX record. This may be dropped. */
  /* We assume the BinX_record was created by the caller */

  if(this_node == in->layout->master_io_node){
    if((status=QIO_read_string(in, BinX, lime_type))!=QIO_SUCCESS){
      printf("%s(%d): Bad BinX record\n",myname,this_node);
      return status;
    }
#ifdef QIO_DEBUG
    printf("%s(%d): BinX = %s\n",myname,this_node,QIO_string_ptr(BinX));
#endif
  }
#endif
  
  /* Verify byte count per site (for field) or total (for global) */
  datum_size_info = QIO_get_typesize(&(in->record_info)) * count;
  if(datum_size != datum_size_info){
    printf("%s(%d): byte count mismatch request %lu != actual %lu\n",
	   myname, this_node, (unsigned long)datum_size, 
	   (unsigned long)datum_size_info);

    return QIO_ERR_BAD_SITELIST;
  }

  /* Nodes read the field */
  if((status=QIO_read_field(in, globaldata, put, count, datum_size, word_size, 
			    arg, &checksum, lime_type))!=QIO_SUCCESS){
    printf("%s(%d): Error reading field data\n",myname,this_node);
    return status;
  }

#ifdef QIO_DEBUG
  printf("%s(%d): done reading field\n",myname,this_node);fflush(stdout);
#endif

  /* Master node reads the checksum */
  status = QIO_SUCCESS;  /* Changed if checksum does not match */
  if(this_node == in->layout->master_io_node){
    xml_checksum = QIO_string_create();
    QIO_string_realloc(xml_checksum,QIO_STRINGALLOC);
    if((status=QIO_read_string(in, xml_checksum, lime_type))
       !=QIO_SUCCESS){
      printf("%s(%d): Error reading checksum\n",myname,this_node);
      return status;
    }
#ifdef QIO_DEBUG
    printf("%s(%d): checksum = %s\n",myname,this_node,
	   QIO_string_ptr(xml_checksum));
#endif
    /* Extract checksum */
    checksum_info_expect = QIO_create_checksum_info(0,0);
    if((status=QIO_decode_checksum_info(checksum_info_expect, xml_checksum))
       !=QIO_SUCCESS){
      printf("%s(%d): bad checksum record\n",myname,this_node);
      return status;
    }
    QIO_string_destroy(xml_checksum);
    
    printf("%s(%d): Read field. datatype %s globaltype %d \n                         precision %s colors %d spins %d count %d\n",
	   myname,this_node,
	   QIO_get_datatype(&(in->record_info)),
	   QIO_get_globaldata(&(in->record_info)),
	   QIO_get_precision(&(in->record_info)),
	   QIO_get_colors(&(in->record_info)),
	   QIO_get_spins(&(in->record_info)),
	   QIO_get_datacount(&(in->record_info)));

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

