/* Strings for XML */

#include <xml.h>
#include <string.h>
#include <stdio.h>
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

/* Size of string in XML */
size_t XML_string_bytes(const XML_string *const xml)
{
  return xml->length;
}

/* Serialize the XML data to a character string */
char *XML_string_ptr(XML_string *xml)
{
  return xml->string;
}

/* String creation */
XML_string *XML_string_create(int length)
{
  XML_string *xml;

  xml = (XML_string *)malloc(sizeof(XML_string));
  if(xml == NULL)return NULL;
  xml->string = NULL;
  xml->length = 0;
  return XML_string_realloc(xml,length);
}

/* Non-destructive string reallocation */
XML_string* XML_string_realloc(XML_string *xml, int length)
{
  int i;
  char *tmp;

  if(xml == NULL) 
    return NULL;
  if(length == 0) 
    return xml;
  
  tmp = (char *)malloc(length);
  if(tmp == NULL)
    return NULL;

  if (xml->length > 0)
  {
    if(xml->string == NULL)
      return NULL;

    /* Follow semantics of realloc - shrink or grow/copy */
    for(i = 0; i < length; i++)
      tmp[i] = '\0';
    strncpy(tmp,xml->string,xml->length);
    free(xml->string);
  }
  else
  {
    for(i = 0; i < length; i++)
      tmp[i] = '\0';
  }

  xml->length = length;
  xml->string = tmp;

  return xml;
}

/* dummy for inserting values into an XML structure */
/* Just point to the given string for now and hope it is invariant */

void XML_string_set(XML_string *xml, const char *const string)
{
  strncpy(xml->string, string, xml->length);
}

void XML_string_destroy(XML_string *xml)
{
  if (xml->length > 0)
    free(xml->string);
  free(xml);
}
