/* DML_vanilla.c */
/* Vanilla versions of QMP-dependent utilities for DML */

#include <lrl.h>
#include <dml.h>
#include <stdio.h>

int DML_send_bytes(char *buf, int size, int tonode){
  printf("ERROR: called DML_send_bytes() in DML_vanilla.c\n");
  exit(1);
}

int DML_get_bytes(char *buf, int size, int fromnode){
  printf("ERROR: called DML_get_bytes() in DML_vanilla.c\n");
  exit(1);
}

void DML_sync(void){
}


