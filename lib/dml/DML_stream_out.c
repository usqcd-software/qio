/* DML_stream_out.c */

/* Extract all the data from the lattice field and send it to the LRL
   record writer */

/* Return the number of bytes written */

#include <lrl.h>
#include <dml.h>
#include <stdio.h>

size_t DML_stream_out(LRL_RecordWriter *lrl_record_out, int globaldata,
	   void (*get)(char *buf, size_t index, int count, void *arg),
           int count, size_t size, int word_size, void *arg, 
	   DML_Layout *layout, int serpar, int volfmt, DML_Checksum *checksum)
{
  /* Global data type. */
  if(globaldata == DML_GLOBAL){
    return
      DML_global_out(lrl_record_out, get, count, size, word_size,
		     arg, layout, checksum);
  }

  /* Lattice field data type */
  else{

    /* Multifile format. Site order is always native */
    if(volfmt == DML_MULTIFILE){
      return 
	DML_multifile_out(lrl_record_out, get, count, size, word_size, 
			  arg, layout, checksum);
    }
    
    /* Serial write.  Site order is always lexicographic. */
    else if(serpar == DML_SERIAL){
      return
	DML_serial_out(lrl_record_out, get, count, size, word_size,
		       arg, layout, checksum);
    }
    
    /* Parallel write.  Site order lexicographic */
    else{
      return
	DML_parallel_out(lrl_record_out, get, count, size, word_size, 
			 arg, layout, checksum);
    }
  }
}
