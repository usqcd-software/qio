/* QIO Utilities */

#include <qio.h>
#include <lrl.h>
#include <dml.h>
#include <xml_string.h>
#include <stdio.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#undef DEBUG

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
/* Returns 0 success; 1 failure */

int QIO_write_string(QIO_Writer *out, int msg_begin, int msg_end,
		     XML_String *xml,
		     const LIME_type lime_type)
{
  LRL_RecordWriter *lrl_record_out;
  char *buf;
  size_t check, rec_size;
  char myname[] = "QIO_write_string";

  buf = XML_string_ptr(xml);
  rec_size = strlen(buf)+1;  /* Include terminating null */

  lrl_record_out = LRL_open_write_record(out->lrl_file_out, 1, msg_begin,
					 msg_end, &rec_size, 
					 lime_type);
  check = LRL_write_bytes(lrl_record_out, buf, rec_size);
#ifdef DEBUG
  printf("%s(%d): wrote bytes\n",myname,out->layout->this_node);fflush(stdout);
#endif

  /* Check byte count */
  if(check != rec_size){
    printf("%s(%d): bytes written %d != expected rec_size %d\n",
	   myname, out->layout->this_node, check, rec_size);
    return 1;
  }
  LRL_close_write_record(lrl_record_out);
  return 0;
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
    return 1;
  }

  /* Generate site list */
  if(DML_create_sitelist(out->layout,sitelist)){
    free(sitelist);
    return 1;
  }

  /* Byte reordering for entire sitelist */
#if(!QIO_BIG_ENDIAN)
  DML_byterevn((char *)sitelist, rec_size, sizeof(DML_SiteRank));
#endif

  /* Write site list */
  lrl_record_out = LRL_open_write_record(out->lrl_file_out, 1, msg_begin,
					 msg_end, &rec_size, 
					 lime_type);
  nbytes = LRL_write_bytes(lrl_record_out, (char *)sitelist, rec_size);

  /* Close record when done and clean up*/
  LRL_close_write_record(lrl_record_out);

  free(sitelist);
  return 0;
}

/* Write binary data for a lattice field */
/* Returns 0 success, 1 failure */

int QIO_write_field(QIO_Writer *out, int msg_begin, int msg_end,
	    XML_String *xml_record, 
	    void (*get)(char *buf, size_t index, size_t count, void *arg),
	    size_t datum_size, int word_size, void *arg, 
	    DML_Checksum *checksum,
	    const LIME_type lime_type){
  
  LRL_RecordWriter *lrl_record_out;
  size_t check;
  size_t rec_size;
  size_t volume = out->layout->volume;
  int do_write;
  char myname[] = "QIO_write_field";

  /* Compute record size */
  if(out->volfmt == QIO_SINGLEFILE)
    rec_size = volume * datum_size;  /* Single file holds all the data */
  else{ 
    rec_size = out->layout->sites_on_node * datum_size; /* Multifile */
    printf("%s(%d): sites = %d datum %d\n",myname,out->layout->this_node,
	   out->layout->sites_on_node,datum_size);
  }

#ifdef DEBUG
  printf("%s(%d): rec_size = %d\n",myname,out->layout->this_node,rec_size);
#endif

  /* If multiple nodes are writing to the same file, only
     the master node should actually write the LIME header */
  do_write = ( out->serpar == QIO_SERIAL ) || 
    ( out->layout->this_node == QIO_MASTER_NODE );

  /* Open record */

  lrl_record_out = LRL_open_write_record(out->lrl_file_out, do_write,
					 msg_begin, msg_end, &rec_size, 
					 lime_type);
  /* Write bytes */

  check = DML_stream_out(lrl_record_out, get, datum_size, word_size,
			 arg, out->layout, out->serpar, 
			 out->volfmt, checksum);

  /* Close record when done and clean up*/
  LRL_close_write_record(lrl_record_out);

  if(check != rec_size){
    printf("%s(%d): bytes written %d != expected rec_size %d\n",
	   myname, out->layout->this_node, check, rec_size);
    return 1;
  }

  return 0;
}


/* Read an XML record */
/* Returns 0 success, 1 failure */

int QIO_read_string(QIO_Reader *in, XML_String *xml, LIME_type lime_type){
  char *buf;
  size_t buf_size;
  LRL_RecordReader *lrl_record_in;
  size_t check,rec_size;
  char myname[] = "QIO_read_string";

  /* Open record and find record size */
  if(!in->lrl_file_in)return 0;
  lrl_record_in = LRL_open_read_record(in->lrl_file_in, &rec_size, lime_type);
  if(!lrl_record_in)return 1;

  buf_size = XML_string_bytes(xml);   /* The size allocated for the string */
  buf      = XML_string_ptr(xml);

  /* Realloc if necessary */
  if(rec_size+1 > buf_size){
    XML_string_realloc(xml,rec_size+1);  /* The +1 will insure null terminating string */
  }

  buf_size = XML_string_bytes(xml);   /* Get this again */
  buf      = XML_string_ptr(xml);

  check = LRL_read_bytes(lrl_record_in, buf, rec_size);
  LRL_close_read_record(lrl_record_in);

  if(check != rec_size){
    printf("%s(%d): bytes read %d != expected rec_size %d\n",
	   myname, in->layout->this_node, check, rec_size);
    return 1;
  }

  return 0;
}

/* Read site list */
/* Returns 0 success; 1 failure */

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
    return 1;
  }

  /* Open record and find record size */
  lrl_record_in = LRL_open_read_record(in->lrl_file_in, &rec_size, lime_type);
  if(!lrl_record_in)return 1;

  /* Is record size correct? */
  check = sites*sizeof(DML_SiteRank);
  if(rec_size != check){
    printf("%s(%d): rec size mismatch: found %d expected %d\n",
	   myname, this_node, rec_size, check);
    return 1;
  }
  
  buf = (char *)in->sitelist;
  check = LRL_read_bytes(lrl_record_in, buf, rec_size);

  LRL_close_read_record(lrl_record_in);

#ifdef DEBUG
  printf("%s(%d) site record was read %d\n",myname,in->layout->this_node,check);
#endif

  if(check != rec_size){
    printf("%s(%d): bytes read %d != expected rec_size %d\n",
	   myname, this_node, check, rec_size);
    return 1;
  }

  /* Byte reordering for entire sitelist */
#if(!QIO_BIG_ENDIAN)
  DML_byterevn(buf, rec_size, sizeof(DML_SiteRank));
#endif

  return 0;
}

/* Read binary data for a lattice field */
/* Returns 0 success, 1 failure */

int QIO_read_field(QIO_Reader *in, 
	   void (*put)(char *buf, size_t index, size_t count, void *arg),
	   size_t datum_size, int word_size, void *arg, 
	   DML_Checksum *checksum,
	   LIME_type lime_type){

  LRL_RecordReader *lrl_record_in;
  size_t rec_size, check, buf_size;
  size_t volume = in->layout->volume;
  int this_node = in->layout->this_node;
  char myname[] = "QIO_read_field";

  /* Open record only if we have a file handle */
  if(!in->lrl_file_in) {
#ifdef DEBUG
    printf("%s(%d): skipping LRL_open_read_record %x\n",
	   myname,this_node,in->lrl_file_in);
#endif
  }
  else{
    lrl_record_in = LRL_open_read_record(in->lrl_file_in, 
					 &rec_size, lime_type);
    if(!lrl_record_in)return 1;
    
    /* Check that the record size matches the expected size of the QDP field 
       contained in this file */
    if(in->volfmt == QIO_SINGLEFILE)
      buf_size = volume * datum_size;  /* Single file holds all the data */
    else{ 
      buf_size = in->layout->sites_on_node * datum_size; /* Multifile */
    }
    
    if (rec_size != buf_size){
      printf("%s(%d): rec_size mismatch: found %d expected %d\n",
	     myname, this_node, rec_size, buf_size);
      return 1;
    }
  }

  /* Nodes read and/or collect data.  Compute checksum */
  check = DML_stream_in(lrl_record_in, put, datum_size, word_size, arg, 
			in->layout, in->serpar, in->siteorder, in->sitelist, 
			in->volfmt,  checksum);
#ifdef DEBUG
  printf("%s(%d): done with DML_stream_in\n", myname,this_node);
#endif

  /* Close record when done and clean up*/
  if(in->lrl_file_in){
    LRL_close_read_record(lrl_record_in);
  
    if(check != rec_size){
      printf("%s(%d): bytes read %d != expected rec_size %d\n",
	     myname, in->layout->this_node,check, rec_size);
      return 1;
    }
  }

  return 0;
}
