/* QIO_open_write.c */

#include <qio.h>
#include <lrl.h>
#include <dml.h>
#include <xml.h>
#include <stdio.h>
#include <malloc.h>

/* Dummy for now */
char file_private_output[] = "XML with QIO version no and lattice size";

QIO_Writer *QIO_open_write(XML_string *xml_file, const char *filename, 
			   int serpar, int siteorder, int mode,
			   QIO_Layout *layout)
{
  QIO_Writer *qio_out;
  LRL_FileWriter *lrl_file_out;
  XML_string *xml_file_private;
  DML_Layout *dml_layout;
  int *latsize;
  int latdim = layout->latdim;
  int this_node = layout->this_node;
  int i;

  /* Make a local copy of lattice size */
  latsize = (int *)malloc(sizeof(int)*latdim);
  for(i=0; i < latdim; ++i)
    latsize[i] = layout->latsize[i];

  /* Construct the layout data from the QIO_Layout structure*/
  dml_layout = (DML_Layout *)malloc(sizeof(QIO_Layout));
  if (layout == NULL)
    return NULL;

  dml_layout->node_number = layout->node_number;
  dml_layout->latsize     = latsize;
  dml_layout->latdim      = layout->latdim;
  dml_layout->volume      = layout->volume;
  dml_layout->this_node   = layout->this_node;

  /* Construct the writer handle */
  qio_out = (QIO_Writer *)malloc(sizeof(QIO_Writer));
  if(qio_out == NULL)return NULL;
  qio_out->lrl_file_out = NULL;
  qio_out->serpar = serpar;
  qio_out->siteorder = siteorder;
  qio_out->layout = dml_layout;
  qio_out->volfmt = QIO_SINGLEFILE;

  /* If parallel, all nodes open the file.  Otherwise, only master does.*/
  if((PARALLEL_WRITE && serpar == QIO_PARALLEL)
     || this_node == QIO_MASTER_NODE){
    lrl_file_out = LRL_open_write_file(filename, mode);
    qio_out->lrl_file_out = lrl_file_out;
  }

  /* Create private file XML */
  /* This is a dummy for now, but we need to write at least
     QIO version number, lattice dimensions, and the volume format */
  xml_file_private = XML_string_create(QIO_MAX_STRING_LEN);
  XML_string_set(xml_file_private, file_private_output);
  
  /* Master node writes the private file XML record */
  if(this_node == QIO_MASTER_NODE){
    if(QIO_write_string(qio_out,xml_file_private)){
      printf("QIO_open_write: error writing private file XML\n");
      return NULL;
    }
    printf("QIO_open_write: private file XML = %s\n",
	   XML_string_ptr(xml_file_private));
  }

  /* Free storage */
  XML_string_destroy(xml_file_private);

  /* Master node writes the user file XML record */
  if(this_node == QIO_MASTER_NODE){
    if(QIO_write_string(qio_out,xml_file)){
      printf("QIO_open_write: error writing user file XML\n");
      return NULL;
    }
    printf("QIO_open_write: user file XML  = %s\n",
	   XML_string_ptr(xml_file));
  }

  return qio_out;
}

