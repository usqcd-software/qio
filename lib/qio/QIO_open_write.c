/* QIO_open_write.c */

#include <qio.h>
#include <lrl.h>
#include <dml.h>
#include <qio_string.h>
#include <qioxml.h>
#include <stdio.h>
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

#undef QIO_DEBUG

/* Opens a file for writing */
/* Writes the private file XML record */
/* Writes the site list if multifile format */
/* Writes the user file XML record */

QIO_Writer *QIO_open_write(QIO_String *xml_file, const char *filename, 
			   int volfmt, QIO_Layout *layout, QIO_ioflag oflag)
{
  QIO_Writer *qio_out;
  LRL_FileWriter *lrl_file_out;
  QIO_String *xml_file_private;
  DML_Layout *dml_layout;
  int *latsize;
  int latdim = layout->latdim;
  int this_node = layout->this_node;
  int i;
  char *newfilename;
  QIO_FileInfo *file_info;
  int msg_begin, msg_end;
  char myname[] = "QIO_open_write";

  /* Make a local copy of lattice size */
  latsize = (int *)malloc(sizeof(int)*latdim);
  for(i=0; i < latdim; ++i)
    latsize[i] = layout->latsize[i];

  /* Construct the layout data from the QIO_Layout structure*/
  dml_layout = (DML_Layout *)malloc(sizeof(DML_Layout));
  if (!layout){
    printf("%s(%d): can't malloc dml_layout\n",myname,this_node);
    return NULL;
  }

  /* Force single file format if there is only one node */
  if(layout->number_of_nodes==1) volfmt = QIO_SINGLEFILE;

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

  /* Construct the writer handle */
  qio_out = (QIO_Writer *)malloc(sizeof(QIO_Writer));
  if(qio_out == NULL)return NULL;
  qio_out->lrl_file_out = NULL;
  qio_out->volfmt       = volfmt;
  qio_out->layout       = dml_layout;

  /*****************************/
  /* Open the file for writing */
  /*****************************/
  /* Which node does this depends on the user request. */
  /* If parallel write, all nodes open.
     If multifile, all nodes open.
     If writing to partitions, the partition I/O node does.
     In all cases, the master I/O node opens the file. */

  if( (qio_out->volfmt == QIO_MULTIFILE)
      || ((qio_out->volfmt == QIO_PARTFILE) 
	  && (dml_layout->ionode(this_node) == this_node))
      || (this_node == dml_layout->master_io_node) ){
    /* Modifies filename for non master nodes */
    newfilename = QIO_filename_edit(filename, volfmt, dml_layout->this_node,
				    dml_layout->master_io_node);
    lrl_file_out = LRL_open_write_file(newfilename);
    if(lrl_file_out == NULL){
      printf("%s(%d): failed to open file for writing\n",myname,this_node);
      return NULL;
    }
    qio_out->lrl_file_out = lrl_file_out;
    free(newfilename);
  }

  if(this_node == dml_layout->master_io_node){
    if(qio_out->volfmt == QIO_SINGLEFILE)
      printf("%s(%d): Opened %s for writing in singlefile mode\n",
	     myname,this_node,filename);
    else if(qio_out->volfmt == QIO_MULTIFILE)
      printf("%s(%d): Opened %s for writing in multifile mode\n",
	     myname,this_node,filename);
    else if(qio_out->volfmt == QIO_PARTFILE)
      printf("%s(%d): Opened %s for writing in partfile mode\n",
	     myname,this_node,filename);
  }

  /****************************************/
  /* Load the private file info structure */
  /****************************************/

  file_info = QIO_create_file_info(dml_layout->latdim,
				   dml_layout->latsize,volfmt);
  if(!file_info){
    printf("%s(%d): Can't create file info structure\n",myname,this_node);
    return NULL;
  }

  /*******************************/
  /* Encode the private file XML */
  /*******************************/

  xml_file_private = QIO_string_create();
  QIO_string_realloc(xml_file_private,QIO_STRINGALLOC);
  QIO_encode_file_info(xml_file_private, file_info);
  QIO_destroy_file_info(file_info);
  
  /*******************************************/
  /* Write the file header as a LIME message */
  /*******************************************/

  /* The master I/O node writes the full header consisting of
     (1) private file XML
     (2) site list (only if more than one volume)
     (3) user file XML
     If there are other I/O nodes, they write only
     (1) site list
  */
     
  /* First and last records in a message are flagged */
  msg_begin = 1; msg_end = 0;
  
  /* Master node writes the private file XML record */
  /* For parallel output the other nodes pretend to write */
  if(this_node == dml_layout->master_io_node){
    if(QIO_write_string(qio_out, msg_begin, msg_end, 
			xml_file_private, 
			(const LIME_type)"scidac-private-file-xml")){
      printf("%s(%d): error writing private file XML\n",
	     myname,this_node);
      return NULL;
    }
#ifdef QIO_DEBUG
    /* Debug */
    printf("%s(%d): private file XML = %s\n",
	   myname,this_node,QIO_string_ptr(xml_file_private));
#endif
    msg_begin = 0;
  }
  QIO_string_destroy(xml_file_private);

  /* Determine sites to be written and create site list if needed */

  qio_out->sites = QIO_create_sitelist(qio_out->layout,qio_out->volfmt);
  if(qio_out->sites == NULL){
    printf("%s(%d): error creating sitelist\n",
	   myname,this_node);
    return NULL;
  }

#ifdef QIO_DEBUG
  /* Debug */
  printf("%s(%d): sitelist structure created \n",
	 myname,this_node);
  printf("%s(%d): I/O for %d sites \n",
	 myname,this_node,qio_out->sites->number_of_io_sites);
#endif

  /* Next record is last in message for all but master I/O node */
  if (this_node != dml_layout->master_io_node)msg_end = 1;

  if(QIO_write_sitelist(qio_out, msg_begin, msg_end, 
		     (const LIME_type)"scidac-sitelist")){
    printf("%s(%d): error writing the site list\n", myname,this_node);
    return NULL;
  }

  msg_begin = 0;

  /* Master node writes the user file XML record */
  /* For parallel output the other nodes pretend to write */
  if(this_node == dml_layout->master_io_node){
    msg_end = 1;
    if(QIO_write_string(qio_out, msg_begin, msg_end, xml_file, 
			(const LIME_type)"scidac-file-xml")){
      printf("%s(%d): error writing the user file XML\n",
	     myname,this_node);
      return NULL;
    }
#ifdef QIO_DEBUG
    /* Debug */
    printf("%s(%d): user file XML  = %s\n",
	   myname,this_node, QIO_string_ptr(xml_file));
#endif
  }

  return qio_out;
}
