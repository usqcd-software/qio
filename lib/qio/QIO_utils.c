/* QIO Utilities */

#include <qio_config.h>
#include <qio.h>
#include <lrl.h>
#include <dml.h>
#include <qio_string.h>
#include <stdio.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

static int QIO_verbosity_level = QIO_VERB_OFF;

/* Set verbosity level. */
int QIO_verbose (int level)
{
  int old = QIO_verbosity_level;
  QIO_verbosity_level = level;
  return old;
}

/* Check verbosity level */
int QIO_verbosity(){
  return QIO_verbosity_level;
}

/* In case of multifile format we use a common file name stem and add
   a suffix that depends on the node number */

char *QIO_filename_edit(const char *filename, int volfmt, int this_node){

  /* Caller must clean up returned filename */
  int n = strlen(filename) + 12;
  char *newfilename = (char *)malloc(n);

  if(!newfilename){
    printf("QIO_filename_edit: Can't malloc newfilename\n");
    return NULL;
  }

  /* No change if a single file or on the master node */
  if(volfmt == QIO_SINGLEFILE){
    strncpy(newfilename,filename,strlen(filename));
    newfilename[strlen(filename)] = '\0';
    return newfilename;
  }
  snprintf(newfilename,n,"%s.vol%04d",filename,this_node);
  return newfilename;
}


/* Write an XML record */

int QIO_write_string(QIO_Writer *out, 
		     int msg_begin, int msg_end,
		     QIO_String *xml,
		     const LIME_type lime_type)
{
  LRL_RecordWriter *lrl_record_out;
  char *buf;
  off_t actual_rec_size, planned_rec_size;
  char myname[] = "QIO_write_string";

  buf = QIO_string_ptr(xml);
  planned_rec_size = strlen(buf)+1;  /* Include terminating null */

  lrl_record_out = LRL_open_write_record(out->lrl_file_out, 
					 msg_begin,
					 msg_end, &planned_rec_size, 
					 lime_type);
  actual_rec_size = LRL_write_bytes(lrl_record_out, buf, planned_rec_size);
  if(QIO_verbosity() >= QIO_VERB_DEBUG)
    printf("%s(%d): wrote %d bytes\n",myname,out->layout->this_node,
	   actual_rec_size);fflush(stdout);

  /* Check byte count */
  if(actual_rec_size != planned_rec_size){
    printf("%s(%d): bytes written %lu != planned rec_size %lu\n",
	   myname, out->layout->this_node, (unsigned long)actual_rec_size, 
	   (unsigned long)planned_rec_size);
    return QIO_ERR_BAD_WRITE_BYTES;
  }
  LRL_close_write_record(lrl_record_out);
  if(QIO_verbosity() >= QIO_VERB_DEBUG){
    printf("%s(%d): closed string record\n",myname,out->layout->this_node);
    fflush(stdout);
  }
  return QIO_SUCCESS;
}

/* Create list of sites output from this node */

DML_SiteList *QIO_create_sitelist(DML_Layout *layout, int volfmt){
  DML_SiteList *sites;
  char myname[] = "QIO_create_sitelist";

  /* Initialize sitelist structure */
  sites = DML_init_sitelist(volfmt, layout);
  if(sites == NULL){
    printf("%s(%d): Error creating the sitelist structure\n", 
	   myname,layout->this_node);
    return sites;
  }

  /* Populate the sitelist */
  if(DML_fill_sitelist(sites, volfmt, layout)){
    printf("%s(%d): Error building the site list\n", 
	   myname,layout->this_node);
    DML_free_sitelist(sites);
    return NULL;
  }

  return sites;
}

/* Write list of sites (used with multifile and partitioned file formats) */
/* Returns number of bytes written */

int QIO_write_sitelist(QIO_Writer *out, int msg_begin, int msg_end, 
		       const LIME_type lime_type){
  LRL_RecordWriter *lrl_record_out;
  off_t nbytes;
  off_t rec_size;
  DML_SiteRank *outputlist;
  DML_SiteList *sites = out->sites;
  int volfmt = out->volfmt;
  int this_node = out->layout->this_node;
  char myname[] = "QIO_write_sitelist";

  /* Quit if we aren't writing the sitelist */
  /* Single file writes no sitelist.  Multifile always writes one.
     Partitioned file writes only if an I/O node */

  if(volfmt == QIO_SINGLEFILE)return 0;
  if(volfmt == QIO_PARTFILE)
    if(this_node != out->layout->ionode(this_node))return 0;

  /* Make a copy in case we have to byte reverse */
  rec_size = sites->number_of_io_sites * sizeof(DML_SiteRank);
  if(this_node == DML_master_io_node()){
    printf("%s(%d) allocating %d for output sitelist\n",myname,this_node,
	   rec_size);fflush(stdout);
  }

  outputlist = (DML_SiteRank *)malloc(rec_size);
  if(outputlist == NULL){
    printf("%s(%d) no room for output sitelist\n",myname,this_node);fflush(stdout);
    return QIO_ERR_ALLOC;
  }

  memcpy(outputlist, sites->list, rec_size);

  /* Byte reordering for entire sitelist */
  if (! DML_big_endian())
    DML_byterevn((char *)outputlist, rec_size, sizeof(DML_SiteRank));

  /* Write site list */
  lrl_record_out = LRL_open_write_record(out->lrl_file_out, msg_begin,
					 msg_end, &rec_size, lime_type);
  nbytes = LRL_write_bytes(lrl_record_out, (char *)outputlist, rec_size);

  if(nbytes != rec_size){
    printf("%s(%d): Error writing site list. Wrote %lu bytes expected %lu\n", 
	   myname,out->layout->this_node,(unsigned long)rec_size,
	   (unsigned long)nbytes);
    free(outputlist);
    return QIO_ERR_BAD_WRITE_BYTES;
  }

  if(QIO_verbosity() >= QIO_VERB_DEBUG)
    printf("%s(%d): wrote sitelist\n", myname, out->layout->this_node);
  
  /* Close record when done and clean up*/
  LRL_close_write_record(lrl_record_out);

  free(outputlist); 
  return QIO_SUCCESS;
}

/* Write binary data for a lattice field */

int QIO_write_field(QIO_Writer *out, int msg_begin, int msg_end,
	    QIO_String *xml_record, int globaldata,
	    void (*get)(char *buf, size_t index, int count, void *arg),
	    int count, size_t datum_size, int word_size, void *arg, 
	    DML_Checksum *checksum, uint64_t *nbytes,
	    const LIME_type lime_type){
  
  LRL_RecordWriter *lrl_record_out = NULL;
  off_t planned_rec_size;
  int this_node = out->layout->this_node;
  int do_output;
  char myname[] = "QIO_write_field";

  /* Compute record size */
  if(globaldata == QIO_GLOBAL){
    /* Global data */
    planned_rec_size = datum_size;
    if(QIO_verbosity() >= QIO_VERB_DEBUG){
      printf("%s(%d): global data: size %lu\n",myname,out->layout->this_node,
	     (unsigned long)datum_size);
    }
  }
  else{
    /* Field data */
    planned_rec_size = out->sites->number_of_io_sites * datum_size;
    if(QIO_verbosity() >= QIO_VERB_DEBUG){
      printf("%s(%d): field data: sites %lu datum %lu\n",
	     myname,out->layout->this_node,
	     (unsigned long)out->layout->sites_on_node,
	     (unsigned long)datum_size);
    }
  }
  
  /* For global data only the master node opens and writes the record.
     Othewise, all nodes process output, even though only some nodes
     actually write */
  do_output = (globaldata == DML_FIELD) ||
    (this_node == out->layout->master_io_node);

  /* Open record only if we have a file handle and are writing */
  /* Nodes that do not write to a file will have a NULL file handle */

  if(!out->lrl_file_out || !do_output)
    {
      if(QIO_verbosity() >= QIO_VERB_DEBUG)
	printf("%s(%d): skipping LRL_open_write_record\n",
	       myname,this_node);
    }
  else
    {
      lrl_record_out = LRL_open_write_record(out->lrl_file_out, 
		     msg_begin, msg_end, &planned_rec_size, lime_type);
      if(!lrl_record_out)
	return QIO_ERR_OPEN_WRITE;
    }
  
  /* Initialize byte count and checksum */
  *nbytes = 0;
  DML_checksum_init(checksum);

  /* Write bytes */

  if(do_output)
    *nbytes = DML_stream_out(lrl_record_out, globaldata, get, count, 
			     datum_size, word_size, arg, out->layout,
			     out->sites, out->volfmt, checksum);

  /* Close record when done and clean up*/
  if(out->lrl_file_out && do_output)
    LRL_close_write_record(lrl_record_out);

  return QIO_SUCCESS;
}


/* Read an XML record */

int QIO_read_string(QIO_Reader *in, QIO_String *xml, LIME_type *lime_type){
  char *buf;
  off_t buf_size;
  LRL_RecordReader *lrl_record_in;
  off_t actual_rec_size,expected_rec_size;
  int status;
  char myname[] = "QIO_read_string";

  /* Open record and find record size */
  if(!in->lrl_file_in)return QIO_SUCCESS;
  lrl_record_in = LRL_open_read_record(in->lrl_file_in, &expected_rec_size, 
				       lime_type, &status);
  if(!lrl_record_in){
    if(status == LRL_EOF)return QIO_EOF;
    else return QIO_ERR_OPEN_READ;
  }

  buf_size = QIO_string_length(xml);   /* The size allocated for the string */
  buf      = QIO_string_ptr(xml);

  /* Realloc if necessary */
  if(expected_rec_size+1 > buf_size){
    QIO_string_realloc(xml,expected_rec_size+1);  /* +1 for null termination */
  }

  buf_size = QIO_string_length(xml);   /* Get this again */
  buf      = QIO_string_ptr(xml);

  actual_rec_size = LRL_read_bytes(lrl_record_in, buf, expected_rec_size);
  LRL_close_read_record(lrl_record_in);

  if(actual_rec_size != expected_rec_size){
    printf("%s(%d): bytes read %lu != expected rec_size %lu\n",
	   myname, in->layout->this_node, (unsigned long)actual_rec_size, 
	   (unsigned long)expected_rec_size);
    return QIO_ERR_BAD_READ_BYTES;
  }

  return QIO_SUCCESS;
}

/* Read site list */

int QIO_read_sitelist(QIO_Reader *in, LIME_type *lime_type){
  int this_node = in->layout->this_node;
  int volfmt = in->volfmt;
  char myname[] = "QIO_read_sitelist";
  int not_ok = 0;

  /* SINGLEFILE format has no sitelist */
  if(volfmt == QIO_SINGLEFILE)return QIO_SUCCESS;

  /* Only I/O nodes read and verify the sitelist */
  if((volfmt == QIO_MULTIFILE) || 
     ((volfmt == QIO_PARTFILE) 
      && (this_node == in->layout->ionode(this_node))))
    not_ok = DML_read_sitelist(in->sites, 
			       in->lrl_file_in, in->volfmt, 
			       in->layout, lime_type);

  /* Poll all nodes to be sure all sitelists pass */
  DML_sum_int(&not_ok);
  if(not_ok)return QIO_ERR_BAD_SITELIST;
  else {
    if(QIO_verbosity() >= QIO_VERB_DEBUG){
      if(this_node == in->layout->master_io_node)
	printf("%s(%d): sitelist passes test\n",
	       myname,this_node);fflush(stdout);
    }
  }

  return QIO_SUCCESS;
}

/* Read binary data for a lattice field */

int QIO_read_field(QIO_Reader *in, int globaldata,
	   void (*put)(char *buf, size_t index, int count, void *arg),
	   int count, size_t datum_size, int word_size, void *arg, 
	   DML_Checksum *checksum, uint64_t* nbytes,
	   LIME_type *lime_type){

  LRL_RecordReader *lrl_record_in=NULL;
  DML_SiteList *sites = in->sites;
  off_t announced_rec_size, expected_rec_size;
  int this_node = in->layout->this_node;
  int status;
  int do_open;
  char myname[] = "QIO_read_field";

  /* For field data we open the record if the file is being read
     by this node.  For global data only the master node opens
     the record */
  do_open = ( in->lrl_file_in && (globaldata == DML_FIELD) ) || 
    (this_node == in->layout->master_io_node);

  /* Open record only if we have a file handle and are reading */
  /* Nodes that do not read from a file will have a NULL file handle */
  if(!do_open) {
    if(QIO_verbosity() >= QIO_VERB_DEBUG)
      printf("%s(%d): skipping LRL_open_read_record\n",
	     myname,this_node);
  }
  else{
    lrl_record_in = LRL_open_read_record(in->lrl_file_in, 
			 &announced_rec_size, lime_type, &status);
    if(!lrl_record_in){
      if(status == LRL_EOF)return QIO_EOF;
      else return QIO_ERR_OPEN_READ;
    }

    /* Check that the record size matches the expected size of the data */
    if(globaldata == QIO_GLOBAL)
      /* Global data */
      expected_rec_size = datum_size;
    else 
      /* Field data */
      expected_rec_size = sites->number_of_io_sites * datum_size; 
    
    if (announced_rec_size != expected_rec_size){
      printf("%s(%d): rec_size mismatch: found %lu expected %lu\n",
	     myname, this_node, (unsigned long)announced_rec_size, 
	     (unsigned long)expected_rec_size);
      return QIO_ERR_BAD_READ_BYTES;
    }
  }

  /* Initialize byte count and checksum */
  *nbytes = 0;
  DML_checksum_init(checksum);
  
  /* All nodes process input.  Compute checksum and byte count
     for node*/

  *nbytes = DML_stream_in(lrl_record_in, globaldata, put, 
			  count, datum_size, word_size,
			  arg, in->layout,
			  in->sites, in->volfmt, checksum);
  if(QIO_verbosity() >= QIO_VERB_DEBUG){
    printf("%s(%d): done with DML_stream_in\n", myname,this_node);
  }
    
  /* Close record when done and clean up*/
  if(do_open)
    LRL_close_read_record(lrl_record_in);

  return QIO_SUCCESS;
}
