/* QIO_write.c */

#include <qio_config.h>
#include <qio.h>
#include <lrl.h>
#include <dml.h>
#include <qio_string.h>
#include <stdio.h>
#include <string.h>

/* Write a lattice field to a record.  Includes XML */

/* Handles the write operation on the compute nodes as well as the host */
int QIO_generic_write(QIO_Writer *out, QIO_RecordInfo *record_info, 
	      QIO_String *xml_record, 
	      void (*get)(char *buf, size_t index, int count, void *arg),
	      size_t datum_size, int word_size, void *arg,
	      DML_Checksum *checksum, uint64_t *nbytes,
	      int *msg_begin, int *msg_end){

  QIO_String *xml_record_private;
  int this_node = out->layout->this_node;
  int master_io_node = out->layout->master_io_node;
  int status;
  int globaldata = QIO_get_globaldata(record_info);
  int count = QIO_get_datacount(record_info);
  char myname[] = "QIO_generic_write";

  /* Require consistency between the byte count specified in the
     private record metadata and the byte count per site to be written */
  if(datum_size != QIO_get_typesize(record_info) * count)
    {
      printf("%s(%d): bytes per site mismatch %lu != %d * %d\n",
	     myname,this_node,(unsigned long)datum_size,
	     QIO_get_typesize(record_info),
	     QIO_get_datacount(record_info));
      return QIO_ERR_BAD_WRITE_BYTES;
    }

  /* A message consists of the XML, binary payload, and checksums */
  /* First and last records in a message are flagged */
  *msg_begin = 1; *msg_end = 0;

  /* Create private record XML */
  xml_record_private = QIO_string_create();
  QIO_encode_record_info(xml_record_private, record_info);

  /* Master node writes the private record XML record */
  if(this_node == master_io_node){
    if ((status = 
	 QIO_write_string(out, *msg_begin, *msg_end, 
			  xml_record_private, 
			  (const LIME_type)"scidac-private-record-xml"))
	!= QIO_SUCCESS){
      printf("%s(%d): Error writing private record XML\n",
	     myname,this_node);
      return status;
    }

    if(QIO_verbosity() >= QIO_VERB_REG){
      printf("%s(%d): private record XML = \"%s\"\n",
	     myname,this_node,QIO_string_ptr(xml_record_private));
    }
    *msg_begin = 0;
  }
  QIO_string_destroy(xml_record_private);

  /* Master node writes the user record XML record */
  if(this_node == master_io_node){
    if ((status = 
	 QIO_write_string(out, *msg_begin, *msg_end, xml_record, 
			  (const LIME_type)"scidac-record-xml"))
	!= QIO_SUCCESS){
      printf("%s(%d): Error writing user record XML\n",myname,this_node);
      return status;
    }
    if(QIO_verbosity() >= QIO_VERB_DEBUG){
      printf("%s(%d): user record XML = \"%s\"\n",
	     myname,this_node,QIO_string_ptr(xml_record));
    }
    *msg_begin = 0;
  }

#ifdef DO_BINX
  /* Master node writes the BinX record */
  if(this_node == master_io_node){
    if ((status = 
	 QIO_write_string(out, *msg_begin, *msg_end, BinX, 
			  (const LIME_type)"scidac-binx-xml"))
	!= QIO_SUCCESS){
      printf("%s(%d): Error writing BinX\n",myname,this_node);
      return status;
    }
    if(QIO_verbosity() >= QIO_VERB_DEBUG){
      printf("%s(%d): BinX = %s\n",myname,this_node,QIO_string_ptr(BinX));
    }
    *msg_begin = 0;
  }
#endif
  
  /* Next one is last record in message for all but master node */
  if (this_node != master_io_node)*msg_end = 1;
  if((status = 
      QIO_write_field(out, *msg_begin, *msg_end, xml_record, globaldata,
		      get, count, datum_size, word_size, arg, 
		      checksum, nbytes,
		      (const LIME_type)"scidac-binary-data"))
     != QIO_SUCCESS){
    printf("%s(%d): Error writing field data\n",myname,this_node);
    return status;
  }
  if(QIO_verbosity() >= QIO_VERB_DEBUG){
    printf("%s(%d): wrote field\n",myname,this_node);fflush(stdout);
  }

  return QIO_SUCCESS;
}

/* Write a lattice field to a record on the compute nodes. */

int QIO_write(QIO_Writer *out, QIO_RecordInfo *record_info, 
	      QIO_String *xml_record, 
	      void (*get)(char *buf, size_t index, int count, void *arg),
	      size_t datum_size, int word_size, void *arg){

  QIO_String *xml_checksum;
  DML_Checksum checksum;
  uint64_t nbytes;
  QIO_ChecksumInfo *checksum_info;
  int this_node = out->layout->this_node;
  int master_io_node = out->layout->master_io_node;
  int msg_begin, msg_end;
  int status;
  size_t total_bytes;
  size_t volume = out->layout->volume;
  int globaldata = QIO_get_globaldata(record_info);
  char myname[] = "QIO_write";


  status = QIO_generic_write(out, record_info, xml_record, get, datum_size, 
			     word_size, arg, &checksum, &nbytes, 
			     &msg_begin, &msg_end);

  if(status != QIO_SUCCESS)return status;

  /* Combine checksums over all nodes */
  DML_checksum_combine(&checksum);

  /* Sum the bytes written by all nodes */
  DML_sum_uint64_t(&nbytes);

  /* Compute and compare byte count with expected record size */
  if(globaldata == QIO_GLOBAL)total_bytes = datum_size;
  else total_bytes = volume * datum_size;

  if(nbytes != total_bytes){
    printf("%s(%d): bytes written %lu != expected rec_size %lu\n",
	   myname, this_node, (unsigned long)nbytes, 
	   (unsigned long)total_bytes);
    return QIO_ERR_BAD_WRITE_BYTES;
  }


  /* Master node encodes and writes the checksum */
  if(this_node == master_io_node){
    checksum_info = QIO_create_checksum_info(checksum.suma,checksum.sumb);
    xml_checksum = QIO_string_create();
    QIO_encode_checksum_info(xml_checksum, checksum_info);
    
    msg_end = 1;
    if ((status = 
	 QIO_write_string(out, msg_begin, msg_end, xml_checksum,
			  (const LIME_type)"scidac-checksum"))
	!= QIO_SUCCESS){
      printf("%s(%d): Error writing checksum\n",myname,this_node);
      return status;
    }

    if(QIO_verbosity() >= QIO_VERB_REG){
      printf("%s(%d): Wrote field. datatype %s globaltype %d \n              precision %s colors %d spins %d count %d\n",
	     myname,this_node,
	     QIO_get_datatype(record_info),
	     QIO_get_globaldata(record_info),
	     QIO_get_precision(record_info),
	     QIO_get_colors(record_info),
	     QIO_get_spins(record_info),
	     QIO_get_datacount(record_info));
      
      printf("%s(%d): checksum string = %s\n",
	     myname,this_node,QIO_string_ptr(xml_checksum));
    }
    
    QIO_string_destroy(xml_checksum);
  }

  return QIO_SUCCESS;
}


