/* QIO_write.c */

#include <qio.h>
#include <lrl.h>
#include <dml.h>
#include <xml.h>
#include <stdio.h>

/* Dummy for now */
char record_private_output[] = "XML with record number";

int QIO_write(QIO_Writer *out, XML_MetaData *xml_record, 
	      void (*get)(char *buf, const int coords[], void *arg),
	      int datum_size, void *arg){
  /* Return status 0 for success, 1 for failure */

  XML_MetaData *xml_record_private, *xml_checksum;
  DML_Checksum checksum;
  int this_node = out->layout->this_node;

  /* Dummy BinX string */
  char BinX[MAX_BINX];

  /* Create private record XML */
  /* This record should include the site order flag and a
     site list if needed */
  /*** OMITTED FOR NOW ***/
  /* Allocate space for site list if needed */
  /* Get site list */
  /* DML_list_output_sites(out->sitelist, out->serpar, out->siteorder, 
     out->latdim, out->latsize); */
  /* Insert site list into private XML */

  xml_record_private = XML_create(MAX_XML);
  XML_set(xml_record_private, record_private_output);
  
  /* Master node writes the private record XML record */
  if(this_node == QIO_MASTER_NODE){
    if(QIO_write_XML(out, xml_record_private))return 1;
    printf("QIO_write: private record XML = %s\n",
	   XML_string(xml_record_private));
  }

  /* Free storage */
  XML_destroy(xml_record_private);

  /* Master node writes the user file XML record */
  if(this_node == QIO_MASTER_NODE){
    if(QIO_write_XML(out, xml_record))return 1;
    printf("QIO_write: user record XML = %s\n",
	   XML_string(xml_record));
  }

  /* Master node creates and writes the BinX record */
  /* This is a dummy for now */
  sprintf(BinX,"BinX %d bytes per datum",datum_size);

  if(this_node == QIO_MASTER_NODE){
    if(QIO_write_string(out, BinX, strlen(BinX)))return 1;
    printf("QIO_write: BinX = %s\n",BinX);
  }
  
  /* Nodes write the field */
  QIO_write_field(out, QIO_SINGLEFILE, xml_record, 
		  get, datum_size, arg, &checksum);

  printf("QIO_write(%d): wrote field\n",this_node);fflush(stdout);

  /* Master node writes the checksum */
  if(this_node == QIO_MASTER_NODE){
    /* Insert the checksum into XML */
    /*** OMITTED FOR NOW ***/
    xml_checksum = XML_create(MAX_XML);
    XML_set(xml_checksum, "Checksum");
    if(QIO_write_XML(out, xml_checksum))return 1;
    printf("QIO_write: checksum XML = %s\n",
	   XML_string(xml_checksum));
  }

  return 0;
}
