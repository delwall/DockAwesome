#include "dock_geometry.h"
#include "dock_interaction.h"
#include <string.h>

static int side_edge(DockEdge edge)
{
    return edge == DOCK_EDGE_LEFT || edge == DOCK_EDGE_RIGHT;
}

int dock_viewport_length(const DockSettings *s, int count)
{
    int pitch, wanted, wave;
    if (!dock_settings_valid(s) || count < 0 || count > DOCK_MAX_ITEMS) return 0;
    pitch = s->base_size+s->gap;
    wanted = count*pitch+(count > 1 ? s->end_gap : 0)+2*s->max_size+2*s->padding+s->base_size;
    wave = dock_row_length(s,count)+4*(s->max_size-s->base_size)+2*s->padding;
    if (wanted < wave) wanted = wave;
    return wanted < 320 ? 320 : wanted;
}

static int valid(const DockSettings *s, DockEdge edge, const DockPlacement *p,
                 int count, int width, int height)
{
    return dock_settings_valid(s) &&
        (edge == DOCK_EDGE_BOTTOM || side_edge(edge)) &&
        count >= 0 && count <= DOCK_MAX_ITEMS && (!count || p) &&
        width > 0 && height > 0;
}

int dock_panel_thickness(const DockSettings *s)
{
    enum { KNEE = 32-2*DOCK_ICON_INSET };
    int artwork = s->base_size-2*DOCK_ICON_INSET, margin;
    /* Up to the default size the plate keeps the reference margin, 31 px per
       side for 96 px of artwork. Above it the margin grows by an eighth of the
       artwork's growth, so large icons do not sit in a thick slab. */
    if (artwork <= KNEE) margin = (artwork*31+48)/96;
    else margin = (KNEE*31+48)/96+(artwork-KNEE+4)/8;
    return artwork+2*margin;
}

void dock_panel_cross_bounds(const DockSettings *s, int extent, int *inner, int *outer)
{
    int thickness = dock_panel_thickness(s);
    int gap = DOCK_DESKTOP_GAP+s->edge_padding;
    *outer = extent > gap ? extent-gap : 0;
    *inner = *outer > thickness ? *outer-thickness : 0;
}

static void to_virtual(DockEdge edge, int cross, const DockPlacement *actual,
                       DockPlacement *virtual, int count)
{
    int i;
    for (i = 0; i < count; ++i) {
        DockPlacement p = actual[i];
        virtual[i] = p;
        virtual[i].x = p.y;
        virtual[i].y = edge == DOCK_EDGE_LEFT ?
            dock_saturate_int((int64_t)cross-p.x-p.size) : p.x;
    }
}

static void from_virtual(DockEdge edge, int cross, const DockPlacement *virtual,
                         DockPlacement *actual, int count)
{
    int i;
    for (i = 0; i < count; ++i) {
        DockPlacement p = virtual[i];
        actual[i] = p;
        actual[i].x = edge == DOCK_EDGE_LEFT ?
            dock_saturate_int((int64_t)cross-p.y-p.size) : p.y;
        actual[i].y = p.x;
    }
}

/* Reflection is on pixel centers here; rectangle edges have a different
   half-open transform below. Saturation keeps extreme outside-window mouse
   coordinates representable without signed integer overflow. */
static void pointer_virtual(DockEdge edge, int width, int x, int y,
                            int *vx, int *vy)
{
    *vx = y;
    *vy = edge == DOCK_EDGE_LEFT ? dock_saturate_int((int64_t)width-1-x) : x;
}

int dock_edge_layout(const DockSettings *s, DockEdge edge, int count,
    int width, int height, int mx, int my, int active, DockPlacement *out)
{
    int vx, vy;
    if (edge == DOCK_EDGE_BOTTOM)
        return dock_layout(s, count, width, height, mx, my, active, out);
    if (!side_edge(edge) || !out || !valid(s, edge, out, count, width, height)) return 0;
    pointer_virtual(edge, width, mx, my, &vx, &vy);
    if (!dock_layout(s, count, height, width, vx, vy, active, out)) return 0;
    from_virtual(edge, width, out, out, count);
    return 1;
}

int dock_edge_animate(const DockSettings *s, DockEdge edge, DockAnimation *animation,
    DockPlacement *current, const DockPlacement *target, int count)
{
    DockPlacement vc[DOCK_MAX_ITEMS], vt[DOCK_MAX_ITEMS];
    int changed;
    if (edge == DOCK_EDGE_BOTTOM)
        return dock_animate_step(s, animation, current, target, count);
    if (!side_edge(edge) || !dock_settings_valid(s) || !animation || !current || !target ||
        count < 1 || count > DOCK_MAX_ITEMS) return 0;
    /* Use cross origin zero for LEFT. The omitted actual width is a constant
       cross-axis translation and cancels from the target baseline. The core
       permits negative cross coordinates and animates only long-axis centers. */
    to_virtual(edge, 0, current, vc, count);
    to_virtual(edge, 0, target, vt, count);
    changed = dock_animate_step(s, animation, vc, vt, count);
    from_virtual(edge, 0, vc, current, count);
    return changed;
}

int dock_edge_hover(const DockSettings *s, DockEdge edge, const DockPlacement *p,
    int count, int width, int height, int x, int y)
{
    DockPlacement virtual[DOCK_MAX_ITEMS];
    int vx, vy;
    if (edge == DOCK_EDGE_BOTTOM)
        return dock_hover_contains(s, p, count, width, height, x, y);
    if (!side_edge(edge) || !valid(s, edge, p, count, width, height)) return 0;
    to_virtual(edge, width, p, virtual, count);
    pointer_virtual(edge, width, x, y, &vx, &vy);
    return dock_hover_contains(s, virtual, count, height, width, vx, vy);
}

static int near_rect(int x, int y, int64_t left, int64_t top,
    int64_t right, int64_t bottom, int margin)
{
    return (int64_t)x >= left-margin && (int64_t)x < right+margin &&
        (int64_t)y >= top-margin && (int64_t)y < bottom+margin;
}

/* Rectangles are axis aligned and the margin is the same on every side, so
   the test needs no turn into bottom coordinates: each edge measures from
   what it draws. */
int dock_edge_outside(const DockSettings *s, DockEdge edge, const DockPlacement *p,
    int count, int width, int height, const DockRect *include, int x, int y, int margin)
{
    DockRect r;
    int i;
    if (margin < 0 || !dock_edge_panel_reach(s, edge, p, count, width, height, include, &r)) return 0;
    if (near_rect(x, y, r.left, r.top, r.right, r.bottom, margin)) return 0;
    for (i = 0; i < count; ++i)
        if (near_rect(x, y, p[i].x, p[i].y,
                (int64_t)p[i].x+p[i].size, (int64_t)p[i].y+p[i].size, margin)) return 0;
    return 1;
}

/* anywhere: the slot from the position along the row alone, for every
   point (no drop band, row ends or canvas bounds). */
static int insertion_slot(const DockSettings *s, DockEdge edge,
    int count, int width, int height, int x, int y, int anywhere)
{
    int pitch, start, span, expanded_start, slot, inner, outer;
    if (!dock_settings_valid(s) || count < 0 || count > DOCK_MAX_ITEMS ||
        (edge != DOCK_EDGE_BOTTOM && !side_edge(edge)) || width < 1 || height < 1 ||
        (!anywhere && (x < 0 || x >= width || y < 0 || y >= height))) return -1;
    if (side_edge(edge)) {
        int vx, vy, swap = width;
        pointer_virtual(edge, width, x, y, &vx, &vy);
        x = vx; y = vy; width = height; height = swap;
    }
    dock_panel_cross_bounds(s,height,&inner,&outer);
    if (!anywhere && (y < inner || y >= outer)) return -1;
    if (!count) return anywhere || (x >= s->padding && x < width-s->padding) ? 0 : -1;
    pitch = s->base_size+s->gap;
    span = dock_row_length(s,count);
    expanded_start = (width-span-pitch)/2;
    if (!anywhere && (x < expanded_start-s->padding || x >= expanded_start+span+pitch+s->padding))
        return -1;
    start = (width-span)/2;
    for (slot = 0; slot < count; ++slot)
        if (x < start+slot*pitch+s->base_size/2+(slot && slot == count-1 ? s->end_gap : 0)) break;
    return slot;
}

int dock_edge_insertion_slot(const DockSettings *s, DockEdge edge,
    int count, int width, int height, int x, int y)
{
    return insertion_slot(s, edge, count, width, height, x, y, 0);
}

int dock_edge_row_slot(const DockSettings *s, DockEdge edge,
    int count, int width, int height, int x, int y)
{
    return insertion_slot(s, edge, count, width, height, x, y, 1);
}

int dock_edge_gap_layout(const DockSettings *s, DockEdge edge,
    int count, int width, int height, int slot, int x, int y, int active,
    DockPlacement *out, DockPlacement *gap)
{
    DockPlacement expanded[DOCK_MAX_ITEMS];
    int i;
    if (!out || count < 1 || count >= DOCK_MAX_ITEMS || slot < 0 || slot > count ||
        !dock_edge_layout(s,edge,count+1,width,height,x,y,active,expanded)) return 0;
    for (i = 0; i < count; ++i) out[i] = expanded[i+(i >= slot)];
    if (gap) *gap = expanded[slot];
    return 1;
}

int dock_edge_insertion_layout(const DockSettings *s, DockEdge edge,
    int count, int width, int height, int slot, DockPlacement *out)
{
    return dock_edge_gap_layout(s,edge,count,width,height,slot,0,0,0,out,NULL);
}

int dock_edge_panel_rect(const DockSettings *s, DockEdge edge, const DockPlacement *p,
    int count, int width, int height, DockRect *out)
{
    DockRect r;
    int long_axis, cross_axis;
    if (!out || !valid(s, edge, p, count, width, height)) return 0;
    long_axis = edge == DOCK_EDGE_BOTTOM ? width : height;
    cross_axis = edge == DOCK_EDGE_BOTTOM ? height : width;
    if (count) {
        const DockPlacement *last = &p[count-1];
        int first_pos = edge == DOCK_EDGE_BOTTOM ? p[0].x : p[0].y;
        int last_pos = edge == DOCK_EDGE_BOTTOM ? last->x : last->y;
        r.left = dock_clamp(dock_saturate_int((int64_t)first_pos-s->padding), 0, long_axis);
        r.right = dock_clamp(dock_saturate_int((int64_t)last_pos+last->size+s->padding), 0, long_axis);
    } else {
        r.left = dock_clamp(s->padding, 0, long_axis);
        r.right = dock_clamp(long_axis-s->padding, 0, long_axis);
    }
    dock_panel_cross_bounds(s,cross_axis,&r.top,&r.bottom);
    if (r.right < r.left) r.right = r.left;
    if (r.bottom < r.top) r.bottom = r.top;
    if (edge == DOCK_EDGE_LEFT) {
        out->left = width-r.bottom; out->right = width-r.top;
        out->top = r.left; out->bottom = r.right;
    } else if (edge == DOCK_EDGE_RIGHT) {
        out->left = r.top; out->right = r.bottom;
        out->top = r.left; out->bottom = r.right;
    } else *out = r;
    return 1;
}

int dock_edge_panel_reach(const DockSettings *s, DockEdge edge, const DockPlacement *p,
    int count, int width, int height, const DockRect *include, DockRect *out)
{
    DockRect r;
    if (!dock_edge_panel_rect(s, edge, p, count, width, height, &r)) return 0;
    if (include && include->left < include->right && include->top < include->bottom) {
        /* An empty slot (the gap of a drag) has no artwork to extend the
           plate: reach it with the padding the icons get. Mirroring the row
           instead overshot where the zoomed row is not centred (IMG_2594). */
        if (edge == DOCK_EDGE_BOTTOM) {
            int64_t low = (int64_t)include->left-s->padding, high = (int64_t)include->right+s->padding;
            if (low < r.left) r.left = dock_clamp(dock_saturate_int(low), 0, width);
            if (high > r.right) r.right = dock_clamp(dock_saturate_int(high), 0, width);
        } else {
            int64_t low = (int64_t)include->top-s->padding, high = (int64_t)include->bottom+s->padding;
            if (low < r.top) r.top = dock_clamp(dock_saturate_int(low), 0, height);
            if (high > r.bottom) r.bottom = dock_clamp(dock_saturate_int(high), 0, height);
        }
    }
    *out = r;
    return 1;
}

void dock_gap_reset(DockGap *g)
{
    if (!g) return;
    memset(g, 0, sizeof(*g));
    g->slot = -1;
}

void dock_gap_open(DockGap *g, int slot, const DockPlacement *gap, const DockPlacement *from)
{
    if (!g || !gap || slot < 0) return;
    if (g->slot < 0) {
        if (!from) from = gap;
        g->shown = *from;
        dock_animation_reset(&g->animation);
    }
    g->slot = slot; g->target = *gap;
}

void dock_gap_close(DockGap *g, const DockPlacement *row, int count)
{
    if (!g || g->slot < 0) return;
    if (!row || count < 1) { dock_gap_reset(g); return; }
    g->target = row[g->slot < count ? g->slot : count-1];
}

int dock_gap_moving(const DockGap *g)
{
    return g && g->slot >= 0 && memcmp(&g->shown, &g->target, sizeof(g->shown)) != 0;
}

int dock_gap_step(const DockSettings *s, DockEdge edge, DockGap *g)
{
    if (!dock_gap_moving(g)) return 0;
    /* Out-of-range levels cannot animate: show the target, never stall. */
    if (!dock_settings_valid(s) || g->shown.level < 0 || g->shown.level >= s->levels ||
        g->target.level < 0 || g->target.level >= s->levels) {
        g->shown = g->target;
        return 1;
    }
    return dock_edge_animate(s, edge, &g->animation, &g->shown, &g->target, 1);
}

DockRect dock_gap_rect(const DockGap *g)
{
    DockRect r = {0, 0, 0, 0};
    if (g && g->slot >= 0)
        r = (DockRect){g->shown.x, g->shown.y, g->shown.x+g->shown.size, g->shown.y+g->shown.size};
    return r;
}
