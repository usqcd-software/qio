/* DML_stream_out.c */

/* Extract all the data from the lattice field and send it to the LRL
   record writer */

#include <lrl.h>
#include <dml.h>
#include <stdio.h>


int DML_stream_out(LRL_RecordWriter *lrl_record_out, 
		   void (*get)(char *buf, const int coords[], void *arg),
		   size_t size, void *arg, DML_Layout *layout,
		   int serpar, int siteorder, int volfmt, 
		   DML_Checksum *checksum){

  /* Multidump format. Site order is always native */
  if(volfmt == DML_MULTIFILE){
    return 
      DML_multidump_out(lrl_record_out, get, size, arg, layout, checksum);
  }

  /* Serial write.  Site order is always lexicographic. */
  else if(serpar == DML_SERIAL){
    return
      DML_serial_out(lrl_record_out, get, size, arg, layout, checksum);
  }

  /* Parallel write.  Site order lexicographic */
  else if(siteorder == DML_LEX_ORDER){
    return
      DML_parallel_out(lrl_record_out, get, size, arg, layout, checksum);
  }

  /* Parallel write.  Site order natural */
  else{
    return
      DML_checkpoint_out(lrl_record_out, get, size, arg, layout, checksum);
  }
}
