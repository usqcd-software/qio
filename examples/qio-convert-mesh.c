/* Convert between SINGLEFILE and PARTFILE format.
   on single processor. */

#include <stdio.h>
#include <qio.h>
#define MAIN
#include "qio-convert-mesh.h"

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
  layout->sites_on_node = 0;  /* Ignored */
  layout->this_node = 0;      /* Reset */
  layout->number_of_nodes = numnodes;
  return layout;
}

void destroy_mpp_layout(QIO_Layout *layout){
  free(layout);
}

QIO_Mesh_Topology *qio_read_topology(){
  int i;

  QIO_Mesh_Topology *mesh = (QIO_Mesh_Topology *)malloc(sizeof(QIO_Mesh_Topology));

  scanf("%d",&mesh->machdim);
  mesh->machsize = (int *)malloc(mesh->machdim*sizeof(int));
  if(mesh->machsize == NULL){
    printf("Can't malloc machsize with dim %d\n",mesh->machdim);
    return NULL;
  }

  mesh->numnodes = 1;
  for(i = 0; i < mesh->machdim; i++){
    scanf("%d",&mesh->machsize[i]);
    mesh->numnodes *= mesh->machsize[i];
  }
  return mesh;
}

void qio_destroy_topology(QIO_Mesh_Topology *mesh){
  free(mesh->machsize);
  free(mesh);
}

int qio_mesh_convert(QIO_Filesystem *fs, QIO_Mesh_Topology *mesh,
		     int argc, char *argv[])
{

  /* For the case that each nodes reads its own file from the same
     directory path */

  QIO_Layout *mpp_layout;

  int n;
  int status;
  int numnodes = mesh->numnodes;
  int part_to_single;
  int latdim;
  int *latsize;
  QIO_Reader *qio_in;
  char *filename;
  char *stringLFN;
  QIO_String *ildgLFN;

  QIO_verbose(QIO_VERB_LOW);

  /* Command line options */

  n = 1;

  /* Which direction to convert? */
  part_to_single = atoi(argv[n++]);

  /* File name */
  filename = argv[n++];

  /* ildgLFN, if specified. Used only when recombining a file. */
  if(argc > n)
    stringLFN = argv[n++];
  else
    stringLFN = NULL;

  ildgLFN = QIO_string_create();
  QIO_string_set(ildgLFN, stringLFN);

  /* Start from a dummy layout */
  mpp_layout = create_mpp_layout(numnodes, NULL, 0);
  if(!mpp_layout)return 1;

  /* Get lattice dimensions from file */
  qio_in = QIO_open_read_master(filename, mpp_layout, 0, fs->my_io_node,
                       fs->master_io_node);
  if(!qio_in)return 1;

  latdim = QIO_get_reader_latdim(qio_in);
  latsize = QIO_get_reader_latsize(qio_in);

  /* Now create the real layout functions */
  mpp_layout = create_mpp_layout(numnodes, latsize, latdim);
  if(setup_layout(latsize, latdim,mesh->machsize, mesh->machdim)){
    printf("Error in setup_layout\n");
    return 1;
  }

  if(!mpp_layout)return 1;

  /* Do the conversion */
  if(part_to_single == 0)
    {
      printf("Converting %s from SINGLEFILE to PARTFILE\n",filename);
      status = QIO_single_to_part(filename, fs, mpp_layout);
    }
  else if(part_to_single == 1)
    {
      /* ILDG compatible format */
      printf("Converting %s from PARTFILE to SINGLEFILE ILDG\n",filename);
      status = QIO_part_to_single(filename, QIO_ILDGLAT, ildgLFN, 
				  fs, mpp_layout);
    }
  else
    {
      /* SciDAC native format */
      printf("Converting %s from PARTFILE to SINGLEFILE SciDAC\n",filename);
      status = QIO_part_to_single(filename, QIO_ILDGNO, NULL, fs, mpp_layout);
    }

  /* Clean up */
  destroy_mpp_layout(mpp_layout);

  return status;
}

