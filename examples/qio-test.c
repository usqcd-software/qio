/* Test QIO with specified volume format and real (float) fields */
/* Writes a field of values for an 8x4^3 lattice, then a global array,
   closes the file, then reads the field data, peeks at the record
   info, then reads it and compares, then reads the global array and
   compares. */

/* C. DeTar */
/* October 18, 2004 */

#include <stdio.h>
#include <qio.h>
#define MAIN
#include "qio-test.h"

#define NREAL 2
#define NARRAY 3

/* Open a QIO file for writing */

QIO_Writer *open_test_output(char *filename, int volfmt, char *myname){
  QIO_String *xml_file_out;
  char xml_write_file[] = "Dummy user file XML";
  QIO_Writer *outfile;

  /** QIO_verbose(QIO_VERB_DEBUG); **/

  /* Create the file XML */
  xml_file_out = QIO_string_create();
  QIO_string_set(xml_file_out,xml_write_file);

  /* Open the file for writing */
  outfile = QIO_open_write(xml_file_out, filename, volfmt, &layout, 0);
  if(outfile == NULL){
    printf("%s(%d): QIO_open_write returned NULL\n",myname,this_node);
    return NULL;
  }
  QIO_string_destroy(xml_file_out);
  return outfile;
}

int write_real_field(QIO_Writer *outfile, int count, 
		     float *field_out[], char *myname){
  QIO_String *xml_record_out;
  char xml_write_field[] = "Dummy user record XML for field";
  int status;
  QIO_RecordInfo *rec_info;

  /* Create the record info for the field */
  rec_info = QIO_create_record_info(QIO_FIELD, "QDP_F_Real", "F", 0,
				    0, sizeof(float), count);
  /* Create the record XML for the field */
  xml_record_out = QIO_string_create();
  QIO_string_set(xml_record_out,xml_write_field);

  /* Write the record for the field */
  status = QIO_write(outfile, rec_info, xml_record_out, vget_R, 
		     count*sizeof(float), sizeof(float), field_out);
  printf("%s(%d): QIO_write returns status %d\n",myname,this_node,status);
  if(status != QIO_SUCCESS)return 1;

  QIO_destroy_record_info(rec_info);
  QIO_string_destroy(xml_record_out);

  return 0;
}

int write_real_global(QIO_Writer *outfile, int count, float array_out[], 
		      char *myname){
  QIO_String *xml_record_out;
  char xml_write_global[] = "Dummy user record XML for global";
  int status;
  QIO_RecordInfo *rec_info;

  /* Create the record info for the global array */
  xml_record_out = QIO_string_create();
  QIO_string_set(xml_record_out,xml_write_global);
  rec_info = QIO_create_record_info(QIO_GLOBAL, "QLA_F_Real", "F", 0,
				    0, sizeof(float), count);
  /* Write the array to a file */
  status = QIO_write(outfile, rec_info, xml_record_out, vget_r, 
		     count*sizeof(float), sizeof(float), array_out);
  printf("%s(%d): QIO_write returns status %d\n",myname,this_node,status);
  if(status != QIO_SUCCESS)return 1;
  
  QIO_destroy_record_info(rec_info);
  QIO_string_destroy(xml_record_out);

  return 0;
}

QIO_Reader *open_test_input(char *filename, char *myname){
  QIO_String *xml_file_in;
  QIO_Reader *infile;

  /* Create the file XML */
  xml_file_in = QIO_string_create();

  /* Open the file for reading */
  infile = QIO_open_read(xml_file_in, filename, &layout, 0);
  if(infile == NULL){
    printf("%s(%d): QIO_open_read returns NULL.\n",myname,this_node);
    return NULL;
  }

  printf("%s(%d): QIO_open_read done.\n",myname,this_node);
  printf("%s(%d): User file info is \"%s\"\n",myname,this_node,
	 QIO_string_ptr(xml_file_in));

  QIO_string_destroy(xml_file_in);
  return infile;
}

int peek_record_info(QIO_Reader *infile, char *myname){
  QIO_String *xml_record_in;
  QIO_RecordInfo rec_info;
  int status;

  /* Create the record XML */
  xml_record_in = QIO_string_create();
  /* Create placeholder for record_info */

  status = QIO_read_record_info(infile, &rec_info, xml_record_in);
  if(status != QIO_SUCCESS)return 1;
  
  printf("%s(%d): User record info is \"%s\"\n",myname,this_node,
	 QIO_string_ptr(xml_record_in));

  QIO_string_destroy(xml_record_in);
  return 0;
}


int read_real_field(QIO_Reader *infile, int count, 
		    float *field_in[], char *myname)
{
  int status;
  
  /* Read the field record */
  status = QIO_read_record_data(infile, vput_R, sizeof(float)*count, 
				sizeof(float), field_in);
  printf("%s(%d): QIO_read_record_data returns status %d\n",
	 myname,this_node,status);
  if(status != QIO_SUCCESS)return 1;
  return 0;
}

int read_real_global(QIO_Reader *infile, int count, 
		    float array_in[], char *myname)
{
  QIO_String *xml_record_in;
  QIO_RecordInfo rec_info;
  int status;

  /* Create the record XML */
  xml_record_in = QIO_string_create();

  /* Read the global array record */
  status = QIO_read(infile, &rec_info, xml_record_in, 
		    vput_r, sizeof(float)*count, sizeof(float), array_in);
  printf("%s(%d): QIO_read returns status %d\n",
	 myname,this_node,status);
  if(status != QIO_SUCCESS)return 1;
  printf("%s(%d): User record info is \"%s\"\n",myname,this_node,
	 QIO_string_ptr(xml_record_in));

  QIO_string_destroy(xml_record_in);
  return 0;
}


int qio_test(int volfmt, int argc, char *argv[]){

  float array_in[NARRAY], array_out[NARRAY];
  float *field_in[NREAL], *field_out[NREAL];
  QIO_Writer *outfile;
  QIO_Reader *infile;
  float diff_field, diff_array;
  QMP_thread_level_t provided;
  int status;
  int i,volume;
  char filename[] = "binary_real";
  char myname[] = "qio_test";
  
  /* Start message passing */
  QMP_init_msg_passing(&argc, &argv, QMP_THREAD_SINGLE, &provided);

  this_node = mynode();
  printf("%s(%d) QMP_init_msg_passing done\n",myname,this_node);

  /* Lattice dimensions */
  lattice_dim = 4;
  lattice_size[0] = 8;
  lattice_size[1] = 4;
  lattice_size[2] = 4;
  lattice_size[3] = 4;

  volume = 1;
  for(i = 0; i < lattice_dim; i++){
    volume *= lattice_size[i];
  }

  /* Set the mapping of coordinates to nodes */
  setup_layout(lattice_size, 4, QMP_get_number_of_nodes());
  printf("%s(%d) layout set for %d nodes\n",myname,this_node,
	 QMP_get_number_of_nodes());

  /* Build the layout structure */
  layout.node_number     = node_number;
  layout.node_index      = node_index;
  layout.get_coords      = get_coords;
  layout.latsize         = lattice_size;
  layout.latdim          = lattice_dim;
  layout.volume          = volume;
  layout.sites_on_node   = sites_on_node;
  layout.this_node       = this_node;
  layout.number_of_nodes = QMP_get_number_of_nodes();

  /* Open the test output file */
  outfile = open_test_output(filename, volfmt, myname);
  if(outfile == NULL)return 1;

  /* Create the test output field */
  status = vcreate_R(field_out, NREAL);
  if(status)return status;

  /* Set some values for the field */
  vset_R(field_out,NREAL);
  if(status)return status;

  /* Write the real test field */
  status = write_real_field(outfile, NREAL, field_out, myname);
  if(status)return status;

  /* Set some values for the global array */
  for(i = 0; i < NARRAY; i++)
    array_out[i] = i;

  /* Write the real global array */
  status = write_real_global(outfile, NARRAY, array_out, myname);
  if(status)return status;

  /* Close the file */
  QIO_close_write(outfile);
  printf("%s(%d): Closed file for writing\n",myname,this_node);

  /* Set up a dummy input field */
  status = vcreate_R(field_in, NREAL);
  if(status)return status;

  /* Open the test file for reading */
  infile = open_test_input(filename, myname);

  /* Peek at the field record */
  peek_record_info(infile, myname);
  /* Skip the record */

#if(0)

  /* Skip the field */
  status = QIO_next_record(infile);
  if(status != QIO_SUCCESS)return status;

#else

  /* Read the field record */
  status = read_real_field(infile, NREAL, field_in, myname);
  if(status)return status;

#endif

  /* Read the global array record */
  status = read_real_global(infile, NARRAY, array_in, myname);
  if(status)return status;

  /* Close the file */
  QIO_close_read(infile);
  printf("%s(%d): Closed file for reading\n",myname,this_node);

  /* Compare the input and output fields */
  diff_field = vcompare_R(field_out, field_in, sites_on_node, NREAL);
  if(this_node == 0){
    printf("%s(%d): Comparison of in and out fields |in - out|^2 = %e\n",
	   myname,this_node,diff_field);
  }

  /* Compare the input and output global arrays */
  diff_array = vcompare_r(array_out, array_in, NREAL);
  if(this_node == 0){
    printf("%s(%d): Comparison of in and out arrays |in - out|^2 = %e\n",
	   myname, this_node, diff_array);
  }

  /* Clean up */
  vdestroy_R(field_out, NREAL);
  vdestroy_R(field_in, NREAL);

  /* Shut down QMP */
  QMP_finalize_msg_passing();

  /* Report result */
  if(diff_field + diff_array > 0){
    printf("%s(%d): Test failed\n",myname,this_node);
    return 1;
  }
  printf("%s(%d): Test passed\n",myname,this_node);

  return 0;
}
