#ifndef AMIGA_DRAG_PREVIEW_H
#define AMIGA_DRAG_PREVIEW_H
#include "dock_core.h"
struct Screen;
struct Window;
typedef struct AmigaDragPreview AmigaDragPreview;

/* A transient native window extends the drag beyond the Dock's surface.
   It shows picture (the icon centred at its top, with the Remove label
   under it once armed) over a save-under of the screen, redrawn on each
   move; shaped, only the picture's outline. Reopen it for another picture.
   Intuition moves the window and restores what it uncovers. */
AmigaDragPreview *amiga_drag_preview_open(struct Screen *screen, const DockImage *icon,
    const DockImage *picture, int shaped, int pointer_x, int pointer_y);
/* Once the last move has landed, show the save-under for the new place.
   Call first thing after each wake: until then the window shows the old
   place's background. Returns 1 when it drew. */
int amiga_drag_preview_settle(AmigaDragPreview *preview);
/* Returns 1 when the window moves (to where icon reports it). */
int amiga_drag_preview_move(AmigaDragPreview *preview, int pointer_x, int pointer_y);
/* The window's private copy of the icon and its top left on the screen
   once the last move lands, for the dock to draw the same picture in step. */
/* A number no other preview had: its icon can reuse a freed one's address. */
unsigned long amiga_drag_preview_stamp(const AmigaDragPreview *preview);
const DockImage *amiga_drag_preview_icon(const AmigaDragPreview *preview, int *x, int *y);
int amiga_drag_preview_owns(const AmigaDragPreview *preview, const struct Window *window);
void amiga_drag_preview_close(AmigaDragPreview *preview);
#endif
