/* DML_stream_in.c */

/* Extract all the data from the lattice field and send it to the LRL
   record writer */

#include <lrl.h>
#include <dml.h>

size_t DML_stream_in(LRL_RecordReader *lrl_record_in, int globaldata,
	     void (*put)(char *buf, size_t index, int count, void *arg),
	     int count, size_t size, int word_size, void *arg, 
             DML_Layout *layout, int serpar, int siteorder, 
             DML_SiteRank sitelist[], int volfmt, DML_Checksum *checksum)
{
  /* Global data */
  if(globaldata == DML_GLOBAL){
    return DML_global_in(lrl_record_in,
                         put, count, size, word_size, arg, layout, checksum);
  }

  /* Field data */
  else{
    /* Multifile format. Site order specified by sitelist */
    if(volfmt == DML_MULTIFILE){
      return DML_multifile_in(lrl_record_in, sitelist, 
		      put, count, size, word_size, arg, layout, checksum);
    }
    
    /* Serial read.  Site order always lexicographic. */
    else if(serpar == DML_SERIAL){
      return DML_serial_in(lrl_record_in, 
		   put, count, size, word_size, arg, layout, checksum);
    }
    
    /* Parallel read.  Site order always lexicographic. */
    else {
      return DML_parallel_in(lrl_record_in, 
		     put, count, size, word_size, arg, layout, checksum);
    }
  }
}
