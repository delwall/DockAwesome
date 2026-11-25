#ifndef DOCK_SORT_H
#define DOCK_SORT_H

#include <stddef.h>

/* How a Folder Stack orders its drawer, chosen per stack in its menu and
   kept in the preferences under dock_sort_key. Name is the default. */
typedef enum { DOCK_SORT_NAME, DOCK_SORT_DATE, DOCK_SORT_KIND, DOCK_SORT_COUNT } DockSort;

typedef struct {
    char name[108];
    int drawer;
    /* The entry's and its .info's dates (days, ticks in the day; zero when
       there is no .info): an icon read for these is still current. */
    unsigned long stamp[4];
} DockFolderEntry;

/* Names as AmigaDOS and utility.library's Stricmp compare them: ASCII and
   Latin-1 letters upper-cased, then byte order. */
int dock_sort_compare_names(const char *a, const char *b);
/* Name: by name. Date: the entry's own date, newest first. Kind: drawers,
   then files by extension (none first), each then by name. Ties end in
   byte order, so every order is total and does not depend on qsort. */
void dock_sort_entries(DockFolderEntry *entries, size_t count, DockSort sort);
/* "name", "date", "kind"; NULL for a value out of range. */
const char *dock_sort_key(DockSort sort);
/* 1 and *sort for a known key, else 0 (a newer build's key). */
int dock_sort_parse(const char *key, DockSort *sort);

#endif
