#ifndef DOCK_PREFS_H
#define DOCK_PREFS_H

#include "dock_geometry.h"
#include "dock_sort.h"
#include <stdio.h>

#define DOCK_PREFS_PATH_CAP 512

/* What a left click on a Folder Stack does, chosen per stack in its menu
   ("Click to"): show its box, or open its drawer as a window. Show Stack
   is the default for a drawer, Open Folder for a disk (a volume root, a
   path ending in ':'), which shows its box only when chosen. */
typedef enum { DOCK_CLICK_STACK, DOCK_CLICK_FOLDER, DOCK_CLICK_COUNT } DockClick;

/* The click a pin has until one is chosen: Open Folder for a path ending
   in ':', else Show Stack. */
DockClick dock_click_default(const char *path);

typedef struct {
    int size, zoom, levels;
    int scale_percent; /* Zero when zoom is an explicit pixel size. */
    DockEdge edge;
    int pip_x_offset, pip_y_offset;
    int count;
    /* Legacy, read only: 1.0 beta 5 and older kept one Folder Stack here,
       apart from the pins. dock_prefs_migrate moves it into paths; it is
       never written. */
    char folder[DOCK_PREFS_PATH_CAP];
    char paths[DOCK_MAX_ITEMS][DOCK_PREFS_PATH_CAP];
    /* Each pin's Folder Stack order. In the file, "sorts" maps a path to
       its key and lists only paths not sorted by name; a key for no pin,
       or one this build does not know, reads as name. */
    DockSort sorts[DOCK_MAX_ITEMS];
    /* Each pin's left click, kept like sorts: "clicks" maps a path to
       "stack" or "folder" and lists only pins whose click is not their
       default (dock_click_default); a value this build does not know
       reads as that default. */
    DockClick clicks[DOCK_MAX_ITEMS];
} DockPrefs;

void dock_prefs_defaults(DockPrefs *prefs);
/* Read a version-1 JSON object. A failed read leaves prefs unchanged. */
int dock_prefs_read(FILE *file, DockPrefs *prefs);
/* Write a complete JSON object, without "folder", with "sorts" only when
   a pin is not sorted by name and "clicks" only when a pin's click is not
   its default. The caller handles flushing and replacement. */
int dock_prefs_write(FILE *file, const DockPrefs *prefs);
/* 1.0 beta 5 and older kept one Folder Stack apart from the pins: make it
   the last pin, showing its box (also a disk, which was a stack there),
   unless it is one already. 1 when prefs changed. */
int dock_prefs_migrate(DockPrefs *prefs);

#endif
