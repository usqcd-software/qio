/* QIO_read.c */

#include <qio.h>
#include <lrl.h>
#include <dml.h>
#include <xml.h>
#include <stdio.h>
#include <string.h>

int QIO_read(QIO_Reader *in, XML_MetaData *xml_record, 
	     void (*put)(char *buf, const int coords[], void *arg),
	     int datum_size, void *arg){
  /* Return status 0 for success, 1 for failure */

  XML_MetaData *xml_record_private;
  DML_Checksum checksum;
  XML_MetaData *xml_checksum;
  int this_node = in->layout->this_node;
  /* Dummy BinX string */
  char BinX[] = "                                                                                            ";

  /* Allocate space for private record XML */
  xml_record_private = XML_create(MAX_XML);
  
  /* Master node reads the private record XML record */
  if(this_node == QIO_MASTER_NODE){
    if(!QIO_read_XML(in, xml_record_private))return 1;
    printf("QIO_read: private XML = %s\n",XML_string(xml_record_private));
  }

  /* Interpret the private record XML */
  /* We need the site order and the site list if applicable */
  /*** OMITTED FOR NOW ***/
  in->siteorder = QIO_LEX_ORDER;
  in->sitelist = NULL;

  /* Free storage */
  XML_destroy(xml_record_private);

  /* Master node reads the user file XML record */
  if(this_node == QIO_MASTER_NODE){
    if(!QIO_read_XML(in, xml_record))return 1;
    printf("QIO_read: user XML = %s\n",XML_string(xml_record));
  }

  /* Master node reads the BinX record */
  /* This is a dummy for now */
  if(this_node == QIO_MASTER_NODE){
    if(!QIO_read_string(in, BinX, strlen(BinX)))return 1;
    printf("QIO_read: BinX = %s\n",BinX);
  }
  
  /* Nodes read the field */
  QIO_read_field(in, QIO_SINGLEFILE, xml_record, 
		  put, datum_size, arg, &checksum);

  /* Master node reads the checksum */
  xml_checksum = XML_create(MAX_XML);
  if(this_node == QIO_MASTER_NODE){
    if(QIO_read_XML(in, xml_checksum))return 1;
    printf("QIO_read: checksum = %s\n",XML_string(xml_checksum));
  }

  /* Compare checksums */
  /*** OMITTED FOR NOW ***/

  return 0;
}

