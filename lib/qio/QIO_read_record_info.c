/* QIO_read_record_info.c */

#include <qio.h>
#include <lrl.h>
#include <dml.h>
#include <xml_string.h>
#include <qioxml.h>
#include <stdio.h>

#define DEBUG

/* Read user record XML */
/* Can be called separately from QIO_read for discovering the record
   contents */

/* Return 0 success. 1 failure. */

int QIO_read_record_info(QIO_Reader *in, QIO_RecordInfo *record_info,
			 XML_String *xml_record){
  /* Caller must allocate *xml_record and *record_info */
  /* Return status 0 for success, 1 for failure from any node. On
     error caller must signal abort to all nodes. */

  XML_String *xml_record_private;
  int this_node = in->layout->this_node;
  int length;
  char myname[] = "QIO_read_record_info";
  DIME_type dime_type=NULL;
  
  /* Read user and private record XML if not already done */
  if(in->read_state == QIO_RECORD_XML_NEXT){

    /* Allocate space for record info structure */
    in->record_info = (QIO_RecordInfo *)malloc(sizeof(QIO_RecordInfo));
    if(in->record_info == NULL){
      printf("%s(%d) malloc of record_info failed\n",myname,this_node);
      return 1;
    }

    /* Allocate space for user record XML */
    in->xml_record = XML_string_create(0);
    
    /* Master node reads and interprets the private record XML record */
    if(this_node == QIO_MASTER_NODE){
      /* Initialize private record XML - will be allocated by read_string */
      xml_record_private = XML_string_create(0);

      /* Read private record XML */
      if(QIO_read_string(in, xml_record_private, dime_type ))return 1;
#ifdef DEBUG
      printf("%s(%d): private XML = %s\n",myname,this_node,
	     XML_string_ptr(xml_record_private));
#endif
      /* Decode the private record XML */
      QIO_decode_record_info(in->record_info, xml_record_private);
      printf("%s(%d) Datatype %s precision %s colors %d spins %d count %d\n",
	     myname,this_node,
	     QIO_get_datatype(in->record_info),
	     QIO_get_precision(in->record_info),
	     QIO_get_colors(in->record_info),
	     QIO_get_spins(in->record_info),
	     QIO_get_datacount(in->record_info));

      /* Free storage */
      XML_string_destroy(xml_record_private);

      /* Check for private values that QIO needs */
      if(!in->record_info->typesize.occur ||
	 !in->record_info->datacount.occur){
	printf("%s(%d): bad private XML record\n",myname,this_node);
	return 1;
      }
    }
    
    /* Broadcast the private record data to all nodes */
    DML_broadcast_bytes((char *)in->record_info, 
			sizeof(QIO_RecordInfo));

#ifdef DEBUG
    printf("%s(%d): Done broadcasting private record data\n",
	   myname,this_node);
#endif
    /* Master node reads the user XML record */
    if(this_node == QIO_MASTER_NODE){
      if(QIO_read_string(in, in->xml_record, dime_type))return 1;
#ifdef DEBUG
      printf("%s(%d): user XML = %s\n",myname,this_node,
	     XML_string_ptr(in->xml_record));
#endif
      length = XML_string_bytes(in->xml_record);
    }

    /* Broadcast the user xml record to all nodes */
    /* First broadcast length */
    DML_broadcast_bytes((char *)&length,sizeof(int));

    /* Receiving nodes resize their strings */
    /* length+1 to allow for the terminating \0 */
    if(this_node != QIO_MASTER_NODE){
      XML_string_realloc(in->xml_record,length+1);
    }
    DML_broadcast_bytes(XML_string_ptr(in->xml_record),length+1);

    /* Set state in case record is reread */
    in->read_state = QIO_RECORD_DATA_NEXT;
  }

  /* Copy record info on all calls */
  *record_info = *(in->record_info);
  XML_string_copy(xml_record,in->xml_record);

#ifdef DEBUG
  printf("%s(%d): Finished\n",myname,this_node);
#endif

  return 0;
}
