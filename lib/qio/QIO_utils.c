/* QIO Utilities */

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

#undef QIO_DEBUG

/* In case of multifile format we use a common file name stem and add
   a suffix that depends on the node number */

char *QIO_filename_edit(const char *filename, int volfmt, int this_node){
  /* Caller must clean up returned filename */
  int n = strlen(filename) + 12;
  char *newfilename = (char *)malloc(n);

  /* No change if a single file or on the master node */
  if(volfmt == QIO_SINGLEFILE || this_node == QIO_MASTER_NODE){
    strncpy(newfilename,filename,strlen(filename));
    newfilename[strlen(filename)] = '\0';
    return newfilename;
  }
  if(!newfilename){
    printf("QIO_filename_edit: Can't malloc newfilename\n");
    return NULL;
  }

  snprintf(newfilename,n,"%s.vol%03d",filename,this_node);
  return newfilename;
}

/* Write an XML record */

int QIO_write_string(QIO_Writer *out, int msg_begin, int msg_end,
		     QIO_String *xml,
		     const LIME_type lime_type)
{
  LRL_RecordWriter *lrl_record_out;
  char *buf;
  size_t check, rec_size;
  char myname[] = "QIO_write_string";

  buf = QIO_string_ptr(xml);
  rec_size = strlen(buf)+1;  /* Include terminating null */

  lrl_record_out = LRL_open_write_record(out->lrl_file_out, 1, msg_begin,
					 msg_end, &rec_size, 
					 lime_type);
  check = LRL_write_bytes(lrl_record_out, buf, rec_size);
#ifdef QIO_DEBUG
  printf("%s(%d): wrote bytes\n",myname,out->layout->this_node);fflush(stdout);
#endif

  /* Check byte count */
  if(check != rec_size){
    printf("%s(%d): bytes written %d != expected rec_size %d\n",
	   myname, out->layout->this_node, check, rec_size);
    return QIO_ERR_BAD_WRITE_BYTES;
  }
  LRL_close_write_record(lrl_record_out);
  return QIO_SUCCESS;
}

/* Write list of sites (used with multifile format) */
/* Returns number of bytes written */

int QIO_write_sitelist(QIO_Writer *out, int msg_begin, int msg_end, 
		       const LIME_type lime_type){
  LRL_RecordWriter *lrl_record_out;
  size_t nbytes;
  size_t rec_size;
  int sites_this_node = out->layout->sites_on_node;
  size_t datum_size = sizeof(DML_SiteRank);
  DML_SiteRank *sitelist;

  /* Create sitelist */
  rec_size = sites_this_node * datum_size;

  /* Allocate space */
  sitelist = (DML_SiteRank *)malloc(rec_size);
  if(!sitelist){
    printf("QIO_write_sitelist: Node %d can't malloc sitelist\n",
	   out->layout->this_node);
    return QIO_ERR_ALLOC;
  }

  /* Generate site list */
  if(DML_create_sitelist(out->layout,sitelist)){
    free(sitelist);
    return QIO_ERR_ALLOC;
  }

  /* Byte reordering for entire sitelist */
  if (! DML_big_endian())
    DML_byterevn((char *)sitelist, rec_size, sizeof(DML_SiteRank));

  /* Write site list */
  lrl_record_out = LRL_open_write_record(out->lrl_file_out, 1, msg_begin,
					 msg_end, &rec_size, 
					 lime_type);
  nbytes = LRL_write_bytes(lrl_record_out, (char *)sitelist, rec_size);

  /* Close record when done and clean up*/
  LRL_close_write_record(lrl_record_out);

  free(sitelist);
  return QIO_SUCCESS;
}

/* Write binary data for a lattice field */

int QIO_write_field(QIO_Writer *out, int msg_begin, int msg_end,
	    QIO_String *xml_record, int globaldata,
	    void (*get)(char *buf, size_t index, int count, void *arg),
	    int count, size_t datum_size, int word_size, void *arg, 
	    DML_Checksum *checksum,
	    const LIME_type lime_type){
  
  LRL_RecordWriter *lrl_record_out;
  size_t check;
  size_t rec_size;
  size_t volume = out->layout->volume;
  int do_write;
  char myname[] = "QIO_write_field";

  /* Compute record size */
  if(globaldata == QIO_GLOBAL){
    rec_size = datum_size; /* Global data */
#ifdef QIO_DEBUG
    printf("%s(%d): global data: size %d\n",myname,out->layout->this_node,
	   datum_size);
#endif
  }
  else{
    if(out->volfmt == QIO_SINGLEFILE){
      rec_size = volume * datum_size;  /* Single file holds all the data */
#ifdef QIO_DEBUG
      printf("%s(%d): singlefile field data sites = %d datum %d\n",
	     myname,out->layout->this_node,
	     volume,datum_size);
#endif
    }
    else{ 
      rec_size = out->layout->sites_on_node * datum_size; /* Multifile */
#ifdef QIO_DEBUG
      printf("%s(%d): multifile field sites = %d datum %d\n",
	     myname,out->layout->this_node,
	     out->layout->sites_on_node,datum_size);
#endif
    }
  }
  
#ifdef QIO_DEBUG
  printf("%s(%d): rec_size = %d\n",myname,out->layout->this_node,rec_size);
#endif

  /* In all cases the master node writes the record header */
  /* For multifile all nodes write the record header */
  do_write = ( out->layout->this_node == QIO_MASTER_NODE ) ||
    ( out->volfmt == QIO_MULTIFILE );

  /* Open record */

  lrl_record_out = LRL_open_write_record(out->lrl_file_out, do_write,
					 msg_begin, msg_end, &rec_size, 
					 lime_type);
  /* Write bytes */

  check = DML_stream_out(lrl_record_out, globaldata, get, count, datum_size, 
			 word_size, arg, out->layout, out->serpar, 
			 out->volfmt, checksum);

  /* Close record when done and clean up*/
  LRL_close_write_record(lrl_record_out);

  if(check != rec_size){
    printf("%s(%d): bytes written %d != expected rec_size %d\n",
	   myname, out->layout->this_node, check, rec_size);
    return QIO_ERR_BAD_WRITE_BYTES;
  }

  return QIO_SUCCESS;
}


/* Read an XML record */

int QIO_read_string(QIO_Reader *in, QIO_String *xml, LIME_type lime_type){
  char *buf;
  size_t buf_size;
  LRL_RecordReader *lrl_record_in;
  size_t check,rec_size;
  char myname[] = "QIO_read_string";

  /* Open record and find record size */
  if(!in->lrl_file_in)return QIO_SUCCESS;
  lrl_record_in = LRL_open_read_record(in->lrl_file_in, &rec_size, lime_type);
  if(!lrl_record_in)return QIO_ERR_OPEN_READ;

  buf_size = QIO_string_bytes(xml);   /* The size allocated for the string */
  buf      = QIO_string_ptr(xml);

  /* Realloc if necessary */
  if(rec_size+1 > buf_size){
    QIO_string_realloc(xml,rec_size+1);  /* The +1 will insure null terminating string */
  }

  buf_size = QIO_string_bytes(xml);   /* Get this again */
  buf      = QIO_string_ptr(xml);

  check = LRL_read_bytes(lrl_record_in, buf, rec_size);
  LRL_close_read_record(lrl_record_in);

  if(check != rec_size){
    printf("%s(%d): bytes read %d != expected rec_size %d\n",
	   myname, in->layout->this_node, check, rec_size);
    return QIO_ERR_BAD_READ_BYTES;
  }

  return QIO_SUCCESS;
}

/* Read site list */

int QIO_read_sitelist(QIO_Reader *in, LIME_type lime_type){
  char *buf;
  size_t buf_size;
  LRL_RecordReader *lrl_record_in;
  size_t check,rec_size;
  int sites;
  int this_node = in->layout->this_node;
  char myname[] = "QIO_read_sitelist";

  /* The number of sites expected per file */
  sites = in->layout->sites_on_node;
  in->sitelist = (DML_SiteRank *)malloc(sites*sizeof(DML_SiteRank));
  
  if(!in->sitelist){
    printf("%s(%d) can't malloc sitelist\n",myname,in->layout->this_node);
    return QIO_ERR_ALLOC;
  }

  /* Open record and find record size */
  lrl_record_in = LRL_open_read_record(in->lrl_file_in, &rec_size, lime_type);
  if(!lrl_record_in)QIO_ERR_OPEN_READ;

  /* Is record size correct? */
  check = sites*sizeof(DML_SiteRank);
  if(rec_size != check){
    printf("%s(%d): rec size mismatch: found %d expected %d\n",
	   myname, this_node, rec_size, check);
    return QIO_ERR_BAD_READ_BYTES;
  }
  
  buf = (char *)in->sitelist;
  check = LRL_read_bytes(lrl_record_in, buf, rec_size);

  LRL_close_read_record(lrl_record_in);

#ifdef QIO_DEBUG
  printf("%s(%d) site record was read %d\n",myname,in->layout->this_node,check);
#endif

  if(check != rec_size){
    printf("%s(%d): bytes read %d != expected rec_size %d\n",
	   myname, this_node, check, rec_size);
    return QIO_ERR_BAD_READ_BYTES;
  }

  /* Byte reordering for entire sitelist */
  if (! DML_big_endian())
    DML_byterevn(buf, rec_size, sizeof(DML_SiteRank));

  return QIO_SUCCESS;
}

/* Read binary data for a lattice field */

int QIO_read_field(QIO_Reader *in, int globaldata,
	   void (*put)(char *buf, size_t index, int count, void *arg),
	   int count, size_t datum_size, int word_size, void *arg, 
	   DML_Checksum *checksum,
	   LIME_type lime_type){

  LRL_RecordReader *lrl_record_in;
  size_t rec_size, check, buf_size;
  size_t volume = in->layout->volume;
  int this_node = in->layout->this_node;
  char myname[] = "QIO_read_field";

  /* Open record only if we have a file handle */
  if(!in->lrl_file_in) {
#ifdef QIO_DEBUG
    printf("%s(%d): skipping LRL_open_read_record %x\n",
	   myname,this_node,in->lrl_file_in);
#endif
  }
  else{
    lrl_record_in = LRL_open_read_record(in->lrl_file_in, 
					 &rec_size, lime_type);
    if(!lrl_record_in)return QIO_ERR_OPEN_READ;
    
    /* Check that the record size matches the expected size of the data */
    if(globaldata == QIO_GLOBAL){
      buf_size = datum_size; /* Global data */
    }
    else{  /* Field data */
      if(in->volfmt == QIO_SINGLEFILE)
	buf_size = volume * datum_size;  /* Single file holds all the data */
      else{ 
	buf_size = in->layout->sites_on_node * datum_size; /* Multifile */
      }
    }
    
    if (rec_size != buf_size){
      printf("%s(%d): rec_size mismatch: found %d expected %d\n",
	     myname, this_node, rec_size, buf_size);
      return QIO_ERR_BAD_READ_BYTES;
    }
  }

  /* Nodes read and/or collect data.  Compute checksum */
  check = DML_stream_in(lrl_record_in, globaldata, put, 
			count, datum_size, word_size,
			arg, in->layout, in->serpar, in->siteorder, 
			in->sitelist, in->volfmt,  checksum);
#ifdef QIO_DEBUG
  printf("%s(%d): done with DML_stream_in\n", myname,this_node);
#endif

  /* Close record when done and clean up*/
  if(in->lrl_file_in){
    LRL_close_read_record(lrl_record_in);
  
    if(check != rec_size){
      printf("%s(%d): bytes read %d != expected rec_size %d\n",
	     myname, in->layout->this_node,check, rec_size);
      return QIO_ERR_BAD_READ_BYTES;
    }
  }

  return QIO_SUCCESS;
}
