/* Host file conversion.  FOR SERIAL PROCESSING ONLY! */
/* Test conversion from SINGLEFILE to PARTFILE format. */
/* Here we test the simplest case that each node reads its own file
   from the same directory path */

/* Usage

   qio-host-test1 numnodes <0=single_to_part 1=part_to_single> <filename>

*/

#include <stdio.h>
#include <qio.h>
#include "qio-test.h"

int self_io_node(int node){return node;}

int zero_master_io_node(){return 0;}

QIO_Filesystem *create_simple_fs(int numnodes){
  QIO_Filesystem *fs;

  fs = (QIO_Filesystem *)malloc(sizeof(QIO_Filesystem));
  if(!fs){
    printf("Can't malloc fs\n");
    return NULL;
  }
  fs->number_io_nodes = numnodes;
  fs->type = QIO_SINGLE_PATH;
  fs->my_io_node = self_io_node;
  fs->master_io_node = zero_master_io_node;
  fs->io_node = NULL;
  fs->node_path = NULL;

  return fs;
}

void destroy_simple_fs(QIO_Filesystem *fs){
  free(fs);
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

  /* Create layout and file system structure */
  fs = create_simple_fs(numnodes);
  if(!fs)return 1;

  status = qio_host_test(fs, argc, argv);

  destroy_simple_fs(fs);

  return status;
}
