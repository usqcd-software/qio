/* QIO_read.c */

#include <qio.h>
#include <lrl.h>
#include <dml.h>
#include <xml_string.h>
#include <qioxml.h>
#include <stdio.h>
#include <string.h>

#undef QIO_DEBUG

/* Reads a lattice field from a record.  Includes XML and checksum */
/* Calls QIO_read_record_info and QIO_read_record_data */

int QIO_read(QIO_Reader *in, QIO_RecordInfo *record_info,
	     XML_String *xml_record, 
	     void (*put)(char *buf, size_t index, size_t count, void *arg),
	     size_t datum_size, int word_size, void *arg){

  /* Caller must allocate *record_info, *xml_record and *BinX.
     Caller must signal abort to all nodes upon failure. */

  int status;
  int this_node = in->layout->this_node;
  char myname[] = "QIO_read";

  /* Read info if not already done */
  status = QIO_read_record_info(in, record_info, xml_record);
#ifdef QIO_DEBUG
  printf("%s(%d): QIO_read_record_info returned %d\n",
	 myname,this_node,status);
  fflush(stdout);
#endif
  if(status!=QIO_SUCCESS)return status;

  /* Read data */
  status = QIO_read_record_data(in, put, datum_size, word_size, arg);
#ifdef QIO_DEBUG
  printf("%s(%d): QIO_read_record_data returned %d\n",
	 myname,this_node,status);
#endif

  return status;
}
