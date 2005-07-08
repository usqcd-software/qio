/* Host file conversion.  FOR SERIAL PROCESSING ONLY! */
/* Test conversion between SINGLEFILE and PARTFILE format. */

#include <stdio.h>
#include <qio.h>
#define MAIN
#include "qio-test.h"

QIO_Layout *create_mpp_layout(int numnodes, int *latsize, int latdim){
  int i;
  size_t volume;
  QIO_Layout *layout;

  layout = (QIO_Layout *)malloc(sizeof(QIO_Layout));
  if(!layout){
    printf("Can't malloc layout\n");
    return NULL;
  }
  volume = 1;
  for(i = 0; i < latdim; i++)
    volume *= latsize[i];

  layout->node_number = node_number;
  layout->node_index = node_index;
  layout->get_coords = get_coords;
  layout->num_sites = num_sites;
  layout->latsize = latsize;
  layout->latdim = latdim;
  layout->volume = volume;
  layout->sites_on_node = sites_on_node;
  layout->this_node = 0;      /* Reset */
  layout->number_of_nodes = numnodes;
  return layout;
}

void destroy_mpp_layout(QIO_Layout *layout){
  free(layout);
}

int qio_host_test(QIO_Filesystem *fs, int argc, char *argv[])
{

  /* For the case that each nodes reads its own file from the same
     directory path */

  QIO_Layout *mpp_layout;

  int n;
  int status;
  int numnodes,part_to_single;
  int latdim;
  int *latsize;
  QIO_Reader *qio_in;
  char *filename;
  char *newfilename;

  QIO_verbose(QIO_VERB_DEBUG);

  /* Command line options */

  if(argc < 4){
    fprintf(stderr,"Usage %s numnodes <0=single_to_part 1=part_to_single> <filename>\n",argv[0]);
    return 1;
  }

  n = 1;  /* arg 1 was number of nodes */

  /* Number of nodes */
  numnodes = atoi(argv[n++]);

  /* Which direction to convert? */
  part_to_single = atoi(argv[n++]);

  /* File name */
  filename = argv[n++];

  /* Start from a dummy layout */
  sites_on_node = 0;
  mpp_layout = create_mpp_layout(numnodes, NULL, 0);
  if(!mpp_layout)return 1;

  /* Get lattice dimensions from file */
  if(part_to_single)
    {
      /* If converting part to single, add the path to the file name */
      newfilename = QIO_set_filepath(fs,filename,fs->master_io_node());
      qio_in = QIO_open_read_master(newfilename, mpp_layout, 
				    NULL, fs->my_io_node,fs->master_io_node);
      free(newfilename);
    }
  else
    {
      /* If converting single to part, open the single file */
      qio_in = QIO_open_read_master(filename, mpp_layout, 
				    NULL, fs->my_io_node,fs->master_io_node);
    }

  if(!qio_in)return 1;
  latdim = QIO_get_reader_latdim(qio_in);
  latsize = QIO_get_reader_latsize(qio_in);

  /* Now create the real layout functions */
  setup_layout(latsize, latdim, numnodes);
  mpp_layout = create_mpp_layout(numnodes, latsize, latdim);

  if(!mpp_layout)return 1;

  /* Do the conversion */
  if(part_to_single)
    {
      printf("Converting %s from PARTFILE to SINGLEFILE SciDAC\n",filename);
      status = QIO_part_to_single(filename, QIO_ILDGNO, fs, mpp_layout);
    }
  else
    {
      printf("Converting %s from SINGLEFILE to PARTFILE\n",filename);
      status = QIO_single_to_part(filename, fs, mpp_layout);
    }

  /* Clean up */
  destroy_mpp_layout(mpp_layout);

  return status;
}

