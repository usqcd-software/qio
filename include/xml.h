/* Dummy XML header for qio-toy */

#ifndef XML_H
#define XML_H

#include <stdio.h>
#define MAX_XML 256

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct {
  char *string;
  size_t length;
} XML_MetaData;

XML_MetaData *XML_create(int length);
void XML_set(XML_MetaData *xml, char *string);
size_t XML_bytes(XML_MetaData *xml);
char * XML_string(XML_MetaData *xml);
void XML_destroy(XML_MetaData *xml);

#ifdef __cplusplus
}
#endif

#endif /* XML_H */
