/* DML_vanilla.c */
/* Vanilla versions of QMP-dependent utilities for DML */

#include <lrl.h>
#include <dml.h>
#include <stdio.h>
#include <stdlib.h>

void DML_broadcast_bytes(char *buf, size_t size){}

/* Sum a size_t over all nodes (32 bit) */
void DML_sum_size_t(size_t *ipt){}

/* Sum an int over all nodes (16 or 32 bit) */
void DML_sum_int(int *ipt){}

int DML_send_bytes(char *buf, size_t size, int tonode){
  printf("ERROR: called DML_send_bytes() in DML_vanilla.c\n");
  exit(1);
}

int DML_get_bytes(char *buf, size_t size, int fromnode){
  printf("ERROR: called DML_get_bytes() in DML_vanilla.c\n");
  exit(1);
}

void DML_global_xor(u_int32 *x){}

void DML_sync(void){}


