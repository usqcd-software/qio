/* QIO_open_read.c */

#include <qio.h>
#include <lrl.h>
#include <dml.h>
#include <qio_string.h>
#include <qioxml.h>
#include <stdio.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#undef QIO_DEBUG

/* Opens a file for reading */
/* Gets the volume format code from the file header */
/* Reads, interprets, and broadcasts the private file XML record */
/* Reads the site list if partfile or multifile format */
/* Reads and broadcasts the user file XML record */

QIO_Reader *QIO_open_read(QIO_String *xml_file, const char *filename, 
			  QIO_Layout *layout, QIO_ioflag iflag){

  /* Calling program must allocate *xml_file */

  QIO_Reader *qio_in;
  LRL_FileReader *lrl_file_in = NULL;
  QIO_String *xml_file_private;
  DML_Layout *dml_layout;
  QIO_FileInfo *file_info_expect, *file_info_found;
  int *latsize;
  int latdim = layout->latdim;
  int this_node = layout->this_node;
  int i;
  LIME_type lime_type = NULL;
  int length;
  int status;
  char *newfilename;
  int volfmt;
  char myname[] = "QIO_open_read";

  /* Make a local copy of lattice size */
  latsize = (int *)malloc(sizeof(int)*latdim);
  for(i=0; i < latdim; ++i)
    latsize[i] = layout->latsize[i];

  /* Construct the layout data from the QIO_Layout structure*/
  dml_layout = (DML_Layout *)malloc(sizeof(DML_Layout));
  if (layout == NULL)
    return NULL;

  dml_layout->node_number        = layout->node_number;
  dml_layout->node_index         = layout->node_index;
  dml_layout->get_coords         = layout->get_coords;
  dml_layout->latsize            = latsize;
  dml_layout->latdim             = layout->latdim;
  dml_layout->volume             = layout->volume;
  dml_layout->sites_on_node      = layout->sites_on_node;
  dml_layout->this_node          = layout->this_node;
  dml_layout->number_of_nodes    = layout->number_of_nodes;
			         
  dml_layout->ionode             = DML_io_node;
  dml_layout->master_io_node     = DML_master_io_node();

  /* Construct the reader handle */
  qio_in = (QIO_Reader *)malloc(sizeof(QIO_Reader));
  if(qio_in == NULL){
    printf("%s(%d): Can't malloc QIO_Reader\n",myname,this_node);
    return NULL;
  }
  qio_in->lrl_file_in = NULL;
  qio_in->layout      = dml_layout;
  qio_in->read_state  = QIO_RECORD_XML_NEXT;

  /*******************************************************************/
  /* Master I/O node opens the file for reading and reads the header */
  /*******************************************************************/

  /* First, only the global master node opens the file, regardless of
     whether it will be read by all nodes */
  if(this_node == dml_layout->master_io_node){
    lrl_file_in = LRL_open_read_file(filename);
    if(lrl_file_in == NULL){
      printf("%s(%d): Can't open %s\n",myname,this_node,filename);
      return NULL;
    }
    qio_in->lrl_file_in = lrl_file_in;
  }

#ifdef QIO_DEBUG
  printf("%s(%d) Reading xml_file_private\n",myname,this_node);fflush(stdout);
#endif

  /* Create structure to hold what the file says */
  file_info_found = QIO_create_file_info(0,NULL,0);

  /* Master node reads and decodes the private file XML record */
  /* For parallel input the other nodes pretend to read */
  if(this_node == dml_layout->master_io_node){
    xml_file_private = QIO_string_create();
    QIO_string_realloc(xml_file_private,QIO_STRINGALLOC);
    if((status = 
	QIO_read_string(qio_in, xml_file_private, lime_type))
       !=QIO_SUCCESS){
      printf("%s(%d): error %d reading private file XML\n",
	     myname,this_node,status);
      return NULL;
    }
#ifdef QIO_DEBUG
    printf("%s(%d): private file XML = %s\n",myname,this_node,
	   QIO_string_ptr(xml_file_private));
#endif
    /* Decode the file info */
    QIO_decode_file_info(file_info_found, xml_file_private);
    QIO_string_destroy(xml_file_private);

    /* Create structure with what we expect */
    /* We discover and respond to the volume format parameter.
       A zero here implies a matching value is not required. */
    file_info_expect = 
      QIO_create_file_info(dml_layout->latdim, dml_layout->latsize, 
			   dml_layout->master_io_node);	    
					    
    /* Compare what we found and what we expected */
    if((status = 
	QIO_compare_file_info(file_info_found,file_info_expect,
			      myname,this_node))!=QIO_SUCCESS){
      printf("%s(%d): Error %d checking file info\n",myname,this_node,status);
      return NULL;
    }
    QIO_destroy_file_info(file_info_expect);
  }

  /* Broadcast the file info to all nodes */
  DML_broadcast_bytes((char *)file_info_found, sizeof(QIO_FileInfo),
		      this_node, dml_layout->master_io_node);
  
#ifdef QIO_DEBUG
  printf("%s(%d): private file info was broadcast\n",
	 myname,this_node);fflush(stdout);
#endif

  volfmt = QIO_get_volfmt(file_info_found);
  QIO_destroy_file_info(file_info_found);
  
  /*********************************************************************/
  /* Act on the volume format.  Other nodes open their file (if needed)*/
  /*********************************************************************/

  /* Open any additional file handles as needed */
  /* Read the sitelist if needed */

  if(volfmt == QIO_SINGLEFILE)
    {
      /* One file for all nodes */
      printf("%s(%d): opened %s for reading in singlefile mode\n",
	     myname,this_node,filename);fflush(stdout);
      qio_in->volfmt = QIO_SINGLEFILE;
    }
  else if(volfmt == QIO_PARTFILE)
    {
      /* One file per machine partition in lexicographic order */
      printf("%s(%d): opened %s for reading in partfile mode\n",
	     myname,this_node,filename);fflush(stdout);
      qio_in->volfmt = QIO_PARTFILE;

      /* All the partition I/O nodes open their files.  */
      if(this_node == dml_layout->ionode(this_node)){
	/* (The global master has already opened its file) */
	if(this_node != dml_layout->master_io_node){
	  /* Construct the file name based on the partition I/O node number */
	  newfilename = QIO_filename_edit(filename, qio_in->volfmt, this_node,
					  dml_layout->master_io_node);
	  /* Open the file */
	  lrl_file_in = LRL_open_read_file(newfilename);
	  qio_in->lrl_file_in = lrl_file_in; 
	}
      }
    }
  else if(volfmt == QIO_MULTIFILE)
    {
      /* One file per node */
      printf("%s(%d): opened %s for reading in multifile mode\n",
	     myname,this_node,filename);fflush(stdout);
      qio_in->volfmt = QIO_MULTIFILE;
      /* The non-master nodes open their files */
      if(this_node != dml_layout->master_io_node){
	/* Edit file name */
	newfilename = QIO_filename_edit(filename, qio_in->volfmt, 
				layout->this_node, dml_layout->master_io_node);
	lrl_file_in = LRL_open_read_file(newfilename);
	qio_in->lrl_file_in = lrl_file_in; 
      }
    }

  else 
    {
      printf("%s(%d): bad volfmt parameter %d\n",myname,this_node,volfmt);
      return NULL;
    }

  /****************************************/
  /* Nodes read their site lists (if any) */
  /****************************************/

  /* Create the expected sitelist.  Input must agree exactly. */

  qio_in->sites = QIO_create_sitelist(qio_in->layout, qio_in->volfmt);
  if(qio_in->sites == NULL){
    printf("%s(%d): error creating sitelist\n",
	   myname,this_node);
    return NULL;
  }
  
  if((status = QIO_read_sitelist(qio_in, lime_type))!= QIO_SUCCESS){
    printf("%s(%d): Error %d reading site list\n",myname,this_node,status);
    return NULL;
  }
#ifdef QIO_DEBUG
  printf("%s(%d): Sitelist was read\n",myname,this_node);fflush(stdout);
#endif
  

  /*************************************************************/
  /* Master node reads and broadcasts the user file XML record */
  /*************************************************************/

#ifdef QIO_DEBUG
  printf("%s(%d): Reading user file XML\n",myname,this_node);fflush(stdout);
#endif
  
  /* Assumes the xml_file structure was created by caller */
  if(this_node == dml_layout->master_io_node){
    if((status = 
	QIO_read_string(qio_in, xml_file, lime_type))!= QIO_SUCCESS){
      printf("%s(%d): error %d reading user file XML\n",
	     myname,this_node,status);
      return NULL;
    }
    
#ifdef QIO_DEBUG
    printf("%s(%d): file XML = %s\n",
	   myname,this_node,QIO_string_ptr(xml_file));
#endif
    length = QIO_string_length(xml_file);
  }
  
  /* Broadcast the user xml file to all nodes */
  /* First broadcast length */
  DML_broadcast_bytes((char *)&length,sizeof(int),
		      this_node, dml_layout->master_io_node);
  
  /* Receiving nodes resize their strings */
  if(this_node != dml_layout->master_io_node){
    QIO_string_realloc(xml_file,length);
  }

  /* Then broadcast the string itself */
  DML_broadcast_bytes(QIO_string_ptr(xml_file),length,
		      this_node, dml_layout->master_io_node);
  
#ifdef QIO_DEBUG
  printf("%s(%d): Done with user file XML\n",myname,this_node);fflush(stdout);
#endif
  
  return qio_in;
}
