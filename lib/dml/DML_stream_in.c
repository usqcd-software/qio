/* DML_stream_in.c */

/* Take all the data from the LRL record reader and put it in the
   lattice field  */

#include <lrl.h>
#include <dml.h>

uint64_t DML_stream_in(LRL_RecordReader *lrl_record_in, int globaldata,
	     void (*put)(char *buf, size_t index, int count, void *arg),
	     int count, size_t size, int word_size, void *arg, 
             DML_Layout *layout, DML_SiteList *sites, 
             int volfmt, DML_Checksum *checksum)
{
  /* Global data */
  if(globaldata == DML_GLOBAL){
    return DML_global_in(lrl_record_in,
                         put, count, size, word_size, arg, layout, 
			 volfmt, checksum);
  }

  /* Field data */
  else{
    /* Partition I/O only.  Nodes are assigned to disjoint I/O partitions */
    return 
      DML_partition_in(lrl_record_in, put, count, size, word_size, 
		       arg, layout, sites, volfmt, checksum);
  }
}
