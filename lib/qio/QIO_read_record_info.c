/* QIO_read_record_info.c */

#include <qio.h>
#include <lrl.h>
#include <dml.h>
#include <qio_string.h>
#include <qioxml.h>
#include <stdio.h>
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

/* Read user record XML */
/* Can be called separately from QIO_read for discovering the record
   contents without reading the field itself */

int QIO_read_private_record_info(QIO_Reader *in, QIO_RecordInfo *record_info)
{
  /* Caller must allocate *record_info */

  QIO_String *xml_record_private;
  int this_node = in->layout->this_node;
  int status;
  char myname[] = "QIO_read_private_record_info";
  LIME_type lime_type=NULL;
  
  /* Read private record XML if not already done */
  if(in->read_state == QIO_RECORD_INFO_PRIVATE_NEXT){

    /* Allocate space for internal copy of user record XML */
    in->xml_record = QIO_string_create();
    
    /* Master node reads and interprets the private record XML record */
    if(this_node == in->layout->master_io_node){
      /* Initialize private record XML - will be allocated by read_string */
      xml_record_private = QIO_string_create();
      
      /* Read private record XML */
      if((status=QIO_read_string(in, xml_record_private, &lime_type ))
	 != QIO_SUCCESS)return status;

      if(QIO_verbosity() >= QIO_VERB_DEBUG){
	printf("%s(%d): private XML = \"%s\"\n",myname,this_node,
	       QIO_string_ptr(xml_record_private));
      }

      /* Decode the private record XML */
      status = QIO_decode_record_info(&(in->record_info), xml_record_private);
      if(status != 0)return QIO_ERR_PRIVATE_REC_INFO;
      
      /* Free storage */
      QIO_string_destroy(xml_record_private);
      
      /* Check for private values that QIO needs */
      if(!in->record_info.typesize.occur ||
	 !in->record_info.datacount.occur){
	printf("%s(%d): Error reading private XML record\n",myname,this_node);
	return QIO_ERR_PRIVATE_REC_INFO;
      }
    }
    /* Set state in case record is reread */
    in->read_state = QIO_RECORD_INFO_USER_NEXT;
  }    

  /* Copy record info on all calls */
  *record_info = in->record_info;

  return QIO_SUCCESS;
}

int QIO_read_user_record_info(QIO_Reader *in, QIO_String *xml_record){
  /* Caller must allocate *xml_record */

  int this_node = in->layout->this_node;
  int length;
  int status;
  char myname[] = "QIO_read_user_record_info";
  LIME_type lime_type=NULL;
  
  /* Read user record XML if not already done */
  if(in->read_state == QIO_RECORD_INFO_USER_NEXT){
    
    /* Master node reads the user XML record */
    if(this_node == in->layout->master_io_node){
      if((status=QIO_read_string(in, in->xml_record, &lime_type))
	 != QIO_SUCCESS){
	printf("%s(%d): Error reading user record XML\n",myname,this_node);
	return status;
      }

      if(QIO_verbosity() >= QIO_VERB_DEBUG){
	printf("%s(%d): user XML = \"%s\"\n",myname,this_node,
	       QIO_string_ptr(in->xml_record));
      }
      length = QIO_string_length(in->xml_record);
    }

    /* Set state in case record is being reread */
    in->read_state = QIO_RECORD_DATA_NEXT;
  }

  /* Copy user record info (for this node) */
  QIO_string_copy(xml_record,in->xml_record);

  return QIO_SUCCESS;
}

    
/* Read private and user record XML on compute node(s) */
/* Can be called separately from QIO_read for discovering the record
   contents without reading the field itself */

int QIO_read_record_info(QIO_Reader *in, QIO_RecordInfo *record_info,
			 QIO_String *xml_record){
  /* Caller must allocate *xml_record and *record_info */

  int this_node = in->layout->this_node;
  int length;
  int status;
  char myname[] = "QIO_read_record_info";
  
  /* Read private record XML if not already done */

  status = QIO_read_private_record_info(in, record_info);
  if(status != QIO_SUCCESS)return status;

  /* Broadcast the private record data to all nodes */
  DML_broadcast_bytes((char *)&(in->record_info), 
		      sizeof(QIO_RecordInfo), this_node, 
		      in->layout->master_io_node);
  
  if(QIO_verbosity() >= QIO_VERB_DEBUG){
    printf("%s(%d): Done broadcasting private record info\n",
	   myname,this_node);
  }

  /* Read user record XML if not already done */

  status = QIO_read_user_record_info(in, xml_record);
  if(status != QIO_SUCCESS)return status;
  
  /* Broadcast the user xml record to all nodes */
  /* First broadcast length */

  length = QIO_string_length(in->xml_record);
  DML_broadcast_bytes((char *)&length,sizeof(int), this_node, 
		      in->layout->master_io_node);
  
  /* Receiving nodes resize their strings */
  if(this_node != in->layout->master_io_node){
    QIO_string_realloc(in->xml_record,length);
  }
  DML_broadcast_bytes(QIO_string_ptr(in->xml_record),length,
		      this_node, in->layout->master_io_node);
  
  if(QIO_verbosity() >= QIO_VERB_DEBUG){
    printf("%s(%d): Done broadcasting user record info \"%s\"\n",
	   myname,this_node,QIO_string_ptr(in->xml_record));
  }

  if(QIO_verbosity() >= QIO_VERB_DEBUG){
    printf("%s(%d): Finished\n",myname,this_node);
  }

  /* Copy user record info */
  QIO_string_copy(xml_record,in->xml_record);

  return QIO_SUCCESS;
}

