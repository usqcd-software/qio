/* Test of QIO with data of type global "real" array */
 
/* C. DeTar */
/* August 10, 2004 */

#include <stdio.h>
#define QDP_Precision 'F'
#define QDP_Nc 3
#include <qdp.h>
#include <qdpio.h>
#include <qdp_string.h>
#include <qla.h>
#define NARRAY 3

int lattice_dim = 4;
int lattice_size[4] = { 4,4,4,4 };

int main(int argc, char *argv[]){

  QLA_Real array_in[NARRAY], array_out[NARRAY];
  QDP_String *xml_file_out, *xml_record_out, *xml_file_in, *xml_record_in;
  QDP_Writer *outfile;
  QDP_Reader *infile;
  QLA_Real diff;
  QLA_Real fact = 2.;
  int status,i;
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

  /* Build an output QLA array */
  for(i = 0; i < NARRAY; i++)
    array_out[i] = i;

  /* Create the file XML */
  xml_file_out = QDP_string_create();
  QDP_string_set(xml_file_out,xml_write_file);
  printf("File XML set\n");
  
  /* Create the record XML */
  xml_record_out = QDP_string_create();
  QDP_string_set(xml_record_out,xml_write_record);
  printf("Record XML set\n");

  /* Write the array to a file */
  outfile = QDP_open_write(xml_file_out, filename, QDP_SINGLEFILE, 0);
  printf("Opened file for writing\n");
  status = QDP_F_vwrite_r(outfile, xml_record_out, array_out, NARRAY);
  printf("QDP_F_vwrite_r returns status %d\n",status);
  QDP_close_write(outfile);
  printf("Closed file\n");

  /* Create the file XML */
  xml_file_in = QDP_string_create();

  /* Create the record XML */
  xml_record_in = QDP_string_create();

  /* Open the file for reading */
  infile = QDP_open_read(xml_file_in, filename, 0);

  /* Read the record */
  status = QDP_F_vread_r(infile, xml_record_in, array_in, NARRAY);
  printf("QDP_F_vread_r returns status %d\n",status);
  QDP_close_read(infile);

  /* Compare the input and output arrays */
  diff = 0;
  for(i = 0; i < NARRAY; i++){
    array_out[i] = array_out[i] - array_in[i];
    diff += array_out[i]*array_out[i];
    printf("node(%d)Comparison of %d in and out arrays |in - out|^2 = %e\n",
	   QDP_this_node,i,diff);
  }

  QDP_string_destroy(xml_file_out);
  QDP_string_destroy(xml_record_out);
  QDP_string_destroy(xml_file_in);
  QDP_string_destroy(xml_record_in);

  /* cleanup */
  /* shutdown QDP */
  QDP_finalize();
  return 0;
}
