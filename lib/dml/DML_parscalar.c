/* DML_parscalar.c */
/* QMP-dependent utilities for DML */

#include <lrl.h>
#include <qmp.h>
#include <dml.h>

void DML_broadcast_bytes(char *buf, size_t size)
{
  QMP_broadcast(buf, (QMP_u32_t)size);
}

/* Sum a size_t over all nodes (32 bit) */
void DML_sum_size_t(size_t *ipt)
{
  QMP_s32_t work = *ipt;
  QMP_sum_int(&work);
  *ipt = work;
}

/* Sum an int over all nodes (16 or 32 bit) */
void DML_sum_int(int *ipt)
{
  QMP_s32_t work = *ipt;
  QMP_sum_int(&work);
  *ipt = work;
}

int DML_send_bytes(char *buf, size_t size, int tonode){
  QMP_msgmem_t mm;
  QMP_msghandle_t mh;

  mm = QMP_declare_msgmem(buf, size);
  mh = QMP_declare_send_to(mm, tonode, 0);
  QMP_start(mh);
  QMP_wait(mh);
  QMP_free_msghandle(mh);
  QMP_free_msgmem(mm);
}

int DML_get_bytes(char *buf, size_t size, int fromnode){
  QMP_msgmem_t mm;
  QMP_msghandle_t mh;

  mm = QMP_declare_msgmem(buf, size);
  mh = QMP_declare_receive_from(mm, fromnode, 0);
  QMP_start(mh);
  QMP_wait(mh);
  QMP_free_msghandle(mh);
  QMP_free_msgmem(mm);
}

void DML_global_xor(u_int32 *x){
  long work = (long)*x;
  QMP_global_xor(&work);
  *x = (u_int32)work;
}

void DML_sync(void){
  while(QMP_wait_for_barrier(1000)==QMP_TIMEOUT);
}

