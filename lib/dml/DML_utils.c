/* DML_utils.c */
/* Utilities for DML */

#include <lrl.h>
#include <dml.h>
#include <stdio.h>
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

/* Iterators for lexicographic order */

void DML_lex_init(int *dim, int coords[], int latdim, int latsize[])
{
  int d;
  for(d = 0; d < latdim; d++)coords[d] = 0;
  *dim = 0;
}

/* Recursively update the coordinate counter */
int DML_lex_next(int *dim, int coords[], int latdim, int latsize[])
{
  if(++coords[*dim] < latsize[*dim]){
    *dim = 0;
    return 1;
  }
  else{
    coords[*dim] = 0;
    if(++(*dim) < latdim)return DML_lex_next(dim, coords, latdim, latsize);
    else return 0;
  }
}

/* Convert linear index to lexicographic coordinate */

void DML_lex_coords(int coords[], int latdim, int latsize[], int rcv_coords)
{
  int dim;
  int index = rcv_coords;

  for(dim = 0; dim < latdim; dim++){
    coords[dim] = index % latsize[dim];
    index /= latsize[dim];
  }
}

/* Serial write */

int DML_serial_out(LRL_RecordWriter *lrl_record_out, 
		   void (*get)(char *buf, const int coords[], void *arg),
		   size_t size, void *arg, DML_Layout *layout,
		   DML_Checksum *checksum)
{
  char *buf;
  int current_node, new_node;
  int *coords; int dim;
  int this_node = layout->this_node;
  int latdim = layout->latdim;
  int *latsize = layout->latsize;

  /* Allocate buffer for datum */
  buf = (char *)malloc(size);
  if(buf == NULL)return 1;

  printf("DML_serial_out\n");

  /* Initialize checksum */
  /*** OMITTED FOR NOW ****/

  /* Barrier */
  DML_sync();

  /* Allocate coordinate counter */
  coords = (int *)malloc(latdim*sizeof(int));
  if(coords == NULL)return 1;
  
  /* Initialize current source node */
  current_node = DML_MASTER_NODE;
  
  /* Iterate over all sites in lexicographic order */
  DML_lex_init(&dim, coords, latdim, latsize);
  do {
    
    /* Send nodes must wait for a ready signal from the master node
       to prevent message pileups on the master node */
    new_node = layout->node_number(coords);
    if(new_node != current_node){
      if(this_node == DML_MASTER_NODE && new_node != DML_MASTER_NODE)
	DML_send_bytes(buf,4,new_node); /* junk message */
      if(this_node == new_node && new_node != DML_MASTER_NODE)
	DML_get_bytes(buf,4,DML_MASTER_NODE);
      current_node = new_node;
    }
    
    /* Master node receives the data and writes it */
    if(this_node == DML_MASTER_NODE){
      if(current_node == DML_MASTER_NODE){
	/* Data on the master node is just copied to the write buffer */
	get(buf,coords,arg);
      }
      else{
	/* Data from any other node is received in the write buffer */
	DML_get_bytes(buf,size,current_node);
      }
      
      /* Update checksum here */
      /*** OMITTED FOR NOW ***/
      /* Do byte ordering here */
      /*** OMITTED FOR NOW ***/
      
      /* Write the datum */
      if(!LRL_write_bytes(lrl_record_out,buf,size))return 1;
    }
    /* All other nodes send the data */
    else{
      if(this_node == current_node){
	get(buf,coords,arg);
	DML_send_bytes(buf,size,DML_MASTER_NODE);
      }
    }
    
  } while(DML_lex_next(&dim, coords, latdim, latsize));
  
  return 0;
}

int DML_multidump_out(LRL_RecordWriter *lrl_record_out, 
		      void (*get)(char *buf, const int coords[], void *arg),
		      size_t size, void *arg, DML_Layout *layout, 
		      DML_Checksum *checksum){
  return 0;
}

int DML_parallel_out(LRL_RecordWriter *lrl_record_out, 
		     void (*get)(char *buf, const int coords[], void *arg),
		     size_t size, void *arg, DML_Layout *layout, 
		     DML_Checksum *checksum){
  return 0;
}

int DML_checkpoint_out(LRL_RecordWriter *lrl_record_out, 
		       void (*get)(char *buf, const int coords[], void *arg),
		       size_t size, void *arg, DML_Layout *layout, 
		       DML_Checksum *checksum){
  return 0;
}

int DML_multidump_in(LRL_RecordReader *lrl_record_in, size_t sitelist[],
		     void (*put)(char *buf, const int coords[], void *arg),
		     size_t size, void *arg, DML_Layout *layout, 
		     DML_Checksum *checksum){
  return 0;

}

int DML_serial_in(LRL_RecordReader *lrl_record_in, int siteorder, 
		  size_t sitelist[],
		  void (*put)(char *buf, const int coords[], void *arg),
		  size_t size, void *arg, DML_Layout* layout,
		  DML_Checksum *checksum){
  char *buf;
  int dest_node;
  size_t rcv_rank, rcv_coords;
  int *coords;
  int this_node = layout->this_node;
  int latdim = layout->latdim;
  int *latsize = layout->latsize;
  size_t volume = layout->volume;

  /* Allocate buffer for datum */
  buf = (char *)malloc(size);
  if(buf == NULL)return 1;

  /* Allocate coordinate counter */
  coords = (int *)malloc(latdim*sizeof(int));
  if(coords == NULL)return 1;
  
  /* Initialize checksum */
  /*** OMITTED FOR NOW ****/

  /* Unlimited barrier */
  DML_sync();

  /* Run through all sites */
  for(rcv_rank = 0; rcv_rank < volume; rcv_rank++)
    {
      /* Determine receiving coordinates for the next site datum */
      if(siteorder == DML_LEX_ORDER)
	rcv_coords = rcv_rank;
      else
	rcv_coords = sitelist[rcv_rank];
      DML_lex_coords(coords, latdim, latsize, rcv_coords);
      
      /* The node that gets the next datum */
      dest_node = layout->node_number(coords);
      
      if(this_node == DML_MASTER_NODE){
	/* Master node gets the next value */
	if(!LRL_read_bytes(lrl_record_in, buf, size))return 1;

	/* If destination elsewhere, send it */
	if(dest_node != DML_MASTER_NODE){
	  DML_send_bytes(buf, size, dest_node);
	}
      }

      /* Other nodes receive from the master node */
      else {
	if(this_node == dest_node){
	  DML_get_bytes(buf, size, DML_MASTER_NODE);
	}
      }
	  
      /* Process data before inserting */
      if(this_node == dest_node){
	/* Do byte reversal if necessary */
	/*** OMITTED FOR NOW ***/
	/* Accumulate checksum */
	/*** OMITTED FOR NOW ***/
	  put(buf,coords,arg);
      }
    }

  /* Global reduction of checksum */
  /*** OMITTED FOR NOW ***/

  return 0;
}

int DML_parallel_in(LRL_RecordReader *lrl_record_in, int siteorder, 
		    size_t sitelist[],
		    void (*put)(char *buf, const int coords[], void *arg),
		    size_t size, void *arg, DML_Layout *layout,
		    DML_Checksum *checksum){
  return 0;
}
