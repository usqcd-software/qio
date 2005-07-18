/* Host file conversion.  FOR SERIAL PROCESSING ONLY! */
/* Test conversion from SINGLEFILE to PARTFILE format. */
/* Here we test the case that pairs of nodes are assigned to
   an I/O node and each I/O node has a different directory path */

/* Usage

   qio-host-test1 numnodes <0=single_to_part 1=part_to_single> <filename>

*/

#include <qio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "qio-test.h"

#define PATHPREFIX "testpath"

/* Must conform with table below */
int my_io_node(int node){return 2*(node/2);}

int zero_master_io_node(){return 0;}

QIO_Filesystem *create_multi_fs(int numnodes){
  QIO_Filesystem *fs;
  int *io_node_table;
  char **io_node_path;
  char *cmd;
  char mkdircmd[] = "mkdir -p";
  int i;
  int number_io_nodes;
  int lenpathprefix = strlen(PATHPREFIX);
  int lensuffix, lenpath;
  int status;


  /* Number of I/O nodes must conform with the function my_io_node above */
  number_io_nodes = (numnodes + 1)/2;

  /* Build table listing I/O nodes */
  io_node_table = (int *)calloc(number_io_nodes, sizeof(int));
  if(!io_node_table)return NULL;

  for(i = 0; i < number_io_nodes; i++){
    io_node_table[i] = 2*i;
  }

  /* Build table of path names */
  io_node_path = (char **)calloc(number_io_nodes, sizeof(char *));
  if(!io_node_path)return NULL;

  /* We attach a suffix with a character length that depends on the number of
     required digits */
  lensuffix = log10((double)numnodes) + 2;

  lenpath = lenpathprefix + lensuffix + 1;
  for(i = 0; i < number_io_nodes; i++){
    io_node_path[i] = (char *)calloc(lenpath, sizeof(char));
    if(!io_node_path[i])return NULL;
    sprintf(io_node_path[i],"%s%0d",PATHPREFIX,io_node_table[i]);
  }

  /* Create the necessary directories */
  cmd = (char *)calloc(lenpath + strlen(mkdircmd), sizeof(char));
  if(!cmd)return NULL;
  for(i = 0; i < number_io_nodes; i++){
    printf("Creating directory %s\n",io_node_path[i]);
    sprintf(cmd,"%s %s",mkdircmd,io_node_path[i]);
    status = system(cmd);
    if(status){
      printf("Error creating directory\n");
      return NULL;
    }
  }  

  fs = (QIO_Filesystem *)malloc(sizeof(QIO_Filesystem));
  if(!fs){
    printf("Can't malloc fs\n");
    return NULL;
  }
  fs->number_io_nodes = number_io_nodes;
  fs->type = QIO_MULTI_PATH;
  fs->my_io_node = my_io_node;
  fs->master_io_node = zero_master_io_node;
  fs->io_node = io_node_table;
  fs->node_path = io_node_path;

  return fs;
}

void destroy_multi_fs(QIO_Filesystem *fs){
  int i;

  if(fs != NULL){
    if(fs->io_node != NULL)
      free(fs->io_node);
    if(fs->node_path != NULL){
      for(i = 0; i < fs->number_io_nodes; i++)
	if(fs->node_path[i] != NULL)
	  free(fs->node_path[i]);
      free(fs->node_path);
    }
    free(fs);
  }
}

int main(int argc, char *argv[]){
  QIO_Filesystem *fs;
  int status;
  int numnodes;

  if(argc < 2){
    fprintf(stderr,"Usage %s <numnodes> <0=single_to_part 1=part_to_single> <filename>\n",argv[0]);
    return 1;
  }

  /* Number of nodes */
  numnodes = atoi(argv[1]);

  /* Build I/O node table */
  

  /* Create layout and file system structure */
  fs = create_multi_fs(numnodes);
  if(!fs)return 1;

  status = qio_host_test(fs, argc, argv);

  destroy_multi_fs(fs);

  return status;
}
