/* Utilities for testing QIO */
#include <stdio.h>
#include "qio-test.h"

/* Internal factory function for array of real field data */
void vput_R(char *buf, size_t index, int count, void *qfin)
{
  float **field = (float **)qfin;
  float *dest;
  float *src = (float *)buf;
  int i;

/* For the site specified by "index", move an array of "count" data
   from the read buffer to an array of fields */

  for(i=0; i<count; i++) {
    dest = field[i] + index;
    *dest = *(src + i);
  }
}

/* Internal factory function for array of real field data */
void vget_R(char *buf, size_t index, int count, void *qfin)
{
  float **field = (float **)qfin;
  float *src;
  float *dest = (float *)buf;
  int i;

/* For the site specified by "index", move an array of "count" data
   from the array of fields to the write buffer */
  for(i = 0; i < count; i++, dest++) {
    src = field[i] + index;
    *dest = *src;
  }
}

/* Internal factory function for array of real global data */
void vput_r(char *buf, size_t index, int count, void *qfin)
{
  float *array = (float *)qfin;
  float *src = (float *)buf;
  int i;

  /* Move buffer to array */
  for(i=0; i<count; i++) {
    array[i] = src[i];
  }
}

/* Internal factory function for array of real global data */
void vget_r(char *buf, size_t index, int count, void *qfin)
{
  float *array = (float *)qfin;
  float *dest = (float *)buf;
  int i;

  /* Move from array to buffer */
  for(i = 0; i < count; i++) {
    dest[i] = array[i];
  }
}

/* function used for setting real field */
void vfill_R(float *r, int coords[],int i){
  /* Set value equal to something */
  *r = 100*i + coords[0] + 
    lattice_size[0]*(coords[1] + lattice_size[1]*
		     (coords[2] + lattice_size[2]*coords[3]));
}

void vset_R(float *field[], int count){
  int x[4];
  int index,i;
  for(i = 0; i < count; i++)
    for(x[3] = 0; x[3] < lattice_size[3]; x[3]++)
      for(x[2] = 0; x[2] < lattice_size[2]; x[2]++)
	for(x[1] = 0; x[1] < lattice_size[1]; x[1]++)
	  for(x[0] = 0; x[0] < lattice_size[0]; x[0]++)
	    {
	      if(node_number(x) == this_node){
		index = node_index(x);
		vfill_R(field[i] + index, x,i);
	      }
	    }
}


/* create an array of real fields */
int vcreate_R(float *field_out[], int count){
  int i;
  /* Create an output field */
  for(i = 0; i < count; i++){
    field_out[i] = (float *)malloc(sizeof(float)*sites_on_node);
    if(field_out[i] == NULL){
      printf("vcreate_R(%d): Can't malloc field_out\n",this_node);
      return 1;
    }
  }

  return 0;
}

/* destroy array of fields */
void vdestroy_R(float *field[], int count){
  int i;
  for(i = 0; i < count; i++)
    free(field[i]);
}

/* compare real fields */
float vcompare_R(float *fielda[], float *fieldb[], 
		 int sites_on_node, int count){
  int i, j;
  float diff;
  float sum2 = 0;

  for(i = 0; i < count; i++)
    for(j = 0; j < sites_on_node; j++){
      diff = fielda[i][j] - fieldb[i][j];
      sum2 += diff*diff;
    }

  /* Global sum */
  QMP_sum_float(&sum2);
  return sum2;
}
							    
/* compare real arrays */
float vcompare_r(float arraya[], float arrayb[], int count){
  int j;
  float diff;
  float sum2 = 0;
  
  for(j = 0; j < count; j++){
    diff = arraya[j] - arrayb[j];
    sum2 += diff*diff;
  }

  /* Global sum (all nodes should have the same data) */
  QMP_sum_float(&sum2);
  return sum2;
}
							    
