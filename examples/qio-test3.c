/* Test of QIO.  Attempt to read a complex field from a specified,
   previously generated file */
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
void fill_complex(QLA_Complex *r, int coords[]){
  /* Set value equal to something */
  r->real = 2*(coords[0] + lattice_size[0]*(coords[1] + lattice_size[1]*
				   (coords[2] + lattice_size[2]*coords[3])));
  r->imag = 1 + 2*(coords[0] + lattice_size[0]*(coords[1] + lattice_size[1]*
				   (coords[2] + lattice_size[2]*coords[3])));
}

void set_complex(QDP_Complex *field){
  /* set values through a function */
  QDP_C_eq_func(field, fill_complex, QDP_all);
}

int main(int argc, char *argv[]){

  QDP_Complex *field_in, *field_out;
  QDP_String *xml_file_in, *xml_record_in;
  QDP_Reader *infile;
  QLA_Real diff;
  int i,spacetime;
  int status;
  char filename[128];

  printf("Name of input file ... ");
  scanf("%s",filename);
  printf("Spacetime dimension ... ");
  scanf("%d",&spacetime);
  printf("Coordinate dimensions ...");
  for(i = 0; i < spacetime; i++)
    scanf("%d",&lattice_size[i]);

  /* Initialize QDP */
  QDP_initialize(argc,argv);
  printf("QDP initialized\n");
  
  /* set lattice size and create layout */
  QDP_set_latsize(spacetime, lattice_size);
  QDP_create_layout();
  printf("QDP layout created\n");

  /* Set up a dummy input QDP field */
  field_in = QDP_create_C();

  /* Create the file XML */
  xml_file_in = QDP_string_create();

  /* Create the record XML */
  xml_record_in = QDP_string_create();

  /* Open the file for reading */
  infile = QDP_open_read(xml_file_in, filename, 0);

  /* Peek at the record */
  QDP_read_record_info(infile, xml_record_in);

  /* Read the record */
  status = QDP_read_C(infile, xml_record_in, field_in);
  printf("QDP_read_C returns status %d\n",status);
  QDP_close_read(infile);

  /* free fields */
  QDP_destroy_C(field_in);
  QDP_string_destroy(xml_file_in);
  QDP_string_destroy(xml_record_in);

  /* cleanup */
  /* shutdown QDP */
  QDP_finalize();
  return 0;
}
