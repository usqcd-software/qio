/* Safe strings for QIO */

#include <qio_config.h>
#include <qio_string.h>
#include <stdio.h>
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

/* Size of string */
size_t QIO_string_bytes(const QIO_String *const qs)
{
  return qs->length;
}

/* Serialize the data to a character string */
char *QIO_string_ptr(QIO_String *qs)
{
  return qs->string;
}

/* String creation */
QIO_String *QIO_string_create(int length)
{
  QIO_String *qs;

  qs = (QIO_String *)malloc(sizeof(QIO_String));
  if(qs == NULL)return NULL;
  qs->string = NULL;
  qs->length = 0;
  return QIO_string_realloc(qs,length);
}

/* Non-destructive string reallocation */
QIO_String* QIO_string_realloc(QIO_String *qs, int length)
{
  int i,min;
  char *tmp;

  if(qs == NULL) 
    return NULL;
  if(length == 0) 
    return qs;
  
  tmp = (char *)malloc(length);
  if(tmp == NULL)
    {
      printf("QIO_string_realloc: Can't malloc size %d\n",length);
      return NULL;
    }
  
  for(i = 0; i < length; i++) tmp[i] = '\0';

  if(qs->string != NULL)
    {
      /* Follow semantics of realloc - shrink or grow/copy */
      min = length > qs->length ? qs->length : length;
      strncpy(tmp,qs->string,min);
      tmp[min-1] = '\0';  /* Assure null termination */
      free(qs->string);
    }      

  qs->length = length;
  qs->string = tmp;
      
  return qs;
}

/* String creation convenience*/
QIO_String *QIO_string_set(const char *const string)
{
  QIO_String *qs;
  size_t len = strlen(string) + 1;

  qs = QIO_string_create(len);
  if(qs == NULL)return NULL;
  strncpy(qs->string, string, len); /* Assure null termination with +1 above */

  return qs;
}

void QIO_string_destroy(QIO_String *qs)
{
  if (qs->length > 0)
    free(qs->string);
  free(qs);
}

void QIO_string_copy(QIO_String *dest, QIO_String *src)
{
  int length = QIO_string_bytes(src);
  QIO_string_realloc(dest,length);
  strncpy(dest->string, src->string, length);
  dest->string[length-1] = '\0';  /* Assure null termination */
}
