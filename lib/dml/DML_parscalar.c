/* DML_qmp.c */
/* QMP-dependent utilities for DML */

#include <lrl.h>
#include <qmp.h>
#include <dml.h>

int DML_send_bytes(char *buf, int size, int tonode){
  QMP_msgmem_t mm;
  QMP_msghandle_t mh;

  mm = QMP_declare_msgmem(buf, size);
  mh = QMP_declare_send_to(mm, tonode, 0);
  QMP_start(mh);
  QMP_wait(mh);
  QMP_free_msghandle(mh);
  QMP_free_msgmem(mm);
}

int DML_get_bytes(char *buf, int size, int fromnode){
  QMP_msgmem_t mm;
  QMP_msghandle_t mh;

  mm = QMP_declare_msgmem(buf, size);
  mh = QMP_declare_receive_from(mm, fromnode, 0);
  QMP_start(mh);
  QMP_wait(mh);
  QMP_free_msghandle(mh);
  QMP_free_msgmem(mm);
}

void DML_sync(void){
  while(QMP_wait_for_barrier(1000)==QMP_TIMEOUT);
}


