#ifndef DOCK_TEXT_H
#define DOCK_TEXT_H

#include <stddef.h>

/* Compares at most length bytes without regard to ISO 8859-1 case. */
int dock_strnicmp(const char *a, const char *b, size_t length);

/* A window title names the document `path` (its first `length` bytes): as a
   whole part between the labels window chrome adds, or in double quotes. */
int dock_title_names_document(const char *title, const char *path, size_t length);

/* A qualified path in `title` with the same file name as `path`, copied to
   `candidate` (capacity bytes). Returns its length, or 0. The caller checks
   through DOS that it is the same file. */
size_t dock_title_document_alias(const char *title, const char *path, char *candidate,
    size_t capacity);

/* Appends `text` to the string in `out` in Shell double quotes, where *
   escapes " and itself. Returns 0 when it does not fit in `size` bytes. */
int dock_shell_quote(char *out, size_t size, const char *text);

/* The bytes read as text: no control codes but tab, line ends, form feed
   and escape. An empty range is not text. */
int dock_text_bytes(const unsigned char *bytes, size_t count);

#endif
