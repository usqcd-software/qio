/* Read and write private record, file, and checksum XML for SciDAC
   binary file format */

#include <stdio.h>
#include <string.h>
#include <qioxml.h>
#include <xml_string.h>
#include <type32.h>
#include <sys/types.h>
#include <time.h>

/* Same as strpbrk: Find next occurrence of any character in tokens */
char *QIO_next_token(char *parse_pt, char *tokens){
  char *t;
  int found;
  for(; *parse_pt != '\0'; parse_pt++){
    found = 0;
    for(t = tokens; *t != '\0'; t++)if(*parse_pt == *t){
      found++;
      break;
    }
    if(found)break;
  }
  return parse_pt;
}

/* Opposite of strpbrk: Find next occurrence of any character not in tokens */
char *QIO_next_nontoken(char *parse_pt, char *tokens){
  char *t;
  int found;
  for(; *parse_pt != '\0'; parse_pt++){
    found = 0;
    for(t = tokens; *t != '\0'; t++)if(*parse_pt == *t){
      found++;
      break;
    }
    if(!found)break;
  }
  return parse_pt;
}

/* Almost same as strncat, but exits if n < 0 and decrements n
   for a successful copy */
char *QIO_strncat(char *s1, char *s2, int *n){
  char *c;
  if(*n < 0)return s1;
  c = strncat(s1,s2,*n);
  *n -= strlen(s2);
  if(*n < 0)*n = 0;
  return c;
}

/* Find the next tag */
/* On return left_angle points to open "<" and return value
   to end ">" or to end of string, whichever comes first */
char *QIO_next_tag(char *parse_pt, char *tag, char **left_angle){
  char *begin_tag;
  int n;

  /* Initially results are null strings */
  tag[0] = '\0';

  /* Look for next "<" */
  parse_pt = QIO_next_token(parse_pt, "<");

  /* Mark left angle or '\0' */
  *left_angle = parse_pt;

  /* Exit if end of string was reached before finding < */
  if(!*parse_pt)return parse_pt;

  /* Move past '<' */
  parse_pt++;

  /* Tag starts at first nonwhite */
  parse_pt = QIO_next_nontoken(parse_pt, " \t\n\r");

  begin_tag = parse_pt;

  /* Exit if end of string was reached before finding start of tag */
  if(!*parse_pt)return parse_pt;

  /* Tag ends at next white or > */
  parse_pt = QIO_next_token(parse_pt, " >\t\n\r");

  /* Count characters in tag */
  n = parse_pt - begin_tag;
  
  /* Truncate if oversized */
  if(n > QIO_MAXTAG-1){
    n = QIO_MAXTAG-1;
    printf("QIO_next_tag: tag too long - truncated\n");
  }

  /* Copy tag and terminate with null */
  strncpy(tag, begin_tag, n);
  tag[n] = '\0';

  /* Scan to closing '>' */
  parse_pt = QIO_next_token(parse_pt, ">");

  if(!*parse_pt)return parse_pt;
  
  /* Move past '>' */
  parse_pt++;

  return parse_pt;
}

char *QIO_get_tag_value(char *parse_pt, char *tag, char *value_string){
  char tmptag[QIO_MAXTAG];

  char *tag_ptr = tag;
  char *value_string_ptr = value_string;
  char *begin_tag_ptr;
  char *begin_value_string;
  int end_tag = 0;
  int i,n;

  /* Initially results are null strings */
  *tag = '\0';
  *value_string = '\0';

  /* Get tag */

  parse_pt = QIO_next_tag(parse_pt, tag, &begin_tag_ptr);
  if(!*parse_pt)return parse_pt;

  /* Parse value */

  /* Value starts at first nonwhite */
  parse_pt = QIO_next_nontoken(parse_pt, " \t\n\r");

  /* Exit if end of string was reached before finding beginning of value */
  if(!*parse_pt)return parse_pt;

  /* Mark beginning of value */
  begin_value_string = parse_pt;

  /* Look for matching end tag */
  while(!end_tag){
    parse_pt = QIO_next_tag(parse_pt, tmptag, &begin_tag_ptr);
    /* Stop when matching tag or end of string found */
    end_tag = !*parse_pt || (tmptag[0] == '/' && strcmp(tmptag+1,tag)==0);
  }

  /* Copy value as string */

  n = begin_tag_ptr - begin_value_string;
  if(n > QIO_MAXVALUESTRING-1){
    n = QIO_MAXVALUESTRING - 1;
    printf("QIO_get_tag_value: string truncated");
  }
  strncpy(value_string, begin_value_string, n );
  /* Terminate the string */
  value_string[n] = '\0';

  /* Trim trailing white space from value string */
  for(i = n-1; i >= 0; i--){
    if(value_string[i] != ' ' 
       && value_string[i] != '\t'
       && value_string[i] != '\n'
       && value_string[i] != '\r' )break;
    value_string[i] = '\0';
  }
  return parse_pt;
}

void QIO_decode_as_string(char *tag, char *value_string, 
			    QIO_TagCharValue *tag_value){
  int n = strlen(value_string);

  if(strcmp(tag,tag_value->tag)==0){
    if(n > QIO_MAXVALUESTRING-1){
      n = QIO_MAXVALUESTRING - 1;
      printf("QIO_decode_as_string: string truncated");
    }
    strncpy(tag_value->value,value_string,QIO_MAXVALUESTRING);
    tag_value->value[QIO_MAXVALUESTRING-1] = '\0';  /* null termination */
    tag_value->occur++;
  }
}

void QIO_decode_as_int(char *tag, char *value_string, 
			 QIO_TagIntValue *tag_value){
  if(strcmp(tag,tag_value->tag)==0){
    tag_value->value = atoi(value_string);
    tag_value->occur++;
  }
}

void QIO_decode_as_hex32(char *tag, char *value_string, 
			    QIO_TagHex32Value *tag_value){
  if(strcmp(tag,tag_value->tag)==0){
    if(sscanf(value_string,"%x",&tag_value->value)==1)tag_value->occur++;
  }
}

void QIO_decode_as_intlist(char *tag, char *value_string, 
			     QIO_TagIntArrayValue *tag_value){
  int i;
  char *s;
  
  if(strcmp(tag,tag_value->tag)==0){
    for(s = strtok(value_string," "), i = 0; 
	s && *s && i < QIO_MAXINTARRAY; 
	s = strtok('\0'," "),i++)
      tag_value->value[i] = atoi(s);
    
    tag_value->n = i;

    /* Trouble if we didn't hit the end of the string and the array is full */
    if(s && i == QIO_MAXINTARRAY){
      printf("QIO_decode_as_intlist: exceeded internal array dimensions %d\n", QIO_MAXINTARRAY);
    }
    else if(tag_value->n > 0)tag_value->occur++;
  }
}

char *QIO_write_tag(char *buf,char *tag,int *remainder){
  QIO_strncat(buf,"<",remainder);
  QIO_strncat(buf,tag,remainder);
  QIO_strncat(buf,">",remainder);
  return strchr(buf,'\0');
}

char *QIO_write_endtag(char *buf,char *tag,int *remainder){
  QIO_strncat(buf,"</",remainder);
  QIO_strncat(buf,tag,remainder);
  QIO_strncat(buf,">",remainder);
  return strchr(buf,'\0');
}

char *QIO_encode_as_string(char *buf, QIO_TagCharValue *tag_value,
			     int *remainder){

  /* Don't write value unless occurs */
  if(!tag_value->occur)return buf;
  buf = QIO_write_tag(buf, tag_value->tag, remainder);
  snprintf(buf,*remainder,"%s",tag_value->value);
  *remainder -= strlen(tag_value->value);
  if(*remainder <= 0){
    printf("QIO_encode_as_string: Buffer overflow\n");
    return buf;
  }
  buf = QIO_write_endtag(buf, tag_value->tag, remainder);
  return buf;
}

#define QIO_MAXINTSTRING 16
char *QIO_encode_as_int(char *buf, QIO_TagIntValue *tag_value,
			  int *remainder){
  char int_string[QIO_MAXINTSTRING];

  /* Don't write value unless occurs */
  if(!tag_value->occur)return buf;
  buf = QIO_write_tag(buf, tag_value->tag,remainder);
  snprintf(int_string,QIO_MAXINTSTRING,"%d",tag_value->value);
  *remainder -= strlen(int_string);
  if(*remainder <= 0){
    printf("QIO_encode_as_int: Buffer overflow\n");
    return buf;
  }
  buf = strcat(buf, int_string);
  buf = QIO_write_endtag(buf, tag_value->tag, remainder);
  return buf;
}

char *QIO_encode_as_hex32(char *buf, QIO_TagHex32Value *tag_value,
			  int *remainder){
  char int_string[QIO_MAXINTSTRING];

  /* Don't write value unless occurs */
  if(!tag_value->occur)return buf;
  buf = QIO_write_tag(buf, tag_value->tag,remainder);
  snprintf(int_string,QIO_MAXINTSTRING,"%x",tag_value->value);
  *remainder -= strlen(int_string);
  if(*remainder <= 0){
    printf("QIO_encode_as_hex32: Buffer overflow\n");
    return buf;
  }
  buf = strcat(buf, int_string);
  buf = QIO_write_endtag(buf, tag_value->tag, remainder);
  return buf;
}

char *QIO_encode_as_intlist(char *buf, 
			      QIO_TagIntArrayValue *tag_value, 
			      int n, int *remainder){
  int i;
  char int_string[QIO_MAXINTSTRING];
  
  /* Don't write value unless occurs */
  if(!tag_value->occur)return buf;
  buf = QIO_write_tag(buf, tag_value->tag, remainder);
  if(*remainder <= 0){
    printf("QIO_encode_as_intlist: Buffer overflow\n");
    return buf;
  }
  
  for(i = 0; i < n ; i++){
    snprintf(int_string, QIO_MAXINTSTRING, "%d ", tag_value->value[i]);
    *remainder -= strlen(int_string);
    if(*remainder <= 0){
      printf("QIO_encode_as_intlist: Buffer overflow\n");
      return buf;
    }
    buf = strcat(buf, int_string);
  }
  buf = QIO_write_endtag(buf, tag_value->tag, remainder);
  return buf;
}

int QIO_check_string_occur(QIO_TagCharValue *tag_value){
  if(tag_value->occur != 1){
    printf("QIO_check_string_occur: error %s tag count %d\n",
	   tag_value->tag, tag_value->occur);
    return 1;
  }
  return 0;
}

int QIO_check_int_occur(QIO_TagIntValue *tag_value){
  if(tag_value->occur != 1){
    printf("QIO_check_int_occur: error %s tag count %d\n",
	   tag_value->tag, tag_value->occur);
    return 1;
  }
  return 0;
}

int QIO_check_intarray_occur(QIO_TagIntArrayValue *tag_value){
  if(tag_value->occur != 1){
    printf("QIO_check_intarray_occur: error %s tag count %d\n",
	   tag_value->tag, tag_value->occur);
    return 1;
  }
  return 0;
}

int QIO_check_hex32_occur(QIO_TagHex32Value *tag_value){
  if(tag_value->occur != 1){
    printf("QIO_check_hex32_occur: error %s tag count %d\n",
	   tag_value->tag, tag_value->occur);
    return 1;
  }
  return 0;
}

int QIO_decode_record_info(QIO_RecordInfo *record_info, 
			XML_String *record_string){
  char *parse_pt = XML_string_ptr(record_string);
  char tag[QIO_MAXTAG];
  char value_string[QIO_MAXVALUESTRING];
  int errors = 0;
  QIO_RecordInfo template = QIO_RECORD_INFO_TEMPLATE;
  
  /* Initialize from template */
  *record_info = template;

  /* Scan string until null character is reached */
  while(*parse_pt){
    parse_pt = QIO_get_tag_value(parse_pt, tag, value_string);
    
    QIO_decode_as_string(tag,value_string,&record_info->version);
    QIO_decode_as_string(tag,value_string,&record_info->date);
    QIO_decode_as_string(tag,value_string,&record_info->datatype);
    QIO_decode_as_string(tag,value_string,&record_info->precision);
    QIO_decode_as_int   (tag,value_string,&record_info->colors);
    QIO_decode_as_int   (tag,value_string,&record_info->spins);
    QIO_decode_as_int   (tag,value_string,&record_info->typesize);
    QIO_decode_as_int   (tag,value_string,&record_info->datacount);
  }

  /* Check for completeness */
  
  errors += QIO_check_string_occur(&record_info->version);
  errors += QIO_check_string_occur(&record_info->datatype);
  errors += QIO_check_string_occur(&record_info->precision);
  errors += QIO_check_int_occur   (&record_info->typesize);
  errors += QIO_check_int_occur   (&record_info->datacount);

  return errors;
}

void QIO_encode_record_info(XML_String *record_string, 
			  QIO_RecordInfo *record_info){
  char *buf;
  int remainder;
  XML_string_realloc(record_string, QIO_STRINGALLOC);
  buf  = XML_string_ptr(record_string);
  remainder = XML_string_bytes(record_string);
  
  /* Initialize string: encoding is done by appending */
  *buf = '\0';
  buf = QIO_encode_as_string(buf,&record_info->version, &remainder);
  buf = QIO_encode_as_string(buf,&record_info->date, &remainder);
  buf = QIO_encode_as_string(buf,&record_info->datatype, &remainder);
  buf = QIO_encode_as_string(buf,&record_info->precision, &remainder);
  buf = QIO_encode_as_int   (buf,&record_info->colors, &remainder);
  buf = QIO_encode_as_int   (buf,&record_info->spins, &remainder);
  buf = QIO_encode_as_int   (buf,&record_info->typesize, &remainder);
  buf = QIO_encode_as_int   (buf,&record_info->datacount, &remainder);
}


int QIO_decode_file_info(QIO_FileInfo *file_info, 
			  XML_String *file_string){
  char *parse_pt = XML_string_ptr(file_string);
  char tag[QIO_MAXTAG];
  char value_string[QIO_MAXVALUESTRING];
  int errors = 0;
  QIO_FileInfo template = QIO_FILE_INFO_TEMPLATE;
  
  /* Initialize from template */
  *file_info = template;

  /* Scan string until null character is reached */
  while(*parse_pt){
    parse_pt = QIO_get_tag_value(parse_pt, tag, value_string);
    
    QIO_decode_as_string (tag,value_string,&file_info->version);
    QIO_decode_as_int    (tag,value_string,&file_info->spacetime);
    QIO_decode_as_intlist(tag,value_string,&file_info->dims);
    QIO_decode_as_int    (tag,value_string,&file_info->multifile);
  }

  /* Check for completeness */
  
  errors += QIO_check_string_occur  (&file_info->version);
  errors += QIO_check_int_occur     (&file_info->spacetime);
  errors += QIO_check_intarray_occur(&file_info->dims);
  errors += QIO_check_int_occur     (&file_info->multifile);

  /* Did we get all the spacetime dimensions */
  if(file_info->spacetime.value != file_info->dims.n){
    printf("QIO_decode_file_info: mismatch in spacetime dimensions\n");
    errors++;
  }

  return errors;
}

void QIO_encode_file_info(XML_String *file_string, 
			     QIO_FileInfo *file_info){
  
  char *buf;
  int remainder;
  XML_string_realloc(file_string, QIO_STRINGALLOC);
  buf  = XML_string_ptr(file_string);
  remainder = XML_string_bytes(file_string);

  /* Initialize string: encoding is done by appending */
  *buf = '\0';
  buf = QIO_encode_as_string (buf,&file_info->version, &remainder);
  buf = QIO_encode_as_int    (buf,&file_info->spacetime, &remainder);
  buf = QIO_encode_as_intlist(buf,&file_info->dims, 
			      file_info->spacetime.value, &remainder);
  buf = QIO_encode_as_int    (buf,&file_info->multifile, &remainder);
}

int QIO_decode_checksum_info(QIO_ChecksumInfo *checksum, 
			     XML_String *file_string){
  char *parse_pt = XML_string_ptr(file_string);
  char tag[QIO_MAXTAG];
  char value_string[QIO_MAXVALUESTRING];
  int errors = 0;
  QIO_ChecksumInfo template = QIO_CHECKSUM_INFO_TEMPLATE;
  
  /* Initialize from template */
  *checksum = template;

  /* Scan string until null character is reached */
  while(*parse_pt){
    parse_pt = QIO_get_tag_value(parse_pt, tag, value_string);
    
    QIO_decode_as_string (tag,value_string,&checksum->version);
    QIO_decode_as_hex32  (tag,value_string,&checksum->suma);
    QIO_decode_as_hex32  (tag,value_string,&checksum->sumb);
  }

  /* Check for completeness */
  
  errors += QIO_check_string_occur  (&checksum->version);
  errors += QIO_check_hex32_occur   (&checksum->suma);
  errors += QIO_check_hex32_occur   (&checksum->sumb);

  return errors;
}

void QIO_encode_checksum_info(XML_String *checksum_string, 
			      QIO_ChecksumInfo *checksum){
  
  char *buf;
  int remainder;
  XML_string_realloc(checksum_string, QIO_STRINGALLOC);
  buf  = XML_string_ptr(checksum_string);
  remainder = XML_string_bytes(checksum_string);

  /* Initialize string: encoding is done by appending */
  *buf = '\0';
  buf = QIO_encode_as_string (buf,&checksum->version, &remainder);
  buf = QIO_encode_as_hex32  (buf,&checksum->suma, &remainder);
  buf = QIO_encode_as_hex32  (buf,&checksum->sumb, &remainder);
}

/* Utilities for loading file_info values */

int QIO_insert_file_version(QIO_FileInfo *file_info, char *version){
  file_info->version.occur = 0;
  if(!version)return 1;
  strncpy(file_info->version.value, version, QIO_MAXVALUESTRING);
  file_info->version.value[QIO_MAXVALUESTRING-1] = '\0';
  file_info->version.occur = 1;
  if(strlen(version) >= QIO_MAXVALUESTRING)return 1;
  else return 0;
}

int QIO_insert_spacetime_dims(QIO_FileInfo *file_info, 
			      int spacetime, int *dims){
  int i;

  file_info->spacetime.occur = 0;
  file_info->dims.occur = 0;
  if(!spacetime)return 1;
  if(!dims)return 1;
  file_info->spacetime.value =  spacetime;
  for(i = 0; i < spacetime; i++)
    file_info->dims.value[i] = dims[i];
  file_info->spacetime.occur = 1;
  file_info->dims.occur = 1;
  return 0;
}

int QIO_insert_multifile(QIO_FileInfo *file_info, int multifile){
  file_info->multifile.occur = 0;
  if(!multifile)return 1;
  file_info->multifile.value = multifile;
  file_info->multifile.occur = 1;
  return 0;
}

/* Utilities for loading record_info values */

int QIO_insert_record_version(QIO_RecordInfo *record_info, char *version){
  record_info->version.occur = 0;
  if(!version)return 1;
  strncpy(record_info->version.value, version, QIO_MAXVALUESTRING);
  record_info->version.value[QIO_MAXVALUESTRING-1] = '\0';
  record_info->version.occur = 1;
  if(strlen(version) >= QIO_MAXVALUESTRING)return 1;
  else return 0;
}

int QIO_insert_record_date(QIO_RecordInfo *record_info, char *date_string){
  int n;

  record_info->date.occur = 0;
  if(!date_string)return 1;
  strncpy(record_info->date.value, date_string, QIO_MAXVALUESTRING);
  /* Edit date: replace trailing end-of-line by blank and add UTC */
  n = strlen(record_info->date.value);
  if(record_info->date.value[n-1] == '\n')record_info->date.value[n-1] = ' ';
  strncpy(record_info->date.value + n,"UTC",QIO_MAXVALUESTRING - n);
  record_info->date.value[QIO_MAXVALUESTRING-1] = '\0';
  record_info->date.occur = 1;
  if(strlen(date_string) + 3 >= QIO_MAXVALUESTRING)return 1;
  else return 0;
}

int QIO_insert_datatype(QIO_RecordInfo *record_info, char* datatype){
  record_info->datatype.occur = 0;
  if(!record_info)return 1;
  strncpy(record_info->datatype.value, datatype, QIO_MAXVALUESTRING);
  record_info->datatype.value[QIO_MAXVALUESTRING-1] = '\0';
  record_info->datatype.occur = 1;
  if(strlen(datatype) >= QIO_MAXVALUESTRING)return 1;
  else return 0;
}

int QIO_insert_precision(QIO_RecordInfo *record_info, char* precision){
  record_info->precision.occur = 0;
  if(!precision)return 1;
  strncpy(record_info->precision.value, precision, QIO_MAXVALUESTRING);
  record_info->precision.value[QIO_MAXVALUESTRING-1] = '\0';
  record_info->precision.occur = 1;
  if(strlen(precision) >= QIO_MAXVALUESTRING)return 1;
  else return 0;
}

int QIO_insert_colors(QIO_RecordInfo *record_info, int colors){
  record_info->colors.occur = 0;
  if(!colors)return 1;
  record_info->colors.value = colors;
  record_info->colors.occur = 1;
  return 0;
}

int QIO_insert_spins(QIO_RecordInfo *record_info, int spins){
  record_info->spins.occur = 0;
  if(!spins)return 1;
  record_info->spins.value = spins;
  record_info->spins.occur = 1;
  return 0;
}

int QIO_insert_typesize(QIO_RecordInfo *record_info, int typesize){
  record_info->typesize.occur = 0;
  if(!typesize)return 1;
  record_info->typesize.value = typesize;
  record_info->typesize.occur = 1;
  return 0;
}

int QIO_insert_datacount(QIO_RecordInfo *record_info, int datacount){
  record_info->datacount.occur = 0;
  if(!datacount)return 1;
  record_info->datacount.value = datacount;
  record_info->datacount.occur = 1;
  return 0;
}

/* Utility for loading checksum values */

int QIO_insert_checksum_version(QIO_ChecksumInfo *checksum_info, 
				char *version){
  checksum_info->version.occur = 0;
  if(!version)return 1;
  strncpy(checksum_info->version.value, version, QIO_MAXVALUESTRING);
  checksum_info->version.value[QIO_MAXVALUESTRING-1] = '\0';
  checksum_info->version.occur = 1;
  if(strlen(version) >= QIO_MAXVALUESTRING)return 1;
  else return 0;
}

int QIO_insert_suma_sumb(QIO_ChecksumInfo *checksum_info, 
			 u_int32 suma, u_int32 sumb){
  checksum_info->suma.occur = 0;
  checksum_info->sumb.occur = 0;
  if(!suma || !sumb)return 1;
  checksum_info->suma.value = suma;
  checksum_info->sumb.value = sumb;
  checksum_info->suma.occur = 1;
  checksum_info->sumb.occur = 1;
  return 0;
}



/* Accessors for file info */

char *QIO_get_file_version(QIO_FileInfo *file_info){
  return file_info->version.value;
}

int QIO_get_spacetime(QIO_FileInfo *file_info){
  return file_info->spacetime.value;
}

int *QIO_get_dims(QIO_FileInfo *file_info){
  return file_info->dims.value;
}

int QIO_get_multifile(QIO_FileInfo *file_info){
  return file_info->multifile.value;
}

int QIO_defined_spacetime(QIO_FileInfo *file_info){
  return file_info->spacetime.occur;
}

int QIO_defined_dims(QIO_FileInfo *file_info){
  return file_info->dims.occur;
}

int QIO_defined_multifile(QIO_FileInfo *file_info){
  return file_info->multifile.occur;
}


/* Accessors for record info */

char *QIO_get_datatype(QIO_RecordInfo *record_info){
  return record_info->datatype.value;
}

char *QIO_get_precision(QIO_RecordInfo *record_info){
  return record_info->precision.value;
}

int QIO_get_colors(QIO_RecordInfo *record_info){
  return record_info->colors.value;
}

int QIO_get_spins(QIO_RecordInfo *record_info){
  return record_info->spins.value;
}

int QIO_get_typesize(QIO_RecordInfo *record_info){
  return record_info->typesize.value;
}

int QIO_get_datacount(QIO_RecordInfo *record_info){
  return record_info->datacount.value;
}

int QIO_defined_datatype(QIO_RecordInfo *record_info){
  return record_info->datatype.occur;
}

int QIO_defined_precision(QIO_RecordInfo *record_info){
  return record_info->precision.occur;
}

int QIO_defined_colors(QIO_RecordInfo *record_info){
  return record_info->colors.occur;
}

int QIO_defined_spins(QIO_RecordInfo *record_info){
  return record_info->spins.occur;
}

int QIO_defined_typesize(QIO_RecordInfo *record_info){
  return record_info->typesize.occur;
}

int QIO_defined_datacount(QIO_RecordInfo *record_info){
  return record_info->datacount.occur;
}

/* Accessors for record info */

u_int32 QIO_get_suma(QIO_ChecksumInfo *checksum_info){
  return checksum_info->suma.value;
}

u_int32 QIO_get_sumb(QIO_ChecksumInfo *checksum_info){
  return checksum_info->sumb.value;
}

int QIO_defined_suma(QIO_ChecksumInfo *checksum_info){
  return checksum_info->suma.occur;
}

int QIO_defined_sumb(QIO_ChecksumInfo *checksum_info){
  return checksum_info->sumb.occur;
}


/* Utilities for creating structures from templates */

QIO_FileInfo *QIO_create_file_info(int spacetime, int *dims, int multifile){
  QIO_FileInfo template = QIO_FILE_INFO_TEMPLATE;
  QIO_FileInfo *file_info;
  
  file_info = (QIO_FileInfo *)malloc(sizeof(QIO_FileInfo));
  if(!file_info)return NULL;
  
  *file_info = template;
  QIO_insert_file_version(file_info,QIO_FILEFORMATVERSION);
  QIO_insert_spacetime_dims(file_info,spacetime,dims);
  QIO_insert_multifile(file_info,multifile);
  return file_info;
}

void QIO_destroy_file_info(QIO_FileInfo *file_info){
  free(file_info);
}

/* Compare only fields that occur in the expected structure */
int QIO_compare_file_info(QIO_FileInfo *found, QIO_FileInfo *expect,
			  char *myname, int this_node){
  int i, n, ok;
  int *dims_expect, *dims_found;

  if(QIO_defined_spacetime(expect))
    if(!QIO_defined_spacetime(found) &&
       QIO_get_spacetime(found) != QIO_get_spacetime(expect))
      {
	printf("%s(%d):Spacetime dimension mismatch expected %d found %d \n",
	       myname, this_node,
	       QIO_get_spacetime(expect), QIO_get_spacetime(found));
	return 1;
      }
  
  if(QIO_defined_dims(expect)){
    if(!QIO_defined_dims(found))
      {
	printf("%s:Dimensions missing\n",myname,this_node);
	return 1;
      }
    
    dims_expect = QIO_get_dims(expect);
    dims_found = QIO_get_dims(found);
    n = QIO_get_spacetime(expect);
    ok = 1;
    
    for(i = 0; i < n; i++)
      if(dims_expect[i] != dims_found[i])ok = 0;
    
    if(!ok){
      printf("%s(%d): lattice dimensions do not match\n",myname,this_node);
      printf("Expected ");
      for(i = 0; i < n; i++)printf(" %d", dims_expect[i]);
      printf("\nFound   ");
      for(i = 0; i < n; i++)printf(" %d", dims_found[i]);
      printf("\n");
      return 1;
    }
  }
  
  if(QIO_defined_multifile(expect))
    if(!QIO_defined_multifile(found) &&
       QIO_get_multifile(found) != QIO_get_multifile(expect))
      {
	printf("%s(%d):Multifile parameter mismatch expected %d found %d \n",
	       myname,this_node,
	       QIO_get_multifile(expect),QIO_get_multifile(found));
	return 1;
      }
  
  return 0;
}

QIO_RecordInfo *QIO_create_record_info(char *datatype, char *precision, 
					int colors, int spins, int typesize, 
					int datacount){
  QIO_RecordInfo template = QIO_RECORD_INFO_TEMPLATE;
  QIO_RecordInfo *record_info;
  time_t cu_time;
  
  record_info = (QIO_RecordInfo *)malloc(sizeof(QIO_RecordInfo));
  if(!record_info)return NULL;
  time(&cu_time);

  *record_info = template;
  QIO_insert_record_version(record_info,QIO_RECORDFORMATVERSION);
  QIO_insert_record_date(record_info,asctime(gmtime(&cu_time)));
  QIO_insert_datatype(record_info,datatype);
  QIO_insert_precision(record_info,precision);
  QIO_insert_colors(record_info,colors);
  QIO_insert_spins(record_info,spins);
  QIO_insert_typesize(record_info,typesize);
  QIO_insert_datacount(record_info,datacount);
  return record_info;
}

void QIO_destroy_record_info(QIO_RecordInfo *record_info){
  free(record_info);
}

/* Compare only fields that occur in the expected record info */
int QIO_compare_record_info(QIO_RecordInfo *found, QIO_RecordInfo *expect){
  char myname[] = "QIO_compare_record_info";

  if(QIO_defined_datatype(expect))
    if(!QIO_defined_datatype(found) && 
       strncmp(QIO_get_datatype(found),QIO_get_datatype(expect),
	       QIO_MAXVALUESTRING))
      {
	printf("%s:Datatype mismatch expected %s found %s \n",myname,
	       QIO_get_datatype(expect),QIO_get_datatype(found));
	return 1;
      }

  if(QIO_defined_precision(expect))
    if(!QIO_defined_precision(found) &&
       strncmp(QIO_get_precision(found),QIO_get_precision(expect),
	       QIO_MAXVALUESTRING))
      {
	printf("%s:Precision mismatch expected %s found %s \n",myname,
	       QIO_get_precision(expect),QIO_get_precision(found));
	return 1;
      }

  if(QIO_defined_colors(expect))
    if(!QIO_defined_colors(found) &&
       QIO_get_colors(found) != QIO_get_colors(expect))
      {
	printf("%s:Colors mismatch expected %d found %d \n",myname,
	       QIO_get_colors(expect),QIO_get_colors(found));
	return 1;
      }

  if(QIO_defined_spins(expect))
    if(!QIO_defined_spins(found) &&
       QIO_get_spins(found)  != QIO_get_spins(expect))
      {
	printf("%s:Spins mismatch expected %d found %d \n",myname,
	       QIO_get_spins(expect),QIO_get_spins(found));
	return 1;
      }

  if(QIO_defined_typesize(expect))
    if(!QIO_defined_typesize(found) &&
       QIO_get_typesize(found) != QIO_get_typesize(expect))
      {
	printf("%s:Typesize mismatch expected %d found %d \n",myname,
	       QIO_get_typesize(expect),QIO_get_typesize(found));
	return 1;
      }

  if(QIO_defined_datacount(expect))
    if(!QIO_defined_datacount(found) &&
       QIO_get_datacount(found) != QIO_get_datacount(expect))
      {
	printf("%s:Datacount mismatch expected %d found %d \n",myname,
	       QIO_get_datacount(expect),QIO_get_datacount(found));
	return 1;
      }

  return 0;
}

QIO_ChecksumInfo *QIO_create_checksum_info(u_int32 suma, u_int32 sumb){
  QIO_ChecksumInfo template = QIO_CHECKSUM_INFO_TEMPLATE;
  QIO_ChecksumInfo *checksum_info;
  
  checksum_info = (QIO_ChecksumInfo *)malloc(sizeof(QIO_ChecksumInfo));
  if(!checksum_info)return NULL;

  *checksum_info = template;
  QIO_insert_checksum_version(checksum_info,QIO_CHECKSUMFORMATVERSION);
  QIO_insert_suma_sumb(checksum_info,suma,sumb);
  return checksum_info;
}

void QIO_destroy_checksum_info(QIO_ChecksumInfo *checksum_info){
  free(checksum_info);
}

int QIO_compare_checksum_info(QIO_ChecksumInfo *found, 
			      QIO_ChecksumInfo *expect, 
			      char *myname, int this_node){

  if(!QIO_defined_suma(found) || !QIO_defined_sumb(found)){
    printf("%s(%d): checksum info missing\n");
    return 1;
  }

  if(QIO_get_suma(expect) != QIO_get_suma(found) || 
     QIO_get_sumb(expect) != QIO_get_sumb(found)){
    printf("%s(%d): Checksum mismatch.  Found %x %x.  Expected %x %x\n",
	   myname,this_node,QIO_get_suma(found),QIO_get_sumb(found),
	   QIO_get_suma(expect),QIO_get_sumb(expect) );
    return 1;
  }
  else
    printf("%s(%d): Checksums %x %x OK\n",myname,this_node,
	   QIO_get_suma(found),QIO_get_sumb(found));

  return 0;
}


