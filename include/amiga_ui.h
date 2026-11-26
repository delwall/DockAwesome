#ifndef AMIGA_UI_H
#define AMIGA_UI_H

#include "dock_geometry.h"
#include "dock_prefs.h"
#include <exec/types.h>

struct Screen;
struct Window;
typedef struct DockUI DockUI;

/* edge uses DOCK_EDGE_* from dock_geometry.h. UI edits remain a draft. */
typedef struct {
    int size, scale_percent, edge;
} DockUIPrefs;
enum {
    DOCK_UI_NONE, DOCK_UI_SETTINGS, DOCK_UI_ABOUT, DOCK_UI_QUIT,
    DOCK_UI_SAVE, DOCK_UI_CANCEL, DOCK_UI_REFRESH, DOCK_UI_CLOSE_ITEM, DOCK_UI_QUIT_ITEM,
    DOCK_UI_KEEP_ITEM, DOCK_UI_NEW_SHELL, DOCK_UI_SHOW_SHELL, DOCK_UI_QUIT_SHELL,
    DOCK_UI_OPEN_PATH,
    /* A Folder Stack's menu: sort it; minus DOCK_UI_SORT_NAME a DockSort. */
    DOCK_UI_SORT_NAME, DOCK_UI_SORT_DATE, DOCK_UI_SORT_KIND,
    /* A Folder Stack's menu: what its left click does; minus
       DOCK_UI_CLICK_STACK a DockClick. */
    DOCK_UI_CLICK_STACK, DOCK_UI_CLICK_FOLDER
};
typedef struct {
    int type;
    DockUIPrefs prefs;
    char path[DOCK_PREFS_PATH_CAP];
} DockUIAction;
typedef int (*DockUIReadIcon)(const void *context, const char *path, DockImage *icon);

/* The caller keeps screen and the graphics/intuition libraries alive.
   The flags come from DockProfile (ui_shapes, ui_glass, ui_rgb_pens,
   aa_text): shaped windows, RGBA glass on an RTG screen, native drawing in
   RGB (else the nearest palette pens), and antialiased text through
   cybergraphics alpha. */
DockUI *dock_ui_create(struct Screen *screen, int window_shapes, int glass, int rgb_pens, int alpha_text);
void dock_ui_set_converters(DockUI *ui, DockConvertRGB24 encode, DockConvertRGB565 decode,
    DockConvertRGB565 encode565, DockMixHalf mix_half);
/* Returns the largest Size the dock can show for a draft (its edge and
   scale). Settings never offer a larger Size. */
typedef int (*DockUIFit)(void *context, const DockUIPrefs *draft);
/* Shows work in progress in the open dialog: a busy pointer and the status
   as its title. NULL restores the normal pointer and title. */
void dock_ui_set_busy(DockUI *ui, const char *status);
void dock_ui_set_fit_hook(DockUI *ui, DockUIFit hook, void *context);
/* --profile: the display's field counter, for the Folder Stack's frame log. */
void dock_ui_set_field_counter(DockUI *ui, const volatile ULONG *fields);
/* A menu's glass is built from a capture of the screen bitmap at
   (left,top), which never holds the PiP overlay: the hook puts what the
   dock shows there into it, once, before the glass is tinted. */
typedef void (*DockUIUnder)(void *context, DockImage *capture, int left, int top);
void dock_ui_set_under_hook(DockUI *ui, DockUIUnder hook, void *context);
/* --pip-dump: the open menu's tinted glass with its rim (before its rows
   are drawn), or NULL. */
const DockImage *dock_ui_menu_glass(const DockUI *ui);
/* --pip-dump: the open menu's window as the screen shows it, read back
   into out (the caller frees out->pixels); 0 without glass. */
int dock_ui_menu_pixels(DockUI *ui, DockImage *out);
void dock_ui_destroy(DockUI *ui);
ULONG dock_ui_signal_mask(const DockUI *ui);
/* drawer: a Folder Stack's menu, with sort (a DockSort) and click (a
   DockClick) checked, and a Close row when item_action is
   DOCK_UI_CLOSE_ITEM (a disk whose window is open). */
int dock_ui_open_popup(DockUI *ui, DockEdge edge, int anchor_x, int anchor_y,
    const struct Window *native_dock,
    int item_action, int can_keep, int drawer, int sort, int click);
/* sort (a DockSort) orders the drawer and the drawers opened inside it. */
int dock_ui_open_folder(DockUI *ui, DockEdge edge, int x, int y, const char *path, int sort,
    int icon_size, DockUIReadIcon read_icon, const void *context, int profile);
void dock_ui_dismiss_popup(DockUI *ui);
int dock_ui_popup_bounds(const DockUI *ui, DockRect *bounds);
void dock_ui_popup_span(const DockUI *ui, int row, int *left, int *right);
int dock_ui_folder_pending(const DockUI *ui);
/* The Folder Stack view is open and not closing. */
int dock_ui_folder_open(const DockUI *ui);
/* The Folder Stack view is opening or closing. */
int dock_ui_folder_moving(const DockUI *ui);
/* Turn the still Folder Stack's tip toward the stack pin's center at
   (x,y) on the screen; the box stays where it is. */
void dock_ui_point_folder_tip(DockUI *ui, int x, int y);
/* The still Folder Stack box as composed for the screen (glass, labels,
   icons), or NULL while it moves, loads or is closed: for --pip-dump.
   *page counts the pages shown (openings, drawers inside, page flips). */
const DockImage *dock_ui_folder_artwork(const DockUI *ui, unsigned long *page);
/* How far the Folder Stack box is out on screen: 0 (hidden) to 256 (open). */
int dock_ui_folder_progress(const DockUI *ui);
ULONG dock_ui_folder_frame_delay(const DockUI *ui);
/* Only icons are left to load in the open Folder Stack: no motion. */
int dock_ui_folder_loading(const DockUI *ui);
/* Reads one more icon of path's first page into the icon cache, for a
   Folder Stack that opens with its icons. Call while the dock is idle.
   slot: 0..DOCK_MAX_ITEMS-1, one read-ahead state each; it starts over
   (and rereads the drawer) when path, sort, icon_size or the anchor differ.
   sort: the stack's DockSort, so the page read is the one it opens with.
   (edge,x,y): the anchor the box would open at, for the real page size.
   1 while icons of the first page remain (or a popup is open); 0 when the
   page is read, the drawer cannot be read, or the cache has no free entry
   (the prefetch never evicts). */
int dock_ui_prefetch_folder(DockUI *ui, int slot, DockEdge edge, int x, int y,
    const char *path, int sort, int icon_size, DockUIReadIcon read_icon, const void *context);
/* The Folder Stack icon cache: live entries, entries, and the live entries
   of another path an open box overwrote. */
void dock_ui_folder_cache_stats(const DockUI *ui, int *used, int *capacity, unsigned long *evicted);
int dock_ui_folder_step(DockUI *ui);
void dock_ui_finish_folder_motion(DockUI *ui);
/* The label for the dock's own image: its height with the tip, its width
   for a text, and the label as RGBA (with tip: pointing down at tip_x, -1
   the middle). All 0 without antialiased text. */
#define DOCK_UI_LABEL_GAP 8 /* between the label's tip and the zoomed icon */
int dock_ui_label_height(DockUI *ui);
int dock_ui_label_width(DockUI *ui, const char *text, int max_width);
int dock_ui_label_image(DockUI *ui, const char *text, int max_width, int tip, int tip_x, DockImage *out);
/* A flat label pill with hard edges as RGBA (alpha 0 outside); the caller
   frees out->pixels. */
int dock_ui_pill_image(DockUI *ui, const char *text, DockImage *out);
int dock_ui_dialog_bounds(const DockUI *ui, DockRect *bounds);
int dock_ui_open_settings(DockUI *ui, const DockUIPrefs *initial);
int dock_ui_open_about(DockUI *ui);
/* Nonblocking: call until zero after a wake. Every returned message is replied.
   SAVE leaves the settings window open until the caller commits successfully. */
int dock_ui_poll(DockUI *ui, DockUIAction *action);
void dock_ui_close_dialog(DockUI *ui);
int dock_ui_owns_window(const DockUI *ui, const struct Window *window);
int dock_ui_busy(const DockUI *ui);
/* A dialog, a menu or a moving Folder Stack is open: the dock holds its
   row and passes presses through. A still Folder Stack does not. */
int dock_ui_holds_dock(const DockUI *ui);

#endif
