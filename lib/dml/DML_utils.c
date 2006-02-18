/* DML_utils.c */
/* Utilities for DML */

#include <qio_config.h>
#include <lrl.h>
#include <dml.h>
#include <stdio.h>
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <assert.h>
#include <sys/types.h>
#include <qio_stdint.h>

#undef DML_DEBUG

/* Iterators for lexicographic order */

void DML_lex_init(int *dim, int coords[], int latdim, int latsize[])
{
  int d;
  for(d = 0; d < latdim; d++)coords[d] = 0;
  *dim = 0;
}

/* Recursively update the coordinate counter */
int DML_lex_next(int *dim, int coords[], int latdim, int latsize[])
{
  if(++coords[*dim] < latsize[*dim]){
    *dim = 0;
    return 1;
  }
  else{
    coords[*dim] = 0;
    if(++(*dim) < latdim)return DML_lex_next(dim, coords, latdim, latsize);
    else return 0;
  }
}

/*------------------------------------------------------------------*/
/* Convert linear lexicographic rank to lexicographic coordinate */

void DML_lex_coords(int coords[], const int latdim, const int latsize[], 
		    const DML_SiteRank rcv_coords)
{
  int dim;
  DML_SiteRank rank = rcv_coords;

  for(dim = 0; dim < latdim; dim++){
    coords[dim] = rank % latsize[dim];
    rank /= latsize[dim];
  }
}

/*------------------------------------------------------------------*/
/* Convert coordinate to linear lexicographic rank (inverse of
   DML_lex_coords) */

DML_SiteRank DML_lex_rank(const int coords[], int latdim, int latsize[])
{
  int dim;
  DML_SiteRank rank = coords[latdim-1];

  for(dim = latdim-2; dim >= 0; dim--){
    rank = rank * latsize[dim] + coords[dim];
  }
  return rank;
}

/*------------------------------------------------------------------*/
/* Make temporary space for coords */

int *DML_allocate_coords(int latdim, char *myname, int this_node){
  int *coords;

  coords = (int *)malloc(latdim*sizeof(int));
  if(!coords)printf("%s(%d) can't malloc coords\n",myname,this_node);
  return coords;
}


/*------------------------------------------------------------------*/
/* The message structure holds the site datum and site rank */

/* Accessor: Size of message */
size_t DML_msg_sizeof(size_t size){
  return size + sizeof(DML_SiteRank);
}

/*------------------------------------------------------------------*/
/* Constructor*/
char *DML_allocate_msg(size_t size, char *myname, int this_node){
  char *msg;
  size_t sizeof_msg = DML_msg_sizeof(size);

  msg = (char *)malloc(sizeof_msg);
  if(!msg)printf("%s(%d) can't malloc msg\n",myname,this_node);
  return msg;
}

/*------------------------------------------------------------------*/
/* Accessor: Pointer to datum member of msg */
char *DML_msg_datum(char *msg, size_t size){
  return msg;
}

/*------------------------------------------------------------------*/
/* Accessor: Pointer to rank member of msg */
DML_SiteRank *DML_msg_rank(char *msg, size_t size){
  return (DML_SiteRank *)(msg + size);
}

/*------------------------------------------------------------------*/
/* Count the sitelist for partitioned I/O format */
/* Return code 0 = success; 1 = failure */
int DML_count_partition_sitelist(DML_Layout *layout, DML_SiteList *sites){
  int *coords;
  int latdim = layout->latdim;
  int node;
  int this_node = layout->this_node;
  int number_of_nodes = layout->number_of_nodes;
  int my_io_node = layout->ionode(this_node);
  size_t number_of_io_sites;
  int number_of_my_ionodes;
  char myname[] = "DML_count_partition_sitelist";
#if 0
  DML_SiteRank rank;
  int *latsize = layout->latsize;
  size_t volume = layout->volume;
  int send_node;
#endif

  /* Space for a coordinate vector */
  coords = DML_allocate_coords(latdim, myname, this_node);
  if(!coords)return 1;

  /* Iterate over all nodes, adding up the sites my partition
     writes and counting the number of nodes in my partition */
  number_of_my_ionodes = 0;
  number_of_io_sites = 0;
  for(node = 0; node < number_of_nodes; node++){
    if(layout->ionode(node) == my_io_node){
      /* ( If we are discovering the lattice dimensions, we won't know
	 the number of sites on any node ) */
      if(layout->latdim != 0)
	number_of_io_sites += layout->num_sites(node);
      number_of_my_ionodes++;
    }
  }

#if 0  
  /* Iterate over all sites, storing the lexicographic index
     only for the sites on my partition */
  number_of_io_sites = 0;
  /* Loop over all sites in lexicographic order */
  for(rank = 0; rank < volume; rank++)
    {
      /* Convert rank index to coordinates */
      DML_lex_coords(coords, latdim, latsize, rank);
      /* The node containing these coordinates */
      send_node = layout->node_number(coords);
      if(layout->ionode(send_node) == my_io_node)
	number_of_io_sites++;
    }
#endif


  sites->number_of_io_sites = number_of_io_sites;
  sites->number_of_my_ionodes = number_of_my_ionodes;
  free(coords);
  return 0;
}

/*------------------------------------------------------------------*/
/* Create sitelist structure (but not the sitelist, yet) */
DML_SiteList *DML_init_sitelist(int volfmt, int serpar, DML_Layout *layout){
  DML_SiteList *sites;
  int this_node = layout->this_node;
  size_t number_of_io_sites;
  char myname[] = "DML_init_sitelist";

  sites = (DML_SiteList *)malloc(sizeof(DML_SiteList));
  if(!sites){
    printf("%s: Can't allocate small sitelist structure\n",myname);
    return NULL;
  }
  sites->list               = NULL;
  sites->use_list           = 0;
  sites->number_of_io_sites = 0;
  sites->first              = 0;

  /* Initialize number of I/O sites */

  if(volfmt == DML_SINGLEFILE && serpar == DML_SERIAL){
    /* Single files are always written in lexicographic order so
       single file format doesn't need a sitelist and all nodes count
       through the entire file */
      sites->number_of_io_sites = layout->volume;
  }

  else if(volfmt == DML_MULTIFILE){
    /* Multifile format requires a sitelist for each node */

    sites->use_list = 1;

    /* Each node reads/writes its own sites independently */
    sites->number_of_io_sites = layout->sites_on_node;
  }

  else if(volfmt == DML_PARTFILE || 
	  (volfmt == DML_SINGLEFILE && serpar == DML_PARALLEL)){
    /* Partitioned I/O requires a separate sitelist for each I/O
       partition.  Parallel I/O uses a sitelist to determine which
       sites to read or write. */
    /* Here we just count the number of sites in the list. */

    sites->use_list = 1;
    if(DML_count_partition_sitelist(layout,sites)){
      free(sites); return NULL;
    }
  }
  else{
    /* Bad volfmt */
    printf("%s(%d): bad volume format code = %d\n",
	   myname, this_node, volfmt);
    free(sites); return NULL;
  }

  return sites;

}

/*------------------------------------------------------------------*/
/* Compare site lists. Return 0 if all sites in the list agree and 1
   otherwise */

int DML_compare_sitelists(DML_SiteRank *lista, DML_SiteRank *listb, size_t n){
  size_t i;
  
  /* Scan site list for my node */
  for(i = 0; i < n; i++){
    if(lista[i] != listb[i])return 1;
  }
  
  return 0;
}

/*------------------------------------------------------------------*/
/* Free the sitelist structure */
void DML_free_sitelist(DML_SiteList *sites){
  if(sites == NULL)return;
  if(sites->list != NULL)free(sites->list);
  free(sites);
}

/*------------------------------------------------------------------*/
/* Fill the sitelist for multifile format */    
/* Return code 0 = success; 1 = failure */
int DML_fill_multifile_sitelist(DML_Layout *layout, DML_SiteList *sites){
  int *coords;
  int latdim = layout->latdim;
  int *latsize = layout->latsize;
  int this_node = layout->this_node;
  size_t index;
  char myname[] = "DML_fill_multifile_sitelist";

  /* Each node dumps its own sites */

  /* Space for a coordinate vector */
  coords = DML_allocate_coords(latdim, myname, this_node);
  if(!coords)return 1;
  /* Iterate over sites in storage order on this node */
  for(index = 0; index < layout->sites_on_node; index++){
    /* Convert storage order to coordinates */
    layout->get_coords(coords,this_node,index);
    /* Convert coordinate to lexicographic rank */
    sites->list[index] = DML_lex_rank(coords,latdim,latsize);
  }

  free(coords);
  return 0;
}

/*------------------------------------------------------------------*/
/* Fill the sitelist for partitioned I/O format */
/* Return code 0 = success; 1 = failure */
int DML_fill_partition_sitelist(DML_Layout *layout, DML_SiteList *sites){
  size_t index, index2, node_index;
  int *coords;
  int latdim = layout->latdim;
  int *latsize = layout->latsize;
  int node;
  int node_sites;
  int this_node = layout->this_node;
  int my_io_node = layout->ionode(this_node);
  int number_of_nodes = layout->number_of_nodes;
  size_t number_of_io_sites = sites->number_of_io_sites;
  DML_SiteRank *list        = sites->list;
  DML_SiteRank tmp;
  char myname[] = "DML_fill_partition_sitelist";
#if 0
  DML_SiteRank rank;
  size_t volume = layout->volume;
  int send_node;
#endif

  /* Space for a coordinate vector */
  coords = DML_allocate_coords(latdim, myname, this_node);
  if(!coords)return 1;

  /* Fill the list in storage order first */
  index = 0;
  for(node = 0; node < number_of_nodes; node++){
    /* Find the nodes on my partition */
    if(layout->ionode(node) == my_io_node){
      node_sites = layout->num_sites(node);
      for(node_index = 0; node_index < node_sites; node_index++){
	layout->get_coords(coords,node,node_index);
	list[index] = DML_lex_rank(coords,latdim,latsize);
	index++;
      }
    }
  }
  if(index != number_of_io_sites){
    printf("%s(%d) Internal error. Can't count I/O sites\n",
	   myname,this_node);
    return 1;
  }

  /* Put the site list in ascending lexicographic rank order */
  /* Dumb sort algorithm */
  for(index = 0; index < number_of_io_sites-1; index++){
    for(index2 = index+1; index2 < number_of_io_sites; index2++){
      if(list[index] > list[index2]){
	tmp = list[index];
	list[index] = list[index2];
	list[index2] = tmp;
      }
    }
  }
  
  free(coords);
  return 0;
}

/*------------------------------------------------------------------*/
/* Create and populate the sitelist for output */
/* Return code 0 = success; 1 = failure */
int DML_fill_sitelist(DML_SiteList *sites, int volfmt, int serpar,
		      DML_Layout *layout){
  int this_node = layout->this_node;
  char myname[] = "DML_fill_sitelist";

  if(sites->use_list == 0)return 0;

  /* Allocate the list */

  sites->list = 
    (DML_SiteRank *)malloc(sizeof(DML_SiteRank)*sites->number_of_io_sites);
  if(sites->list == NULL)return 1;

  /* Fill the list */

  if(volfmt == DML_MULTIFILE){
  /* Multifile format requires a sitelist */
    return DML_fill_multifile_sitelist(layout,sites);
  }
  else if(volfmt == DML_PARTFILE || 
	  (volfmt == DML_SINGLEFILE && serpar == DML_PARALLEL)){
    /* Partitioned I/O requires a sitelist on the I/O node */
    /* Singlefile parallel I/O requires the same sitelist as partfile
       to determine which sites to write/read */
    return DML_fill_partition_sitelist(layout,sites);
  }
  else {
    /* Bad volfmt */
    printf("%s(%d): bad volume format code = %d\n",
	   myname, this_node, volfmt);
    return 1;
  }
  return 0;
}

/*------------------------------------------------------------------*/
/* Read and check the sitelist for input */
/* return 0 for success and 1 for failure */
int DML_read_sitelist(DML_SiteList *sites, LRL_FileReader *lrl_file_in,
		      int volfmt, DML_Layout *layout,
		      LIME_type *lime_type){
  uint64_t check, announced_rec_size;
  int this_node = layout->this_node;
  LRL_RecordReader *lrl_record_in;
  DML_SiteRank *inputlist;
  int not_ok;
  int status;
  char myname[] = "DML_read_sitelist";

  if(sites->use_list == 0)return 0;

  /* Open sitelist record */
  lrl_record_in = LRL_open_read_record(lrl_file_in, &announced_rec_size, 
				       lime_type, &status);
  if(!lrl_record_in)return 1;

  /* Require that the record size matches expectations */
  check = sites->number_of_io_sites * sizeof(DML_SiteRank);
  /* Ignore a mismatch if we are trying to discover the lattice dimension */
  if(!layout->discover_dims_mode && announced_rec_size != check){
    printf("%s(%d): sitelist size mismatch: found %lu expected %lu lime type %s\n",
	   myname, this_node, (unsigned long)announced_rec_size,
	   (unsigned long)check, *lime_type);
    printf("%s(%d): latdim = %d\n",myname, this_node,layout->latdim);
    return 1;
  }

  /* Allocate check list according to record size */
  
  inputlist = (DML_SiteRank *)malloc(announced_rec_size);
  if(inputlist == NULL)return 1;
  
  /* Read the site list and close the record */
  check = LRL_read_bytes(lrl_record_in, (char *)inputlist, announced_rec_size);
  
  LRL_close_read_record(lrl_record_in);
    
#ifdef DML_DEBUG
  printf("%s(%d) sitelist record was read with %lu bytes\n",myname,
	 layout->this_node,(unsigned long)check);
#endif
 
  /* Check bytes read */
  if(check != announced_rec_size){
    printf("%s(%d): bytes read %lu != expected rec_size %lu\n",
	   myname, this_node, (unsigned long)check, 
	   (unsigned long)announced_rec_size);
    free(inputlist); return 1;
  }
 
  /* Byte reordering for entire sitelist */
  if (! DML_big_endian())
    DML_byterevn((char *)inputlist, announced_rec_size, sizeof(DML_SiteRank));
  
  /* All input sitelists must agree exactly with what we expect */
  /* Unless we are reading in discovery mode */
  if(!layout->discover_dims_mode){
    not_ok = DML_compare_sitelists(sites->list, inputlist, 
				   sites->number_of_io_sites);
    
    if(not_ok)
      printf("%s(%d): sitelist does not conform to I/O layout.\n",
	     myname,this_node);

    /* Return 1 if not OK and 0 if OK */
    free(inputlist); 
    return not_ok;
  }
  else
    return 0;
}


/*------------------------------------------------------------------*/
/* Initialize site iterator for I/O processing */
/* Returns lexicographic rank of the first site */
DML_SiteRank DML_init_site_loop(DML_SiteList *sites){
  sites->current_index = 0;
  if(sites->use_list){
    return sites->list[sites->current_index];
  }
  else{
    sites->current_rank = sites->first;
    return sites->current_rank;
  }
}

/*------------------------------------------------------------------*/
/* Iterator for sites processed by I/O */
/* Returns 0 when iteration is complete. 1 when not and updates rank. */
int DML_next_site_loop(DML_SiteRank *rank, DML_SiteList *sites){
  sites->current_index++;
  if(sites->current_index >= sites->number_of_io_sites)return 0;
  if(sites->use_list){
    *rank = sites->list[sites->current_index];
  }
  else{
    sites->current_rank++;
    *rank = sites->current_rank;
  }
  return 1;
}
 
/*------------------------------------------------------------------*/
/* Checksum "class" */
/* We do a crc32 sum on the site data -- then do two lexicographic-
   rank-based bit rotations and XORs on the resulting crc32
   checksum */

/* Initialize checksums */
void DML_checksum_init(DML_Checksum *checksum){
  checksum->suma = 0;
  checksum->sumb = 0;
}

/* Accumulate checksums */
void DML_checksum_accum(DML_Checksum *checksum, DML_SiteRank rank, 
			char *buf, size_t size){

  DML_SiteRank rank29 = rank;
  DML_SiteRank rank31 = rank;
  uint32_t work = DML_crc32(0, (unsigned char*)buf, size);

  rank29 %= 29; rank31 %= 31;

  checksum->suma ^= work<<rank29 | work>>(32-rank29);
  checksum->sumb ^= work<<rank31 | work>>(32-rank31);
}

/* Combine checksums over all nodes */
void DML_checksum_combine(DML_Checksum *checksum){
  DML_global_xor(&checksum->suma);
  DML_global_xor(&checksum->sumb);
}


/* Add single checksum set to the total */
void DML_checksum_peq(DML_Checksum *total, DML_Checksum *checksum){
  total->suma ^= checksum->suma;
  total->sumb ^= checksum->sumb;
}


/*------------------------------------------------------------------*/
/* Is this a big endian architecture? Return 1 or 0. */
int DML_big_endian(void)
{
  union {
    int  l;
    char c[sizeof(int)];
  } u;
  u.l = 1;

  return (u.c[sizeof(int)-1] == 1 ? 1 : 0); 
}


/* Do byte reversal on n contiguous 32-bit words */
void DML_byterevn32(uint32_t w[], size_t n)
{
  uint32_t old,newv;
  size_t j;

  assert(sizeof(uint32_t) == 4);
  
  for(j=0; j<n; j++)
    {
      old = w[j];
      newv = old >> 24 & 0x000000ff;
      newv |= old >> 8 & 0x0000ff00;
      newv |= old << 8 & 0x00ff0000;
      newv |= old << 24 & 0xff000000;
      w[j] = newv;
    }
}


/* Do byte reversal on n contiguous 64-bit words */
void DML_byterevn64(uint32_t w[], size_t n)
{
  uint32_t tmp;
  size_t j;

  assert(sizeof(uint32_t) == 4);
  
  /* First swap pairs of 32-bit words */
  for(j=0; j<n; j++){
    tmp = w[2*j];
    w[2*j] = w[2*j+1];
    w[2*j+1] = tmp;
  }

  /* Then swap bytes in 32-bit words */
  DML_byterevn32(w, 2*n);
}

/* Do byte reversal on size bytes of contiguous words,
   each word consisting of word_size bytes 
   word_size = 4 or 8 are the only choices */

void DML_byterevn(char *buf, size_t size, int word_size){
  if(word_size == 4)
    DML_byterevn32((uint32_t *)buf, size/word_size);
  else if(word_size == 8)
    DML_byterevn64((uint32_t *)buf, size/word_size);
  else{
    printf("DML_byterevn: illegal word_size %d\n",word_size);
  }
}

/*------------------------------------------------------------------*/
/* Read and write buffer management */

/* Compute number of sites worth of data that fit in allowed space */
/* The number is supposed to be a multiple of "factor" */
size_t DML_max_buf_sites(size_t size, int factor){
  return ((size_t)DML_BUF_BYTES/(size*factor))*factor;
}

/*------------------------------------------------------------------*/
char *DML_allocate_buf(size_t size, size_t max_buf_sites){
  char *lbuf;

  if(max_buf_sites == 0)return NULL;
  lbuf = (char*)malloc(max_buf_sites*size);
  return lbuf;
}

/*------------------------------------------------------------------*/
/* Increment buffer count. Write buffer when full or last site processed */
size_t DML_write_buf_next(LRL_RecordWriter *lrl_record_out, size_t size,
			  char *lbuf, size_t buf_sites, size_t max_buf_sites, 
			  size_t isite, size_t max_dest_sites, 
			  uint64_t *nbytes, char *myname, 
			  int this_node, int *err){
  size_t new_buf_sites = buf_sites + 1;

  *err = 0;
  
  /* write buffer when full or last site processed */
  if( (new_buf_sites == max_buf_sites) || (isite == max_dest_sites - 1))
    {
      if(LRL_write_bytes(lrl_record_out,lbuf,new_buf_sites*size)
	 != new_buf_sites*size){
	printf("%s(%d) write error\n",myname,this_node);
	*err = -1;
	return 0;
      }
      *nbytes += new_buf_sites*size;
      /* Reset buffer */
      memset((void *)lbuf, 0, max_buf_sites*size);
      new_buf_sites = 0;
    }
  return new_buf_sites;
}

/*------------------------------------------------------------------*/
/* Seek and write buffer.
   For now we work with only one site at a time */

size_t DML_seek_write_buf(LRL_RecordWriter *lrl_record_out, 
			  DML_SiteRank seeksite, size_t size,
			  char *lbuf, size_t buf_sites, size_t max_buf_sites, 
			  size_t isite, size_t max_dest_sites, 
			  uint64_t *nbytes, char *myname, 
			  int this_node, int *err){

  size_t new_buf_sites = buf_sites + 1;

  /* Seeking makes buffering more complicated.  We take the easy way
     out until we are forced to find an intelligent way to do this */

  max_buf_sites = 1;  /* Force a max one-site buffer. Should be
			 changed in the future. */

  *err = 0;
  
  /* write buffer when full or last site processed */
  if( (new_buf_sites == max_buf_sites) || (isite == max_dest_sites - 1))
    {
      /* Seek to the appropriate position in the record */
      if(LRL_seek_write_record(lrl_record_out,(off_t)size*seeksite)
	 != LRL_SUCCESS){
	*err = -1;
	return 0;
      }
      /* Write to the current position */
      if(LRL_write_bytes(lrl_record_out,lbuf,new_buf_sites*size)
	 != new_buf_sites*size){
	printf("%s(%d) write error\n",myname,this_node);
	*err = -1;
	return 0;
      }
      *nbytes += new_buf_sites*size;
      /* Reset buffer */
      memset((void *)lbuf, 0, max_buf_sites*size);
      new_buf_sites = 0;
    }
  return new_buf_sites;
}

/*------------------------------------------------------------------*/
/* Seek and read buffer.  We work with only one site at a time for now */


size_t DML_seek_read_buf(LRL_RecordReader *lrl_record_in, 
			 DML_SiteRank seeksite, size_t size,
			 char *lbuf, size_t *buf_extract, size_t buf_sites, 
			 size_t max_buf_sites, size_t isite, 
			 size_t max_send_sites, 
			 uint64_t *nbytes, char *myname, int this_node,
			 int *err){
  /* Number of available sites in read buffer */
  size_t new_buf_sites = buf_sites;   

  /* Seeking makes buffering more complicated.  We take the easy way
     out until we are forced to find an intelligent way to do this */

  max_buf_sites = 1;  /* Force a max one-site buffer. Should be
			 changed in the future. */
  *err = 0;

  if(*buf_extract == buf_sites){  
    /* new buffer length  = remaining sites, but never bigger 
       than the buffer capacity */
    new_buf_sites = max_send_sites - isite;
    if(new_buf_sites > max_buf_sites) new_buf_sites = max_buf_sites; 

    /* Seek to the appropriate position in the record */
    if(LRL_seek_read_record(lrl_record_in,(off_t)size*seeksite)
       != LRL_SUCCESS){
      *err = -1;
      return 0;
    }

    /* Fill the buffer */
    if( LRL_read_bytes(lrl_record_in, lbuf, new_buf_sites*size) 
	!= new_buf_sites*size){
      printf("%s(%d) read error\n", myname,this_node); 
      *err = -1;
      return 0;
    }
    *nbytes += new_buf_sites*size;
    *buf_extract = 0;  /* reset counter */
  }  /* end of the buffer read */

  return new_buf_sites;
}

/*------------------------------------------------------------------*/
/* Get new buffer if no data remains to be processed */

size_t DML_read_buf_next(LRL_RecordReader *lrl_record_in, size_t size,
			 char *lbuf, size_t *buf_extract, size_t buf_sites, 
			 size_t max_buf_sites, size_t isite, 
			 size_t max_send_sites, 
			 uint64_t *nbytes, char *myname, int this_node,
			 int *err){
  /* Number of available sites in read buffer */
  size_t new_buf_sites = buf_sites;   

  *err = 0;

  if(*buf_extract == buf_sites){  
    /* new buffer length  = remaining sites, but never bigger 
       than the buffer capacity */
    new_buf_sites = max_send_sites - isite;
    if(new_buf_sites > max_buf_sites) new_buf_sites = max_buf_sites; 
    /* Fill the buffer */
    if( LRL_read_bytes(lrl_record_in, lbuf, new_buf_sites*size) 
	!= new_buf_sites*size){
      printf("%s(%d) read error\n", myname,this_node); 
      *err = -1;
      return 0;
    }
    *nbytes += new_buf_sites*size;
    *buf_extract = 0;  /* reset counter */
  }  /* end of the buffer read */

  return new_buf_sites;
}

/*------------------------------------------------------------------*/
/* Determine the node that does my I/O */
int DML_my_ionode(int volfmt, int serpar, DML_Layout *layout){

  if(volfmt == DML_SINGLEFILE){
    if(serpar == DML_SERIAL)
      return layout->master_io_node;
    else
      return layout->this_node;
  }
  else if(volfmt == DML_MULTIFILE){
    return layout->this_node;
  }
  else if(volfmt == DML_PARTFILE){
    return layout->ionode(layout->this_node);
  }
  else {
    printf("DML_my_ionode: Bad volfmt code %d\n",volfmt);
    return 0;
  }
}

/*------------------------------------------------------------------*/
/* Synchronize the writers */

int DML_synchronize_out(LRL_RecordWriter *lrl_record_out, DML_Layout *layout){
  void *state_ptr;
  size_t state_size;
  int status;
  int master_io_node = layout->master_io_node;
  char myname[] = "DML_synchronize_out";

  /* DML isn't supposed to know the inner workings of LRL or LIME,
     so the state is captured as a string of bytes that only LRL
     understands.  All nodes create their state structures. */
  LRL_get_writer_state(lrl_record_out, &state_ptr, &state_size);

  /* The broadcast assumes all nodes are in the synchronization group 
     which is what we want for singlefile parallel I/O.
     If we decide later to do partfile parallel I/O we will need to
     change it */
  DML_broadcast_bytes((char *)state_ptr, state_size, layout->this_node, 
		      master_io_node);

  /* All nodes but the master node set their states */
  if(layout->this_node != master_io_node)
    LRL_set_writer_state(lrl_record_out, state_ptr);

  LRL_destroy_writer_state_copy(state_ptr);

  return 0;
}

/*------------------------------------------------------------------*/

/* The following four procedures duplicate the functionality
   of DML_partition_out.  They were broken out to allow
   finer high-level control of record writing.  After they
   have been tested, they will replace DML_partition_out */

/* See DML_partition_out below for a description */
/* This routine opens a record and prepares to write a field */

DML_RecordWriter *DML_partition_open_out(
	   LRL_RecordWriter *lrl_record_out, size_t size, 
	   size_t set_buf_sites, DML_Layout *layout, DML_SiteList *sites,
	   int volfmt, int serpar, DML_Checksum *checksum)
{
  char *outbuf;
  int *coords;
  int this_node = layout->this_node;
  int my_io_node;
  int latdim = layout->latdim;
  size_t max_buf_sites;
  DML_RecordWriter *dml_record_out;
  char myname[] = "DML_partition_open_out";

  dml_record_out = (DML_RecordWriter *)malloc(sizeof(DML_RecordWriter));
  if(!dml_record_out){
    printf("%s(%d): No space for DML_RecordWriter\n",myname,this_node);
    return NULL;
  }

  /* Get my I/O node */
  my_io_node = DML_my_ionode(volfmt, serpar, layout);

  /* Allocate buffer for writing or sending data */
  /* I/O node needs a large buffer.  Others only enough for one site */
  /* But if set_buf_sites > 0 it determines the size of the buffer */
  if(this_node == my_io_node){
    max_buf_sites = DML_max_buf_sites(size,1);
    if(set_buf_sites > 0)max_buf_sites = set_buf_sites;
  }
  else{
    max_buf_sites = 1;
  }

  outbuf = DML_allocate_buf(size,max_buf_sites);
  if(!outbuf){
    printf("%s(%d) can't malloc outbuf\n",myname,this_node);
    return 0;
  }

  /* Allocate lattice coordinate */
  coords = DML_allocate_coords(latdim, myname, this_node);
  if(!coords){free(outbuf);return NULL;}
  
  /* Initialize checksum */
  DML_checksum_init(checksum);
  
  /* Save/set the initial state */

  dml_record_out->lrl_rw          = lrl_record_out;
  dml_record_out->outbuf          = outbuf;
  dml_record_out->buf             = outbuf;
  dml_record_out->coords          = coords;
  dml_record_out->checksum        = checksum;
  dml_record_out->current_node    = my_io_node;
  dml_record_out->my_io_node      = my_io_node;
  dml_record_out->nbytes          = 0;
  dml_record_out->isite           = 0;
  dml_record_out->buf_sites       = 0;
  dml_record_out->max_buf_sites   = max_buf_sites;
  dml_record_out->max_dest_sites  = sites->number_of_io_sites;

  return dml_record_out;
}

/*------------------------------------------------------------------*/
/* See DML_partition_out below for a description */
/* This procedure writes one site's worth of data to an open record at
   a position specified by the site rank "seeksite" relative to the
   beginning of the binary data */

int DML_partition_sitedata_out(DML_RecordWriter *dml_record_out,
	   void (*get)(char *buf, size_t index, int count, void *arg),
	   DML_SiteRank snd_coords, int count, size_t size, int word_size, 
	   void *arg, DML_Layout *layout)
{

  LRL_RecordWriter *lrl_record_out = dml_record_out->lrl_rw;
  char *outbuf                     = dml_record_out->outbuf;
  char *buf                        = dml_record_out->buf;
  int *coords                      = dml_record_out->coords;
  DML_Checksum *checksum           = dml_record_out->checksum;
  int current_node	           = dml_record_out->current_node;
  int my_io_node                   = dml_record_out->my_io_node;
  uint64_t nbytes                  = dml_record_out->nbytes;
  size_t isite                     = dml_record_out->isite;
  size_t buf_sites                 = dml_record_out->buf_sites;
  size_t max_buf_sites             = dml_record_out->max_buf_sites;
  size_t max_dest_sites            = dml_record_out->max_dest_sites;

  int this_node    = layout->this_node;
  int latdim       = layout->latdim;
  int *latsize     = layout->latsize;

  int new_node;
  int err;
  char scratch_buf[4];
  char myname[] = "DML_partition_sitedata_out";

  scratch_buf[0] = scratch_buf[1] = scratch_buf[2] = scratch_buf[3] = '\0';

  /* Convert lexicographic rank to coordinates */
  DML_lex_coords(coords, latdim, latsize, snd_coords);
  
  /* Node that has this data and sends it to my_io_node */
  new_node = layout->node_number(coords);
  
  /* Send nodes must wait for a ready signal from the I/O node
     to prevent message pileups on the I/O node */
  
  /* CTS only if changing data source node */
  if(new_node != current_node){
    DML_clear_to_send(scratch_buf,4,my_io_node,new_node);
    current_node = new_node;
  }
  
  /* Fetch site data and copy to the write buffer */
  if(this_node == current_node){
    /* Fetch directly to the buffer */
    buf = outbuf + size*buf_sites;
    get(buf,layout->node_index(coords),count,arg);
  }
  
  /* Send result to my I/O node. Avoid I/O node sending to itself. */
  if (current_node != my_io_node) 
    {
#if 1
      /* Data from any other node is received in the I/O node write buffer */
      if(this_node == my_io_node){
	buf = outbuf + size*buf_sites;
      }
      DML_route_bytes(buf,size,current_node,my_io_node);
#else
      /* Data from any other node is received in the I/O node write buffer */
      if(this_node == my_io_node){
	buf = outbuf + size*buf_sites;
	DML_get_bytes(buf,size,current_node);
      }
      
      /* All other nodes send the data */
      if(this_node == current_node){
	DML_send_bytes(buf,size,my_io_node);
      }
#endif
    }
  
  /* my_io_node writes the data */
  if(this_node == my_io_node)
    {
      /* Do byte reordering before checksum */
      if (! DML_big_endian())
	DML_byterevn(buf, size, word_size);
      
      /* Update checksum */
      DML_checksum_accum(checksum, snd_coords, buf, size);
      
      /* Write the buffer when full */
      
      buf_sites = DML_seek_write_buf(lrl_record_out, snd_coords, size,
				     outbuf, buf_sites, max_buf_sites, 
				     isite, max_dest_sites, &nbytes,
				     myname, this_node, &err);
      if(err < 0) {return 1;}
    }
  isite++;

  /* Save changes to state */

  dml_record_out->current_node    = current_node;
  dml_record_out->nbytes          = nbytes;
  dml_record_out->isite           = isite;
  dml_record_out->buf_sites       = buf_sites;

  return 0;
}
  
/*------------------------------------------------------------------*/
/* See DML_partition_out below for a description */
/* This routine closes an open record and cleans up */

/* See DML_partition_out below for a description */
/* This writes one site's worth of data to an open record */

int DML_partition_allsitedata_out(DML_RecordWriter *dml_record_out, 
	   void (*get)(char *buf, size_t index, int count, void *arg),
	   int count, size_t size, int word_size, void *arg, 
           DML_Layout *layout, DML_SiteList *sites)
{

  DML_SiteRank snd_coords;

  /* Iterate over all sites processed by this I/O partition */

  snd_coords      = DML_init_site_loop(sites);
  do {
    if(DML_partition_sitedata_out(dml_record_out, get,
		  snd_coords, count, size, word_size, arg, layout) != 0)
      return 1;
    
  } while(DML_next_site_loop(&snd_coords, sites));
  
  return 0;
}

  
/* See DML_partition_out below for a description */
/* This routine closes an open record and cleans up */

uint64_t DML_partition_close_out(DML_RecordWriter *dml_record_out)
{

  uint64_t nbytes = dml_record_out->nbytes;

  if(dml_record_out == NULL)return 0;
  if(dml_record_out->coords != NULL)
    free(dml_record_out->coords);
  if(dml_record_out->outbuf != NULL)
    free(dml_record_out->outbuf);
  free(dml_record_out);

  /* Number of bytes written by this node only */
  return nbytes;
}

/*------------------------------------------------------------------*/
/* Each I/O node (or the master node) receives data from all of its
   nodes and writes it to its file.
   Returns the checksum and number of bytes written by this node only */

/* In order to be nondeadlocking, this algorithm requires that the set
   of nodes containing sites belonging to any single I/O node are
   disjoint from the corresponding set for any other I/O node.  This
   algorithm is intended for SINGLEFILE/SERIAL, MULTIFILE, and
   PARTFILE modes. */

uint64_t DML_partition_out(LRL_RecordWriter *lrl_record_out, 
	   void (*get)(char *buf, size_t index, int count, void *arg),
	   int count, size_t size, int word_size, void *arg, 
	   DML_Layout *layout, DML_SiteList *sites, int volfmt, 
	   int serpar, DML_Checksum *checksum)
{
  char *buf,*outbuf,*scratch_buf;
  int current_node, new_node;
  int *coords;
  int this_node = layout->this_node;
  int my_io_node;
  int latdim = layout->latdim;
  int *latsize = layout->latsize;
  size_t isite,buf_sites,max_buf_sites,max_dest_sites;
  int err;
  DML_SiteRank snd_coords;
  uint64_t nbytes = 0;
  char myname[] = "DML_partition_out";

  /* Get my I/O node */
  my_io_node = DML_my_ionode(volfmt, serpar, layout);

  /* Allocate buffer for writing or sending data */
  /* I/O node needs a large buffer.  Others only enough for one site */
  if(this_node == my_io_node)
    max_buf_sites = DML_max_buf_sites(size,1);
  else
    max_buf_sites = 1;

  if(serpar == DML_PARALLEL)
    max_buf_sites = 1;

  outbuf = DML_allocate_buf(size,max_buf_sites);
  if(!outbuf){
    printf("%s(%d) can't malloc outbuf\n",myname,this_node);
    return 0;
  }

  scratch_buf = DML_allocate_buf(4,1);
  if(!scratch_buf){
    printf("%s(%d) can't malloc scratch_buf\n",myname,this_node);
    return 0;
  }
  memset(scratch_buf,0,4);

  /* Allocate lattice coordinate */
  coords = DML_allocate_coords(latdim, myname, this_node);
  if(!coords){free(outbuf);return 0;}
  
  /* Initialize checksum */
  DML_checksum_init(checksum);
  
#ifdef DML_DEBUG
  if (! DML_big_endian())
    printf("%s(%d): byte reversing %d\n",myname,this_node,word_size);
#endif
  
  /* Maximum number of sites to be processed */
  max_dest_sites = sites->number_of_io_sites;
  isite = 0;  /* Running count of all sites */
  
  current_node = my_io_node;
  
  /* Loop over the sending coordinates */
  buf = outbuf;
  buf_sites = 0;   /* Count of sites in the output buffer */
  snd_coords = DML_init_site_loop(sites);

  do {
    /* Convert lexicographic rank to coordinates */
    DML_lex_coords(coords, latdim, latsize, snd_coords);

    /* Node that sends data */
    new_node = layout->node_number(coords);

    /* Send nodes must wait for a ready signal from the I/O node
       to prevent message pileups on the I/O node */
    

    /* CTS only if changing data source node */
    if(new_node != current_node){
      DML_clear_to_send(scratch_buf,4,my_io_node,new_node);
      current_node = new_node;
    }

    /* Copy to the write buffer */
    if(this_node == current_node){
      /* Fetch directly to the buffer */
      buf = outbuf + size*buf_sites;
      get(buf,layout->node_index(coords),count,arg);
    }

    /* Send result to my I/O node. Avoid I/O node sending to itself. */
    if (current_node != my_io_node) 
    {
#if 1
      /* Data from any other node is received in the I/O node write buffer */
      if(this_node == my_io_node){
	buf = outbuf + size*buf_sites;
      }
      DML_route_bytes(buf,size,current_node,my_io_node);
#else
      /* Data from any other node is received in the I/O node write buffer */
      if(this_node == my_io_node){
	buf = outbuf + size*buf_sites;
	DML_get_bytes(buf,size,current_node);
      }
    
      /* All other nodes send the data */
      if(this_node == current_node){
	DML_send_bytes(buf,size,my_io_node);
      }
#endif
    }

    /* Now write data */
    if(this_node == my_io_node)
    {
      /* Do byte reordering before checksum */
      if (! DML_big_endian())
	DML_byterevn(buf, size, word_size);
      
      /* Update checksum */
      DML_checksum_accum(checksum, snd_coords, buf, size);
      
      /* Write the buffer when full */

      if(serpar == DML_SERIAL)
	buf_sites = DML_write_buf_next(lrl_record_out, size,
				       outbuf, buf_sites, max_buf_sites, 
				       isite, max_dest_sites, &nbytes,
				       myname, this_node, &err);
      else
	buf_sites = DML_seek_write_buf(lrl_record_out, snd_coords, size,
				       outbuf, buf_sites, max_buf_sites, 
				       isite, max_dest_sites, &nbytes,
				       myname, this_node, &err);
      if(err < 0) {free(outbuf); free(coords); return 0;}
    }
    isite++;
  } while(DML_next_site_loop(&snd_coords, sites));
  
  free(coords);
  free(scratch_buf);
  free(outbuf);

  /* Number of bytes written by this node only */
  return nbytes;
}

/*------------------------------------------------------------------*/
/* The master node fetches the global data in one call to "get"  and writes */
/* Returns the number of bytes written */

size_t DML_global_out(LRL_RecordWriter *lrl_record_out, 
	   void (*get)(char *buf, size_t index, int count, void *arg),
	   int count, size_t size, int word_size, void *arg, 
           DML_Layout *layout, int volfmt, 
	   DML_Checksum *checksum)
{
  char *buf;
  int this_node = layout->this_node;
  size_t nbytes = 0;
  char myname[] = "DML_global_out";
  
  /* Allocate buffer for datum */
  buf = (char *)malloc(size);
  if(!buf){
    printf("%s(%d) can't malloc buf\n",myname,this_node);
    return 0;
  }
  
  /* Initialize checksum */
  DML_checksum_init(checksum);
  
#ifdef DML_DEBUG
  if (! DML_big_endian())
    printf("%s(%d): byte reversing %d\n",myname,this_node,word_size);
#endif
  
  /* Master node writes all the data */
  if(this_node == layout->master_io_node){
    /* Get all the data.  0 for the unused site index */
    get(buf,0,count,arg);
    
    /* Do byte reordering before checksum */
    if (! DML_big_endian())
      DML_byterevn(buf, size, word_size);
    
    /* Do checksum.  Straight crc32. */
    DML_checksum_accum(checksum, 0, buf, size);
    
    /* Write all the data */
    nbytes = LRL_write_bytes(lrl_record_out,(char *)buf,size);
    if( nbytes != size){
      free(buf); return 0;}
  }
  
  free(buf);
  return nbytes;
}

/* Each node writes its data to its own private file.  The order of
   sites is sequential according to the layout storage order, which
   is not likely to be lexicographic. */

/* Returns the number of bytes written by this node alone */

uint64_t DML_multifile_out(LRL_RecordWriter *lrl_record_out, 
	      void (*get)(char *buf, size_t index, int count, void *arg),
	      int count, size_t size, int word_size, void *arg, 
	      DML_Layout *layout, DML_Checksum *checksum)
{
  
  size_t max_buf_sites, buf_sites;
  size_t isite, max_dest_sites;
  uint64_t nbytes = 0;
  DML_SiteRank rank;
  int this_node = layout->this_node;
  char myname[] = "DML_multifile_out";
  char *lbuf, *buf=NULL;
  int *coords;
  int err;

  /* Allocate buffer for writing */
  max_buf_sites = DML_max_buf_sites(size,1);
  lbuf = DML_allocate_buf(size,max_buf_sites);
  if(!lbuf){
    printf("%s(%d): Can't malloc lbuf\n",myname,this_node);
    return 0;
  }

  /* Allocate coordinate */
  coords = DML_allocate_coords(layout->latdim,myname,this_node);
  if(!coords){free(buf); return 0;}

  /* Initialize checksum */
  DML_checksum_init(checksum);

  buf_sites = 0;

  max_dest_sites = layout->sites_on_node;

  /** TODO: VECTORIZE THE TRANSFER - CD **/
  /* Loop over the storage order index of sites on the local node */
  for(isite = 0; isite < max_dest_sites; isite++){

    /* The coordinates of this site */
    layout->get_coords(coords, this_node, isite);

    /* The lexicographic rank of this site */
    rank = DML_lex_rank(coords, layout->latdim, layout->latsize);

    /* Fetch directly to the buffer */
    buf = lbuf + size*buf_sites;
    get(buf, isite, count, arg);

    /* Accumulate checksums as the values are inserted into the buffer */
    
    /* Do byte reversal if needed */
    if (! DML_big_endian())
      DML_byterevn(buf, size, word_size);
    
    DML_checksum_accum(checksum, rank, buf, size);
    
    buf_sites = DML_write_buf_next(lrl_record_out, size,
				   lbuf, buf_sites, max_buf_sites, 
				   isite, max_dest_sites, &nbytes,
				   myname, this_node, &err);
    if(err < 0) {free(lbuf); free(coords); return 0;}
  } /* isite */

  free(lbuf);   free(coords);
  
  /* Return the number of bytes written by this node only */
  return nbytes;

} /* DML_multifile_out */

/*------------------------------------------------------------------*/
/* Each node reads its data from its own private file.  The order of
   sites is assumed to be sequential according to the layout storage
   order. */

/* Returns the number of bytes read by this node alone */

uint64_t DML_multifile_in(LRL_RecordReader *lrl_record_in, 
	     DML_SiteRank sitelist[],
	     void (*put)(char *buf, size_t index, int count, void *arg),
	     int count, size_t size, int word_size, void *arg, 
	     DML_Layout *layout, DML_Checksum *checksum)
{
  size_t buf_sites, buf_extract, max_buf_sites;
  size_t isite, max_send_sites;
  uint64_t nbytes = 0;
  DML_SiteRank rank;
  int this_node = layout->this_node;
  char myname[] = "DML_multifile_in";
  char *lbuf, *buf;
  int *coords;
  int err;

  /* Allocate buffer for reading */
  max_buf_sites = DML_max_buf_sites(size,1);
  lbuf = DML_allocate_buf(size,max_buf_sites);
  if(!lbuf)return 0;

  /* Allocate coordinate */
  coords = DML_allocate_coords(layout->latdim, myname, this_node);
  if(!coords){free(lbuf);return 0;}

  /* Initialize checksum */
  DML_checksum_init(checksum);

  buf_sites = 0;      /* Length of current read buffer */
  buf_extract = 0;    /* Counter for current site in read buffer */
  max_send_sites = layout->sites_on_node;

  /** TODO: VECTORIZE THE TRANSFER - CD **/
  /* Loop over the storage order site index for this node */
  for(isite = 0; isite < max_send_sites; isite++){

    /* The coordinates of this site */
    layout->get_coords(coords, this_node, isite);

    /* The lexicographic rank of this site */
    rank = DML_lex_rank(coords, layout->latdim, layout->latsize);

    /* Refill buffer if necessary */
    buf_sites = DML_read_buf_next(lrl_record_in, size, lbuf, 
				  &buf_extract, buf_sites, max_buf_sites, 
				  isite, max_send_sites, &nbytes, 
				  myname, this_node, &err);
    if(err < 0){free(lbuf);free(coords);return 0;}
    
    /* Copy data directly from the buffer */
    buf = lbuf + size*buf_extract;

    /* Accumulate checksums as the values are inserted into the buffer */
    DML_checksum_accum(checksum, rank, buf, size);
    
    /* Do byte reversal after checksum if needed */
    if (! DML_big_endian())
      DML_byterevn(buf, size, word_size);

    put(buf, isite, count, arg);
    
    buf_extract++;
  } /* isite */

  free(lbuf);   free(coords);
  
  /* Return the number of bytes read by this node only */
  return nbytes;
}

/*------------------------------------------------------------------*/
/* Synchronize the readers */

int DML_synchronize_in(LRL_RecordReader *lrl_record_in, DML_Layout *layout){
  LRL_RecordReader *lrl_record_in_copy;
  void *state_ptr;
  size_t state_size;
  int status;
  int master_io_node = layout->master_io_node;
  char myname[] = "DML_synchronize_in";

  /* DML isn't supposed to know the inner workings of LRL or LIME,
     so the state is captured as a string of bytes that only LRL
     understands */
  LRL_get_reader_state(lrl_record_in, &state_ptr, &state_size);

  /* The broadcast assumes all nodes are in the synchronization group 
     which is what we want for singlefile parallel I/O.
     If we decide later to do partfile parallel I/O we will need to
     change it */
  DML_broadcast_bytes((char *)state_ptr, state_size, layout->this_node, 
		      master_io_node);
  
  /* All nodes but the master node set their states */
  if(layout->this_node != master_io_node)
    LRL_set_reader_state(lrl_record_in, state_ptr);

  LRL_destroy_reader_state_copy(state_ptr);

  return 0;
}

/*------------------------------------------------------------------*/
/* The following four procedures duplicate the functionality
   of DML_partition_in.  They were broken out to allow
   finer high-level control of record reading.  After they
   have been tested, they will replace DML_partition_in */

/* See DML_partition_in below for a description */
/* This routine opens a record and prepares to read a field */

DML_RecordReader *DML_partition_open_in(LRL_RecordReader *lrl_record_in, 
	  size_t size, size_t set_buf_sites, DML_Layout *layout, 
 	  DML_SiteList *sites, int volfmt, int serpar, DML_Checksum *checksum)
{
  char *inbuf;
  int my_io_node;
  int *coords;
  int this_node = layout->this_node;
  int latdim = layout->latdim;
  size_t max_buf_sites;
  DML_RecordReader *dml_record_in;
  char myname[] = "DML_partition_open_in";

  dml_record_in = (DML_RecordReader *)malloc(sizeof(DML_RecordReader));
  if(!dml_record_in){
    printf("%s(%d): No space for DML_RecordReader\n",myname,this_node);
    return NULL;
  }

  /* Get my I/O node */
  my_io_node = DML_my_ionode(volfmt, serpar, layout);

  /* Allocate buffer for reading or receiving data */
  /* I/O node needs a large buffer.  Others only enough for one site */
  if(this_node == my_io_node){
    max_buf_sites = DML_max_buf_sites(size,1);
    if(set_buf_sites > 0)max_buf_sites = set_buf_sites;
  }
  else
    max_buf_sites = 1;

 
  inbuf = DML_allocate_buf(size,max_buf_sites);
  if(!inbuf){
    printf("%s(%d) can't malloc inbuf\n",myname,this_node);
    return 0;
  }
 

  /* Allocate coordinate counter */
  coords = DML_allocate_coords(latdim, myname, this_node);
  if(!coords){free(inbuf); return 0;}
  
  /* Initialize checksum */
  DML_checksum_init(checksum);

#ifdef DML_DEBUG
  if (! DML_big_endian())
    printf("%s(%d): byte reversing %d\n",myname,this_node,word_size);
#endif

  /* Save/set the initial state */

  dml_record_in->lrl_rr          = lrl_record_in;
  dml_record_in->inbuf           = inbuf;
  dml_record_in->coords          = coords;
  dml_record_in->checksum        = checksum;
  dml_record_in->current_node    = my_io_node;
  dml_record_in->my_io_node      = my_io_node;
  dml_record_in->nbytes          = 0;
  dml_record_in->isite           = 0;
  dml_record_in->buf_sites       = 0;
  dml_record_in->buf_extract     = 0;
  dml_record_in->max_buf_sites   = max_buf_sites;
  dml_record_in->max_send_sites  = sites->number_of_io_sites;

  return dml_record_in;
}

/*------------------------------------------------------------------*/
/* See DML_partition_in below for a description */
/* This routine opens a record and prepares to read a field */

int DML_partition_sitedata_in(DML_RecordReader *dml_record_in, 
	  void (*put)(char *buf, size_t index, int count, void *arg),
	  DML_SiteRank rcv_coords, int count, size_t size, int word_size, 
	  void *arg, DML_Layout *layout)
{

  LRL_RecordReader *lrl_record_in  = dml_record_in->lrl_rr;
  char *inbuf                      = dml_record_in->inbuf;
  int *coords                      = dml_record_in->coords;
  DML_Checksum *checksum           = dml_record_in->checksum;
  int current_node	           = dml_record_in->current_node;
  int my_io_node                   = dml_record_in->my_io_node;
  uint64_t nbytes                  = dml_record_in->nbytes;
  size_t isite                     = dml_record_in->isite;
  size_t buf_sites                 = dml_record_in->buf_sites;
  size_t buf_extract               = dml_record_in->buf_extract;
  size_t max_buf_sites             = dml_record_in->max_buf_sites;
  size_t max_send_sites            = dml_record_in->max_send_sites;
  char *buf=NULL;

  int this_node = layout->this_node;
  int latdim    = layout->latdim;
  int *latsize  = layout->latsize;

  int dest_node;
  int err;
  char myname[] = "DML_partition_sitedata_in";

  /* Convert lexicographic rank to coordinates */
  DML_lex_coords(coords, latdim, latsize, rcv_coords);
  
  /* The node that gets the next datum */
  dest_node = layout->node_number(coords);
  
  if(this_node == my_io_node){
    /* I/O node reads the next value */
    buf_sites = DML_seek_read_buf(lrl_record_in, rcv_coords, size,
				  inbuf, &buf_extract, buf_sites,
				  max_buf_sites, isite, 
				  max_send_sites, &nbytes,
				  myname, this_node, &err);
    
    if(err < 0){
      printf("%s(%d) DML_seek_read_buf returns error\n",
	     myname,this_node);
      return 1;
    }

    /* Location of new datum on I/O node */
    buf = inbuf + size*buf_extract;
  }

  /* Send result to destination node. Avoid I/O node sending to itself. */
  if (dest_node != my_io_node) {
#if 1
    DML_route_bytes(buf,size,my_io_node,dest_node);
#else
    /* If destination elsewhere, send it */
    if(this_node == my_io_node){
      DML_send_bytes(buf, size, dest_node);
    }
    
    /* Other nodes receive from the master node */
    if(this_node == dest_node){
      DML_get_bytes(buf, size, my_io_node);
    }
#endif
  }
  
  /* Process data before inserting */
  if(this_node == dest_node){
    
    /* Accumulate checksum */
    DML_checksum_accum(checksum, rcv_coords, buf, size);
    
    /* Do byte reversal if necessary */
    if (! DML_big_endian())
      DML_byterevn(buf, size, word_size);
    
    /* Store the data */
    put(buf,layout->node_index(coords),count,arg);
  }
  
  buf_extract++;
  isite++;

  /* Save changes to state */

  dml_record_in->current_node    = current_node;
  dml_record_in->nbytes          = nbytes;
  dml_record_in->isite           = isite;
  dml_record_in->buf_sites       = buf_sites;
  dml_record_in->buf_extract     = buf_extract;

  return 0;
}

/*------------------------------------------------------------------*/
/* See DML_partition_in below for a description */
/* This routine opens a record and prepares to read a field */

int DML_partition_allsitedata_in(DML_RecordReader *dml_record_in, 
	  void (*put)(char *buf, size_t index, int count, void *arg),
	  int count, size_t size, int word_size, void *arg, 
	  DML_Layout *layout, DML_SiteList *sites, int volfmt,
	  DML_Checksum *checksum)
{

  DML_SiteRank rcv_coords;

  rcv_coords = DML_init_site_loop(sites);
  do {
    if(DML_partition_sitedata_in(dml_record_in, put, rcv_coords,
		  count, size, word_size, arg, layout) != 0)
      return 1;

  }  while(DML_next_site_loop(&rcv_coords, sites));

  return 0;
}

/*------------------------------------------------------------------*/
/* See DML_partition_in below for a description */
/* This routine opens a record and prepares to read a field */

uint64_t DML_partition_close_in(DML_RecordReader *dml_record_in)
{
  uint64_t nbytes = dml_record_in->nbytes;

  if(dml_record_in == NULL)return 0;
  if(dml_record_in->coords != NULL)
    free(dml_record_in->coords);
  if(dml_record_in->inbuf != NULL)
    free(dml_record_in->inbuf);
  free(dml_record_in);

  /* return the number of bytes read by this node only */
  return nbytes;
}

/*------------------------------------------------------------------*/
/* Each I/O node (or the master I/O node) reads data from its file and
   distributes it to its nodes.
   Returns the number of bytes read by this node only */

/* In order to be nondeadlocking, this algorithm requires that the set
   of nodes containing sites belonging to any single I/O node are
   disjoint from the corresponding set for any other I/O node.  This
   algorithm is intended for SINGLEFILE/SERIAL, MULTIFILE, and
   PARTFILE modes. */

uint64_t DML_partition_in(LRL_RecordReader *lrl_record_in, 
	  void (*put)(char *buf, size_t index, int count, void *arg),
	  int count, size_t size, int word_size, void *arg, 
	  DML_Layout *layout, DML_SiteList *sites, int volfmt,
	  int serpar, DML_Checksum *checksum)
{
  char *buf=NULL,*inbuf;
  int dest_node, my_io_node;
  DML_SiteRank rcv_coords;
  uint64_t nbytes = 0;
  int *coords;
  int this_node = layout->this_node;
  int latdim = layout->latdim;
  int *latsize = layout->latsize;
  size_t isite, buf_sites, buf_extract, max_send_sites, max_buf_sites;
  int err;
  char myname[] = "DML_partition_in";

  /* Get my I/O node */
  my_io_node = DML_my_ionode(volfmt, serpar, layout);

  /* Allocate buffer for reading or receiving data */
  /* I/O node needs a large buffer.  Others only enough for one site */
  if(this_node == my_io_node)
    max_buf_sites = DML_max_buf_sites(size,1);
  else
    max_buf_sites = 1;
  
  if(serpar == DML_PARALLEL)
    max_buf_sites = 1;

  inbuf = DML_allocate_buf(size,max_buf_sites);
  if(!inbuf){
    printf("%s(%d) can't malloc inbuf\n",myname,this_node);
    return 0;
  }
 

  /* Allocate coordinate counter */
  coords = DML_allocate_coords(latdim, myname, this_node);
  if(!coords){free(buf); return 0;}
  
  /* Initialize checksum */
  DML_checksum_init(checksum);

#ifdef DML_DEBUG
  if (! DML_big_endian())
    printf("%s(%d): byte reversing %d\n",myname,this_node,word_size);
#endif

  /* Maximum number of sites to be sent */
  max_send_sites = sites->number_of_io_sites;
  isite = 0;          /* Running count of sites processed */

  /* Loop over the receiving sites */
  buf_extract = 0;    /* Counter for current site in read buffer */
  buf = inbuf;        /* Address of current bytes */
  buf_sites = 0;      /* Number of sites in current read buffer */
  rcv_coords = DML_init_site_loop(sites);

  do {
    /* Convert lexicographic rank to coordinates */
    DML_lex_coords(coords, latdim, latsize, rcv_coords);
    
    /* The node that gets the next datum */
    dest_node = layout->node_number(coords);
    
    if(this_node == my_io_node){
      /* I/O node reads the next value */
      if(serpar == DML_SERIAL)
	buf_sites = DML_read_buf_next(lrl_record_in, size,
				      inbuf, &buf_extract, buf_sites,
				      max_buf_sites, isite, 
				      max_send_sites, &nbytes,
				      myname, this_node, &err);

      else
	buf_sites = DML_seek_read_buf(lrl_record_in, rcv_coords, size,
				      inbuf, &buf_extract, buf_sites,
				      max_buf_sites, isite, 
				      max_send_sites, &nbytes,
				      myname, this_node, &err);
      
      if(err < 0){
        printf("%s(%d) DML_seek_read_buf returns error\n",
               myname,this_node);
        free(inbuf);free(coords);
        return 0;
      }

      /* Location of new datum on I/O node */
      buf = inbuf + size*buf_extract;
    }

    /* Send result to destination node. Avoid I/O node sending to itself. */
    if (dest_node != my_io_node) {
#if 1
      DML_route_bytes(buf,size,my_io_node,dest_node);
#else
      /* If destination elsewhere, send it */
      if(this_node == my_io_node){
	DML_send_bytes(buf, size, dest_node);
      }
      
	/* Other nodes receive from the master node */
      if(this_node == dest_node){
	DML_get_bytes(buf, size, my_io_node);
      }
#endif
    }
    
    /* Process data before inserting */
    if(this_node == dest_node){
      
      /* Accumulate checksum */
      DML_checksum_accum(checksum, rcv_coords, buf, size);
      
      /* Do byte reversal if necessary */
      if (! DML_big_endian())
	DML_byterevn(buf, size, word_size);
      
      /* Store the data */
      put(buf,layout->node_index(coords),count,arg);
    }

    buf_extract++;
    isite++;

  }  while(DML_next_site_loop(&rcv_coords, sites));

  free(inbuf); free(coords);

  /* return the number of bytes read by this node only */
  return nbytes;
}

/*------------------------------------------------------------------*/
/* The master node reads all the global data at once,
   broadcasts to all nodes and calls "put" */

/* Returns the number of bytes read */

size_t DML_global_in(LRL_RecordReader *lrl_record_in, 
	  void (*put)(char *buf, size_t index, int count, void *arg),
	  int count, size_t size, int word_size, void *arg, 
          DML_Layout* layout, int volfmt, int broadcast_globaldata,
	  DML_Checksum *checksum)
{
  char *buf;
  int this_node = layout->this_node;
  size_t nbytes = 0;
  char myname[] = "DML_global_in";

  /* Allocate buffer for datum */
  buf = (char *)malloc(size);
  if(!buf){
    printf("%s(%d) can't malloc buf\n",myname,this_node);
    return 0;
  }

  /* Initialize checksum */
  DML_checksum_init(checksum);

  if(this_node == layout->master_io_node){
    /* Read all the data */
    nbytes = LRL_read_bytes(lrl_record_in, (char *)buf, size);
    if(nbytes != size){
      free(buf); return 0;
    }
    
    /* Do checksum.  Straight crc32. */
    DML_checksum_accum(checksum, 0, buf, size);

    /* Do byte reordering if needed */
    if (! DML_big_endian())
      DML_byterevn(buf, size, word_size);
    
  }

  /* We turn off broadcasting, for example, if we are doing
     single-processor file conversion */
  if(broadcast_globaldata){
    /* Broadcast the result to node bufs */
    DML_broadcast_bytes(buf, size, this_node, layout->master_io_node);
    /* All nodes store their data. Unused site index is 0. */
    put(buf,0,count,arg);
  }
  else{
    /* Only the master I/O node stores its data */
    if(this_node == layout->master_io_node)
      put(buf,0,count,arg);
  }

  free(buf);
  return nbytes;
}

