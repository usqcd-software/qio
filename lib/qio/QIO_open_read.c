/* QIO_open_read.c */

#include <qio.h>
#include <lrl.h>
#include <dml.h>
#include <xml_string.h>
#include <qioxml.h>
#include <stdio.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#undef QIO_DEBUG


#undef PARALLEL_READ
#if defined(QIO_USE_PARALLEL_READ)
#define PARALLEL_READ 1
#else
#define PARALLEL_READ 0
#endif

/* Opens a file for reading */
/* Discovers whether the file is single or multifile format */
/* Reads, interprets, and broadcasts the private file XML record */
/* Reads the site list if multifile format */
/* Reads and broadcasts the user file XML record */

QIO_Reader *QIO_open_read(XML_String *xml_file, const char *filename, 
			  int serpar, QIO_Layout *layout){

  /* Calling program must allocate *xml_file */

  QIO_Reader *qio_in;
  LRL_FileReader *lrl_file_in = NULL;
  XML_String *xml_file_private;
  DML_Layout *dml_layout;
  QIO_FileInfo *file_info_expect, *file_info_found;
  int *latsize;
  int latdim = layout->latdim;
  int this_node = layout->this_node;
  int i;
  LIME_type lime_type = NULL;
  int length;
  int check;
  char *newfilename;
  int *dims;
  char myname[] = "QIO_open_read";

  /* Make a local copy of lattice size */
  latsize = (int *)malloc(sizeof(int)*latdim);
  for(i=0; i < latdim; ++i)
    latsize[i] = layout->latsize[i];

  /* Construct the layout data from the QIO_Layout structure*/
  dml_layout = (DML_Layout *)malloc(sizeof(QIO_Layout));
  if (layout == NULL)
    return NULL;

  dml_layout->node_number     = layout->node_number;
  dml_layout->node_index      = layout->node_index;
  dml_layout->get_coords      = layout->get_coords;
  dml_layout->latsize         = latsize;
  dml_layout->latdim          = layout->latdim;
  dml_layout->volume          = layout->volume;
  dml_layout->sites_on_node   = layout->sites_on_node;
  dml_layout->this_node       = layout->this_node;
  dml_layout->number_of_nodes = layout->number_of_nodes;

  /* Construct the reader handle */
  qio_in = (QIO_Reader *)malloc(sizeof(QIO_Reader));
  if(qio_in == NULL){
    printf("%s(%d): Can't malloc QIO_Reader\n",myname,this_node);
    return NULL;
  }
  qio_in->lrl_file_in = NULL;
  qio_in->serpar      = serpar;
  qio_in->layout      = dml_layout;
  qio_in->read_state  = QIO_RECORD_XML_NEXT;

  /* First, only the master node opens the file, regardless of
     whether it will be read by all nodes */
  if(this_node == QIO_MASTER_NODE){
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

  /* Create structure with what the file says */
  file_info_found = QIO_create_file_info(0,NULL,0);

  /* Master node reads and decodes the private file XML record */
  if(this_node == QIO_MASTER_NODE){
    xml_file_private = XML_string_create(QIO_STRINGALLOC);
    if(QIO_read_string(qio_in, xml_file_private, lime_type)){
      printf("%s(%d): error reading private file XML\n",myname,this_node);
      return NULL;
    }
#ifdef QIO_DEBUG
    printf("%s(%d): private file XML = %s\n",myname,this_node,
	   XML_string_ptr(xml_file_private));
#endif
    QIO_decode_file_info(file_info_found, xml_file_private);
    XML_string_destroy(xml_file_private);

    /* Create structure with what we expect */
    /* We discover and respond to the multifile parameter.
       A zero here implies a matching value is not required. */
    file_info_expect = 
      QIO_create_file_info(dml_layout->latdim, dml_layout->latsize, 0);	    
					    
    /* Compare what we found and what we expected */
    if(QIO_compare_file_info(file_info_found,file_info_expect,
			     myname,this_node)){
      return NULL;
    }
    QIO_destroy_file_info(file_info_expect);
  }

  /* Broadcast the file info to all nodes */
  DML_broadcast_bytes((char *)file_info_found, sizeof(QIO_FileInfo));
  
#ifdef QIO_DEBUG
  printf("%s(%d): private file info was broadcast\n",
	 myname,this_node);fflush(stdout);
#endif
  
  /* We need the volume format for consistency checking */
  /* If parallel read possible, and parallel read requested,
     all nodes open the file.  Otherwise, only master does.*/
  
  /* Are we reading multiple files? */
  if(QIO_get_multifile(file_info_found) == 1){
    /* Single file */
    printf("%s(%d): reading %s as single file\n",
	   myname,this_node,filename);fflush(stdout);
    qio_in->volfmt = QIO_SINGLEFILE;
    /* Single file site ordering must be lexicographic */
    qio_in->siteorder = QIO_LEX_ORDER;
    /* If parallel read is possible and requested, the remaining nodes
       open the same file */
    if((PARALLEL_READ && qio_in->serpar == QIO_PARALLEL) && 
       this_node != QIO_MASTER_NODE){ 
      lrl_file_in = LRL_open_read_file(filename);
      qio_in->lrl_file_in = lrl_file_in; 
    }
    else{
      /* Otherwise, we must read serially through the master node 
         regardless of what is requested */
      qio_in->serpar == QIO_SERIAL;
    }
  }
  else {
    /* Multifile */
    printf("%s(%d): reading %s as multifile\n",
	   myname,this_node,filename);fflush(stdout);
    qio_in->volfmt = QIO_MULTIFILE;
    /* For now we support only multifile reads with one file per node */
    if(QIO_get_multifile(file_info_found) != layout->number_of_nodes){
      printf("%s(%d): multifile volume count %d must match number_of_nodes %d\n",
	     myname,this_node,QIO_get_multifile(file_info_found), 
	     layout->number_of_nodes);
      return NULL;
    }
    /* The non-master nodes open their files */
    if(this_node != QIO_MASTER_NODE){
      /* Edit file name */
      newfilename = QIO_filename_edit(filename, qio_in->volfmt, 
				      layout->this_node);
      lrl_file_in = LRL_open_read_file(newfilename);
      qio_in->lrl_file_in = lrl_file_in; 
    }
    
    /* Each node reads its own site list */
    if(QIO_read_sitelist(qio_in, lime_type))return NULL;
#ifdef QIO_DEBUG
    printf("%s(%d): Sitelist was read\n",myname,this_node);fflush(stdout);
#endif
    /* For now we support only a site order that matches the current
       layout exactly */
    check = DML_is_native_sitelist(qio_in->layout,qio_in->sitelist);
    /* Return value is zero for native order, one for nonnative */
    /* Poll all nodes to be sure all have native order */
    DML_sum_int(&check);
    if(check){
      qio_in->siteorder = QIO_LIST_ORDER;
      if(this_node == QIO_MASTER_NODE)
	printf("%s(%d): List-directed reordering not supported\n",
	       myname,this_node);
      return NULL;
    }
    else{
#ifdef QIO_DEBUG
      printf("%s(%d): List is in natural order\n",
	     myname,this_node);fflush(stdout);
#endif
      qio_in->siteorder = QIO_NAT_ORDER;
    }
  }

  QIO_destroy_file_info(file_info_found);

#ifdef QIO_DEBUG
  printf("%s(%d): Reading user file XML\n",myname,this_node);fflush(stdout);
#endif
  
  /* Master node reads the user file XML record */
  /* Assumes xml_file created by caller */
  if(this_node == QIO_MASTER_NODE){
    if(QIO_read_string(qio_in, xml_file, lime_type)){
      printf("%s(%d): error reading user file XML\n",myname,this_node);
      return NULL;
    }
    
#ifdef QIO_DEBUG
    printf("%s(%d): file XML = %s\n",
	   myname,this_node,XML_string_ptr(xml_file));
#endif
    length = XML_string_bytes(xml_file);
  }
  
  /* Broadcast the user xml file to all nodes */
  /* First broadcast length */
  DML_broadcast_bytes((char *)&length,sizeof(int));
  
  /* Receiving nodes resize their strings */
  if(this_node != QIO_MASTER_NODE){
    XML_string_realloc(xml_file,length);
  }
  DML_broadcast_bytes(XML_string_ptr(xml_file),length);
  
#ifdef QIO_DEBUG
  printf("%s(%d): Done with user file XML\n",myname,this_node);fflush(stdout);
#endif
  
  return qio_in;
}
