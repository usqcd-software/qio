/* QIO_write.c */

#include <qio.h>
#include <lrl.h>
#include <dml.h>
#include <xml_string.h>
#include <stdio.h>
#include <string.h>

#define DEBUG

/* Write a lattice field to a record.  Includes XML and checksum */
/* Return 0 success.  1 failure */

int QIO_write(QIO_Writer *out, QIO_RecordInfo *record_info, 
	      XML_String *xml_record, 
	      void (*get)(char *buf, size_t index, size_t count, void *arg),
	      size_t datum_size, int word_size, void *arg){
  /* Return status 0 for success, 1 for failure */

  XML_String *xml_record_private, *xml_checksum;
  DML_Checksum checksum;
  QIO_ChecksumInfo *checksum_info;
  int this_node = out->layout->this_node;
  int msg_begin, msg_end;
  char myname[] = "QIO_write";

  /* Require consistency between the byte count specified in the
     private record metadata and the byte count per site to be written */
  if(datum_size != 
     QIO_get_typesize(record_info) * QIO_get_datacount(record_info)){
    printf("%s(%d): bytes per site mismatch %d != %d * %d\n",
	   myname,this_node,datum_size,
	   QIO_get_typesize(record_info),
	   QIO_get_datacount(record_info));
    return 1;
  }

  /* A message consists of the XML, binary payload, and checksums */
  /* First and last records in a message are flagged */
  msg_begin = 1; msg_end = 0;

  /* Create private record XML */
  xml_record_private = XML_string_create(0);
  QIO_encode_record_info(xml_record_private, record_info);
  
  /* Master node writes the private record XML record */
  if (this_node == QIO_MASTER_NODE)
  {
    if (QIO_write_string(out, msg_begin, msg_end, xml_record_private, 
			 (const LIME_type)"scidac-private-record-xml"))
      return 1;
#ifdef DEBUG
    printf("%s(%d): private record XML = %s\n",
	   myname,this_node,XML_string_ptr(xml_record_private));
#endif
    msg_begin = 0;
  }
  XML_string_destroy(xml_record_private);

  /* Master node writes the user file XML record */
  if (this_node == QIO_MASTER_NODE)
  {
    if (QIO_write_string(out, msg_begin, msg_end, xml_record, 
			 (const LIME_type)"scidac-record-xml"))
      return 1;
#ifdef DEBUG
    printf("%s(%d): user record XML = XXX%sXXX\n",
	   myname,this_node,XML_string_ptr(xml_record));
#endif
    msg_begin = 0;
  }

#ifdef DO_BINX
  /* Master node writes the BinX record */
  if (this_node == QIO_MASTER_NODE)
  {
    if (QIO_write_string(out, msg_begin, msg_end, BinX, 
			 (const LIME_type)"scidac-binx-xml"))
      return 1;
#ifdef DEBUG
    printf("%s(%d): BinX = %s\n",myname,this_node,XML_string_ptr(BinX));
#endif
    msg_begin = 0;
  }
#endif
  
  /* Next one is last record in message for all but master node */
  if (this_node != QIO_MASTER_NODE)msg_end = 1;
  QIO_write_field(out, msg_begin, msg_end, xml_record, 
		  get, datum_size, word_size, arg, &checksum,
		  (const LIME_type)"scidac-binary-data");
#ifdef DEBUG
  printf("%s(%d): wrote field\n",myname,this_node);fflush(stdout);
#endif

  /* Master node encodes and writes the checksum */
  if (this_node == QIO_MASTER_NODE)
  {
    checksum_info = QIO_create_checksum_info(checksum.suma,checksum.sumb);
    xml_checksum = XML_string_create(30);
    QIO_encode_checksum_info(xml_checksum, checksum_info);

    msg_end = 1;
    if (QIO_write_string(out, msg_begin, msg_end, xml_checksum,
	(const LIME_type)"scidac-checksum"))return 1;

#ifdef DEBUG
    printf("%s(%d): checksum string = %s\n",
	   myname,this_node,XML_string_ptr(xml_checksum));
#endif
    XML_string_destroy(xml_checksum);
  }

  return 0;
}
