/* Strings for XML */

#include <qio_config.h>
#include <xml_string.h>
#include <string.h>
#include <stdio.h>
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

/* Size of string in XML */
size_t XML_string_bytes(const XML_String *const xml)
{
  return xml->length;
}

/* Serialize the XML data to a character string */
char *XML_string_ptr(XML_String *xml)
{
  return xml->string;
}

/* String creation */
XML_String *XML_string_create(int length)
{
  XML_String *xml;

  xml = (XML_String *)malloc(sizeof(XML_String));
  if(xml == NULL)return NULL;
  xml->string = NULL;
  xml->length = 0;
  return XML_string_realloc(xml,length);
}

/* Non-destructive string reallocation */
XML_String* XML_string_realloc(XML_String *xml, int length)
{
  int i,min;
  char *tmp;

  if(xml == NULL) 
    return NULL;
  if(length == 0) 
    return xml;
  
  tmp = (char *)malloc(length);
  if(tmp == NULL)
    {
      printf("XML_string_realloc: Can't malloc size %d\n",length);
      return NULL;
    }
  
  for(i = 0; i < length; i++) tmp[i] = '\0';

  if(xml->string != NULL)
    {
      /* Follow semantics of realloc - shrink or grow/copy */
      min = length > xml->length ? xml->length : length;
      strncpy(tmp,xml->string,min);
      tmp[min-1] = '\0';  /* Assure null termination */
      free(xml->string);
    }      

  xml->length = length;
  xml->string = tmp;
      
  return xml;
}

void XML_string_set(XML_String *xml, const char *const string)
{
  strncpy(xml->string, string, xml->length);
  xml->string[xml->length-1] = '\0';  /* Assure null termination */
}

void XML_string_destroy(XML_String *xml)
{
  if (xml->length > 0)
    free(xml->string);
  free(xml);
}

void XML_string_copy(XML_String *dest, XML_String *src)
{
  int length = XML_string_bytes(src);
  XML_string_realloc(dest,length);
  strncpy(dest->string, src->string, length);
  dest->string[length-1] = '\0';  /* Assure null termination */
}
