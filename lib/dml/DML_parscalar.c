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
  return 0;
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
  return 0;
}


int DML_clear_to_send(char *scratch_buf, size_t size, int new_node) 
{
  if (QMP_get_msg_passing_type() != QMP_GRID)
  {
    int this_node = QMP_get_node_number();

    if(this_node == DML_MASTER_NODE && new_node != DML_MASTER_NODE)
      DML_send_bytes(scratch_buf,size,new_node); /* junk message */

    if(this_node == new_node && new_node != DML_MASTER_NODE)
      DML_get_bytes(scratch_buf,size,DML_MASTER_NODE);
  }

  return 0;
}


int DML_route_bytes(char *buf, size_t size, int fromnode, int tonode) 
{
  if (QMP_get_msg_passing_type() == QMP_GRID)
  {
    DML_grid_route(buf, size, fromnode, tonode);
  }
  else
  {
    int this_node = QMP_get_node_number();

    if (this_node == tonode)
      DML_get_bytes(buf,size,fromnode);

    if (this_node == fromnode)
      DML_send_bytes(buf,size,tonode);
  }

  return 0;
}

void DML_global_xor(u_int32 *x){
  long work = (long)*x;
  QMP_global_xor(&work);
  *x = (u_int32)work;
}

void DML_sync(void){
  while(QMP_wait_for_barrier(1000)==QMP_TIMEOUT);
}

