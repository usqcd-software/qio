#ifndef QIO_CONVERT_MESH_H
#define QIO_CONVERT_MESH_H

#include <qio.h>

/* layout_hyper_mesh */
int setup_layout(int len[], int nd, int nsquares2[], int ndim2);
int node_number(const int x[]);
int node_index(const int x[]);
void get_coords(int x[], int node, int index);

typedef struct
{
  int machdim;
  int *machsize;
  int numnodes;
} QIO_Mesh_Topology;

int qio_mesh_convert(QIO_Filesystem *fs, QIO_Mesh_Topology *mesh,
		     int argc, char *argv[]);

QIO_Mesh_Topology *qio_read_topology();
void qio_destroy_topology(QIO_Mesh_Topology *mesh);

#endif /* QIO_CONVERT_MESH_H */

