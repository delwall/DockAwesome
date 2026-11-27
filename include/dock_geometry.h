#ifndef DOCK_GEOMETRY_H
#define DOCK_GEOMETRY_H
#include "dock_core.h"

typedef enum {
    DOCK_EDGE_BOTTOM = 0,
    DOCK_EDGE_LEFT = 1,
    DOCK_EDGE_RIGHT = 2
} DockEdge;

/* Half-open pixel bounds: [left,right) by [top,bottom). */
typedef struct { int left, top, right, bottom; } DockRect;

enum { DOCK_DESKTOP_GAP = 3 };

int dock_panel_thickness(const DockSettings *settings);
void dock_panel_cross_bounds(const DockSettings *settings, int extent, int *inner, int *outer);

/* Preferred long axis, including room for the normal magnification wave.
   Bound this by the available screen length before allocating a surface. */
int dock_viewport_length(const DockSettings *settings, int count);

/* Side layouts reuse bottom geometry with the axes swapped. Icons stay upright. */
int dock_edge_layout(const DockSettings *settings, DockEdge edge, int count,
    int width, int height, int mouse_x, int mouse_y, int active,
    DockPlacement *out);
/* Animation state always contains virtual longitudinal centers. Reset it
   when changing edges or rebuilding the surface. No dimensions are needed:
   the left reflection's constant width cancels from baseline animation. */
int dock_edge_animate(const DockSettings *settings, DockEdge edge,
    DockAnimation *animation, DockPlacement *current,
    const DockPlacement *target, int count);
int dock_edge_hover(const DockSettings *settings, DockEdge edge,
    const DockPlacement *placement, int count, int width, int height,
    int x, int y);
/* External insertion uses stable resting centers, independent of the
   current animation. Returns 0..count, or -1 outside the drop band. */
int dock_edge_insertion_slot(const DockSettings *settings, DockEdge edge,
    int count, int width, int height, int x, int y);
/* The same slot from the position along the row alone, for any point:
   0 before the first centre, count past the last; -1 on invalid input. */
int dock_edge_row_slot(const DockSettings *settings, DockEdge edge,
    int count, int width, int height, int x, int y);
int dock_edge_insertion_layout(const DockSettings *settings, DockEdge edge,
    int count, int width, int height, int slot, DockPlacement *out);
/* Lay out the complete row, then return the neighbors and the vacant slot
   separately. Coordinates are on the actual surface, as for normal hover. */
int dock_edge_gap_layout(const DockSettings *settings, DockEdge edge,
    int count, int width, int height, int slot, int x, int y, int active,
    DockPlacement *out, DockPlacement *gap);
/* True only beyond the plate as drawn (reaching over include, see
   dock_edge_panel_reach) AND every icon box, with margin pixels of
   tolerance around each. Coordinates may lie outside the surface. */
int dock_edge_outside(const DockSettings *settings, DockEdge edge,
    const DockPlacement *placement, int count, int width, int height,
    const DockRect *include, int x, int y, int margin);
/* Uses the existing bottom shelf thickness and padding, transformed as
   rectangle edges, then clipped to the actual surface. */
int dock_edge_panel_rect(const DockSettings *settings, DockEdge edge,
    const DockPlacement *placement, int count, int width, int height,
    DockRect *out);
/* The plate as drawn: dock_edge_panel_rect, reaching along the row over
   include (an empty slot, surface coordinates; NULL or empty for none) by
   the icons' padding, clipped to the surface. */
int dock_edge_panel_reach(const DockSettings *settings, DockEdge edge,
    const DockPlacement *placement, int count, int width, int height,
    const DockRect *include, DockRect *out);

/* Whether PiP must step aside for a held native menu (PiP2 lies over
   menus). dock is the shown PiP box on the screen. Where the system draws
   its menus in windows (ApolloOS), menus holds the count windows in front
   of the dock: only one overlapping the dock counts; AmigaOS 3.x Intuition
   passes its pull-down's area (dock_pulldown_reach). count < 0: nothing
   is known of the menu (MagicMenu); a menu opened within
   DOCK_MENU_REACH of the pointer may reach the dock, and so does one that
   drops from the title bar onto a dock that starts near the top. */
enum { DOCK_MENU_REACH = 256 };
int dock_menu_covers(DockRect dock, int pointer_x, int pointer_y, const DockRect *menus, int count);
/* The area AmigaOS 3.x Intuition's own menus can cover: they drop from the
   screen's bar (bar_height rows), and items_bottom is the lowest row of any
   item or subitem below the drop box's top (MenuItem TopEdge + Height,
   subitems relative to their item). The whole width, with
   DOCK_PULLDOWN_MARGIN rows for the box's borders and Intuition moving a
   subitem box to fit. */
enum { DOCK_PULLDOWN_MARGIN = 16 };
DockRect dock_pulldown_reach(int screen_width, int bar_height, int items_bottom);

/* The empty slot a drag or a drop opens in the row, animated like an icon
   so the plate's ends move with the row: at an end nothing else bounds the
   plate, and set at once it jumped there while the icons slid (IMG_2595).
   slot is -1 when no gap is shown. */
typedef struct {
    DockPlacement shown, target;
    DockAnimation animation;
    int slot;
} DockGap;
void dock_gap_reset(DockGap *gap);
/* Aim at gap (in slot). A gap not yet shown starts as from: where the pin
   was, or the neighbour it opens beside (NULL: at gap). */
void dock_gap_open(DockGap *gap, int slot, const DockPlacement *target, const DockPlacement *from);
/* Close into the neighbour's target in row (count icons). A closed gap
   stays under that icon, inside the plate, until dock_gap_reset: ended
   before the row settles, the plate's end stepped back a pixel. */
void dock_gap_close(DockGap *gap, const DockPlacement *row, int count);
/* One animation step; 1 when the shown gap changed. */
int dock_gap_step(const DockSettings *settings, DockEdge edge, DockGap *gap);
int dock_gap_moving(const DockGap *gap);
/* The shown gap, or an empty rect. */
DockRect dock_gap_rect(const DockGap *gap);
/* The Folder Stack box's buttons: a faint disc with a round-capped
   glyph, drawn 5/6 of the control's size and centred in it. The page
   buttons show a chevron pointing left (DOCK_BUTTON_LEFT) or right
   (DOCK_BUTTON_RIGHT); the parent drawer button (DOCK_BUTTON_PARENT) a
   return arrow: a stem up from the bottom right that turns left into a
   bar ending in an arrowhead. The control (the hit area) keeps its size.
   Positions and lengths in 1/16 px: the glyph is count segments
   segment[i] = {x0, y0, x1, y1} stroked half wide with round caps; ink is
   the bounding box of their end points grown by half, centred on the
   disc's centre (cx, cy): every glyph exactly by its box. */
enum { DOCK_BUTTON_LEFT = -1, DOCK_BUTTON_RIGHT = 1, DOCK_BUTTON_PARENT = 2 };
/* ink: the stroke's box {left, top, right, bottom}, inclusive, 1/16 px. */
typedef struct { int cx, cy, radius, half, count, segment[8][4], ink[4]; } DockButtonGlyph;
void dock_button_glyph(int x, int y, int width, int height, int direction, DockButtonGlyph *out);
/* Pixel (px,py) sampled 4x4: how many samples (0..16) lie in the glyph's
   stroke, and in *disc how many lie in the disc. */
int dock_button_coverage(const DockButtonGlyph *glyph, int px, int py, int *disc);
/* The same button without antialiasing, in whole pixels: a disc of size
   pixels at (left, top), and count polylines stroke[i] = {a, b, c} of 45
   degree or straight lines: a chevron two pixels wide, the return arrow
   one pixel. Centred on the control: a chevron exactly along the direction
   it points and to half a pixel across it when the control is an even
   number of pixels; the arrow to half a pixel both ways then. */
typedef struct { int left, top, size, count, stroke[3][3][2]; } DockButtonPixels;
void dock_button_pixels(int x, int y, int width, int height, int direction, DockButtonPixels *out);

/* The Open Folder tile's glyph: a ring with an arrow, centred exactly on
   the tile's icon box (icon pixels square at left, top); positions in
   1/16 px. The arrow's outlines are in 1/64 of the ring's radius. */
enum { DOCK_OPEN_TAIL = 13, DOCK_OPEN_HEAD = 9 };
extern const signed char dock_open_arrow_tail[DOCK_OPEN_TAIL][2];
extern const signed char dock_open_arrow_head[DOCK_OPEN_HEAD][2];
typedef struct { int cx, cy, radius, outer, inner; } DockOpenGlyph;
void dock_open_glyph(int left, int top, int icon, int radius, int thickness, DockOpenGlyph *out);
/* Row py sampled 4x4: covered[x] = samples (0..16) of pixel px0+x in the
   ring or the arrow, for count pixels. */
void dock_open_glyph_row(const DockOpenGlyph *glyph, int py, int px0, int count, unsigned char *covered);
#endif
