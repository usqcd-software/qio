/* QIO_read.c */

#include <qio.h>
#include <lrl.h>
#include <dml.h>
#include <xml.h>
#include <stdio.h>
#include <string.h>

int QIO_read(QIO_Reader *in, XML_string *xml_record, XML_string *BinX,
	     void (*put)(char *buf, const int coords[], void *arg),
	     int datum_size, void *arg){
  /* Return status 0 for success, 1 for failure */

  XML_string *xml_record_private;
  DML_Checksum checksum;
  XML_string *xml_checksum;
  size_t check;
  int this_node = in->layout->this_node;
  size_t buf_size = datum_size * in->layout->volume;
  DIME_type dime_type=NULL;

  /* Initialize private record XML - will be allocated by read_string */
  xml_record_private = XML_string_create(0);
  
  /* Master node reads the private record XML record */
  if(this_node == QIO_MASTER_NODE){
    if(!QIO_read_string(in, xml_record_private, dime_type ))return 1;
    printf("QIO_read: private XML = %s\n",XML_string_ptr(xml_record_private));
  }

  /* Interpret the private record XML */
  /* We need the site order and the site list if applicable */
  /*** OMITTED FOR NOW ***/
  in->siteorder = QIO_LEX_ORDER;
  in->sitelist = NULL;

  /* Free storage */
  XML_string_destroy(xml_record_private);

  /* Master node reads the user file XML record */
  /* Assume xml_record created by caller */
  if(this_node == QIO_MASTER_NODE){
    if(!QIO_read_string(in, xml_record, dime_type)) return 1;
    printf("QIO_read: user XML = %s\n",XML_string_ptr(xml_record));
  }

  /* Master node reads the BinX record */
  /* Assume xml_record created by caller */
  if(this_node == QIO_MASTER_NODE){
    if(!QIO_read_string(in, BinX, dime_type)) return 1;
    printf("QIO_read: BinX = %s\n",XML_string_ptr(BinX));
  }
  
  /* Nodes read the field */
  check = QIO_read_field(in, QIO_SINGLEFILE, xml_record, 
			 put, datum_size, arg, &checksum, dime_type);

#if 0
  /* Check no errors on reading field */
  /* NOTE: HOW ARE ERRORS SUPPOSE TO WORK IN PARALLEL??? */
  if(check != buf_size){
    printf("QIO_read: error reading field: expected %d bytes but found %d bytes\n",
	   buf_size, check);
    return 1;
  }
#endif

  /* Master node reads the checksum */
  xml_checksum = XML_string_create(0);
  if(this_node == QIO_MASTER_NODE){
    if(!QIO_read_string(in, xml_checksum, dime_type)) return 1;
    printf("QIO_read: checksum = %s\n",XML_string_ptr(xml_checksum));
  }
  /* Need to extract checksum */

  /* Free storage */
  XML_string_destroy(xml_checksum);

  /* Compare checksums */
  /*** OMITTED FOR NOW ***/

  return 0;
}

