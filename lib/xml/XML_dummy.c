/* Dummy for XML */

#include <xml.h>
#include <string.h>
#include <stdio.h>
#include <malloc.h>

/* Size of string in XML */
size_t XML_bytes(XML_MetaData *xml){
  return xml->length;
}

/* Serialize the XML data to a character string */
char *XML_string(XML_MetaData *xml){
  return xml->string;
}

/* dummy XML creation and deletion */

XML_MetaData *XML_create(int length){
  XML_MetaData *xml;
  int i;

  xml = (XML_MetaData *)malloc(sizeof(XML_MetaData));
  if(xml == NULL)return NULL;
  xml->length = length;
  xml->string = (char *)malloc(length);
  if(xml->string == NULL)return NULL;
  for(i = 0; i < length; i++)xml->string[i] = '\0';
  return xml;
}

/* dummy for inserting values into an XML structure */
/* Just point to the given string for now and hope it is invariant */

void XML_set(XML_MetaData *xml, const char *string){
  strncpy(xml->string, string, xml->length);
}

void XML_destroy(XML_MetaData *xml){
  free(xml->string);
  free(xml);
}
