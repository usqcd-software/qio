/* DML_list_sites.c */

/* Extract all the data from the lattice field and send it to the LRL
   record writer */

#include <lrl.h>
#include <dml.h>

int DML_list_output_sites(int sitelist[],
			  int serpar, int siteorder, int volfmt, 
			  int latdim, int latsize[]){

  /* Multidump format. Site order native.  Need local list. */
  if(volfmt == DML_MULTIFILE){
    return 
      DML_multidump_list(sitelist, latdim, latsize);
  }

  /* Serial write.  Site order is always lexicographic. No site list needed */
  else if(serpar == DML_SERIAL){
    return;
  }

  /* Parallel write.  Site order lexicographic. No site list needed. */
  else if(siteorder == DML_LEX_ORDER){
    return;
  }

  /* Parallel write.  Site order natural.  Need global list. */
  else{
    return
      DML_checkpoint_list(sitelist, latdim, latsize);
  }
}
