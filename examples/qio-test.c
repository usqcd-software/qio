/* Test of QIO with data of type "real" */

/* C. DeTar */
/* August 10, 2004 */

#include <stdio.h>
#define QDP_Precision 'F'
#define QDP_Nc 3
#include <qdp.h>
#include <qdpio.h>
#include <qdp_string.h>
#include <qla.h>

int lattice_dim = 4;
int lattice_size[4] = { 4,4,4,4 };

/* function used for setting field */
void fill_real(QLA_Real *r, int coords[]){
  /* Set value equal to something */
  *r = coords[0] + lattice_size[0]*(coords[1] + lattice_size[1]*
				   (coords[2] + lattice_size[2]*coords[3]));
}

void set_real(QDP_Real *field){
  /* set values through a function */
  QDP_R_eq_func(field, fill_real, QDP_all);
}

int main(int argc, char *argv[]){

  QDP_Real *field_in, *field_out;
  QDP_String *xml_file_out, *xml_record_out, *xml_file_in, *xml_record_in;
  QDP_Writer *outfile;
  QDP_Reader *infile;
  QLA_Real diff;
  int status;
  char filename[] = "binary_real";

  /* Dummy XML strings */
  char xml_write_file[] = "Dummy user file XML";
  char xml_write_record[] = "Dummy user record XML";
  
  /* Initialize QDP */
  QDP_initialize(argc,argv);
  printf("QDP initialized\n");
  
  /* set lattice size and create layout */
  QDP_set_latsize(4, lattice_size);
  QDP_create_layout();
  printf("QDP layout created\n");

  /* Create an output QDP field */
  field_out = QDP_create_R();
  printf("QDP field created\n");

  /* Set some values */
  set_real(field_out);

  /* Create the file XML */
  xml_file_out = QDP_string_create();
  QDP_string_set(xml_file_out,xml_write_file);
  printf("File XML set\n");
  
  /* Create the record XML */
  xml_record_out = QDP_string_create();
  QDP_string_set(xml_record_out,xml_write_record);
  printf("Record XML set\n");

  /* Write the field to a file */
  outfile = QDP_open_write(xml_file_out, filename, QDP_SINGLEFILE, 0);
  printf("Opened file for writing\n");
  status = QDP_write_R(outfile, xml_record_out, field_out);
  printf("Wrote record with status %d\n",status);
  QDP_close_write(outfile);

  /* Set up a dummy input QDP field */
  field_in = QDP_create_R();

  /* Create the file XML */
  xml_file_in = QDP_string_create();

  /* Create the record XML */
  xml_record_in = QDP_string_create();

  /* Open the file for reading */
  infile = QDP_open_read(xml_file_in, filename, 0);
  if(infile == NULL){
    printf("Quitting\n");
    return 1;
  }
  printf("File opened for reading\n");

  /* Peek at the record */
  QDP_read_record_info(infile, xml_record_in);
  printf("Done peeking\n");

  /* Skip the record */
#if(0)
  QDP_next_record(infile);
#endif

  /* Read the record */
  status = QDP_read_R(infile, xml_record_in, field_in);
  printf("QDP_read_R returns status %d\n",status);
  QDP_close_read(infile);

  /* Compare the input and output fields */
  QDP_R_meq_R(field_out, field_in, QDP_all);
  QDP_r_eq_norm2_R(&diff, field_out, QDP_all);
  if(QDP_this_node == 0)
    printf("Comparision of in and out fields |in - out|^2 = %e\n",diff);

  /* free fields */
  QDP_destroy_R(field_in);
  QDP_destroy_R(field_out);
  QDP_string_destroy(xml_file_out);
  QDP_string_destroy(xml_record_out);
  QDP_string_destroy(xml_file_in);
  QDP_string_destroy(xml_record_in);

  /* cleanup */
  /* shutdown QDP */
  QDP_finalize();
  return 0;
}
