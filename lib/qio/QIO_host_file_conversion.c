/* Single processor code for converting between SciDAC SINGLEFILE and
   SciDAC PARTFILE format */

#include <qio_config.h>
#include <qio.h>
#include <dml.h>
#include <qio_string.h>
#include <qioxml.h>
#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

typedef struct
{
  char *data;
  size_t datum_size;
  size_t volume;
} s_field;

int QIO_init_scalar_field(s_field *field,int vol,size_t datum_size)
{
  field->datum_size = datum_size;
  field->volume = vol;
  size_t bytes = datum_size*vol;

  field->data = (char *) malloc(bytes);
  if(!field->data){
    printf("QIO_init_scalar_field: Can't malloc field data (%f MB)\n",
	   (float)bytes/1e6);
    return 1;
  }
  return 0;
}

typedef struct
{
  s_field *field;
  int node;
  int master_io_node;
} get_put_arg;

void QIO_init_get_put_arg(get_put_arg *arg, s_field *field, int node,
			  int master_io_node)
{
  arg->field = field;
  arg->node = node;
  arg->master_io_node = master_io_node;
}

void QIO_free_scalar_field(s_field *field)
{
  free(field->data);
}

int QIO_bytes_of_word(char *type)
{
  int value;
  switch(*type)
    {
    case 'I':
      value = 4;
      break;
      
    case 'F':
      value = 4;
      break;
      
    case 'D':
      value = 8;
      break;
      
    case 'S':
      value = 4;
      break;
    }
  
  return value;
}



void check_scalar_layout(QIO_Layout *s_layout, QIO_Layout *layout)
{
  /* Force the number of sites to be the entire lattice
     This is probably not necessary*/
  
  if (layout->sites_on_node != layout->volume)
    s_layout->sites_on_node = layout->volume ;
}


/* Copy a chunk of data of length "datum_size" from the input buffer
   to the field */
void QIO_scalar_put( char *s1 , size_t scalar_index, int count, void *s2 )
{
  get_put_arg *arg = s2;
  s_field *field = arg->field;
  size_t datum_size = field->datum_size;
  char *dest = field->data + scalar_index*datum_size;
  memcpy(dest,s1,datum_size);
}


/* Copy a chunk of data of length "datum_size" from the input buffer
   to the field */
void QIO_scalar_put_global( char *s1 , size_t scalar_index, 
			    int count, void *s2 )
{
  get_put_arg *arg = s2;
  s_field *field = arg->field;
  size_t datum_size = field->datum_size;
  char *dest = field->data;

  /* For the scalar case we ignore the scalar_index */
  memcpy(dest,s1,datum_size);
}


/* Copy a chunk of data of length "datum_size" from the input buffer
   to the field */
void QIO_part_put( char *s1 , size_t ionode_index, int count, void *s2 )
{
  get_put_arg *arg = s2;
  s_field *field = arg->field;
  int ionode_node = arg->node;
  size_t datum_size = field->datum_size;
  int scalar_index;
  char *dest;

  /* Convert ionode_index to scalar_index */
  scalar_index = QIO_ionode_to_scalar_index(ionode_node,ionode_index);
  dest = field->data + scalar_index*datum_size;
  memcpy(dest,s1,datum_size);
}


/* Copy a chunk of global data of length "datum_size" from the input buffer
   to the field */
void QIO_part_put_global( char *s1 , size_t ionode_index, int count, void *s2 )
{
  get_put_arg *arg = s2;
  s_field *field = arg->field;
  int node = arg->node;
  int master_io_node = arg->master_io_node;
  size_t datum_size = field->datum_size;
  char *dest = field->data;

  /* Copy buffer only for the master node and ignore the ionode_index */
  if(node == master_io_node)
    memcpy(dest,s1,datum_size);
}


/* Copy a chunk of data of length "datum_size" from the field to the
   output buffer */
void QIO_scalar_get( char *s1 , size_t ionode_index, int count, void *s2 )
{
  get_put_arg *arg = s2;
  s_field *field = arg->field;
  int ionode_node = arg->node;
  size_t datum_size = field->datum_size;
  int scalar_index;

  /* Convert ionode_index to scalar_index */
  scalar_index = QIO_ionode_to_scalar_index(ionode_node,ionode_index);
  char *src = field->data + scalar_index*datum_size;
  memcpy(s1,src,datum_size);
}

void QIO_part_get( char *s1 , size_t scalar_index, int count, void *s2 )
{
  get_put_arg *arg = s2;
  s_field *field = arg->field;
  size_t datum_size = field->datum_size;
  char *src = field->data + scalar_index*datum_size;
  memcpy(s1,src,datum_size);
}

/* my_io_node function for host should be called only for node 0 */
int QIO_host_my_io_node(int node)
{
  return node;
}

/* master I/O node for host */
int QIO_host_master_io_node( void )
{
  return 0;
}


int QIO_set_this_node(QIO_Filesystem *fs, QIO_Layout *layout, int node)
{
  if ( fs->number_io_nodes < layout->number_of_nodes)
    return fs->io_node[node];
  else 
    return node;
}

char *QIO_set_filepath(QIO_Filesystem *fs, 
		  const char * const filename, int node)
{
  char *path, *newfilename;
  int fnlength = strlen(filename);
  int drlength;
  
  if (fs->type == QIO_MULTI_PATH)
    {
      path = fs->node_path[node];
      drlength = strlen(path);
      newfilename = (char *) malloc(fnlength+drlength+2);
      if(!newfilename){
	printf("QIO_set_filepath: Can't malloc newfilename\n");
	return NULL;
      }
      strncpy(newfilename, path, drlength+1);
      strncat(newfilename, "/", 2);
      strncat(newfilename, filename, fnlength+1);
    }
  else if (fs->type == QIO_SINGLE_PATH)
    {
      newfilename = (char *) malloc(fnlength);
      strncpy(newfilename,filename,fnlength+1);
    }
  
  return newfilename;
}

int QIO_single_to_part( const char filename[], QIO_Filesystem *fs,
			QIO_Layout *layout)
{
  QIO_Layout *scalar_layout, *ionode_layout;
  QIO_String *xml_file_in, *xml_record_in;
  QIO_String *xml_file_out, *xml_record_out, *xml_checksum;
  QIO_Reader *infile;
  QIO_Writer **outfile;
  QIO_RecordInfo rec_info;
  DML_Checksum checksum_out, checksum;
  QIO_ChecksumInfo *checksum_info;
  uint64_t nbytes,totnbytes;
  int *msg_begin, *msg_end;
  int i,status,master_io_node_rank;
  int number_io_nodes = fs->number_io_nodes;
  int master_io_node = fs->master_io_node();
  size_t total_bytes,datum_size;
  int typesize,datacount,vol,globaldata,wordsize,volfmt;
  char *newfilename;
  s_field field_in;
  get_put_arg arg;
  char myname[] = "QIO_single_to_part";
 
  if(number_io_nodes <= 1){
   printf("%s: No conversion since number_io_nodes %d <= 1\n",
	  myname,number_io_nodes);
   return 1;
  }
 
  /* Create the file XML */
  xml_file_in = QIO_string_create();
  
  /* Create scalar layout structure */
  scalar_layout = QIO_create_scalar_layout(layout, fs);
  
  /* Create scalar layout structure */
  ionode_layout = QIO_create_ionode_layout(layout, fs);
  
  /* Check on the scalar layout */
  check_scalar_layout(scalar_layout,layout);
  
  /* Which entry in the table is the master node? */
  master_io_node_rank = QIO_get_io_node_rank(master_io_node);
  if(master_io_node_rank < 0){
    printf("%s: Bad Filesystem structure.  Master node %d is not an I/O node\n",
	   myname,master_io_node);
    return QIO_ERR_BAD_IONODE;
  }

  /* Open the file for reading */
  infile = QIO_open_read_master(filename, scalar_layout, 0,
				QIO_host_my_io_node,
				QIO_host_master_io_node);
  
  if (infile->volfmt != QIO_SINGLEFILE)
    {
      printf("%s: File %s format %d is not SINGLEFILE\n",
	     myname, filename, infile->volfmt);
      return QIO_ERR_BAD_VOLFMT;
    }
  
  /* Needed to initialize site list */
  status = QIO_read_check_sitelist(infile);
  if(status != QIO_SUCCESS)return status;

  status = QIO_read_user_file_xml(xml_file_in, infile);
  if(status != QIO_SUCCESS)return status;
  
  /* Create output file XML */
  xml_file_out = QIO_string_create();
  QIO_string_copy(xml_file_out, xml_file_in);
  
  
  /* Set the output volfmt */
  volfmt = QIO_PARTFILE;
  
  outfile = (QIO_Writer **) malloc (sizeof(QIO_Writer *)*number_io_nodes);
  if(!outfile){
    printf("%s: Can't malloc outfiles (%f MB)\n", myname,
	   sizeof(QIO_Writer *)*((float)number_io_nodes)/1e6);
    return QIO_ERR_ALLOC;
  }

  msg_begin = (int *)malloc(sizeof(int)*number_io_nodes);
  msg_end   = (int *)malloc(sizeof(int)*number_io_nodes);
  if(!msg_begin || !msg_end){
    printf("%s: Can't malloc msg_begin, msg_end\n", myname);
    return QIO_ERR_ALLOC;
  }
  
  for (i=0; i < number_io_nodes; i++)
    {
      /* Set this_node */
      ionode_layout->this_node = QIO_set_this_node(fs,layout,i);
      
      /* Set output path according to MULTI/SINGLE PATH flag */
      newfilename = QIO_set_filepath(fs,filename,i);
      if(status)return status;
      
      /* Open to write */
      outfile[i] = QIO_generic_open_write(xml_file_out,newfilename,volfmt,
             ionode_layout,0,fs->my_io_node,fs->master_io_node);
      
      free(newfilename);
      
    }
  
  /***** iterate on records up to EOF ***********/
  while (1)
    {
      /* Create the record XML */
      xml_record_in = QIO_string_create();
      
      /* Read the record info */
      status = QIO_read_private_record_info(infile, &rec_info);
      if (status==QIO_EOF)break;
      if (status!=QIO_SUCCESS) return status;

      status = QIO_read_user_record_info(infile, xml_record_in);
      if (status!=QIO_SUCCESS) return status;
      
      datacount = QIO_get_datacount(&rec_info);
      typesize = QIO_get_typesize(&rec_info);
      wordsize = QIO_bytes_of_word(QIO_get_precision(&rec_info));
      datum_size = typesize*datacount;
      
      /* Create a field */
      globaldata = QIO_get_globaldata(&rec_info);
      if( globaldata == QIO_GLOBAL) vol = 1;
      else vol = scalar_layout->volume;
      
      QIO_init_scalar_field(&field_in,vol,datum_size);
      QIO_init_get_put_arg(&arg, &field_in, QIO_host_master_io_node(),
			   QIO_host_master_io_node());
      /* Read data */
      /* Factory function depends on global/field type */
      if( globaldata == QIO_GLOBAL)
	status = 
	  QIO_read_record_data(infile,QIO_scalar_put_global,datum_size,
			       wordsize,&arg);
      else
	status = 
	  QIO_read_record_data(infile,QIO_scalar_put,datum_size,
			       wordsize,&arg);
      if(status != QIO_SUCCESS)return status;
      
      if(globaldata == QIO_GLOBAL)total_bytes = datum_size;
      else total_bytes = layout->volume * datum_size;

      /* Set record XML */
      xml_record_out = QIO_string_create();
      QIO_string_copy(xml_record_out, xml_record_in);
      
      totnbytes = 0;
      DML_checksum_init(&checksum_out);
      for (i=0; i < number_io_nodes; i++)
	{
	  
	  ionode_layout->this_node = QIO_set_this_node(fs,layout,i);
	  QIO_init_get_put_arg(&arg, &field_in, ionode_layout->this_node,
			       master_io_node);
	  
	  status = QIO_generic_write(outfile[i], &rec_info, xml_record_out, 
				     QIO_scalar_get, datum_size, wordsize, 
				     &arg, &checksum, &nbytes,
				     &msg_begin[i], &msg_end[i]);
	  if(status != QIO_SUCCESS)return status;
	  /* Add partial byte count to total */
	  totnbytes += nbytes;
	  /* Add partial checksum to total */
	  DML_checksum_peq(&checksum_out, &checksum);
	}

      /* Compare byte count with expected record size */
      if(totnbytes != total_bytes)
	{
	  printf("%s: bytes written %lu != expected rec_size %lu\n",
		 myname, (unsigned long)totnbytes, (unsigned long)total_bytes);
	  return QIO_ERR_BAD_WRITE_BYTES;
	}
      
      /* Build and write checksum record */
      checksum_info = QIO_create_checksum_info(checksum_out.suma,
					       checksum_out.sumb);
      xml_checksum = QIO_string_create();
      QIO_encode_checksum_info(xml_checksum, checksum_info);

      msg_end[master_io_node_rank] = 1;
      if ((status = 
	   QIO_write_string(outfile[master_io_node_rank], 
			    msg_begin[master_io_node_rank], 
			    msg_end[master_io_node_rank], xml_checksum,
			    (const LIME_type)"scidac-checksum"))
	  != QIO_SUCCESS){
	printf("%s: Error writing checksum\n",myname);
	return status;
      }

      if(QIO_verbosity() >= QIO_VERB_REG){
	printf("%s: Wrote field. datatype %s globaltype %d \n              precision %s colors %d spins %d count %d\n",
	       myname,
	       QIO_get_datatype(&rec_info),
	       QIO_get_globaldata(&rec_info),
	       QIO_get_precision(&rec_info),
	       QIO_get_colors(&rec_info),
	       QIO_get_spins(&rec_info),
	       QIO_get_datacount(&rec_info));
	
	printf("%s: checksum string = %s\n",
	       myname,QIO_string_ptr(xml_checksum));
      }
      
      QIO_string_destroy(xml_checksum);
      QIO_string_destroy(xml_record_in);
      QIO_string_destroy(xml_record_out);
      QIO_free_scalar_field(&field_in);
      QIO_destroy_checksum_info(checksum_info);
      
    }
  /************* end iteration on records *********/
  
  QIO_close_read(infile);
  QIO_string_destroy(xml_file_in);
  
  
  for (i=0; i < number_io_nodes; i++)
    QIO_close_write(outfile[i]);

  QIO_delete_scalar_layout(scalar_layout);
  QIO_delete_ionode_layout(ionode_layout);

  return QIO_SUCCESS;
}

int QIO_part_to_single( const char filename[], QIO_Filesystem *fs,
			QIO_Layout *layout)
{
  QIO_Layout *scalar_layout, *ionode_layout;
  QIO_String *xml_file_in, *xml_record_in;
  QIO_String *xml_file_out, *xml_record_out, *xml_checksum;
  QIO_Reader **infile;
  QIO_Writer *outfile;
  QIO_RecordInfo rec_info;
  DML_Checksum checksum_out, checksum_in, checksum;
  QIO_ChecksumInfo *checksum_info_out, *checksum_info_expect;
  uint64_t nbytes,totnbytes;
  int msg_begin, msg_end;
  int i,status,master_io_node_rank;
  int number_io_nodes = fs->number_io_nodes;
  int master_io_node = fs->master_io_node();
  size_t total_bytes,datum_size;
  int typesize,datacount,vol,globaldata,wordsize,volfmt;
  char *newfilename;
  s_field field_in;
  get_put_arg arg;
  FILE *check;
  char myname[] = "QIO_part_to_single";

  /* Sanity checks */

  /* No conversion if the number of nodes is not greater than 1 */
  if(number_io_nodes <= 1){
   printf("%s: No conversion since number_io_nodes %d <= 1\n",
	  myname,number_io_nodes);
   return 1;
  }

  /* The single file target must not exist.  Otherwise, QIO_open_read
     confuses it with the input partition file. */
  
  check = fopen(filename,"r");
  if(check){
    printf("%s: No conversion since the file %s already exists\n",
	   myname, filename);
    fclose(check);
    return 1;
  }

  /* Create scalar layout structure */
  scalar_layout = QIO_create_scalar_layout(layout, fs);

  /* Check on the scalar layout */
  check_scalar_layout(scalar_layout,layout);
  
  /* Create scalar layout structure */
  ionode_layout = QIO_create_ionode_layout(layout, fs);

  /* Which entry in the table is the master node? */
  master_io_node_rank = QIO_get_io_node_rank(master_io_node);
  if(master_io_node_rank < 0){
    printf("%s: Bad Filesystem structure.  Master node %d is not an I/O node\n",
           myname, master_io_node);
    return QIO_ERR_BAD_IONODE;
  }

  /* Open the files for reading */
  infile = (QIO_Reader **) malloc (sizeof(QIO_Reader *)*number_io_nodes);
  if(!infile){
    printf("%s: Can't malloc infiles (%f MB)\n",
           myname,sizeof(QIO_Writer *)*((float)number_io_nodes)/1e6);
    return QIO_ERR_ALLOC;
  }
  
  /* Open master I/O file and check volume format */
  i = master_io_node_rank;

  /* Set this node */
  ionode_layout->this_node = QIO_set_this_node(fs,layout,i);
  
  /* Set output path according to MULTI/SINGLE PATH flag */
  newfilename = QIO_set_filepath(fs,filename,i);
  if(status)return status;
  
  /* Open to read */
  infile[i] = QIO_open_read_master(newfilename,ionode_layout,
				   0,fs->my_io_node,fs->master_io_node);
  /* Check the volume format */
  volfmt = infile[i]->volfmt;

  if (volfmt != QIO_PARTFILE){
    printf("%s(%d) File %s volume format must be PARTFILE.  Found %d\n",
	   myname, i, newfilename,infile[i]->volfmt);
    return QIO_ERR_FILE_INFO;
  }
      
  /* Create input file XML */
  xml_file_in = QIO_string_create();

  for (i=0; i < number_io_nodes; i++)
    {
      /* Set this node */
      ionode_layout->this_node = QIO_set_this_node(fs,layout,i);
      
      if(i != master_io_node_rank){
	
	/* Set output path according to MULTI/SINGLE PATH flag */
	newfilename = QIO_set_filepath(fs,filename,i);
	if(status)return status;
	
	/* Open to read */
	infile[i] = QIO_open_read_master(newfilename,ionode_layout,
					 0,fs->my_io_node,fs->master_io_node);

	/* Set volume format */
	infile[i]->volfmt = volfmt;
      }
      
      status = QIO_open_read_nonmaster(infile[i],newfilename);
      if(status != QIO_SUCCESS)return status;
      
      status = QIO_read_check_sitelist(infile[i]);
      if(status != QIO_SUCCESS)return status;
      
      status = QIO_read_user_file_xml(xml_file_in, infile[i]);
      if(status != QIO_SUCCESS)return status;
      
      free(newfilename);
      
    }
  
  /* Create output file XML */
  xml_file_out = QIO_string_create();
  QIO_string_copy(xml_file_out, xml_file_in);
  
  /* Open the files for writing */
  outfile =  QIO_generic_open_write(xml_file_out,filename,QIO_SINGLEFILE,
				    scalar_layout, 0,
				    QIO_host_my_io_node,
				    QIO_host_master_io_node);
  
  /***** iterate on records up to EOF ***********/
  
  while (1)
    {
      /* Create the record XML */
      xml_record_in = QIO_string_create();
      
      /* Read the record info from the master file */
      status = QIO_read_private_record_info(infile[master_io_node_rank], 
					    &rec_info);
      if (status!=QIO_SUCCESS) return status;
      if (status==QIO_EOF)break;
      
      /* Set state for the remaining files */

      for(i = 0; i < number_io_nodes; i++)if(i != master_io_node_rank)
	{
	  /* Copy master record info to nonmaster reader */
	  infile[i]->record_info = rec_info;
	  status = QIO_read_private_record_info(infile[i], &rec_info);
	  if (status!=QIO_SUCCESS) return status;
	  if (status==QIO_EOF)break;
	}

      datacount  = QIO_get_datacount(&rec_info);
      typesize   = QIO_get_typesize(&rec_info);
      wordsize   = QIO_bytes_of_word(QIO_get_precision(&rec_info));
      datum_size = typesize*datacount;
      globaldata = QIO_get_globaldata(&rec_info);

      /* Read the user record info from the master */

      status = QIO_read_user_record_info(infile[master_io_node_rank], 
					 xml_record_in);
      if(status != QIO_SUCCESS)return status;
      
      /* Set state for the remaining files */

      for(i = 0; i < number_io_nodes; i++)if(i != master_io_node_rank)
	{
	  /* Copy master user record info to nonmaster reader */
	  QIO_string_copy(infile[i]->xml_record, xml_record_in);
	  status = QIO_read_user_record_info(infile[i], xml_record_in);
	  if (status!=QIO_SUCCESS) return status;
	}

      if( globaldata == QIO_GLOBAL)
	{
	  vol = 1;
	  total_bytes = datum_size;
	}
      else
	{
	  vol = layout->volume;
	  total_bytes = layout->volume * datum_size;
	}
      
      /* Create space for holding the binary data */
      QIO_init_scalar_field(&field_in,vol,datum_size);
      
      totnbytes = 0;
      DML_checksum_init(&checksum_in);
      for (i=0; i < number_io_nodes; i++)
	{
	  ionode_layout->this_node = QIO_set_this_node(fs,layout,i);
	  
	  QIO_init_get_put_arg(&arg, &field_in, ionode_layout->this_node,
			       master_io_node);

	  /* Read data.  Factory function depends on global/field type */
	  if( globaldata == QIO_GLOBAL)
	    status = 
	      QIO_generic_read_record_data(infile[i],QIO_part_put_global,
					   datum_size, wordsize,
					   &arg,&checksum,&nbytes);
	  else
	    status = 
	      QIO_generic_read_record_data(infile[i],QIO_part_put,
					   datum_size,wordsize,
					   &arg,&checksum,&nbytes);
	  if(status != QIO_EOF && status != QIO_SUCCESS)return status;
	  
	  if(status == QIO_EOF) 
	    printf("%s: ERROR. io_node_rank %d reached end of file\n",
		   myname,i);
	  
	  if(status != QIO_EOF)
	    {
	      /* Add partial byte count to total */
	      totnbytes += nbytes;
	      
	      /* Add partial checksum to total */
	      DML_checksum_peq(&checksum_in, &checksum);
	    }
	}
      /* Compare byte count with expected record size */
      if(totnbytes != total_bytes)
	{
	  printf("%s: ERROR bytes written %lu != expected rec_size %lu\n",
		 myname,
		 (unsigned long)totnbytes, (unsigned long)total_bytes);
	  return QIO_ERR_BAD_WRITE_BYTES;
	}
      
      /* Get checksum from master */

      checksum_info_expect = QIO_read_checksum(infile[master_io_node_rank]);
      if(checksum_info_expect == NULL)return QIO_ERR_CHECKSUM_INFO;

      /* Set state for the remaining files */

      for(i = 0; i < number_io_nodes; i++)if(i != master_io_node_rank)
	QIO_read_checksum(infile[i]);
      
      status = QIO_compare_checksum(master_io_node, 
				    checksum_info_expect, &checksum_in);
      if (status != QIO_SUCCESS) return status;
      
#ifdef undef 
#endif
      
      /* Set record XML */
      xml_record_out = QIO_string_create();
      QIO_string_copy(xml_record_out, xml_record_in);
      
      QIO_init_get_put_arg(&arg, &field_in, scalar_layout->this_node,
			   master_io_node);
      nbytes = 0;
      DML_checksum_init(&checksum_out);
      status = QIO_generic_write(outfile, &rec_info, xml_record_out,
				 QIO_part_get, datum_size, wordsize,
				 &arg, &checksum_out, &nbytes,
				 &msg_begin, &msg_end);
      if(status != QIO_SUCCESS)return status;
      
      checksum_info_out = QIO_create_checksum_info(checksum_out.suma,
                                               checksum_out.sumb);
      xml_checksum = QIO_string_create();
      QIO_encode_checksum_info(xml_checksum, checksum_info_out);
      
      msg_end = 1;
      if ((status =
           QIO_write_string(outfile,
                            msg_begin,
                            msg_end, xml_checksum,
                            (const LIME_type)"scidac-checksum"))
          != QIO_SUCCESS){
        printf("%s: Error writing checksum\n",myname);
        return status;
      }
      
      /* Compare output byte count with input byte count */
      if(totnbytes != nbytes)
	{
	  printf("%s: ERROR: bytes written %lu != expected bytes %lu\n",
		 myname,
		 (unsigned long)totnbytes, (unsigned long)nbytes);
	  return QIO_ERR_BAD_WRITE_BYTES;
	}
      
      /* Compare output checksum with input checksum */
      status = QIO_compare_checksum_info(checksum_info_out, 
					 checksum_info_expect,
					 myname,master_io_node);
      if(status != QIO_SUCCESS){
	printf("%s: ERROR. Output file checksum disagrees with input checksum\n",
	       myname);
	return status;
      }
      

      if(QIO_verbosity() >= QIO_VERB_REG){
	printf("%s: Wrote field. datatype %s globaltype %d \n precision %s colors %d spins %d count %d\n",
	       myname,
	       QIO_get_datatype(&rec_info),
	       QIO_get_globaldata(&rec_info),
	       QIO_get_precision(&rec_info),
	       QIO_get_colors(&rec_info),
	       QIO_get_spins(&rec_info),
	       QIO_get_datacount(&rec_info));
	
	printf("%s: checksum string = %s\n",
	       myname,QIO_string_ptr(xml_checksum));
	
      }

      QIO_string_destroy(xml_checksum);
      QIO_string_destroy(xml_record_in);
      QIO_string_destroy(xml_record_out);
      QIO_free_scalar_field(&field_in);
      QIO_destroy_checksum_info(checksum_info_expect);
      QIO_destroy_checksum_info(checksum_info_out);
      
    }
  /************* end iteration on records *********/
  
  for (i=0; i < number_io_nodes; i++)
    QIO_close_read(infile[i]);
  QIO_string_destroy(xml_file_in);
  
  
  QIO_close_write(outfile);
  
  QIO_delete_scalar_layout(scalar_layout);
  QIO_delete_ionode_layout(ionode_layout);
  
  return QIO_SUCCESS;
}
