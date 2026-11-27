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
    wanted = count*pitch+2*s->max_size+2*s->padding+s->base_size;
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
        if (x < start+slot*pitch+s->base_size/2) break;
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

static void button_segment(DockButtonGlyph *g, int x0, int y0, int x1, int y1)
{
    int *segment = g->segment[g->count++];
    segment[0] = g->cx+x0; segment[1] = g->cy+y0;
    segment[2] = g->cx+x1; segment[3] = g->cy+y1;
}

void dock_button_glyph(int x, int y, int width, int height, int direction, DockButtonGlyph *out)
{
    /* At most 96 px: the 32-bit distances stay in range (fonts give 18). */
    int size = width < height ? (width < 96 ? width : 96) : (height < 96 ? height : 96);
    /* 5/6 of the control, but a 12 px disc where the control has room. */
    int radius = (size*40+3)/6, least = size*8 < 96 ? size*8 : 96;
    if (radius < least) radius = least;
    out->cx = x*16+width*8;
    out->cy = y*16+height*8;
    out->radius = radius;
    /* The stroke shrinks with the glyph: 5/6 of its old 13. */
    out->half = 11;
    out->count = 0;
    if (direction == DOCK_BUTTON_PARENT) {
        /* On a grid of u (one pixel at the box's 19 px control): stem at
           x 3 from y 4 up to 0, a quarter turn of radius 2 about (1, 0)
           into the bar at y -2, the bar to x -3, the head's arms from
           (-1, -4) and (-1, 0). Its box spans -3..3 by -4..4: centred on
           the disc exactly, as every control glyph is (by its box, never
           by an optical offset). One pixel lines (stroke half 9/16 px)
           keep the head crisp at the real size. */
        static const int turn[5][2] = {{1000,0}, {924,383}, {707,707}, {383,924}, {0,1000}};
        int u = (radius*16+63)/127, i;
        out->half = 9;
        button_segment(out,3*u,4*u,3*u,0);
        for (i = 0; i < 4; ++i)
            button_segment(out,u+2*u*turn[i][0]/1000,-2*u*turn[i][1]/1000,
                u+2*u*turn[i+1][0]/1000,-2*u*turn[i+1][1]/1000);
        button_segment(out,u,-2*u,-3*u,-2*u);
        button_segment(out,-u,-4*u,-3*u,-2*u);
        button_segment(out,-u,0,-3*u,-2*u);
    } else {
        int sign = direction < 0 ? -1 : 1, reach = radius*3/16, arm = radius*3/8;
        button_segment(out,-sign*reach,-arm,sign*reach,0);
        button_segment(out,-sign*reach,arm,sign*reach,0);
    }
    {
        int i, k;
        out->ink[0] = out->ink[1] = 0x7fffffff;
        out->ink[2] = out->ink[3] = -0x7fffffff;
        for (i = 0; i < out->count; ++i) for (k = 0; k < 4; ++k) {
            int c = k&1, v = out->segment[i][k];
            if (v-out->half < out->ink[c]) out->ink[c] = v-out->half;
            if (v+out->half > out->ink[c+2]) out->ink[c+2] = v+out->half;
        }
    }
}

/* Squared distance from (px,py) to the segment (x0,y0)-(x1,y1). */
static int segment_distance2(int px, int py, int x0, int y0, int x1, int y1)
{
    int dx = x1-x0, dy = y1-y0, ex = px-x0, ey = py-y0;
    int length = dx*dx+dy*dy, along = ex*dx+ey*dy;
    if (along > 0 && length > 0) {
        if (along >= length) { ex = px-x1; ey = py-y1; }
        else { ex -= dx*along/length; ey -= dy*along/length; }
    }
    return ex*ex+ey*ey;
}

int dock_button_coverage(const DockButtonGlyph *g, int px, int py, int *disc)
{
    int stroke = 0, inside = 0, sx, sy, i, half2 = g->half*g->half, radius2 = g->radius*g->radius;
    /* Only a pixel that reaches the ink's box tests the segments. */
    int near = px*16+16 > g->ink[0] && px*16 <= g->ink[2] && py*16+16 > g->ink[1] && py*16 <= g->ink[3];
    for (sy = 0; sy < 4; ++sy) for (sx = 0; sx < 4; ++sx) {
        int qx = px*16+2+4*sx, qy = py*16+2+4*sy, dx = qx-g->cx, dy = qy-g->cy;
        if (dx*dx+dy*dy <= radius2) ++inside;
        if (near) for (i = 0; i < g->count; ++i) {
            const int *s = g->segment[i];
            if (segment_distance2(qx,qy,s[0],s[1],s[2],s[3]) <= half2) { ++stroke; break; }
        }
    }
    if (disc) *disc = inside;
    return stroke;
}

void dock_button_pixels(int x, int y, int width, int height, int direction, DockButtonPixels *out)
{
    int size = width < height ? width : height;
    int inset = size >= 12 ? (size+6)/12 : 0, drawn = size-2*inset, arm = drawn/5;
    int left = x+(width-size)/2, top = y+(height-size)/2, middle = size/2, i, k;
    out->left = left+inset;
    out->top = top+inset;
    out->size = drawn;
    if (arm < 1) arm = 1;
    if (direction == DOCK_BUTTON_PARENT) {
        /* The return arrow on a grid of n pixels about the middle pixel,
           as in dock_button_glyph: stem up from (3, 4) to (3, 0), a 45
           degree turn into the bar at y -2, the bar to the tip at x -3, the
           head's arms from (-1, -4) and (-1, 0). Below a 15 px control it
           reaches the disc's edge. */
        static const int lines[3][3][2] = {
            {{3,4}, {3,0}, {1,-2}}, {{1,-2}, {-3,-2}, {-3,-2}}, {{-1,-4}, {-3,-2}, {-1,0}}};
        int n = drawn/15 > 1 ? drawn/15 : 1;
        out->count = 3;
        for (i = 0; i < 3; ++i) for (k = 0; k < 3; ++k) {
            out->stroke[i][k][0] = left+middle+n*lines[i][k][0];
            out->stroke[i][k][1] = top+middle+n*lines[i][k][1];
        }
        return;
    }
    /* The chevron spans arm+2 columns: the same parity as the disc's
       centres it exactly. Pointing right, its tip is at column lead. */
    if ((drawn+arm)&1) ++arm;
    out->count = 2;
    for (i = 0; i < 2; ++i) {
        /* Right: (lead-(arm-i), middle-arm) -> (lead+i, middle) ->
           (lead-(arm-i), middle+arm); left mirrors it in the control. */
        int lead = (size+arm-2)/2;
        int along[3] = {lead-(arm-i), lead+i, lead-(arm-i)}, across[3] = {middle-arm, middle, middle+arm};
        for (k = 0; k < 3; ++k) {
            out->stroke[i][k][0] = left+(direction < 0 ? size-1-along[k] : along[k]);
            out->stroke[i][k][1] = top+across[k];
        }
    }
}

/* The arrow's two parts in 1/64 of the ring's radius, centred on the ring
   by their joint bounding box (x -34..34, y -28..28). */
const signed char dock_open_arrow_tail[DOCK_OPEN_TAIL][2] = {
    {3,-12}, {-9,-11}, {-18,-8}, {-25,-3}, {-30,5}, {-33,15},
    {-34,25}, {-32,28}, {-29,27}, {-25,21}, {-20,16}, {-13,13}, {3,11}
};
const signed char dock_open_arrow_head[DOCK_OPEN_HEAD][2] = {
    {0,-25}, {3,-28}, {6,-27}, {32,-3}, {34,0}, {32,4},
    {6,27}, {3,28}, {0,26}
};

void dock_open_glyph(int left, int top, int icon, int radius, int thickness, DockOpenGlyph *out)
{
    out->cx = left*16+icon*8;
    out->cy = top*16+icon*8;
    out->radius = radius;
    /* thickness native pixels plus half a pixel wide, so the ring keeps
       its weight once antialiased. */
    out->outer = radius*16+8;
    out->inner = out->outer-thickness*16-8;
}

/* An outline coordinate in 1/16 px, on a multiple of 4 rounded toward
   zero: samples lie at 2 mod 4, so none falls on a vertex and the
   half-open crossing rule treats both sides of the centre alike. */
static int outline_at(int value, int radius)
{
    int v = value*radius/4;
    return v < 0 ? -(-v & ~3) : v & ~3;
}

/* The span [*left,*right] where the row y (1/16 px from the centre)
   crosses an outline; positions in 1/16 px. */
static int outline_span(const signed char (*outline)[2], int count, int radius, int y, int *left, int *right)
{
    int i, previous, found = 0;
    for (i = 0, previous = count-1; i < count; previous = i++) {
        int y1 = outline_at(outline[previous][1],radius), y2 = outline_at(outline[i][1],radius);
        if ((y1 > y) != (y2 > y)) {
            int x1 = outline_at(outline[previous][0],radius), x2 = outline_at(outline[i][0],radius);
            int hit = x1+(x2-x1)*(y-y1)/(y2-y1);
            if (!found || hit < *left) *left = hit;
            if (!found || hit > *right) *right = hit;
            found = 1;
        }
    }
    return found;
}

static int isqrt(int value)
{
    int root = 0, bit = 1<<28;
    if (value <= 0) return 0;
    while (bit > value) bit >>= 2;
    while (bit) {
        if (value >= root+bit) { value -= root+bit; root = (root>>1)+bit; }
        else root >>= 1;
        bit >>= 2;
    }
    return root;
}

void dock_open_glyph_row(const DockOpenGlyph *g, int py, int px0, int count, unsigned char *covered)
{
    int sy, x;
    memset(covered,0,(size_t)count);
    for (sy = 0; sy < 4; ++sy) {
        int dy = py*16+2+4*sy-g->cy, ring_out = -1, ring_in = -1, sx;
        int tail_l = 0, tail_r = -1, head_l = 0, head_r = -1;
        if (dy*dy <= g->outer*g->outer) ring_out = isqrt(g->outer*g->outer-dy*dy);
        if (dy*dy < g->inner*g->inner) ring_in = isqrt(g->inner*g->inner-dy*dy);
        if (!outline_span(dock_open_arrow_tail,DOCK_OPEN_TAIL,g->radius,dy,&tail_l,&tail_r)) tail_r = tail_l-1;
        if (!outline_span(dock_open_arrow_head,DOCK_OPEN_HEAD,g->radius,dy,&head_l,&head_r)) head_r = head_l-1;
        for (x = 0; x < count; ++x) for (sx = 0; sx < 4; ++sx) {
            int dx = (px0+x)*16+2+4*sx-g->cx, ax = dx < 0 ? -dx : dx;
            if ((ax <= ring_out && ax >= ring_in) || (dx >= tail_l && dx <= tail_r) ||
                (dx >= head_l && dx <= head_r)) ++covered[x];
        }
    }
}

DockRect dock_pulldown_reach(int screen_width, int bar_height, int items_bottom)
{
    if (items_bottom < 0) items_bottom = 0;
    return (DockRect){0,0,screen_width,bar_height+1+items_bottom+DOCK_PULLDOWN_MARGIN};
}

int dock_menu_covers(DockRect dock, int pointer_x, int pointer_y, const DockRect *menus, int count)
{
    int i, dx, dy;
    if (dock.right <= dock.left || dock.bottom <= dock.top) return 0;
    if (count >= 0) {
        for (i = 0; i < count; ++i)
            if (menus[i].left < dock.right && menus[i].right > dock.left &&
                menus[i].top < dock.bottom && menus[i].bottom > dock.top) return 1;
        return 0;
    }
    dx = pointer_x < dock.left ? dock.left-pointer_x : pointer_x >= dock.right ? pointer_x-dock.right+1 : 0;
    dy = pointer_y < dock.top ? dock.top-pointer_y : pointer_y >= dock.bottom ? pointer_y-dock.bottom+1 : 0;
    return dock.top < DOCK_MENU_REACH || (dx < DOCK_MENU_REACH && dy < DOCK_MENU_REACH);
}
