/* QIO utilities for host file conversion */

#include <qio_config.h>
#include <qio.h>
#include <dml.h>
#include <stdio.h>

#define QIO_NON_IO -1

/* Table lookup */
/* Searches node offset table for node belonging to fake node index */
/* Answer is the last node number in the family "io_node_rank" whose
   node_index offset is greater than or equal to the given node_index
   "seek" */
int QIO_offset_lookup(size_t seek, int io_node_rank, QIO_host_utils_s *hu){
  int k,ans,del;
  int *nodelist = hu->QIO_io_family[io_node_rank].node_number;
  int n = hu->QIO_io_family[io_node_rank].n;

  if(n == 0)return -1;  /* Bad table */
  if(n == 1)return nodelist[0];
  
  /* We support nonequal sites per node, but nearly always there will
     be equal sites per node . */
  /* So guess the answer, assuming roughly equal sites per node */
  del = hu->QIO_node_index_offset[nodelist[1]] 
    - hu->QIO_node_index_offset[nodelist[0]];
  if(del <= 0)return -1;   /* Bad table */
  else ans = seek/del;
  if(ans > n) ans = n;

  /* Now refine the answer, first going forward */
  for(k = ans; k < n-1; k++){
    if(hu->QIO_node_index_offset[nodelist[k+1]] > seek)break;
  }
  ans = k;
  /* then going backward */
  for(k = ans; k >= 0; k--){
    if(hu->QIO_node_index_offset[nodelist[k]] <= seek)break;
  }
  ans = k;
  return nodelist[ans];
}

/* Save layout structure as it is on compute (MPP) nodes */

void QIO_init_mpp_layout(QIO_Layout *layout, QIO_host_utils_s *hu){
  hu->QIO_mpp_layout = *layout;
}

/* Save file and I/O structure as it is on compute (MPP) nodes */

void QIO_init_mpp_fs(QIO_Filesystem *fs, QIO_host_utils_s *hu){
  hu->QIO_mpp_fs = *fs;
}

/*---------------------------------------------------------------*/
/* Layout functions that pretend all sites for an I/O node are on
   the I/O node */
/*---------------------------------------------------------------*/
/* If each node does its own IO, faking is not necessary.
   
   Otherwise we create an "ionode" layout.  The nodes are grouped into
   IO families.  Each family shares one I/O node.  Family membership
   is determined by the user-supplied "my_io_node_a" function.  The
   table QIO_io_family lists family membership.  (The list of nodes in
   a family happens to be in ascending numerical order.)  The fake
   node_number function assigns all sites in an IO family to the IO
   node.  A fake node_index function assigns a unique rank to each
   site in the family.  This index is the sum of the true
   compute-node_index and an offset.  Forward mapping from coordinate
   to node_index is accomplished by adding the offset to the true node
   index.  Reverse mapping requires a table lookup to determine the
   true node number.
*/

/* Map coordinates to fake node (the IO node) */
int QIO_ionode_node_number_a(const int coords[], void *arg)
{
  QIO_host_utils_s *hu = (QIO_host_utils_s *)arg;
  return hu->QIO_mpp_fs.my_io_node_a(
                hu->QIO_mpp_layout.node_number_a(
                        coords, hu->QIO_mpp_layout.arg), 
                hu->QIO_mpp_fs.arg);
}

/* Map coordinates to fake node index */
int QIO_ionode_node_index_a(const int coords[], void *arg)
{
  QIO_host_utils_s *hu = (QIO_host_utils_s *)arg;
  /* The actual node that will receive these coordinates */
  int node = hu->QIO_mpp_layout.node_number_a(coords, hu->QIO_mpp_layout.arg);

  /* The fake node_index offset for this node */
  int offset = hu->QIO_node_index_offset[node];
  /* Add the compute node site index to an offset for "this_node" being
   processed */
  return offset + hu->QIO_mpp_layout.node_index_a(coords, hu->QIO_mpp_layout.arg);
}

/* An ionode pretends it owns all the sites belonging to its
   partition.  The fake index for these sites runs over the total
   number of sites on the partition. */
/* Map fake index and node to coordinates */
void QIO_ionode_get_coords_a(int coords[], int ionode_node, int ionode_index, void *arg){
  QIO_host_utils_s *hu = (QIO_host_utils_s *)arg;
  char myname[] = "QIO_ionode_get_coords";
  int k;
  int index,node;

  /* If we are not using the ionode layout, use the mpp layout */
  if(!hu->QIO_fake_ionode_layout){
    hu->QIO_mpp_layout.get_coords_a(coords, ionode_node, ionode_index, hu->QIO_mpp_layout.arg);
    return;
  }

  k = hu->QIO_ionode_to_rank[ionode_node];

  /* The specified node must be an I/O node */
  if(k == QIO_NON_IO){
    printf("%s: This node %d is not an I/O node\n",myname,ionode_node);
    return;
  }

  /* Table lookup returns the true node number for this offset */
  node = QIO_offset_lookup(ionode_index, k, hu);
  if(node < 0){
    printf("%s: Bad QIO_io_family table\n",myname);
    return;
  }
  /* True node index */
  index = ionode_index - hu->QIO_node_index_offset[node];

  /* Set coordinates from true node and index */
  hu->QIO_mpp_layout.get_coords_a(coords, node, index, hu->QIO_mpp_layout.arg);
}

/* The fake number of sites on the given node. */
int QIO_ionode_num_sites_a(int node, void *arg)
{
  QIO_host_utils_s *hu = (QIO_host_utils_s *)arg;
  int k = hu->QIO_ionode_to_rank[node];

  /* Non-IO nodes have no sites in our fake layout scheme */
  if(k == QIO_NON_IO)
    return 0;

  return hu->QIO_io_family[k].num_sites;
}

/* Make table of nodes in each IO family if needed */
/* Return 0 success and 1 failure */
int QIO_create_io_node_table(QIO_host_utils_s *hu){
  char myname[] = "QIO_create_io_node_table";
  int i,j,k,maxinit,io_node;
  int number_of_nodes = hu->QIO_mpp_layout.number_of_nodes;
  int number_io_nodes = hu->QIO_mpp_fs.number_io_nodes;
  size_t next,sum;
#if 0
  int latdim = QIO_mpp_layout.latdim;
  int *latsize = QIO_mpp_layout.latsize;
  size_t volume = QIO_mpp_layout.volume;
  int *coords;
  DML_SiteRank site_rank;
#endif

  /* Create array for the inverse of the fs->io_node table */
  hu->QIO_ionode_to_rank = (int *)calloc(number_of_nodes, sizeof(int));
  if(!hu->QIO_ionode_to_rank){
    printf("%s Can't malloc QIO_ionode_to_rank\n",myname);
    return 1;
  }

  /* Initialize array */
  for(i = 0; i < number_of_nodes; i++)
    hu->QIO_ionode_to_rank[i] = QIO_NON_IO;

  /* Invert the rank to io_node table */
  for(k = 0; k < number_io_nodes; k++){
    hu->QIO_ionode_to_rank[hu->QIO_mpp_fs.io_node[k]] = k;
  }

  /* Create table of node offsets */
  hu->QIO_node_index_offset = (size_t *)calloc(number_of_nodes, sizeof(size_t));
  if(!hu->QIO_node_index_offset){
    printf("%s Can't malloc QIO_node_index_offset\n",myname);
    return 1;
  }

  /* Initialize array */
  for(i = 0; i < number_of_nodes; i++)
    hu->QIO_node_index_offset[i] = 0;

  /* Create the table of IO family membership */
  hu->QIO_io_family = (QIO_IOFamilyMember *)calloc(number_io_nodes, 
			    sizeof(QIO_IOFamilyMember));
  if(!hu->QIO_io_family){
    printf("%s Can't malloc QIO_io_family\n",myname);
    return 1;
  }

  /* Initialize table of I/O families */
  if(number_io_nodes <= 0){
    printf("%s: %d is not a valid number_io_nodes\n",myname,number_io_nodes);
    return 1;
  }
  maxinit = number_of_nodes/number_io_nodes;  /* Estimate number needed */
  if(maxinit == 0){
    printf("%s: %d number_io_nodes exceeds %d number_of_nodes\n",
	   myname,number_io_nodes,number_of_nodes);
    return 1;
  }
  for(k = 0; k < number_io_nodes; k++){
    hu->QIO_io_family[k].n = 0;
    hu->QIO_io_family[k].max = maxinit;
    hu->QIO_io_family[k].node_number = (int *)calloc(maxinit, sizeof(int));
    if(!hu->QIO_io_family[k].node_number){
      printf("%s Can't malloc QIO_io_family[k].node_number\n",myname);
      return 1;
    }
  }

  /* Build table listing the nodes assigned to each I/O node */
  for(i = 0; i < number_of_nodes; i++){
    io_node = hu->QIO_mpp_fs.my_io_node_a(i, hu->QIO_mpp_fs.arg);
    if(io_node >= number_of_nodes){
      printf("%s my_io_node_a function returns %d >= %d number_of_nodes\n",
	     myname,io_node,number_of_nodes);
      return 1;
    }
    k = hu->QIO_ionode_to_rank[io_node];
    if(k == QIO_NON_IO){
      printf("%s I/O node %d not found in io_node table\n",myname,io_node);
      return 1;
    }
    /* Add node to list in table */
    hu->QIO_io_family[k].n++;
    if(hu->QIO_io_family[k].n > hu->QIO_io_family[k].max){
      /* Make space for one more entry */
      hu->QIO_io_family[k].max = hu->QIO_io_family[k].n;
      hu->QIO_io_family[k].node_number 
	= (int *)realloc(hu->QIO_io_family[k].node_number,
			 hu->QIO_io_family[k].max*sizeof(int));
    }
    hu->QIO_io_family[k].node_number[hu->QIO_io_family[k].n-1] = i;
  }

  /* Build table of node_index offsets for each node */
  /* Start by counting sites per node */
  for(i = 0; i < hu->QIO_mpp_layout.number_of_nodes; i++)
    hu->QIO_node_index_offset[i] = hu->QIO_mpp_layout.num_sites_a(i, hu->QIO_mpp_layout.arg);

#if 0
  coords = DML_allocate_coords(latdim,myname,0);
  if(!coords)return 1;
  for(site_rank = 0; site_rank < volume; site_rank++){
    DML_lex_coords(coords,latdim,latsize,site_rank);
    i = hu->QIO_mpp_layout.node_number_a(coords, hu->QIO_mpp_layout.arg);
    hu->QIO_node_index_offset[i]++;
  }
  free(coords);
#endif

  /* Accumulate node_index offsets */
  /* Offsets are computed for each I/O family based on the listed
     order of nodes in the QIO_io_family table */
  for(k = 0; k < number_io_nodes; k++){
    sum = 0;  next = 0;
    for(j = 0; j < hu->QIO_io_family[k].n; j++){
      sum += next;
      next = hu->QIO_node_index_offset[hu->QIO_io_family[k].node_number[j]];
      hu->QIO_node_index_offset[hu->QIO_io_family[k].node_number[j]] = sum;
    }
    hu->QIO_io_family[k].num_sites = sum + next;
  }
  
  return 0;
}

void QIO_delete_io_node_table(QIO_host_utils_s *hu){
  int number_io_nodes = hu->QIO_mpp_fs.number_io_nodes;
  int k;

  if(!hu->QIO_fake_ionode_layout)return;

  if(hu->QIO_ionode_to_rank != NULL)
    free(hu->QIO_ionode_to_rank);
  if(hu->QIO_node_index_offset != NULL)
    free(hu->QIO_node_index_offset);
  if(!hu->QIO_io_family){
    for(k = 0; k < number_io_nodes; k++){
      if(!hu->QIO_io_family[k].node_number){
	free(hu->QIO_io_family[k].node_number);
      }
    }
    free(hu->QIO_io_family);
  }
}

QIO_Layout *QIO_create_ionode_layout(QIO_Layout *layout, QIO_Filesystem *fs, QIO_host_utils_s *hu){
  QIO_Layout *ionode_layout;
  int number_of_nodes = layout->number_of_nodes;
  int number_io_nodes = fs->number_io_nodes;
  char myname[] = "QIO_create_ionode_layout";

  /* Save values in globals for subsequent calls */
  QIO_init_mpp_layout(layout, hu);
  QIO_init_mpp_fs(fs, hu);
  hu->QIO_fake_ionode_layout = 0; /* default value in previously global variable */

  /* Allocate and fill the layout structure */
  ionode_layout = (QIO_Layout *)malloc(sizeof(QIO_Layout));
  if(!ionode_layout){
    printf("%s: Can't malloc ionode_layout\n",myname);
    return NULL;
  }

  /* Start by copying the entire user-supplied structure */
  *ionode_layout = *layout;

  /* Tables are not needed if each node does its own I/O */
  if(number_of_nodes == number_io_nodes){
    hu->QIO_fake_ionode_layout = 0;
    return ionode_layout;
  }

  /* Otherwise we are using a fake ionode layout */
  hu->QIO_fake_ionode_layout = 1;
  ionode_layout->node_number_a  = QIO_ionode_node_number_a;
  ionode_layout->node_index_a   = QIO_ionode_node_index_a;
  ionode_layout->get_coords_a   = QIO_ionode_get_coords_a;
  ionode_layout->num_sites_a    = QIO_ionode_num_sites_a;
  ionode_layout->arg            = (void *)hu;
  /* Create tables */
  if(QIO_create_io_node_table(hu)){
    free(ionode_layout);
    return NULL;
  }

  return ionode_layout;
}

/* Return rank of a given node */
/* Returns -1 if node is not an I/O node */
int QIO_get_io_node_rank(int node, QIO_host_utils_s *hu){
  int rank;

  /* If we are faking the ionode layout, use the table */
  if(hu->QIO_fake_ionode_layout){
    rank = hu->QIO_ionode_to_rank[node];
    if(rank == QIO_NON_IO)return -1;
    return rank;
  }
  /* Otherwise the rank is the same as the node */
  else 
    return node;
}

void QIO_delete_ionode_layout(QIO_Layout *layout, QIO_host_utils_s *hu){
  QIO_delete_io_node_table(hu);
  if(layout != NULL)free(layout);
}

/* The fake scalar layout structure */
/* All sites are stored in lexicographic order on the host, "node 0" */
int QIO_scalar_node_number_a(const int coords[], void *arg){
  return 0;
}

/* Convert coords to lexicographic rank, based on MPP lattice dimension */
/* This is not the standard hypercube layout for a scalar
   machine, but it is fine for file conversion */
/* CAUTION: conversion from DML_SiteRank type to int */
int QIO_scalar_node_index_a(const int coords[], void *arg){
  QIO_host_utils_s *hu = (QIO_host_utils_s *)arg;
  int index = DML_lex_rank(coords, hu->QIO_mpp_layout.latdim, hu->QIO_mpp_layout.latsize);
  return index;
}

void QIO_scalar_get_coords_a(int coords[], int node, int index, void *arg){
  QIO_host_utils_s *hu = (QIO_host_utils_s *)arg;
  int latdim = hu->QIO_mpp_layout.latdim;
  int *latsize = hu->QIO_mpp_layout.latsize;
  DML_lex_coords(coords, latdim, latsize, (DML_SiteRank)index);
}

int QIO_scalar_num_sites_a(int node, void *arg){
  QIO_host_utils_s *hu = (QIO_host_utils_s *)arg;
  return hu->QIO_mpp_layout.volume;
}

/* Convert node index from ionode layout to scalar layout */
/* This would be unnecessary if get/put used coordinates */
int QIO_ionode_to_scalar_index(int ionode_node, int ionode_index, QIO_host_utils_s *hu){
  int latdim = hu->QIO_mpp_layout.latdim;
  int *coords = (int *)calloc(latdim, sizeof(int));
  int scalar_index;

  /* Conversion goes through coordinates */
  QIO_ionode_get_coords_a(coords,ionode_node,ionode_index,hu);
  scalar_index = QIO_scalar_node_index_a(coords, hu);
  free(coords);
  return scalar_index;
}

/* Convert node index from scalar layout to ionode layout */
/* This would be unnecessary if get/put used coordinates */
int QIO_scalar_to_ionode_index(int scalar_node, int scalar_index, QIO_host_utils_s *hu){
  int latdim = hu->QIO_mpp_layout.latdim;
  int *coords = (int *)calloc(latdim, sizeof(int));
  int ionode_index;

  /* Conversion goes through coordinates */
  QIO_scalar_get_coords_a(coords,scalar_node,scalar_index,hu);
  ionode_index = QIO_ionode_node_index_a(coords,hu);
  free(coords);
  return ionode_index;
}

/* Create scalar layout structure that puts the entire lattice on one node 
   with sites in lexicographic order */
/* We really don't need the fs parameter */
QIO_Layout *QIO_create_scalar_layout(QIO_Layout *layout, QIO_Filesystem *fs, QIO_host_utils_s *hu){
  QIO_Layout *scalar_layout;
  char myname[] = "QIO_create_scalar_layout";

  /* Allocate and fill the layout structure */
  scalar_layout = (QIO_Layout *)malloc(sizeof(QIO_Layout));
  if(!scalar_layout){
    printf("%s: Can't malloc scalar_layout\n",myname);
    return NULL;
  }

  /* Start by copying the entire user-supplied structure */
  *scalar_layout = *layout;

  /* Make adjustments */
  scalar_layout->node_number_a  = QIO_scalar_node_number_a;
  scalar_layout->node_index_a   = QIO_scalar_node_index_a;
  scalar_layout->get_coords_a   = QIO_scalar_get_coords_a;
  scalar_layout->num_sites_a    = QIO_scalar_num_sites_a;
  scalar_layout->arg            = hu;
  scalar_layout->this_node      = 0;
  scalar_layout->number_of_nodes= 1;
  scalar_layout->sites_on_node  = hu->QIO_mpp_layout.volume;

  return scalar_layout;
}

void QIO_delete_scalar_layout(QIO_Layout *layout){
  if(layout != NULL)
    free(layout);
}

/* Fake io_node function returns the number of the I/O node
   assigned to a given node.  The only node that should be asking
   is the I/O node itself */
int QIO_ionode_io_node(int node, QIO_host_utils_s *hu){
  char myname[] = "QIO_ionode_io_node";

  if(hu->QIO_ionode_to_rank[node] == QIO_NON_IO){
    printf("%s: Error: %d is not an I/O node\n",myname,node);
  }
  return node;
}
