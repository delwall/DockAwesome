/* Text rules with no OS calls, so the host tests cover them: which window
   title names a pinned document, how a path is quoted for the Shell, and
   what counts as a text file. */
#include "dock_text.h"
#include <string.h>

/* ISO 8859-1 lower case, as utility.library compares names. */
static unsigned char lower(unsigned char c)
{
    if ((c >= 'A' && c <= 'Z') || (c >= 0xc0 && c <= 0xde && c != 0xd7)) return (unsigned char)(c+32);
    return c;
}

int dock_strnicmp(const char *a, const char *b, size_t length)
{
    for (; length; --length, ++a, ++b) {
        unsigned char x = lower((unsigned char)*a), y = lower((unsigned char)*b);
        if (x != y) return x < y ? -1 : 1;
        if (!x) break;
    }
    return 0;
}

/* Window chrome may add a label on either side. Single spaces are legal
   path characters and do not separate a path from that label. */
static int title_separator(const char *text)
{
    if (text[0] == ':' && text[1] == ' ') return 2;
    if (*text == '\t') return 1;
    if (*text != ' ') return 0;
    if (text[1] == ' ' || text[1] == '\t') return 1;
    return (text[1] == '-' || text[1] == '|') && text[2] == ' ' ? 3 : 0;
}

static const char *next_title_part(const char *part)
{
    int separator;
    while (*part && !title_separator(part)) ++part;
    while ((separator = title_separator(part))) part += separator;
    while (*part == ' ' || *part == '\t') ++part;
    return part;
}

int dock_title_names_document(const char *title, const char *path, size_t length)
{
    const char *part = title;
    while (*part) {
        if (*part == '"' && !dock_strnicmp(part+1,path,length) &&
            part[1+length] == '"') return 1;
        if (!dock_strnicmp(part,path,length) &&
            (!part[length] || title_separator(part+length))) return 1;
        part = next_title_part(part);
    }
    return 0;
}

/* Extract only qualified title paths with the same file name as a pin.
   The caller validates the complete path through DOS after releasing IBase. */
size_t dock_title_document_alias(const char *title, const char *path, char *candidate,
    size_t capacity)
{
    const char *name = strrchr(path,'/');
    const char *part;
    size_t length;
    if (!name) name = strrchr(path,':');
    if (!name || !*++name) return 0;
    length = strlen(name);
    for (part = title; *part; part = next_title_part(part)) {
        int quoted = *part == '"';
        const char *start = part+quoted, *volume, *file, *limit = start;
        /* The volume belongs to this part: a label before the path
           ("Editor - RAM:x") is not part of the candidate. */
        if (quoted) while (*limit && *limit != '"') ++limit;
        else while (*limit && !title_separator(limit)) ++limit;
        volume = memchr(start,':',(size_t)(limit-start));
        if (!volume || volume == start || volume[1] == ' ' ||
            memchr(start,'/',(size_t)(volume-start))) continue;
        for (file = volume+1; file < limit; ++file) {
            size_t n;
            const char *end;
            if (file[-1] != ':' && file[-1] != '/') continue;
            if (dock_strnicmp(file,name,length)) continue;
            end = file+length;
            if (quoted ? *end != '"' : (*end && !title_separator(end))) continue;
            n = end-start;
            if (n >= capacity) continue;
            memcpy(candidate,start,n); candidate[n] = 0;
            return n;
        }
    }
    return 0;
}

/* Appends `text` in Shell double quotes, where * escapes " and itself.
   Returns 0 when it does not fit. */
int dock_shell_quote(char *out, size_t size, const char *text)
{
    size_t n = strlen(out);
    if (n+1 >= size) return 0;
    out[n++] = '"';
    for (; *text; ++text) {
        if (n+3 >= size) return 0;
        if (*text == '"' || *text == '*') out[n++] = '*';
        out[n++] = *text;
    }
    out[n++] = '"';
    out[n] = 0;
    return 1;
}

/* The start of a file reads as text: a picture, an archive or any other
   binary document must never reach Execute. */
int dock_text_bytes(const unsigned char *bytes, size_t count)
{
    size_t i;
    for (i = 0; i < count; ++i)
        if (bytes[i] < 32 && bytes[i] != '\t' && bytes[i] != '\n' && bytes[i] != '\r' &&
            bytes[i] != '\f' && bytes[i] != 27) return 0;
    return count > 0;
}
