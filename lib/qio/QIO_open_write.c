/* QIO_open_write.c */

#include <qio.h>
#include <lrl.h>
#include <dml.h>
#include <xml_string.h>
#include <qioxml.h>
#include <stdio.h>
#include <malloc.h>

#undef QIO_DEBUG

#undef PARALLEL_WRITE
#if defined(QIO_USE_PARALLEL_WRITE)
#define PARALLEL_WRITE 1
#else
#define PARALLEL_WRITE 0
#endif

/* Opens a file for writing */
/* Writes the private file XML record */
/* Writes the site list if multifile format */
/* Writes the user file XML record */

QIO_Writer *QIO_open_write(XML_String *xml_file, const char *filename, 
			   int serpar, int volfmt, int mode,
			   QIO_Layout *layout)
{
  QIO_Writer *qio_out;
  LRL_FileWriter *lrl_file_out;
  XML_String *xml_file_private;
  DML_Layout *dml_layout;
  int *latsize;
  int latdim = layout->latdim;
  int this_node = layout->this_node;
  int i;
  char *newfilename;
  QIO_FileInfo *file_info;
  int multifile;
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

  dml_layout->node_number     = layout->node_number;
  dml_layout->node_index      = layout->node_index;
  dml_layout->get_coords      = layout->get_coords;
  dml_layout->latsize         = latsize;
  dml_layout->latdim          = layout->latdim;
  dml_layout->volume          = layout->volume;
  dml_layout->sites_on_node   = layout->sites_on_node;
  dml_layout->this_node       = layout->this_node;
  dml_layout->number_of_nodes = layout->number_of_nodes;

  /* Construct the writer handle */
  qio_out = (QIO_Writer *)malloc(sizeof(QIO_Writer));
  if(qio_out == NULL)return NULL;
  qio_out->lrl_file_out = NULL;
  qio_out->serpar       = serpar;
  qio_out->volfmt       = volfmt;
  qio_out->layout       = dml_layout;

  /* If the system handles parallel writes and the user requests it,
     or if writing in multifile, all nodes open the file.  Otherwise,
     only master does. */

  if((PARALLEL_WRITE && serpar == QIO_PARALLEL)
     || volfmt == QIO_MULTIFILE
     || this_node == QIO_MASTER_NODE){
    /* Modify filename for multifile writes */
    newfilename = QIO_filename_edit(filename, volfmt, layout->this_node);
    lrl_file_out = LRL_open_write_file(newfilename, mode);
    qio_out->lrl_file_out = lrl_file_out;
    free(newfilename);
  }

  /* Load the private file info structure */
  /* Multifile flag */
  if(volfmt == QIO_SINGLEFILE)multifile = 1;
  else multifile = dml_layout->number_of_nodes;

  /* Lattice dimensions */
  file_info = QIO_create_file_info(dml_layout->latdim,
				   dml_layout->latsize,multifile);
  if(!file_info){
    printf("%s(%d): Can't create file info structure\n",myname,this_node);
    return NULL;
  }

  /* Encode the private file XML */
  xml_file_private = XML_string_create(QIO_STRINGALLOC);
  QIO_encode_file_info(xml_file_private, file_info);
  QIO_destroy_file_info(file_info);
  
  /* A message consists of the XML, binary payload, and checksums */
  /* First and last records in a message are flagged */
  msg_begin = 1; msg_end = 0;
  
  /* Master node writes the private file XML record */
  if(this_node == QIO_MASTER_NODE){
    if(QIO_write_string(qio_out, msg_begin, msg_end, xml_file_private, 
	      (const LIME_type)"scidac-private-file-xml")){
      printf("%s(%d): error writing private file XML\n",
	     myname,this_node);
      return NULL;
    }
#ifdef QIO_DEBUG
    /* Debug */
    printf("%s(%d): private file XML = %s\n",
	   myname,this_node,XML_string_ptr(xml_file_private));
#endif
    msg_begin = 0;
  }

  /* Free storage */
  XML_string_destroy(xml_file_private);

  /* Next record is last in message for all but master node */
  if (this_node != QIO_MASTER_NODE)msg_end = 1;

  /* All nodes write a site list if multifile format */
  if(volfmt == QIO_MULTIFILE){
    QIO_write_sitelist(qio_out, msg_begin, msg_end, 
		       (const LIME_type)"scidac-sitelist");
    msg_begin = 0;
  }

  /* Master node writes the user file XML record */
  if(this_node == QIO_MASTER_NODE){
    msg_end = 1;
    if(QIO_write_string(qio_out, msg_begin, msg_end, xml_file, 
			(const LIME_type)"scidac-file-xml")){
      printf("%s(%d): error writing user file XML\n",
	     myname,this_node);
      return NULL;
    }
#ifdef QIO_DEBUG
    /* Debug */
    printf("%s(%d): user file XML  = %s\n",
	   myname,this_node, XML_string_ptr(xml_file));
#endif
  }

  return qio_out;
}
