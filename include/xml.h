/* String support for XML */

#ifndef XML_H
#define XML_H

#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct {
  char *string;
  size_t length;
} XML_string;

XML_string *XML_string_create(int length);
XML_string *XML_string_realloc(XML_string *xml, int length);
void XML_string_set(XML_string *xml, const char *const string);
size_t XML_string_bytes(const XML_string *const xml);
char * XML_string_ptr(XML_string *xml);
void XML_string_destroy(XML_string *xml);

#ifdef __cplusplus
}
#endif

#endif /* XML_H */
