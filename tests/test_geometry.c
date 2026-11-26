#include "dock_geometry.h"
#include "dock_compose.h"
#include "dock_interaction.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

enum { LONG = 640, CROSS = 96, N = 9 };

static void actual_point(DockEdge edge, int vx, int vy, int *x, int *y)
{
    if (edge == DOCK_EDGE_BOTTOM) { *x = vx; *y = vy; }
    else {
        *x = edge == DOCK_EDGE_LEFT ? CROSS-1-vy : vy;
        *y = vx;
    }
}

static void actual_boxes(DockEdge edge, const DockPlacement *v, DockPlacement *p, int count)
{
    int i;
    for (i = 0; i < count; ++i) {
        p[i] = v[i];
        if (edge != DOCK_EDGE_BOTTOM) {
            p[i].x = edge == DOCK_EDGE_LEFT ? CROSS-v[i].y-v[i].size : v[i].y;
            p[i].y = v[i].x;
        }
    }
}

static void dimensions(DockEdge edge, int *w, int *h)
{
    *w = edge == DOCK_EDGE_BOTTOM ? LONG : CROSS;
    *h = edge == DOCK_EDGE_BOTTOM ? CROSS : LONG;
}

static void test_layout_and_baseline(void)
{
    DockSettings s = dock_settings_default();
    DockPlacement rest[N], zoom[N], expected[N], p[N];
    DockRect panel;
    int edge, i, w, h, x, y, focus, baseline;
    assert(dock_layout(&s,N,LONG,CROSS,0,0,0,rest));
    focus = rest[4].x+s.base_size/2;
    assert(dock_layout(&s,N,LONG,CROSS,focus,70,1,zoom));
    assert(zoom[4].size == s.max_size);
    baseline = rest[0].y+rest[0].size;
    for (edge = DOCK_EDGE_BOTTOM; edge <= DOCK_EDGE_RIGHT; ++edge) {
        dimensions((DockEdge)edge,&w,&h);
        actual_point((DockEdge)edge,focus,70,&x,&y);
        assert(dock_edge_layout(&s,(DockEdge)edge,N,w,h,x,y,0,p));
        actual_boxes((DockEdge)edge,rest,expected,N);
        assert(!memcmp(p,expected,sizeof(p)));
        assert(dock_edge_panel_rect(&s,(DockEdge)edge,p,N,w,h,&panel));
        if (edge == DOCK_EDGE_BOTTOM) assert(2*p[0].y+p[0].size == panel.top+panel.bottom);
        else assert(2*p[0].x+p[0].size == panel.left+panel.right);
        assert(dock_edge_layout(&s,(DockEdge)edge,N,w,h,x,y,1,p));
        actual_boxes((DockEdge)edge,zoom,expected,N);
        assert(!memcmp(p,expected,sizeof(p)));
        for (i = 0; i < N; ++i) {
            assert(p[i].size == zoom[i].size && p[i].level == zoom[i].level);
            if (edge == DOCK_EDGE_LEFT) assert(p[i].x == CROSS-baseline);
            else if (edge == DOCK_EDGE_RIGHT) assert(p[i].x+p[i].size == baseline);
            else assert(p[i].y+p[i].size == baseline);
            assert(dock_hit_test(p,N,p[i].x+p[i].size/2,p[i].y+p[i].size/2) == i);
        }
    }
}

static void test_interaction(void)
{
    DockSettings s = dock_settings_default();
    DockPlacement v[N], p[N];
    int edge, vx, vy, x, y, w, h;
    assert(dock_layout(&s,N,LONG,CROSS,350,70,1,v));
    for (edge = DOCK_EDGE_BOTTOM; edge <= DOCK_EDGE_RIGHT; ++edge) {
        dimensions((DockEdge)edge,&w,&h);
        actual_boxes((DockEdge)edge,v,p,N);
        for (vx = -7; vx < LONG+8; vx += 11) for (vy = -7; vy < CROSS+8; vy += 7) {
            actual_point((DockEdge)edge,vx,vy,&x,&y);
            assert(dock_edge_hover(&s,(DockEdge)edge,p,N,w,h,x,y) ==
                dock_hover_contains(&s,v,N,LONG,CROSS,vx,vy));
            assert(dock_edge_outside(&s,(DockEdge)edge,p,N,w,h,NULL,x,y,5) ==
                dock_interaction_outside(&s,v,N,LONG,CROSS,vx,vy,5));
        }
        assert(dock_edge_outside(&s,(DockEdge)edge,p,N,w,h,NULL,INT_MIN,INT_MAX,0));
        assert(!dock_edge_hover(&s,(DockEdge)edge,p,N,w,h,INT_MIN,INT_MAX));
    }
}

static void test_panel_rect(void)
{
    DockSettings s = dock_settings_default();
    DockPlacement v[N], p[N];
    DockRect bottom, r;
    int edge, w, h;
    assert(dock_layout(&s,N,LONG,CROSS,350,70,1,v));
    assert(dock_edge_panel_rect(&s,DOCK_EDGE_BOTTOM,v,N,LONG,CROSS,&bottom));
    assert(bottom.left == v[0].x-s.padding);
    assert(bottom.right == v[N-1].x+v[N-1].size+s.padding);
    assert(bottom.bottom-bottom.top == 46);
    assert(CROSS-bottom.bottom == 3);
    for (edge = DOCK_EDGE_LEFT; edge <= DOCK_EDGE_RIGHT; ++edge) {
        dimensions((DockEdge)edge,&w,&h);
        actual_boxes((DockEdge)edge,v,p,N);
        assert(dock_edge_panel_rect(&s,(DockEdge)edge,p,N,w,h,&r));
        assert(r.top == bottom.left && r.bottom == bottom.right);
        assert(r.left == (edge == DOCK_EDGE_LEFT ? CROSS-bottom.bottom : bottom.top));
        assert(r.right == (edge == DOCK_EDGE_LEFT ? CROSS-bottom.top : bottom.bottom));
        assert(r.left >= 0 && r.right <= w && r.top >= 0 && r.bottom <= h);
        assert((edge == DOCK_EDGE_LEFT ? r.left : w-r.right) == 3);
        {
            int y = (r.top+r.bottom)/2;
            int outer = edge == DOCK_EDGE_LEFT ? r.left : r.right-1;
            int beyond = edge == DOCK_EDGE_LEFT ? r.left-1 : r.right;
            assert(dock_edge_hover(&s,(DockEdge)edge,p,N,w,h,outer,y));
            assert(!dock_edge_hover(&s,(DockEdge)edge,p,N,w,h,beyond,y));
        }
    }
    assert(dock_edge_panel_rect(&s,DOCK_EDGE_BOTTOM,NULL,0,1,1,&r));
    assert(r.left >= 0 && r.left <= r.right && r.right <= 1);
    assert(r.top >= 0 && r.top <= r.bottom && r.bottom <= 1);
    s.edge_padding = 5;
    assert(dock_edge_layout(&s,DOCK_EDGE_RIGHT,N,CROSS,LONG,0,0,0,p));
    assert(dock_edge_panel_rect(&s,DOCK_EDGE_RIGHT,p,N,CROSS,LONG,&r));
    assert(CROSS-r.right == 8);
    assert(CROSS-s.edge_padding-r.right == 3);
    /* A left canvas starting at the gap (SAGA cannot start a PiP at x 0):
       the plate begins at its first column, still 3 px from the screen edge. */
    s.edge_padding = -DOCK_DESKTOP_GAP;
    assert(dock_settings_valid(&s));
    assert(dock_edge_layout(&s,DOCK_EDGE_LEFT,N,CROSS,LONG,0,0,0,p));
    assert(dock_edge_panel_rect(&s,DOCK_EDGE_LEFT,p,N,CROSS,LONG,&r));
    assert(r.left == 0);
    s.edge_padding = -DOCK_DESKTOP_GAP-1;
    assert(!dock_settings_valid(&s));
}

static void test_animation(void)
{
    DockSettings s = dock_settings_default();
    DockPlacement initial[N], reference[N], target[N], current[N], actual_target[N], expected[N];
    DockAnimation core, oriented;
    int edge, tick, w, h;
    assert(dock_layout(&s,N,LONG,CROSS,0,0,0,initial));
    for (edge = DOCK_EDGE_BOTTOM; edge <= DOCK_EDGE_RIGHT; ++edge) {
        dimensions((DockEdge)edge,&w,&h);
        memcpy(reference,initial,sizeof(initial));
        actual_boxes((DockEdge)edge,initial,current,N);
        dock_animation_reset(&core); dock_animation_reset(&oriented);
        for (tick = 0; tick < 140; ++tick) {
            int active = tick < 7 || (tick >= 11 && tick < 90);
            int focus = tick < 30 ? 350 : tick < 60 ? 230 : 450;
            int changed;
            assert(dock_layout(&s,N,LONG,CROSS,focus,70,active,target));
            actual_boxes((DockEdge)edge,target,actual_target,N);
            changed = dock_animate_step(&s,&core,reference,target,N);
            assert(dock_edge_animate(&s,(DockEdge)edge,&oriented,current,actual_target,N) == changed);
            actual_boxes((DockEdge)edge,reference,expected,N);
            assert(!memcmp(current,expected,sizeof(current)));
            assert(!memcmp(core.center,oriented.center,N*sizeof(*core.center)));
            if (edge == DOCK_EDGE_BOTTOM) assert(!memcmp(&core,&oriented,sizeof(core)));
        }
        assert(!memcmp(current,actual_target,sizeof(current)));
        assert(!dock_edge_animate(&s,(DockEdge)edge,&oriented,current,actual_target,N));
    }
}

static void test_counts_and_invalid(void)
{
    DockSettings s = dock_settings_default();
    struct { unsigned int before; DockPlacement p[DOCK_MAX_ITEMS]; unsigned int after; } guarded;
    DockPlacement target[DOCK_MAX_ITEMS];
    DockAnimation animation;
    DockRect r;
    int edge, count;
    guarded.before = 0x12345678U; guarded.after = 0x87654321U;
    for (edge = DOCK_EDGE_BOTTOM; edge <= DOCK_EDGE_RIGHT; ++edge) {
        for (count = 1; count <= DOCK_MAX_ITEMS; count += count == 1 ? 31 : 1) {
            int w = edge == DOCK_EDGE_BOTTOM ? 1400 : CROSS;
            int h = edge == DOCK_EDGE_BOTTOM ? CROSS : 1400;
            int step;
            assert(dock_edge_layout(&s,(DockEdge)edge,count,w,h,w/2,h/2,0,guarded.p));
            assert(dock_edge_layout(&s,(DockEdge)edge,count,w,h,w/2,h/2,1,target));
            dock_animation_reset(&animation);
            for (step = 0; step < 64 && memcmp(guarded.p,target,(size_t)count*sizeof(*target)); ++step)
                dock_edge_animate(&s,(DockEdge)edge,&animation,guarded.p,target,count);
            assert(!memcmp(guarded.p,target,(size_t)count*sizeof(*target)));
            assert(dock_edge_panel_rect(&s,(DockEdge)edge,guarded.p,count,w,h,&r));
        }
        assert(dock_edge_layout(&s,(DockEdge)edge,0,LONG,LONG,0,0,0,guarded.p));
    }
    assert(!dock_edge_layout(&s,(DockEdge)99,1,LONG,CROSS,20,20,1,guarded.p));
    assert(!dock_edge_layout(&s,DOCK_EDGE_LEFT,DOCK_MAX_ITEMS+1,CROSS,LONG,20,20,1,guarded.p));
    assert(!dock_edge_panel_rect(&s,(DockEdge)99,NULL,0,LONG,CROSS,&r));
    assert(guarded.before == 0x12345678U && guarded.after == 0x87654321U);
    s.max_size = s.base_size;
    assert(dock_edge_layout(&s,DOCK_EDGE_LEFT,1,CROSS,LONG,20,320,1,guarded.p));
    assert(dock_edge_panel_rect(&s,DOCK_EDGE_LEFT,guarded.p,1,CROSS,LONG,&r));
    assert(guarded.p[0].size == s.base_size);
    assert(2*guarded.p[0].x+guarded.p[0].size == r.left+r.right);
}

static void test_external_insertion(void)
{
    DockSettings s = dock_settings_default();
    DockPlacement rest[N], expanded[N+1], current[N], target[N];
    DockAnimation animation;
    int edge, w, h, slot, i, x, y, tick;
    for (edge = DOCK_EDGE_BOTTOM; edge <= DOCK_EDGE_RIGHT; ++edge) {
        dimensions((DockEdge)edge,&w,&h);
        assert(dock_edge_layout(&s,(DockEdge)edge,N,w,h,0,0,0,rest));
        assert(dock_edge_layout(&s,(DockEdge)edge,N+1,w,h,0,0,0,expanded));
        memcpy(current,rest,sizeof(current));
        dock_animation_reset(&animation);
        for (slot = 0; slot <= N; ++slot) {
            int center = (edge == DOCK_EDGE_BOTTOM ? expanded[slot].x : expanded[slot].y)+s.base_size/2;
            actual_point((DockEdge)edge,center,CROSS-s.padding-s.base_size/2,&x,&y);
            assert(dock_edge_insertion_slot(&s,(DockEdge)edge,N,w,h,x,y) == slot);
            assert(dock_edge_insertion_layout(&s,(DockEdge)edge,N,w,h,slot,target));
            for (i = 0; i < N; ++i)
                assert(!memcmp(&target[i],&expanded[i+(i >= slot)],sizeof(*target)));
            for (tick = 0; tick < 32 && memcmp(current,target,sizeof(current)); ++tick) {
                dock_edge_animate(&s,(DockEdge)edge,&animation,current,target,N);
                for (i = 1; i < N; ++i) {
                    int prev = edge == DOCK_EDGE_BOTTOM ? current[i-1].x : current[i-1].y;
                    int next = edge == DOCK_EDGE_BOTTOM ? current[i].x : current[i].y;
                    assert(next >= prev+current[i-1].size);
                }
            }
            assert(tick > 1 && !memcmp(current,target,sizeof(current)));
        }
        for (tick = 0; tick < 32 && memcmp(current,rest,sizeof(current)); ++tick)
            dock_edge_animate(&s,(DockEdge)edge,&animation,current,rest,N);
        assert(!memcmp(current,rest,sizeof(current)));
        actual_point((DockEdge)edge,LONG/2,0,&x,&y);
        assert(dock_edge_insertion_slot(&s,(DockEdge)edge,N,w,h,x,y) == -1);
        actual_point((DockEdge)edge,LONG/2,CROSS-s.padding-s.base_size/2,&x,&y);
        assert(dock_edge_insertion_slot(&s,(DockEdge)edge,0,w,h,x,y) == 0);
        assert(!dock_edge_insertion_layout(&s,(DockEdge)edge,DOCK_MAX_ITEMS,w,h,0,target));
    }
    assert(!dock_edge_insertion_layout(&s,DOCK_EDGE_BOTTOM,3,140,CROSS,2,target));
}

static void test_zoomed_reorder_gap(void)
{
    DockSettings s = dock_settings_default();
    DockPlacement full[N], neighbors[N-1], gap;
    int edge, w, h, x, y, slot, i;
    for (edge = DOCK_EDGE_BOTTOM; edge <= DOCK_EDGE_RIGHT; ++edge) {
        dimensions((DockEdge)edge,&w,&h);
        for (slot = 0; slot < N; ++slot) {
            int span = N*(s.base_size+s.gap)-s.gap;
            int center = (LONG-span)/2+slot*(s.base_size+s.gap)+s.base_size/2;
            actual_point((DockEdge)edge,center,CROSS-s.padding-s.base_size/2,&x,&y);
            assert(dock_edge_insertion_slot(&s,(DockEdge)edge,N-1,w,h,x,y) == slot);
            assert(dock_edge_row_slot(&s,(DockEdge)edge,N-1,w,h,x,y) == slot);
            /* Above the plate, off the drop band: the same slot along the row. */
            actual_point((DockEdge)edge,center,0,&x,&y);
            assert(dock_edge_insertion_slot(&s,(DockEdge)edge,N-1,w,h,x,y) == -1);
            assert(dock_edge_row_slot(&s,(DockEdge)edge,N-1,w,h,x,y) == slot);
            actual_point((DockEdge)edge,center,CROSS-s.padding-s.base_size/2,&x,&y);
            assert(dock_edge_layout(&s,(DockEdge)edge,N,w,h,x,y,1,full));
            assert(dock_edge_gap_layout(&s,(DockEdge)edge,N-1,w,h,slot,x,y,1,neighbors,&gap));
            assert(gap.level > 0 && !memcmp(&gap,&full[slot],sizeof(gap)));
            for (i = 0; i < N-1; ++i)
                assert(!memcmp(&neighbors[i],&full[i+(i >= slot)],sizeof(*neighbors)));
            {
                /* The plate reaches over the gap with the icons' padding and
                   no further, at the ends as in the middle (IMG_2594). */
                DockRect include = {gap.x, gap.y, gap.x+gap.size, gap.y+gap.size}, panel;
                int radius, side = edge != DOCK_EDGE_BOTTOM;
                int start = side ? full[0].y : full[0].x;
                int end = (side ? full[N-1].y : full[N-1].x)+full[N-1].size;
                assert(dock_render_panel(&s,edge,neighbors,N-1,w,h,&include,&panel,&radius));
                assert((side ? panel.top : panel.left) == start-s.padding);
                assert((side ? panel.bottom : panel.right) == end+s.padding);
            }
        }
        /* Past either end, even off the canvas: the end slots. */
        actual_point((DockEdge)edge,-50,0,&x,&y);
        assert(dock_edge_row_slot(&s,(DockEdge)edge,N-1,w,h,x,y) == 0);
        actual_point((DockEdge)edge,LONG+50,CROSS+20,&x,&y);
        assert(dock_edge_row_slot(&s,(DockEdge)edge,N-1,w,h,x,y) == N-1);
    }
}

static void test_zoom_room_after_insertion(void)
{
    DockSettings s = dock_settings_default();
    DockPlacement small[8], grown[8];
    int i, small_peak = 0, grown_peak = 0;
    int old_length, reserved_length, needed_length;
    s.max_size = 70;
    old_length = dock_viewport_length(&s,5);
    reserved_length = dock_viewport_length(&s,7);
    needed_length = dock_viewport_length(&s,8);
    assert(old_length == 373 && needed_length == 484);
    assert(old_length < dock_viewport_length(&s,6));
    assert(reserved_length >= dock_viewport_length(&s,6) && reserved_length < needed_length);
    assert(dock_edge_layout(&s,DOCK_EDGE_RIGHT,8,128,old_length,110,old_length/2,0,small));
    assert(dock_edge_layout(&s,DOCK_EDGE_RIGHT,8,128,old_length,110,old_length/2,1,small));
    assert(dock_edge_layout(&s,DOCK_EDGE_RIGHT,8,128,needed_length,110,needed_length/2,1,grown));
    for (i = 0; i < 8; ++i) {
        if (small[i].size > small_peak) small_peak = small[i].size;
        if (grown[i].size > grown_peak) grown_peak = grown[i].size;
    }
    /* The macOS wave grows the row a little less, so the short row fits one
       more pixel of zoom than the previous wave (45). */
    assert(small_peak == 46 && grown_peak >= s.max_size-2);
}

static int along_low(DockEdge edge, DockRect r) { return edge == DOCK_EDGE_BOTTOM ? r.left : r.top; }
static int along_high(DockEdge edge, DockRect r) { return edge == DOCK_EDGE_BOTTOM ? r.right : r.bottom; }

/* The plate's end at one end of the row (high: the far end), as drawn. */
static int plate_end(const DockSettings *s, DockEdge edge, const DockPlacement *row, int count,
    int w, int h, const DockGap *gap, int high)
{
    DockRect include = dock_gap_rect(gap), r;
    assert(dock_edge_panel_reach(s,edge,row,count,w,h,&include,&r));
    return high ? along_high(edge,r) : along_low(edge,r);
}

/* Steps row and gap together until both settle; records the plate end each
   tick. Returns the tick count. */
static int run_gap(const DockSettings *s, DockEdge edge, DockAnimation *animation,
    DockPlacement *row, const DockPlacement *target, int count, int w, int h,
    DockGap *gap, int high, int *ends)
{
    int tick;
    for (tick = 0; memcmp(row,target,sizeof(*row)*count) || dock_gap_moving(gap); ++tick) {
        assert(tick < 32);
        dock_edge_animate(s,edge,animation,row,target,count);
        dock_gap_step(s,edge,gap);
        ends[tick] = plate_end(s,edge,row,count,w,h,gap,high);
    }
    return tick;
}

/* A gap opening or closing at either end moves the plate's end as smoothly
   as the icons move the middle: monotonic, the first step at most half the
   way (set at once it jumped there in one frame, IMG_2595), and the same at
   both ends and on every edge. */
static void test_gap_motion(void)
{
    enum { n = N-1 };
    DockSettings settings[2];
    int k, edge, high, phase, i;
    settings[0] = dock_settings_default();
    settings[1] = settings[0];
    settings[1].base_size = 36; settings[1].max_size = 68; /* ApolloOS: size 36, 190 % */
    for (k = 0; k < 2; ++k) for (edge = DOCK_EDGE_BOTTOM; edge <= DOCK_EDGE_RIGHT; ++edge) {
        const DockSettings *s = &settings[k];
        int w, h, ticks[2][2], steps[2][2][32];
        dimensions((DockEdge)edge,&w,&h);
        for (high = 0; high < 2; ++high) {
            DockPlacement row[n], rest[n], target[n], gap;
            DockAnimation animation;
            DockGap g;
            DockRect hole;
            int x, y, along, slot = high ? n : 0, end = high ? n-1 : 0, from, to;
            assert(dock_edge_layout(s,(DockEdge)edge,n,w,h,0,0,0,rest));
            memcpy(row,rest,sizeof(row));
            /* The pointer half a pitch past the end icon, on the row. */
            along = high ? (edge == DOCK_EDGE_BOTTOM ? rest[n-1].x : rest[n-1].y)+s->base_size+s->gap :
                (edge == DOCK_EDGE_BOTTOM ? rest[0].x : rest[0].y)-s->gap;
            actual_point((DockEdge)edge,along,CROSS-s->padding-s->base_size/2,&x,&y);
            assert(dock_edge_gap_layout(s,(DockEdge)edge,n,w,h,slot,x,y,1,target,&gap));
            hole = (DockRect){gap.x,gap.y,gap.x+gap.size,gap.y+gap.size};
            from = plate_end(s,(DockEdge)edge,rest,n,w,h,NULL,high);
            to = high ? along_high((DockEdge)edge,hole)+s->padding : along_low((DockEdge)edge,hole)-s->padding;
            dock_animation_reset(&animation);
            dock_gap_reset(&g);
            assert(g.slot == -1 && !dock_gap_moving(&g) && dock_gap_rect(&g).right == 0);
            dock_gap_open(&g,slot,&gap,&row[end]);
            /* Hidden under the end icon at first: the plate is unchanged. */
            assert(plate_end(s,(DockEdge)edge,row,n,w,h,&g,high) == from);
            for (phase = 0; phase < 2; ++phase) {
                int a = phase ? to : from, b = phase ? from : to, ends[32], count, previous = a;
                if (phase) dock_gap_close(&g,rest,n);
                count = run_gap(s,(DockEdge)edge,&animation,row,phase ? rest : target,n,w,h,&g,high,ends);
                assert(count > 2 && a != b && ends[count-1] == b);
                assert(2*abs(ends[0]-a) <= abs(b-a));
                for (i = 0; i < count; ++i) {
                    int step = ends[i]-previous;
                    assert(step*(b-a) >= 0);
                    previous = ends[i];
                    steps[high][phase][i] = high ? step : -step;
                }
                ticks[high][phase] = count;
            }
            /* Closed, it stays under the end icon (inside the plate) until
               the caller ends it with the row settled. */
            assert(g.slot == slot && !memcmp(&g.shown,&rest[end],sizeof(g.shown)));
            dock_gap_reset(&g);
            assert(g.slot == -1 && dock_gap_rect(&g).right == 0);
        }
        /* Both ends alike: the same ticks and mirrored steps within a pixel. */
        for (phase = 0; phase < 2; ++phase) {
            assert(ticks[0][phase] == ticks[1][phase]);
            for (i = 0; i < ticks[0][phase]; ++i)
                assert(abs(steps[0][phase][i]-steps[1][phase][i]) <= 1);
        }
    }
}

/* "Near the dock" is measured from what is drawn: margin pixels past the
   plate at both ends of the row and outside its outer side, on every edge,
   and past a gap's reach where one is shown (IMG_2595: the press's frozen,
   zoomed row reached ~77 px further on the left). */
static void test_outside_shown(void)
{
    enum { MARGIN = 24, n = N-1 };
    DockSettings s = dock_settings_default();
    int edge;
    for (edge = DOCK_EDGE_BOTTOM; edge <= DOCK_EDGE_RIGHT; ++edge) {
        DockPlacement rest[n], gap;
        DockRect plate, hole;
        int w, h, x, y, inner, outer, cross, low, high, reach;
        dimensions((DockEdge)edge,&w,&h);
        dock_panel_cross_bounds(&s,CROSS,&inner,&outer);
        cross = (inner+outer)/2;
        assert(dock_edge_layout(&s,(DockEdge)edge,n,w,h,0,0,0,rest));
        assert(dock_edge_panel_rect(&s,(DockEdge)edge,rest,n,w,h,&plate));
        low = along_low((DockEdge)edge,plate); high = along_high((DockEdge)edge,plate);
        actual_point((DockEdge)edge,low-MARGIN,cross,&x,&y);
        assert(!dock_edge_outside(&s,(DockEdge)edge,rest,n,w,h,NULL,x,y,MARGIN));
        actual_point((DockEdge)edge,low-MARGIN-1,cross,&x,&y);
        assert(dock_edge_outside(&s,(DockEdge)edge,rest,n,w,h,NULL,x,y,MARGIN));
        actual_point((DockEdge)edge,high+MARGIN-1,cross,&x,&y);
        assert(!dock_edge_outside(&s,(DockEdge)edge,rest,n,w,h,NULL,x,y,MARGIN));
        actual_point((DockEdge)edge,high+MARGIN,cross,&x,&y);
        assert(dock_edge_outside(&s,(DockEdge)edge,rest,n,w,h,NULL,x,y,MARGIN));
        actual_point((DockEdge)edge,(low+high)/2,inner-MARGIN,&x,&y);
        assert(!dock_edge_outside(&s,(DockEdge)edge,rest,n,w,h,NULL,x,y,MARGIN));
        actual_point((DockEdge)edge,(low+high)/2,inner-MARGIN-1,&x,&y);
        assert(dock_edge_outside(&s,(DockEdge)edge,rest,n,w,h,NULL,x,y,MARGIN));
        /* A drop preview's gap before the first icon: that side moves out. */
        assert(dock_edge_gap_layout(&s,(DockEdge)edge,n,w,h,0,0,0,0,rest,&gap));
        hole = (DockRect){gap.x,gap.y,gap.x+gap.size,gap.y+gap.size};
        reach = along_low((DockEdge)edge,hole)-s.padding;
        assert(dock_edge_panel_rect(&s,(DockEdge)edge,rest,n,w,h,&plate));
        assert(reach < along_low((DockEdge)edge,plate));
        actual_point((DockEdge)edge,reach-MARGIN,cross,&x,&y);
        assert(!dock_edge_outside(&s,(DockEdge)edge,rest,n,w,h,&hole,x,y,MARGIN));
        actual_point((DockEdge)edge,reach-MARGIN-1,cross,&x,&y);
        assert(dock_edge_outside(&s,(DockEdge)edge,rest,n,w,h,&hole,x,y,MARGIN));
        actual_point((DockEdge)edge,reach-MARGIN,cross,&x,&y);
        assert(dock_edge_outside(&s,(DockEdge)edge,rest,n,w,h,NULL,x,y,MARGIN));
    }
}

/* The box's parent drawer and page buttons. The ink (the segments' end
   points grown by the half width) is centred on the disc exactly for the
   chevrons and the return arrow, at every size (by the box, never an
   optical offset); the rasterised pixel bounding box agrees with the
   disc's; left and right mirror each other sample for sample; from a 15 px
   control on, the disc is 5/6 of the control (below, a 12 px floor). */
static void button_bounds(int x, int size, int direction, int box[8])
{
    DockButtonGlyph g, m;
    int px, py;
    dock_button_glyph(x,x,size,size,direction,&g);
    if (direction != DOCK_BUTTON_PARENT) dock_button_glyph(x,x,size,size,-direction,&m);
    box[0] = box[1] = box[4] = box[5] = INT_MAX;
    box[2] = box[3] = box[6] = box[7] = INT_MIN;
    for (py = x; py < x+size; ++py) for (px = x; px < x+size; ++px) {
        int disc, stroke = dock_button_coverage(&g,px,py,&disc), mirror_disc;
        if (direction != DOCK_BUTTON_PARENT)
            assert(dock_button_coverage(&m,2*x+size-1-px,py,&mirror_disc) == stroke && mirror_disc == disc);
        if (stroke) {
            if (px < box[0]) box[0] = px;
            if (py < box[1]) box[1] = py;
            if (px > box[2]) box[2] = px;
            if (py > box[3]) box[3] = py;
        }
        if (disc) {
            if (px < box[4]) box[4] = px;
            if (py < box[5]) box[5] = py;
            if (px > box[6]) box[6] = px;
            if (py > box[7]) box[7] = py;
        }
    }
}

static void test_button_glyph(void)
{
    static const int directions[3] = {DOCK_BUTTON_LEFT, DOCK_BUTTON_RIGHT, DOCK_BUTTON_PARENT};
    int size, x, d;
    for (size = 8; size <= 40; ++size) for (x = 0; x < 3; ++x) for (d = 0; d < 3; ++d) {
        int direction = directions[d], o = 5+x, box[8], k, i;
        int lo[2] = {INT_MAX, INT_MAX}, hi[2] = {INT_MIN, INT_MIN};
        DockButtonGlyph g;
        DockButtonPixels n;
        button_bounds(o,size,direction,box);
        assert(box[0] <= box[2] && box[4] <= box[6]);
        /* Centres in half pixels: stroke and disc agree exactly up to a
           24 px control (the box's fonts give 18 or 19); above, a cap's
           faint edge sample can add half a pixel. */
        dock_button_glyph(o,o,size,size,direction,&g);
        for (k = 0; k < 2; ++k) {
            int offset = box[k]+box[k+2]-box[k+4]-box[k+6];
            if (size <= 24 && (direction != DOCK_BUTTON_PARENT || size >= 12)) assert(offset == 0);
            else assert(abs(offset) <= 1);
        }
        assert(g.radius >= (size*8 < 96 ? size*8 : 96) && g.radius <= size*8);
        if (size >= 15) assert(g.radius*6 >= size*40-6 && g.radius*6 <= size*40+6);
        assert(g.count == (direction == DOCK_BUTTON_PARENT ? 8 : 2));
        for (k = 0; k < 2; ++k) {
            int low = INT_MAX, high = INT_MIN;
            for (i = 0; i < g.count; ++i) {
                int a = g.segment[i][k], b = g.segment[i][k+2];
                if (a < low) low = a;
                if (b < low) low = b;
                if (a > high) high = a;
                if (b > high) high = b;
            }
            assert(g.ink[k] == low-g.half && g.ink[k+2] == high+g.half);
            assert(low+high == 2*(k ? g.cy : g.cx));
            /* The return arrow: its tip is the leftmost point, its stem
               the rightmost and lowest. */
            if (direction == DOCK_BUTTON_PARENT)
                assert(g.segment[6][k+2] == (k ? (g.segment[6][1]+g.segment[7][1])/2 : low) &&
                    g.segment[0][k] == high);
        }
        if (direction != DOCK_BUTTON_PARENT)
            assert(direction*(g.segment[0][2]-g.cx) > 0 && g.segment[0][3] == g.cy);
        /* Native: a centred disc inside the control; the lines are 45
           degree or straight, their pixels span the end points: centred
           exactly along the pointing direction, to half a pixel across it
           for an even control. */
        dock_button_pixels(o,o,size,size,direction,&n);
        assert(2*(n.left-o)+n.size == size && 2*(n.top-o)+n.size == size);
        assert(n.count == (direction == DOCK_BUTTON_PARENT ? 3 : 2));
        for (i = 0; i < n.count; ++i) for (k = 0; k < 3; ++k) {
            int c;
            if (k) {
                int dx = abs(n.stroke[i][k][0]-n.stroke[i][k-1][0]);
                int dy = abs(n.stroke[i][k][1]-n.stroke[i][k-1][1]);
                assert(dx == dy || !dx || !dy);
            }
            for (c = 0; c < 2; ++c) {
                if (n.stroke[i][k][c] < lo[c]) lo[c] = n.stroke[i][k][c];
                if (n.stroke[i][k][c] > hi[c]) hi[c] = n.stroke[i][k][c];
            }
        }
        /* The native return arrow: the stem's foot is the right bottom
           corner, the head's tip the leftmost point. */
        if (direction == DOCK_BUTTON_PARENT)
            assert(n.stroke[0][0][0] == hi[0] && n.stroke[0][0][1] == hi[1] && n.stroke[2][1][0] == lo[0]);
        for (k = 0; k < 2; ++k) {
            int edge = k ? n.top : n.left, along = direction == DOCK_BUTTON_PARENT ? k == 1 : k == 0;
            if (direction != DOCK_BUTTON_PARENT || size >= 15) assert(lo[k] > edge && hi[k] < edge+n.size-1);
            assert(lo[k]+hi[k]-(2*edge+n.size-1) ==
                (along && direction != DOCK_BUTTON_PARENT ? 0 : (size+1)%2));
        }
    }
}

/* The Open Folder tile's glyph: the arrow's outlines are centred on the
   ring by their box, and the rasterised glyph's pixel box, and the arrow's
   alone, are centred exactly on the tile's icon box for every icon size
   and pixel phase. */
static void test_open_glyph(void)
{
    int icon, left, k, i;
    for (k = 0; k < 2; ++k) {
        int low = 127, high = -128;
        for (i = 0; i < DOCK_OPEN_TAIL; ++i) {
            if (dock_open_arrow_tail[i][k] < low) low = dock_open_arrow_tail[i][k];
            if (dock_open_arrow_tail[i][k] > high) high = dock_open_arrow_tail[i][k];
        }
        for (i = 0; i < DOCK_OPEN_HEAD; ++i) {
            if (dock_open_arrow_head[i][k] < low) low = dock_open_arrow_head[i][k];
            if (dock_open_arrow_head[i][k] > high) high = dock_open_arrow_head[i][k];
        }
        assert(low+high == 0);
    }
    for (icon = 16; icon <= DOCK_MAX_ICON_SIZE; ++icon) for (left = 3; left < 5; ++left)
        for (k = 0; k < 2; ++k) {
        int radius = (icon*36+50)/100, thickness = ((icon/2-2)+6)/13, top = left+1, px, py;
        int box[4] = {INT_MAX, INT_MAX, INT_MIN, INT_MIN};
        unsigned char covered[2*DOCK_MAX_ICON_SIZE+8];
        DockOpenGlyph g;
        if (thickness < 1) thickness = 1;
        if (thickness > radius/2) thickness = radius/2;
        dock_open_glyph(left,top,icon,radius,thickness,&g);
        assert(2*g.cx == 16*(2*left+icon) && 2*g.cy == 16*(2*top+icon));
        /* k = 1: the arrow alone, without the ring. */
        if (k) g.outer = g.inner = 0;
        for (py = top-2; py < top+icon+2; ++py) {
            dock_open_glyph_row(&g,py,left-2,icon+4,covered);
            for (px = 0; px < icon+4; ++px) if (covered[px]) {
                if (left-2+px < box[0]) box[0] = left-2+px;
                if (py < box[1]) box[1] = py;
                if (left-2+px > box[2]) box[2] = left-2+px;
                if (py > box[3]) box[3] = py;
            }
        }
        assert(box[0]+box[2]+1 == 2*left+icon && box[1]+box[3]+1 == 2*top+icon);
    }
}

int main(void)
{
    test_layout_and_baseline();
    test_interaction();
    test_panel_rect();
    test_animation();
    test_counts_and_invalid();
    test_external_insertion();
    test_zoomed_reorder_gap();
    test_zoom_room_after_insertion();
    test_gap_motion();
    test_outside_shown();
    test_button_glyph();
    test_open_glyph();
    puts("geometry: bottom/left/right layout, upright hit/slot/outside, half-open panel bounds, animation equivalence and centred box buttons (page chevrons, return arrow) and Open Folder glyph PASS");
    return 0;
}
