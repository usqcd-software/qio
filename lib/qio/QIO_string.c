/* Safe strings for QIO */

#include <qio_config.h>
#include <qio_string.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

/*
 *  QIO_String manipulation utilities
 */

/* String creation */
QIO_String *QIO_string_create(void)
{
  QIO_String *qs;

  qs = (QIO_String *)malloc(sizeof(QIO_String));
  if(qs == NULL) return NULL;
  qs->string = NULL;
  qs->length = 0;
  return qs;
}

void QIO_string_destroy(QIO_String *qs)
{
  if(qs->length>0) free(qs->string);
  free(qs);
}

/* set String */
void QIO_string_set(QIO_String *qs, const char *const string)
{
  if(string==NULL) {
    if(qs->length>0) free(qs->string);
    qs->string = NULL;
    qs->length = 0;
  } else {
    size_t len = strlen(string) + 1;

    qs->string = (char *)realloc(qs->string, len);
    memcpy(qs->string, string, len);
    qs->length = len;
  }
}

/* Size of string */
size_t QIO_string_length(const QIO_String *const qs)
{
  return qs->length;
}

/* Return pointer to string data */
char *QIO_string_ptr(QIO_String *qs)
{
  return qs->string;
}

void QIO_string_copy(QIO_String *dest, QIO_String *src)
{
  size_t len;
  if(src == NULL)return;
  if(dest == NULL)return;

  len = src->length;
  dest->string = (char *)realloc(dest->string, len);
  memcpy(dest->string, src->string, len);
  dest->length = len;
}

/* Non-destructive string reallocation */
void QIO_string_realloc(QIO_String *qs, int length)
{
  int i,min;
  char *tmp;

  if(qs == NULL || length == 0)return;
  
  tmp = (char *)malloc(length);
  if(tmp == NULL)
    {
      printf("QIO_string_realloc: Can't malloc size %d\n",length);
      return;
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
}

