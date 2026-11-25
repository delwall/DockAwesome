#ifndef AMIGA_FOLDER_H
#define AMIGA_FOLDER_H

#include "dock_sort.h"

/* DOS errors fail instead of opening volume requesters that block the main task. */
void *amiga_requesters_off(void);
void amiga_requesters_restore(void *previous);

/* Returns a snapshot in sort's order (dock_sort_entries). The caller frees
   *entries. Empty folders succeed. */
int amiga_folder_read(const char *path, DockSort sort, DockFolderEntry **entries, size_t *count);

#endif
