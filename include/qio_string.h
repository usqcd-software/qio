/* String support for a safe string type */

#ifndef QIO_STRING_H
#define QIO_STRING_H

#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct {
  char *string;
  size_t length;
} QIO_String;

QIO_String *QIO_string_create(int length);
QIO_String *QIO_string_realloc(QIO_String *qs, int length);
QIO_String *QIO_string_set(const char *const string);
void QIO_string_copy(QIO_String *dest, QIO_String *src);
size_t QIO_string_bytes(const QIO_String *const qs);
char * QIO_string_ptr(QIO_String *qs);
void QIO_string_destroy(QIO_String *qs);

#ifdef __cplusplus
}
#endif

#endif /* QIO_STRING_H */
