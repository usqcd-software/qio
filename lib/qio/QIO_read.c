/* QIO_read.c */

#include <qio.h>
#include <lrl.h>
#include <dml.h>
#include <xml_string.h>
#include <qioxml.h>
#include <stdio.h>
#include <string.h>

#define DEBUG

/* Reads a lattice field from a record.  Includes XML and checksum */
/* Calls QIO_read_record_info and QIO_read_record_data */

/* Return 0 success.  1 failure */

int QIO_read(QIO_Reader *in, QIO_RecordInfo *record_info,
	     XML_String *xml_record, 
	     void (*put)(char *buf, size_t index, size_t count, void *arg),
	     size_t datum_size, int word_size, void *arg){

  /* Caller must allocate *record_info, *xml_record and *BinX.
     Return status 0 for success, 1 for failure.
     Caller must signal abort to all nodes upon failure. */

  int status;
  int this_node = in->layout->this_node;
  char myname[] = "QIO_read";

  /* Read info if not already done */
  status = QIO_read_record_info(in, record_info, xml_record);
#ifdef DEBUG
  printf("%s(%d): QIO_read_record_info returned %d\n",
	 myname,this_node,status);
  fflush(stdout);
#endif
  if(status)return 1;

  /* Read data */
  status = QIO_read_record_data(in, put, datum_size, word_size, arg);
#ifdef DEBUG
  printf("%s(%d): QIO_read_record_data returned %d\n",
	 myname,this_node,status);
#endif
  if(status)return 1;

  return 0;
}
