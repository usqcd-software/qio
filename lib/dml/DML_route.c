#include <qmp.h>
#include <qio_config.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#if ( defined(HAVE_QMP_ROUTE) && defined(QIO_USE_QMP_ROUTE) )
/*#warning "Using native QMP_route since it is available and enabled"*/

/* Use native version of QMP_route since it is available */
QMP_status_t DML_grid_route(void* buffer, size_t count,
			    size_t src, size_t dest)
{
  return QMP_route(buffer, count, src, dest);
}
#else
/*#warning Using DML GRID ROUTE*/

static int
get_path_dir(int src, int dest, int size)
{
  int len, dir=1;

  len = dest - src;
  if(len<0) dir *= -1;
  if(2*abs(len)>size) {
    dir *= -1;
  }

  return dir;
}

/* Private implementation of route method */
QMP_status_t DML_grid_route(void* buffer, size_t count,
			    size_t src, size_t dest)
{
  int *src_coords;      /* Coordinates of the source */
  int *dst_coords;      /* Coordinates of the destination */
  int *my_coords;       /* my coordinates */
  const int *machine_size;    /* size of machine */
  int ndim, me, i;
  int on_path, path_leg;
  QMP_mem_t *mem;
  QMP_msgmem_t msgmem;

  /* Check to see if the logical topology is declared or not */
  if(QMP_logical_topology_is_declared() == QMP_FALSE) {
    QMP_fprintf(stderr, "%s: QMP logical topology not declared\n", __func__);
    return QMP_TOPOLOGY_EXISTS;
  }

  /* Topology is declared */
  /* Get its details */
  /* I don't think I should free machine size since it's const */
  ndim = QMP_get_logical_number_of_dimensions();
  machine_size = QMP_get_logical_dimensions();

  /* Get my node number -- use it to see whether I am on the path */
  me = QMP_get_node_number();

  /* Allocate space for the coordinates */
  /* Must free these later */
  src_coords = QMP_get_logical_coordinates_from(src);
  if( src_coords == NULL ) { 
    QMP_fprintf(stderr, "%s: QMP_get_logical_coordinates_from failed\n", __func__);
    return QMP_NOMEM_ERR;
  }

  dst_coords = QMP_get_logical_coordinates_from(dest);
  if( dst_coords == NULL ) { 
    QMP_fprintf(stderr, "%s: QMP_get_logical_coordinates_from failed\n", __func__);
    return QMP_NOMEM_ERR;
  }

  my_coords = QMP_get_logical_coordinates_from(me);
  if( my_coords == NULL ) { 
    QMP_fprintf(stderr, "%s: QMP_get_logical_coordinates_from failed\n", __func__);
    return QMP_NOMEM_ERR;
  }


  /* now see if we are on the path */
  on_path = 1;

  i = 0;
  while((i<ndim)&&(my_coords[i]==dst_coords[i])) i++;
  path_leg = i;  /* which leg of the path we are on */

  i = path_leg + 1;
  while((i<ndim)&&(my_coords[i]==src_coords[i])) i++;
  if(i<ndim) on_path = 0;

  if(path_leg<ndim) {
    int dir;
    dir = get_path_dir(src_coords[path_leg], dst_coords[path_leg],
		       machine_size[path_leg]);

    if(src_coords[path_leg] <= dst_coords[path_leg]) {
      if(dir==1) {
	if( (my_coords[path_leg]<src_coords[path_leg]) ||
	    (my_coords[path_leg]>dst_coords[path_leg]) ) on_path = 0;
      } else {
	if( (my_coords[path_leg]>src_coords[path_leg]) &&
	    (my_coords[path_leg]<dst_coords[path_leg]) ) on_path = 0;
      }
    } else {
      if(dir==1) {
	if( (my_coords[path_leg]<dst_coords[path_leg]) ||
	    (my_coords[path_leg]>src_coords[path_leg]) ) on_path = 0;
      } else {
	if( (my_coords[path_leg]>dst_coords[path_leg]) &&
	    (my_coords[path_leg]<src_coords[path_leg]) ) on_path = 0;
      }
    }
  }

  if(on_path) {
    int recv_axis, recv_dir=0;
    int send_axis, send_dir=0;

    /* figure out send and recv nodes */
    if(me==src) {
      recv_axis = -1;
    } else {
      i = path_leg;
      if(i==ndim) i--;
      while(my_coords[i] == src_coords[i]) i--;
      recv_dir = -get_path_dir(src_coords[i], dst_coords[i], machine_size[i]);
      recv_axis = i;
    }

    if(me==dest) {
      send_axis = -1;
    } else {
      i = path_leg;
      send_dir = get_path_dir(src_coords[i], dst_coords[i], machine_size[i]);
      send_axis = i;
    }

    if((recv_axis<0)||(send_axis<0)) {
      mem = NULL;
      msgmem = QMP_declare_msgmem(buffer, count);
    } else {
      mem = QMP_allocate_memory(count);
      msgmem = QMP_declare_msgmem(QMP_get_memory_pointer(mem), count);
    }

    /* do recv if necessary */
    if(recv_axis>=0) {
      QMP_msghandle_t mh;
      QMP_status_t err;

      mh = QMP_declare_receive_relative(msgmem, recv_axis, recv_dir, 0);
      if(mh == NULL) { 
	QMP_fprintf(stderr, "QMP_declare_receive_relative returned NULL\n");
	return QMP_BAD_MESSAGE;
      }

      err = QMP_start(mh);
      if(err != QMP_SUCCESS) { 
	QMP_fprintf(stderr, "QMP_start() failed on receive in DML_route\n"); 
	return QMP_ERROR;
      }

      err = QMP_wait(mh);
      if( err != QMP_SUCCESS ) { 
	QMP_fprintf(stderr, "QMP_wait() recv on send in DML_route\n");
	return QMP_ERROR;
      }

      QMP_free_msghandle(mh);
    }

    /* do send if necessary */
    if(send_axis>=0) {
      QMP_msghandle_t mh;
      QMP_status_t err;

      mh = QMP_declare_send_relative(msgmem, send_axis, send_dir, 0);
      if(mh == NULL) { 
	QMP_fprintf(stderr, "QMP_declare_receive_relative returned NULL\n");
	return QMP_BAD_MESSAGE;
      }

      err = QMP_start(mh);
      if(err != QMP_SUCCESS) { 
	QMP_fprintf(stderr, "QMP_start() failed on receive in DML_route\n"); 
	return QMP_ERROR;
      }

      err = QMP_wait(mh);
      if( err != QMP_SUCCESS ) { 
	QMP_fprintf(stderr, "QMP_wait() recv on send in DML_route\n");
	return QMP_ERROR;
      }

      QMP_free_msghandle(mh);
    }

    QMP_free_msgmem(msgmem);
    if(mem) QMP_free_memory(mem);

  }

  free(src_coords);
  free(dst_coords);
  free(my_coords);

  return(QMP_SUCCESS);
}
#endif
