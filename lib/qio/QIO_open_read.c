/* QIO_open_read.c */

#include <qio.h>
#include <lrl.h>
#include <dml.h>
#include <xml.h>
#include <stdio.h>
#include <stdlib.h>

QIO_Reader *QIO_open_read(XML_MetaData *xml_file, const char *filename, int serpar,
			  QIO_Layout *layout){

  QIO_Reader *qio_in;
  LRL_FileReader *lrl_file_in;
  LRL_RecordReader *lrl_record_in;
  XML_MetaData *xml_file_private;
  DML_Layout *dml_layout;
  int *latsize;
  int latdim = layout->latdim;
  int this_node = layout->this_node;
  int i;

  /* Make a local copy of layout lattize */
  latsize = (int *)malloc(sizeof(int)*latdim);
  for(i=0; i < latdim; ++i)
    latsize[i] = layout->latsize[i];

  /* Construct the layout data from the QIO_Layout structure*/
  dml_layout = (DML_Layout *)malloc(sizeof(QIO_Layout));
  if (layout == NULL)
    return NULL;

  dml_layout->node_number = layout->node_number;
  dml_layout->latsize     = layout->latsize;
  dml_layout->latdim      = layout->latdim;
  dml_layout->volume      = layout->volume;
  dml_layout->this_node   = layout->this_node;

  /* Construct the reader handle */
  qio_in = (QIO_Reader *)malloc(sizeof(QIO_Reader));
  if(qio_in == NULL){
    printf("QIO_open_read: Can't malloc QIO_Reader\n");
    return NULL;
  }
  qio_in->lrl_file_in = NULL;
  qio_in->serpar = serpar;
  qio_in->volfmt = QIO_SINGLEFILE;
  qio_in->layout = dml_layout;

  /* If parallel, all nodes open the file.  Otherwise, only master does.*/
  if((PARALLEL_READ && serpar == QIO_PARALLEL)
     || this_node == QIO_MASTER_NODE){
    lrl_file_in = LRL_open_read_file(filename);
    qio_in->lrl_file_in = lrl_file_in;
  }

  /* Create private file XML and make space to hold input XML*/
  xml_file_private = XML_create(MAX_XML);
  
  /* Master node reads the private file XML record */
  if(this_node == QIO_MASTER_NODE){
    if(!QIO_read_XML(qio_in, xml_file_private)){
      printf("QIO_open_read: error reading private file XML\n");
      return NULL;
    }
    printf("QIO_open_read: private file XML = %s\n",
	   XML_string(xml_file_private));
  }
  
  /* Here we should process the private XML data */
  /* We need the volume format for consistency checking */
  /*** OMITTED FOR NOW ***/
  qio_in->volfmt = QIO_SINGLEFILE;
  
  /* Then we free the storage */
  XML_destroy(xml_file_private);
  
  /* Master node reads the user file XML record */
  if(this_node == QIO_MASTER_NODE){
    if(!QIO_read_XML(qio_in, xml_file)){
      printf("QIO_open_read: error reading user file XML\n");
      return NULL;
    }
    printf("QIO_open_read: file XML = %s\n",XML_string(xml_file));
  }
  return qio_in;
}

