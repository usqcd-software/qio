/* DML_utils.c */
/* Utilities for DML */

#include <lrl.h>
#include <dml.h>
#include <stdio.h>
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#include <assert.h>
#include <crc32.h>
#include <sys/types.h>

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

/* Convert linear lexicographic rank to lexicographic coordinate */

void DML_lex_coords(int coords[], int latdim, int latsize[], 
		    DML_SiteRank rcv_coords)
{
  int dim;
  DML_SiteRank rank = rcv_coords;

  for(dim = 0; dim < latdim; dim++){
    coords[dim] = rank % latsize[dim];
    rank /= latsize[dim];
  }
}

/* Convert coordinate to linear lexicographic rank (inverse of
   DML_lex_coords) */

DML_SiteRank DML_lex_rank(int coords[], int latdim, int latsize[])
{
  int dim;
  DML_SiteRank rank = coords[latdim-1];

  for(dim = latdim-2; dim >= 0; dim--){
    rank = rank * latsize[dim] + coords[dim];
  }
  return rank;
}

/* Make temporary space for coords */

int *DML_allocate_coords(int latdim, char *myname, int this_node){
  int *coords;

  coords = (int *)malloc(latdim*sizeof(int));
  if(!coords)printf("%s(%d) can't malloc coords\n",myname,this_node);
  return coords;
}


/* The message structure holds the site datum and site rank */

/* Accessor: Size of message */
size_t DML_msg_sizeof(size_t size){
  return size + sizeof(DML_SiteRank);
}

/* Constructor*/
char *DML_allocate_msg(size_t size, char *myname, int this_node){
  char *msg;
  size_t sizeof_msg = DML_msg_sizeof(size);

  msg = malloc(sizeof_msg);
  if(!msg)printf("%s(%d) can't malloc msg\n",myname,this_node);
  return msg;
}

/* Accessor: Pointer to datum member of msg */
char *DML_msg_datum(char *msg, size_t size){
  return msg;
}

/* Accessor: Pointer to rank member of msg */
DML_SiteRank *DML_msg_rank(char *msg, size_t size){
  return (DML_SiteRank *)(msg + size);
}

/* Create a list of sites for this node in the order of storage for
   the prevailing layout */

int DML_create_sitelist(DML_Layout *layout,DML_SiteRank sitelist[]){
  size_t index;
  int *coords;
  int latdim = layout->latdim;
  int *latsize = layout->latsize;
  char myname[] = "DML_create_sitelist";

  /* Allocate coordinate */
  coords = DML_allocate_coords(latdim, myname, layout->this_node);
  if(!coords)return 1;
  
  /* Generate site list */
  for(index = 0; index < layout->sites_on_node; index++){
    layout->get_coords(coords,layout->this_node,index);
    /* Convert coordinate to lexicographic rank */
    sitelist[index] = DML_lex_rank(coords,latdim,latsize);
  }
  free(coords);
  return 0;
}

/* Check site list. Return 0 if it is in native order and 1 otherwise */

int DML_is_native_sitelist(DML_Layout *layout, DML_SiteRank sitelist[]){
  size_t index;
  int *coords;
  int latdim = layout->latdim;
  int *latsize = layout->latsize;
  char myname[] = "DML_is_native_sitelist";
  
  /* Allocate coordinate */
  coords = DML_allocate_coords(latdim, myname, layout->this_node);
  if(!coords)return 0;
  
  /* Generate site list */
  for(index = 0; index < layout->sites_on_node; index++){
    layout->get_coords(coords,layout->this_node,index);
    /* Convert coordinate to lexicographic rank */
    if(sitelist[index] != DML_lex_rank(coords,latdim,latsize)){
      free(coords);
      return 1;
    }
  }

  free(coords);
  return 0;
}

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
  u_int32 work = DML_crc32(0, buf, size);

  rank29 %= 29; rank31 %= 31;

  checksum->suma ^= work<<rank29 | work>>(32-rank29);
  checksum->sumb ^= work<<rank31 | work>>(32-rank31);
}

/* Combine checksums over all nodes */
void DML_checksum_combine(DML_Checksum *checksum){
  DML_global_xor(&checksum->suma);
  DML_global_xor(&checksum->sumb);
}

/* Do byte reversal on n contiguous 32-bit words */

void DML_byterevn32(u_int32 w[], size_t n)
{
  u_int32 old,newv;
  size_t j;

  assert(sizeof(u_int32) == 4);
  
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

void DML_byterevn64(u_int32 w[], size_t n)
{
  u_int32 tmp;
  size_t j;

  assert(sizeof(u_int32) == 4);
  
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
    DML_byterevn32((u_int32 *)buf, size/word_size);
  else if(word_size == 8)
    DML_byterevn64((u_int32 *)buf, size/word_size);
  else{
    printf("DML_byterevn: illegal word_size %d\n",word_size);
  }
}

/* Read and write buffer management */

/* Compute number of sites worth of data that fit in allowed space */
/* The number is supposed to be a multiple of "factor" */
size_t DML_max_buf_sites(size_t size, int factor){
  return ((size_t)DML_BUF_BYTES/(size*factor))*factor;
}

char *DML_allocate_buf(size_t size, size_t max_buf_sites, int this_node){
  char *lbuf;
  char myname[] = "DML_allocate_buf";

  if(max_buf_sites == 0)return NULL;
  lbuf = malloc(max_buf_sites*size);
  if(!lbuf)
    printf("%s(%d): Can't malloc lbuf\n",myname,this_node);
    
  return lbuf;
}

/* Increment buffer count. Write buffer when full or last site processed */
size_t DML_write_buf_next(LRL_RecordWriter *lrl_record_out, size_t size,
			  char *lbuf, size_t buf_sites, size_t max_buf_sites, 
			  size_t isite, size_t max_dest_sites, size_t *nbytes,
			  char *myname, int this_node){
  size_t new_buf_sites = buf_sites + 1;
  
  /* write buffer when full or last site processed */
  if( (new_buf_sites == max_buf_sites) || (isite == max_dest_sites - 1))
    {
      if(LRL_write_bytes(lrl_record_out,lbuf,new_buf_sites*size)
	 != new_buf_sites*size){
	printf("%s(%d) write error\n",myname,this_node);
	return -1;
      }
      *nbytes += new_buf_sites*size;
      /* Reset buffer */
      memset((void *)lbuf, 0, max_buf_sites*size);
      new_buf_sites = 0;
    }
  return new_buf_sites;
}

/* Get new buffer if no data remains to be processed */

size_t DML_read_buf_next(LRL_RecordReader *lrl_record_in, int size,
			 char *lbuf, size_t *buf_extract, size_t buf_sites, 
			 size_t max_buf_sites, size_t isite, 
			 size_t max_send_sites, 
			 size_t *nbytes, char *myname, int this_node){
  size_t new_buf_sites = buf_sites;

  if(*buf_extract == buf_sites){  
    /* new buffer length  = remaining sites, but never bigger 
       than the buffer capacity */
    new_buf_sites = max_send_sites - isite;
    if(new_buf_sites > max_buf_sites) new_buf_sites = max_buf_sites; 
    /* Fill the buffer */
    if( LRL_read_bytes(lrl_record_in, lbuf, new_buf_sites*size) 
	!= new_buf_sites*size){
      printf("%s(%d) read error\n", myname,this_node); 
      return -1;
    }
    *nbytes += new_buf_sites*size;
    *buf_extract = 0;  /* reset counter */
  }  /* end of the buffer read */

  return new_buf_sites;
}

/* The master node receives data from all nodes and writes it to one
   file.  The order of data is lexicographic. */

/* Returns the number of bytes written */

/* Todo: Should use buffers.  Site iteration can be done over siterank */

int DML_serial_out(LRL_RecordWriter *lrl_record_out, 
	   void (*get)(char *buf, size_t index, size_t count, void *arg),
	   size_t size, int word_size, void *arg, DML_Layout *layout,
	   DML_Checksum *checksum)
{
  char *buf;
  int current_node, new_node;
  int *coords; int dim;
  int this_node = layout->this_node;
  int latdim = layout->latdim;
  int *latsize = layout->latsize;
  DML_SiteRank snd_coords;
  size_t nbytes = 0;
  char myname[] = "DML_serial_out";

  /* Allocate buffer for datum */
  buf = (char *)malloc(size);
  if(!buf){
    printf("%s(%d) can't malloc buf\n",myname,this_node);
    return 1;
  }

  /* Initialize checksum */
  DML_checksum_init(checksum);

  /* Barrier */
  DML_sync();

#if(BIG_ENDIAN != 1)
  printf("%s(%d): byte reversing %d\n",myname,this_node,word_size);
#endif

  /* Allocate coordinate counter */
  coords = DML_allocate_coords(latdim, myname, this_node);
  if(!coords)return 0;
  
  /* Initialize current source node */
  current_node = DML_MASTER_NODE;
  
  /* Iterate over all sites in lexicographic order */
  DML_lex_init(&dim, coords, latdim, latsize);
  snd_coords = 0;
  /* Loop over the coordinates in lexicographic order */
  /** Could have just used a scalar counter - CD */
  do {
    
    /* Send nodes must wait for a ready signal from the master node
       to prevent message pileups on the master node */
    new_node = layout->node_number(coords);
    if(new_node != current_node){
      if(this_node == DML_MASTER_NODE && new_node != DML_MASTER_NODE)
	DML_send_bytes(buf,4,new_node); /* junk message */
      if(this_node == new_node && new_node != DML_MASTER_NODE)
	DML_get_bytes(buf,4,DML_MASTER_NODE);
      current_node = new_node;
    }
    
    /* Master node receives the data and writes it */
    if(this_node == DML_MASTER_NODE){
      if(current_node == DML_MASTER_NODE){
	/* Data on the master node is just copied to the write buffer */
	get(buf,layout->node_index(coords),1,arg);
      }
      else{
	/* Data from any other node is received in the write buffer */
	DML_get_bytes(buf,size,current_node);
      }
      
      /* Do byte reordering before checksum */
#if(BIG_ENDIAN != 1)
      DML_byterevn(buf, size, word_size);
#endif
      
      /* Update checksum */
      DML_checksum_accum(checksum, snd_coords, buf, size);
      
      /* Write the datum */
      if(LRL_write_bytes(lrl_record_out,buf,size) != size)return 0;
      nbytes += size;
    }
    /* All other nodes send the data */
    else{
      if(this_node == current_node){
	get(buf,layout->node_index(coords),1,arg);
	DML_send_bytes(buf,size,DML_MASTER_NODE);
      }
    }
    
    snd_coords++;
  } while(DML_lex_next(&dim, coords, latdim, latsize));
  
  free(coords);
  /* Return the number of bytes written by all nodes */
  DML_sum_size_t(&nbytes);
  return nbytes;
}

/* Each node writes its data to its own private file.  The order of
   sites is sequential according to the layout storage order, which
   is not likely to be lexicographic. */

/* Returns the number of bytes written by this node alone */

int DML_multifile_out(LRL_RecordWriter *lrl_record_out, 
	      void (*get)(char *buf, size_t index, size_t count, void *arg),
	      size_t size, int word_size, void *arg, 
	      DML_Layout *layout, DML_Checksum *checksum){
  
  size_t max_buf_sites, buf_sites;
  size_t isite, max_dest_sites;
  size_t nbytes = 0;
  DML_SiteRank rank;
  int this_node = layout->this_node;
  char myname[] = "DML_multifile_out";
  char *lbuf, *buf;
  int *coords;

  /* Allocate buffer for writing */
  max_buf_sites = DML_max_buf_sites(size,1);
  lbuf = DML_allocate_buf(size,max_buf_sites,this_node);
  if(!lbuf)return 0;

  /* Allocate coordinate */
  coords = DML_allocate_coords(layout->latdim,myname,this_node);
  if(!coords)return 0;

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
    get(buf, isite, 1, arg);

    /* Accumulate checksums as the values are inserted into the buffer */
    
    /* Do byte reversal if needed */
#if(BIG_ENDIAN != 1)
    DML_byterevn(buf, size, word_size);
#endif
    
    DML_checksum_accum(checksum, rank, buf, size);
    
    buf_sites = DML_write_buf_next(lrl_record_out, size,
				   lbuf, buf_sites, max_buf_sites, 
				   isite, max_dest_sites, &nbytes,
				   myname, this_node);
    if(buf_sites < 0) return 0;
  } /* isite */

  free(lbuf);   free(coords);
  
  /* Combine checksums over all nodes */
  DML_checksum_combine(checksum);

  /* Return the number of bytes written by this node only */
  return nbytes;

} /* DML_multifile_out */


/* The file is divided up into roughly equal chunks, one per node.
   Each node receives data it needs from other nodes and writes its
   own chunk.  The order of data is lexicographic. */

/* Returns the number of bytes written by all nodes */
int DML_parallel_out(LRL_RecordWriter *lrl_record_out, 
	     void (*get)(char *buf, size_t index, size_t count, void *arg),
	     size_t size, int word_size, void *arg, 
	     DML_Layout *layout, DML_Checksum *checksum){
  
  size_t sites_in_chunk, sites_in_last_chunk, max_sites, max_buf_sites;
  size_t isite, ksite, site_block, max_dest_sites;
  size_t buf_sites;
  DML_SiteRank rank, snd_rank, chk_rank;
  size_t buf_insert;
  size_t sizeof_msg;
  size_t nbytes = 0;
  int this_node = layout->this_node;
  int number_of_nodes = layout->number_of_nodes;
  int destnode, sendnode;
  off_t offset;
  char myname[] = "DML_parallel_out";
  char *lbuf, *buf;
  int *coords;
  char *msg;

  printf("%s(%d) WARNING: THIS CODE HAS NOT BEEN DEBUGGED!\n",
	 myname,this_node);

  /* The number of sites worth of data in the chunk that we write */
  sites_in_chunk = layout->volume/number_of_nodes;

  /* In case we have an inhomogeneous layout, the last node takes up
     the excess or deficit.  */
  sites_in_last_chunk = layout->volume - 
    sites_in_chunk*(layout->number_of_nodes - 1);

  /* Maximum number of sites written by any node */
  max_sites = sites_in_chunk > sites_in_last_chunk ? 
    sites_in_chunk : sites_in_last_chunk;

  /* Byte offset from beginning of the binary record */
  offset = size*sites_in_chunk*this_node;

  /* Position the writer at the first byte we write */
  LRL_seek_write_record(lrl_record_out, offset);

  /* Initialize checksum */
  DML_checksum_init(checksum);

  /* Barrier */
  DML_sync();

#if(BIG_ENDIAN != 1)
  printf("%s(%d): byte reversing %d\n",myname,this_node,word_size);
#endif

  /* Number of sites between barriers to prevent message pileups */
  site_block = DML_COMM_BLOCK;

  /* Allocate buffer for writing */
  /* It should be a multiple of site_block */
  max_buf_sites = DML_max_buf_sites(size,site_block);
  lbuf = DML_allocate_buf(size,max_buf_sites,this_node);
  if(!lbuf)return 0;

  /* Allocate coordinate */
  coords = DML_allocate_coords(layout->latdim, myname, this_node);
  if(!coords)return 0;

  /* Allocate space for message - actually a dynamically allocated
     structure */
  /* "size" bytes for datum followed by "int" bytes for rank */
  msg = DML_allocate_msg(size, myname, this_node);
  if(!msg)return 0;
  sizeof_msg = DML_msg_sizeof(size);

  /* All nodes participate in this distribution sequence:
  
     Cycle through nodes, moving a site_block's worth of values to the
     destnode before proceeding to the next destnode in sequence.
     Data is buffered and written when the buffer is full.
     We don't know if this pattern is generally optimal.  

     It is possible that messages arrive at a node in an order
     different from the order of sending so we include the site
     rank in the message to be sure it goes where it belongs */
  
  /* Clear buffer as a precaution. */
  memset((void *)lbuf, 0, max_buf_sites*size);
  buf_sites = 0;  /* Counts sites in output buffer */
  
  /* Loop over blocks of sites */
  for(ksite=0; ksite < max_sites; ksite += site_block){
    /* destnode is the node receiving the site value */
    for(destnode=0; destnode<number_of_nodes; destnode++){
      
      /* Max number of values the destnode gets altogether */
      max_dest_sites = sites_in_chunk;
      if(destnode == number_of_nodes - 1)
	max_dest_sites = sites_in_last_chunk;

      /* Loop over the site rank for our chunk of sites relative
	 to the start of our chunk */
      for(isite=ksite; 
	  isite<max_dest_sites && isite<ksite+site_block; isite++){
	
	/* The lexicographic rank of the site the destnode gets */
	rank = destnode*sites_in_chunk + isite;
	
	/* The coordinates corresponding to this site */
	DML_lex_coords(coords, layout->latdim, layout->latsize, 
		       rank);
	
	/* The node that has this site.  It must send it to destnode. */
	sendnode = layout->node_number(coords);
	
	/* Node sendnode sends site value to destnode */
	if(this_node==sendnode && destnode!=sendnode){
	  /* Message consists of datum and site rank */
	  get(DML_msg_datum(msg,size),layout->node_index(coords),1,arg);
	  *DML_msg_rank(msg,size) = rank;
	  DML_send_bytes(msg,size,destnode);
	}
	/* Node destnode receives a message */
	else if(this_node==destnode){
	  if(destnode==sendnode){ 
	    /* Just fetch directly to the buffer if data is local */
	    buf_insert = buf_sites;
	    buf = lbuf + size*buf_insert;
	    get(buf, layout->node_index(coords), 1, arg);
	    chk_rank = rank;  /* Remember for checksum calculation */
	  }
	  else {
	    /* Receive a message */
	    DML_get_bytes(DML_msg_datum(msg,size),sizeof_msg,sendnode);
	    /* Extract lexicographic rank from message */
	    snd_rank = *DML_msg_rank(msg,size);
	    
	    /* The buffer location is then */
	    buf_insert = (snd_rank % sites_in_chunk) % max_buf_sites;
	    buf = lbuf + size*buf_insert;

	    /* Move data to buffer */
	    memcpy((void *)buf, (void *)DML_msg_datum(msg,size), size);
	    chk_rank = snd_rank;
	  }
	  
	  /* Accumulate checksums as the values are inserted into the
	     buffer at the receiving node */

	  /* Do byte reversal if needed */
#if(BIG_ENDIAN != 1)
	  DML_byterevn(buf, size, word_size);
#endif
	  DML_checksum_accum(checksum, chk_rank, buf, size);

	  /* Write buffer if full */
	  buf_sites = DML_write_buf_next(lrl_record_out, size,
					 lbuf, buf_sites, max_buf_sites, 
					 isite, max_dest_sites, &nbytes,
					 myname, this_node);
	  if(buf_sites < 0) return 0;
	} /* else if(this_node==destnode) */
      } /* isite */
    } /* destnode */
    /* To prevent message pileups */
    DML_sync();
  } /* ksite */
  
  free(lbuf);   free(coords);    free(msg);

  /* Combine checksums over all nodes */
  DML_checksum_combine(checksum);
  
  /* Return the number of bytes written by all nodes */
  DML_sum_size_t(&nbytes);
  return nbytes;
}

/* Each node reads its data from its own private file.  The order of
   sites is assumed to be sequential according to the layout storage
   order. */

/* Returns the number of bytes read by this node alone */

int DML_multifile_in(LRL_RecordReader *lrl_record_in, 
	     DML_SiteRank sitelist[],
	     void (*put)(char *buf, size_t index, size_t count, void *arg),
	     size_t size, int word_size, void *arg, 
	     DML_Layout *layout, DML_Checksum *checksum){

  size_t buf_sites, buf_extract, max_buf_sites;
  size_t isite, max_send_sites;
  size_t nbytes = 0;
  DML_SiteRank rank;
  int this_node = layout->this_node;
  char myname[] = "DML_multifile_in";
  char *lbuf, *buf;
  int *coords;

  /* Allocate buffer for reading */
  max_buf_sites = DML_max_buf_sites(size,1);
  lbuf = DML_allocate_buf(size,max_buf_sites,this_node);
  if(!lbuf)return 0;

  /* Allocate coordinate */
  coords = DML_allocate_coords(layout->latdim, myname, this_node);
  if(!coords)return 0;

  /* Initialize checksum */
  DML_checksum_init(checksum);

  buf_sites = 0;
  buf_extract = 0;
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
				  myname, this_node);
    if(buf_sites < 0)return 0;
    
    /* Copy data directly from the buffer */
    buf = lbuf + size*buf_extract;

    /* Accumulate checksums as the values are inserted into the buffer */
    DML_checksum_accum(checksum, rank, buf, size);
    
    /* Do byte reversal after checksum if needed */
#if(BIG_ENDIAN != 1)
    DML_byterevn(buf, size, word_size);
#endif
    put(buf, isite, 1, arg);
    
    buf_extract++;
  } /* isite */

  free(lbuf);   free(coords);
  
  /* Combine checksums over all nodes */
  DML_checksum_combine(checksum);

  /* Return the number of bytes read by this node only */
  return nbytes;
}

/* The master node reads data from one file and distributes it to all
   nodes.  It is assumed the order of data is lexicographic. */

/* Returns the number of bytes read */
/* Todo: Should use buffers.  Site iteration can be done over siterank */

int DML_serial_in(LRL_RecordReader *lrl_record_in, 
	  void (*put)(char *buf, size_t index, size_t count, void *arg),
	  size_t size, int word_size, void *arg, DML_Layout* layout,
	  DML_Checksum *checksum){
  char *buf;
  int dest_node;
  DML_SiteRank rcv_rank, rcv_coords;
  size_t nbytes = 0;
  int *coords;
  int this_node = layout->this_node;
  int latdim = layout->latdim;
  int *latsize = layout->latsize;
  size_t volume = layout->volume;
  char myname[] = "DML_serial_in";

  /* Allocate buffer for datum */
  buf = (char *)malloc(size);
  if(buf == NULL)return 0;

  /* Allocate coordinate counter */
  coords = DML_allocate_coords(latdim, myname, this_node);
  if(!coords)return 0;
  
  /* Initialize checksum */
  DML_checksum_init(checksum);

  /* Unlimited barrier */
  DML_sync();

#if(BIG_ENDIAN != 1)
  printf("%s(%d): byte reversing %d\n",myname,this_node,word_size);
#endif

  /* Loop over the coordinate rank */
  for(rcv_rank = 0; rcv_rank < volume; rcv_rank++)
    {
      /* Determine receiving coordinates for the next site datum */
      /* Always use lexicographic order */
      rcv_coords = rcv_rank;
      DML_lex_coords(coords, latdim, latsize, rcv_coords);
      
      /* The node that gets the next datum */
      dest_node = layout->node_number(coords);
     
      if(this_node == DML_MASTER_NODE){
	/* Master node gets the next value */
	if(LRL_read_bytes(lrl_record_in, buf, size) != size)return 0;
	nbytes += size;

	/* If destination elsewhere, send it */
	if(dest_node != DML_MASTER_NODE){
	  DML_send_bytes(buf, size, dest_node);
	}
      }

      /* Other nodes receive from the master node */
      else {
	if(this_node == dest_node){
	  DML_get_bytes(buf, size, DML_MASTER_NODE);
	}
      }
	  
      /* Process data before inserting */
      if(this_node == dest_node){

	/* Accumulate checksum */
	DML_checksum_accum(checksum, rcv_coords, buf, size);

	/* Do byte reversal if necessary */
#if(BIG_ENDIAN != 1)
	DML_byterevn(buf, size, word_size);
#endif
	/* Store the data */
	put(buf,layout->node_index(coords),1,arg);
      }
    }
  /* Combine checksums over all nodes */
  DML_checksum_combine(checksum);

  /* Return the number of bytes read by all nodes */
  DML_sum_size_t(&nbytes);
  return nbytes;
}

/* The file is divided up into roughly equal chunks, one per node.
   Each node reads its own chunk and sends the data to the correct
   node.  It is assumed that the data is in lexicographic order */

/* Returns the number of bytes read by all nodes */

int DML_parallel_in(LRL_RecordReader *lrl_record_in, 
	    void (*put)(char *buf, size_t index, size_t count, void *arg),
	    size_t size, int word_size, void *arg, DML_Layout *layout,
	    DML_Checksum *checksum){

  size_t sites_in_chunk, sites_in_last_chunk, max_sites, max_buf_sites;
  size_t isite, ksite, site_block, max_send_sites;
  DML_SiteRank rank, rcv_rank, chk_rank;
  size_t buf_extract, buf_sites;
  size_t sizeof_msg;
  size_t nbytes = 0;
  int this_node = layout->this_node;
  int number_of_nodes = layout->number_of_nodes;
  int destnode, sendnode;
  off_t offset;
  char myname[] = "DML_parallel_in";
  char *lbuf, *buf;
  int *coords;
  char *msg;

  printf("%s(%d) WARNING: THIS CODE HAS NOT BEEN DEBUGGED!\n",
	 myname,this_node);

  /* The number of sites worth of data in the chunk that we read */
  sites_in_chunk = layout->volume/number_of_nodes;

  /* In case we have an inhomogeneous layout, the last node takes up
     the excess */
  sites_in_last_chunk = layout->volume - 
    sites_in_chunk*(layout->number_of_nodes - 1);

  /* Maximum number of sites read by any node */
  max_sites = sites_in_chunk > sites_in_last_chunk ? 
    sites_in_chunk : sites_in_last_chunk;

  /* Byte offset for this node from beginning of the binary record */
  offset = size*sites_in_chunk*this_node;

  /* Position the reader at the first byte we write */
  LRL_seek_read_record(lrl_record_in, offset);

  /* Initialize checksum */
  DML_checksum_init(checksum);

  /* Barrier */
  DML_sync();

#if(BIG_ENDIAN != 1)
  printf("%s(%d): byte reversing %d\n",myname,this_node,word_size);
#endif

  /* Number of sites between barriers to prevent message pileups */
  site_block = DML_COMM_BLOCK;

  /* Allocate buffer for reading */
  /* It should hold data for a multiple of site_block sites */
  max_buf_sites = DML_max_buf_sites(size,site_block);
  lbuf = DML_allocate_buf(size,max_buf_sites,this_node);
  if(!lbuf)return 0;

  /* Allocate coordinate */
  coords = DML_allocate_coords(layout->latdim, myname, this_node);
  if(!coords)return 0;

  /* Allocate space for message - actually a dynamically allocated
     structure */
  /* "size" bytes for datum followed by "int" bytes for rank */
  msg = DML_allocate_msg(size, myname, this_node);
  if(!msg)return 0;
  sizeof_msg = DML_msg_sizeof(size);

  buf_sites = 0;
  buf_extract = 0;
  
  /* All nodes participate in this distribution sequence:
  
     Cycle through nodes, moving a site_block's worth of values from
     the sendnode before proceeding to the next sendnode in sequence.
     Data is buffered and filled when the buffer is processed.  We
     don't know if this pattern is generally optimal.

     It is possible that messages arrive at a node in an order
     different from the order of sending so we include the site
     rank in the message to be sure it goes where it belongs */

  /* Loop over blocks of sites */
  for(ksite=0; ksite < max_sites; ksite += site_block){
    /* sendnode is the node sending the site value */
    for(sendnode=0; sendnode<number_of_nodes; sendnode++){

      /* Max number of values the sendnode sends altogether */
      max_send_sites = sites_in_chunk;
      if(destnode == number_of_nodes - 1)
	max_send_sites = sites_in_last_chunk;

      /* Loop over the site rank for our chunk of sites relative
	 to the start of our chunk */
      for(isite=ksite; 
	  isite<max_send_sites && isite<ksite+site_block; isite++){
	
	/* The lexicographic rank of the site the sendnode sends */
	rank = sendnode*sites_in_chunk + isite;
	
	/* The coordinates corresponding to this site */
	DML_lex_coords(coords, layout->latdim, layout->latsize, 
		       rank);
	
	/* The node that receives this site from sendnode. */
	destnode = layout->node_number(coords);
	  
	/* Node sendnode reads, and sends site to destnode */
	  if(this_node==sendnode){
	    buf_sites = DML_read_buf_next(lrl_record_in, size,
					  lbuf, &buf_extract, buf_sites,
					  max_buf_sites, isite, 
					  max_send_sites, &nbytes,
					  myname, this_node);
	    if(buf_sites < 0)return 0;
	    
	    /* Sending node does byte reversal and accumulates checksums
	       as the values are sent from its buffer */
	    buf = lbuf + size*buf_extract;
	    DML_checksum_accum(checksum, rank, buf, size);
#if(BIG_ENDIAN != 1)
	    DML_byterevn(buf, size, word_size);
#endif
	    if(destnode==sendnode){
	      /* Just copy directly from the buffer if data is local */
	      put(buf, layout->node_index(coords), 1, arg);
	    }
	    else {
	      /* Send to destnode */
	      /* Message consists of datum and site rank */
	      memcpy((void *)DML_msg_datum(msg,size), (void *)buf, size);
	      *DML_msg_rank(msg,size) = rank;
	      DML_send_bytes(DML_msg_datum(msg,size),sizeof_msg,destnode);
	    }
	    buf_extract++;
	  }
	  /* The node which contains this site reads a message */
	  else {	/* for all nodes other than node sendnode */
	    if(this_node==destnode){
	      DML_get_bytes(DML_msg_datum(msg,size), sizeof_msg, sendnode);
	      rcv_rank = *DML_msg_rank(msg,size);
	      /* The coordinates corresponding to this site */
	      DML_lex_coords(coords, layout->latdim, layout->latsize, 
			     rcv_rank);
	      /* Store the data */
	      put(DML_msg_datum(msg,size), layout->node_index(coords), 1, arg);
	    }
	  }
	} /* sites in site_block */
    } /* sendnodes */

    /* To prevent message pileups */
    DML_sync();
  }  /** end over blocks **/

  free(lbuf);   free(coords);    free(msg);

  /* Combine checksums over all nodes */
  DML_checksum_combine(checksum);

  /* Return the number of bytes read by all nodes */
  DML_sum_size_t(&nbytes);
  return nbytes;
}
