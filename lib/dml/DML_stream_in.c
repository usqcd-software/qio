/* DML_stream_in.c */

/* Extract all the data from the lattice field and send it to the LRL
   record writer */

#include <lrl.h>
#include <dml.h>

int DML_stream_in(LRL_RecordReader *lrl_record_in, 
		  void (*put)(char *buf, const int coords[], void *arg),
		  size_t size, void *arg, DML_Layout *layout,
		  int serpar, int siteorder, size_t sitelist[],
		  int volfmt, DML_Checksum *checksum){
  
  /* Multidump format. Site order is always native */
  if(volfmt == DML_MULTIFILE){
    return DML_multidump_in(lrl_record_in, sitelist, 
			    put, size, arg, layout, checksum);
  }

  /* Serial read.  Site order determined by file. */
  else if(serpar == DML_SERIAL){
    return DML_serial_in(lrl_record_in, siteorder, sitelist, 
			 put, size, arg, layout, checksum);
  }

  /* Parallel read.  Site order determined by file. */
  else {
    return DML_parallel_in(lrl_record_in, siteorder, sitelist, 
			   put, size, arg, layout, checksum);
  }
}
