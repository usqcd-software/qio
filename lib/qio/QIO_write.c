/* QIO_write.c */

#include <qio.h>
#include <lrl.h>
#include <dml.h>
#include <xml.h>
#include <stdio.h>
#include <string.h>

/* Dummy for now */
char record_private_output[] = "XML with record number";

int QIO_write(QIO_Writer *out, XML_string *xml_record, XML_string *BinX,
	      void (*get)(char *buf, const int coords[], void *arg),
	      int datum_size, void *arg){
  /* Return status 0 for success, 1 for failure */

  XML_string *xml_record_private, *xml_checksum;
  DML_Checksum checksum;
  int this_node = out->layout->this_node;

  /* Create private record XML */
  /* This record should include the site order flag and a
     site list if needed */
  /*** OMITTED FOR NOW ***/
  /* Allocate space for site list if needed */
  /* Get site list */
  /* DML_list_output_sites(out->sitelist, out->serpar, out->siteorder, 
     out->latdim, out->latsize); */
  /* Insert site list into private XML */

  xml_record_private = XML_string_create(strlen(record_private_output)+1);
  XML_string_set(xml_record_private, record_private_output);
  
  /* Master node writes the private record XML record */
  if (this_node == QIO_MASTER_NODE)
  {
    if (QIO_write_string(out, xml_record_private))
      return 1;
    printf("QIO_write: private record XML = XXX%sXXX\n",
	   XML_string_ptr(xml_record_private));
  }

  /* Free storage */
  XML_string_destroy(xml_record_private);

  /* Master node writes the user file XML record */
  if (this_node == QIO_MASTER_NODE)
  {
    if (QIO_write_string(out, xml_record))
      return 1;
    printf("QIO_write: user record XML = XXX%sXXX\n",
	   XML_string_ptr(xml_record));
  }

  /* Master node creates and passes in the BinX record to write */
  if (this_node == QIO_MASTER_NODE)
  {
    if (QIO_write_string(out, BinX))
      return 1;
    printf("QIO_write: BinX = %s\n",XML_string_ptr(BinX));
  }
  
  /* Nodes write the field */
  QIO_write_field(out, QIO_SINGLEFILE, xml_record, 
		  get, datum_size, arg, &checksum);

  printf("QIO_write(%d): wrote field\n",this_node);fflush(stdout);

  /* Master node writes the checksum */
  if (this_node == QIO_MASTER_NODE)
  {
    /* Insert the checksum into a string */
    size_t some_checksum_here = 0;       /*** DUMMY FOR NOW ***/
    xml_checksum = XML_string_create(30);
    /* NOTE: should use   snprintf here - is it really portable ?? */
/*    sprintf(XML_string_ptr(xml_checksum), "%d", some_checksum_here); */
    snprintf(XML_string_ptr(xml_checksum), XML_string_bytes(xml_checksum),
	     "%d", some_checksum_here);

    if (QIO_write_string(out, xml_checksum))
      return 1;
    printf("QIO_write: checksum string = %s\n",
	   XML_string_ptr(xml_checksum));

    XML_string_destroy(xml_checksum);
  }

  return 0;
}
