/* String support for XML */

#ifndef XML_STRING_H
#define XML_STRING_H

#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct {
  char *string;
  size_t length;
} XML_String;

XML_String *XML_string_create(int length);
XML_String *XML_string_realloc(XML_String *xml, int length);
void XML_string_set(XML_String *xml, const char *const string);
void XML_string_copy(XML_String *dest, XML_String *src);
size_t XML_string_bytes(const XML_String *const xml);
char * XML_string_ptr(XML_String *xml);
void XML_string_destroy(XML_String *xml);

#ifdef __cplusplus
}
#endif

#endif /* XML_STRING_H */
