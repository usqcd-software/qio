/* DML_stream_out.c */

/* Extract all the data from the lattice field and send it to the LRL
   record writer */

/* Return the number of bytes written */

#include <lrl.h>
#include <dml.h>
#include <stdio.h>

uint64_t DML_stream_out(LRL_RecordWriter *lrl_record_out, int globaldata,
	   void (*get)(char *buf, size_t index, int count, void *arg),
           int count, size_t size, int word_size, void *arg, 
	   DML_Layout *layout, DML_SiteList *sites,
	   int volfmt, DML_Checksum *checksum)
{
  /* Global data type. */
  if(globaldata == DML_GLOBAL){
    return
      DML_global_out(lrl_record_out, get, count, size, word_size,
		     arg, layout, volfmt, checksum);
  }

  /* Lattice field data type */
  else{
    return 
      DML_partition_out(lrl_record_out, get, count, size, word_size, 
			arg, layout, sites, volfmt, checksum);
  }
}
