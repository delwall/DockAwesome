/* AppWindow ownership of the plate for external Workbench drags. Shape
   changes and ownership checks run in the application task, never from
   the input handler. No per-drag allocation, timer, patch or input replay. */
#include "amiga_drop_catcher.h"
#include "amiga_caps.h"
#include "amiga_abi.h"
#include <exec/ports.h>
#include <devices/inputevent.h>
#include <proto/input.h>
#include <graphics/clip.h>
#include <graphics/gfxbase.h>
#include <graphics/layers.h>
#include <graphics/regions.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/screens.h>
#include <inline/macros.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/layers.h>
#include <proto/wb.h>
#include <utility/hooks.h>
#include <workbench/workbench.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

enum { DESKTOP_ICON_CAP = 128 };

struct AmigaDropCatcher {
    struct Screen *screen;
    struct Window *window;
    struct Region *normal, *catch_region;
    struct BitMap *backing;
    struct Hook backfill;
    int left, top, width, height;
    int shaped; /* catch by shape (ApolloOS) or by moving the window */
    int armed;  /* the window covers armed_at and waits for a press there */
    /* Where the window catches (screen coordinates, inside the box): the
       plate (rest), moved to without shapes, shaped to with them.
       rest_armed: it stays there between gestures (with shapes, and on
       the Directory Opus desktop). */
    struct Rectangle armed_at, rest;
    int rest_armed;
    int dopus_desktop; /* the pointer has been on a Directory Opus 5 desktop */
    /* The dock's own windows, which the armed window stays behind. */
    int (*ours)(void *data, struct Window *window);
    void *ours_data;
    uint64_t order_check_at; /* armed: check its place in front from then */
    int repaint; /* armed: the corner pixel still shows what the move carried */
    AmigaDropCatcherStats stats;
    uint32_t serial;
    uint64_t deadline, next_icons, icons_not_before;
    int have_serial, active, waiting, failed;
    /* No shapes, armed: the window shows a copy of the screen taken when it
       armed. windows: the other layers on the dock's box at that moment,
       see box_windows; once they differ the copy shows what is gone. */
    ULONG windows;
    uint64_t rearm_at;
    volatile int backing_valid;
    const struct Window *desktop;
    int icon_count, icons_incomplete;
    struct Rectangle icons[DESKTOP_ICON_CAP];
    struct Rectangle labels[DESKTOP_ICON_CAP];
};

extern ULONG dock_backfill_entry(void);
extern ULONG dock_workbench_hit_entry(void);

ULONG dock_workbench_hit(struct Hook *hook, const struct IconSelectMsg *icon)
{
    AmigaDropCatcher *c = hook->h_Data;
    int left, top, right, bottom, label_half, center, index;
    if (!icon || icon->ism_Length < offsetof(struct IconSelectMsg,ism_Height)+sizeof(icon->ism_Height) ||
        !icon->ism_ParentWindow || c->icon_count == DESKTOP_ICON_CAP ||
        (c->desktop && c->desktop != icon->ism_ParentWindow)) {
        c->icons_incomplete = 1;
        return ISMACTION_Stop;
    }
    c->desktop = icon->ism_ParentWindow;
    index = c->icon_count++;
    left = icon->ism_Left; top = icon->ism_Top;
    right = left+icon->ism_Width; bottom = top+icon->ism_Height;
    c->icons[index] = (struct Rectangle){left-2, top-2, right+1, bottom+1};
    /* Allow for centered labels. Their font can differ from the window. */
    label_half = icon->ism_Name ? (int)strlen((const char *)icon->ism_Name)*6+2 : 0;
    center = left+icon->ism_Width/2;
    c->labels[index] = (struct Rectangle){center-label_half, bottom-2, center+label_half, bottom+19};
    return ISMACTION_Ignore;
}

static void invalidate_desktop_icons(AmigaDropCatcher *c)
{
    c->desktop = NULL;
    c->next_icons = 0;
}

static unsigned int refresh_desktop_icons(AmigaDropCatcher *c, uint64_t now, uint32_t frequency)
{
    struct Hook hook = {0};
    if (WorkbenchBase->lib_Version < 45 || WorkbenchBase->lib_NegSize < 126 ||
        now < c->next_icons || now < c->icons_not_before ||
        (PeekQualifier() & (IEQUALIFIER_LEFTBUTTON|IEQUALIFIER_RBUTTON|IEQUALIFIER_MIDBUTTON)))
        return 0;
    c->next_icons = now+(uint64_t)frequency*4;
    c->icons_not_before = now+frequency/4;
    c->desktop = NULL; c->icon_count = c->icons_incomplete = 0;
    hook.h_Entry = dock_workbench_hit_entry;
    hook.h_Data = c;
    if (!ChangeWorkbenchSelectionA(NULL, &hook, NULL) || c->icons_incomplete) c->desktop = NULL;
    return AMIGA_DROP_SCANNED;
}

static int rect_contains(const struct Rectangle *r, int x, int y)
{
    return x >= r->MinX && x <= r->MaxX && y >= r->MinY && y <= r->MaxY;
}

/* The first two fields are the public backfill message prefix. Bounds are
   bitmap coordinates and already clipped by Layers. Don't lock a layer or
   use a layer-aware RastPort operation from its own callback. */
struct DockBackfillMessage { struct Layer *layer; struct Rectangle bounds; LONG offset_x, offset_y; };

ULONG dock_backfill_paint(struct Hook *hook, struct RastPort *port,
                         const struct DockBackfillMessage *msg)
{
    AmigaDropCatcher *c = hook->h_Data;
    int x = msg->bounds.MinX, y = msg->bounds.MinY;
    int sx = x-c->left, sy = y-c->top;
    int w = msg->bounds.MaxX-x+1, h = msg->bounds.MaxY-y+1;
    /* Where the window grows or moves behind another, Layers (AmigaOS 3.x)
       fills that part in a bitmap of its own and copies it to the screen
       once it comes in front. The offset places it, relative to the layer.
       Left unpainted, that bitmap's old memory showed as noise over the
       window the receiver armed behind (Coffin, IMG_2601). */
    if (port->BitMap != c->screen->RastPort.BitMap) {
        sx = msg->layer->bounds.MinX+(int)msg->offset_x-c->left;
        sy = msg->layer->bounds.MinY+(int)msg->offset_y-c->top;
    }
    if (!c->backing_valid ||
        sx < 0 || sy < 0 || sx+w > c->width || sy+h > c->height || w <= 0 || h <= 0) return 0;
    BltBitMap(c->backing,sx,sy,port->BitMap,x,y,w,h,0xc0,0xff,NULL);
    return 0;
}

static APTR signal_port_owner(const struct MsgPort *port)
{
    return port && (port->mp_Flags & PF_ACTION) == PA_SIGNAL ? port->mp_SigTask : NULL;
}

/* 1 when (x, y) is on a window of the desktop a drag can start from: a
   Workbench (or Wanderer) drawer or icon, or a Directory Opus 5 lister.
   2 on the Directory Opus desktop itself: its icons are private, so an
   icon drag cannot be told from a selection box there. Port ownership is
   stronger than title matching and has been verified on the target:
   WORKBENCH and the desktop UserPort signal the same task. */
static int workbench_client_start(const AmigaDropCatcher *c, int x, int y)
{
    struct Layer *layer;
    struct Window *window;
    int i, result = 0;
    /* A competing native drag may already own Layers. Retry from the main
       loop instead of blocking input while trying to inspect its source. */
    if (!AttemptSemaphore(&c->screen->LayerInfo.Lock)) return -1;
    layer = WhichLayer(&c->screen->LayerInfo, x, y);
    window = layer ? layer->Window : NULL;
    if (window && window != c->window && window->WScreen == c->screen &&
        x >= window->LeftEdge + window->BorderLeft &&
        x < window->LeftEdge + window->Width - window->BorderRight &&
        y >= window->TopEdge + window->BorderTop &&
        y < window->TopEdge + window->Height - window->BorderBottom) {
        APTR owner = signal_port_owner(window->UserPort);
        Forbid();
        result = amiga_caps_desktop_task(owner);
        Permit();
        if (result == 2) result = window->Flags & WFLG_BACKDROP ? 2 : 1;
        if (result && window == c->desktop) {
            x -= window->LeftEdge; y -= window->TopEdge;
            result = 0;
            for (i = 0; i < c->icon_count; ++i) {
                if (rect_contains(&c->icons[i], x, y) || rect_contains(&c->labels[i], x, y)) {
                    result = 1;
                    break;
                }
            }
        }
    }
    ReleaseSemaphore(&c->screen->LayerInfo.Lock);
    return result;
}

/* A window scrolled under the plate keeps its layer, and the wheel may
   scroll it without an input event (ApolloOS): its scrollers' knobs tell.
   The window cannot close while the caller holds the layer list; Forbid
   keeps its gadget list still. */
static ULONG window_scrollers(ULONG stamp, const struct Window *w)
{
    const struct Gadget *g;
    int n = 0;
    Forbid();
    for (g = w->FirstGadget; g && n < 32; g = g->NextGadget, ++n) {
        const struct PropInfo *p = g->SpecialInfo;
        if ((g->GadgetType & GTYP_GTYPEMASK) != GTYP_PROPGADGET || !p) continue;
        stamp = (stamp ^ (((ULONG)p->HorizPot<<16) | p->VertPot)) * 16777619UL;
    }
    Permit();
    return stamp;
}

/* The layers that lie on r, other than the receiver's own, as one number:
   which they are, where, and how far their windows are scrolled. 0 when
   the layer list is busy. */
static ULONG box_windows(const AmigaDropCatcher *c, const struct Rectangle *r)
{
    struct Layer_Info *info = &c->screen->LayerInfo;
    const struct Layer *layer;
    ULONG stamp = 2166136261UL;
    if (!AttemptSemaphore(&info->Lock)) return 0;
    for (layer = info->top_layer; layer; layer = layer->back) {
        const struct Rectangle *b = &layer->bounds;
        if (layer == c->window->WLayer || b->MaxX < r->MinX || b->MinX > r->MaxX ||
            b->MaxY < r->MinY || b->MinY > r->MaxY) continue;
        stamp = (stamp ^ (ULONG)layer) * 16777619UL;
        stamp = (stamp ^ (((ULONG)(UWORD)b->MinX<<16) | (UWORD)b->MinY)) * 16777619UL;
        stamp = (stamp ^ (((ULONG)(UWORD)b->MaxX<<16) | (UWORD)b->MaxY)) * 16777619UL;
        if (layer->Window) stamp = window_scrollers(stamp, layer->Window);
    }
    ReleaseSemaphore(&info->Lock);
    return stamp ? stamp : 1;
}

/* The frontmost window on r of another program (not a backdrop), or NULL;
   with before_own, only one in front of the receiver. busy when the layer
   list is. Depth changes are deferred, so the caller may ask for one while
   the list is held here: no window can close before it is released. */
static struct Window *foreign_front(const AmigaDropCatcher *c, const struct Rectangle *r,
    int before_own, int *busy, struct Window *place_behind)
{
    struct Layer_Info *info = &c->screen->LayerInfo;
    const struct Layer *layer;
    struct Window *found = NULL;
    if (!AttemptSemaphore(&info->Lock)) { *busy = 1; return NULL; }
    *busy = 0;
    for (layer = info->top_layer; layer; layer = layer->back) {
        const struct Rectangle *b = &layer->bounds;
        struct Window *w = layer->Window;
        if (layer == c->window->WLayer) {
            if (before_own) break;
            continue;
        }
        if (!w || (w->Flags & WFLG_BACKDROP) || (c->ours && c->ours(c->ours_data, w)) ||
            b->MaxX < r->MinX || b->MinX > r->MaxX || b->MaxY < r->MinY || b->MinY > r->MaxY) continue;
        found = w;
        break;
    }
    if (place_behind) {
        if (found) MoveWindowInFrontOf(place_behind, found);
        else WindowToBack(place_behind);
    }
    ReleaseSemaphore(&info->Lock);
    return found;
}

/* The move from 1x1 onto the box carries the window's one pixel along:
   the screen's corner, shown at the box's corner (a stray dot beside the
   plate, AmiKit). The backfill only fills what the move uncovered. Paint
   that pixel from the copy once Intuition has moved the window. */
static void repaint_corner(AmigaDropCatcher *c)
{
    struct Window *w = c->window;
    const struct Rectangle *r = &c->armed_at;
    if (w->LeftEdge != r->MinX || w->TopEdge != r->MinY ||
        w->Width != r->MaxX-r->MinX+1 || w->Height != r->MaxY-r->MinY+1) return;
    /* A held drag owns the layers: try again on a later pass. */
    if (!AttemptLockLayerRom(w->WLayer)) return;
    BltBitMapRastPort(c->backing, r->MinX-c->left, r->MinY-c->top, w->RPort, 0, 0, 1, 1, 0xc0);
    UnlockLayerRom(w->WLayer);
    c->repaint = 0;
}

static int inside(const AmigaDropCatcher *c, int x, int y)
{
    return x >= c->left && x < c->left + c->width &&
           y >= c->top && y < c->top + c->height;
}

/* Where a drop is caught now: where the armed window lies, else the box. */
static struct Rectangle catch_rect(const AmigaDropCatcher *c)
{
    if (c->armed) return c->armed_at;
    return (struct Rectangle){c->left, c->top, c->left+c->width-1, c->top+c->height-1};
}

static int inside_catch(const AmigaDropCatcher *c, int x, int y)
{
    struct Rectangle r = catch_rect(c);
    return x >= r.MinX && x <= r.MaxX && y >= r.MinY && y <= r.MaxY;
}

int amiga_drop_catcher_contains(const AmigaDropCatcher *c, int x, int y)
{
    return c && inside_catch(c, x, y);
}

void amiga_drop_catcher_rect(const AmigaDropCatcher *c, int *left, int *top, int *right, int *bottom)
{
    struct Rectangle r = catch_rect(c);
    *left = r.MinX; *top = r.MinY; *right = r.MaxX+1; *bottom = r.MaxY+1;
}

void amiga_drop_catcher_set_owner(AmigaDropCatcher *c, int (*ours)(void *data, struct Window *window), void *data)
{
    if (!c) return;
    c->ours = ours; c->ours_data = data;
}

void amiga_drop_catcher_set_rest(AmigaDropCatcher *c, int left, int top, int width, int height)
{
    int right = left+width, bottom = top+height;
    if (!c) return;
    if (left < c->left) left = c->left;
    if (top < c->top) top = c->top;
    if (right > c->left+c->width) right = c->left+c->width;
    if (bottom > c->top+c->height) bottom = c->top+c->height;
    /* Nothing left: rest on the whole box. */
    if (right <= left || bottom <= top) {
        left = c->left; top = c->top; right = c->left+c->width; bottom = c->top+c->height;
    }
    /* A rest-armed window elsewhere moves on the next arm_for_pointer. */
    c->rest = (struct Rectangle){left, top, right-1, bottom-1};
}

/* Cover armed_at (catching) or not. Shapes report the region they
   replace, which must be the expected one; ChangeWindowBox and
   WindowToFront are carried out by Intuition shortly after. */
static int set_catching(AmigaDropCatcher *c, int catching)
{
    if (c->shaped) {
        struct Region *expected = catching ? c->normal : c->catch_region;
        /* The plate alone, as without shapes: what the window uncovers
           Workbench redraws. Over the whole box, a drawer's icons above
           the plate blinked out at every desktop drop (ApolloOS,
           IMG_2599). The catch shape is not installed here, so it may
           change. */
        if (catching) {
            const struct Rectangle *r = &c->armed_at;
            struct Rectangle shape = {r->MinX-c->window->LeftEdge, r->MinY-c->window->TopEdge,
                r->MaxX-c->window->LeftEdge, r->MaxY-c->window->TopEdge};
            ClearRegion(c->catch_region);
            if (!OrRectRegion(c->catch_region, &shape)) return 0;
        }
        struct Region *previous = amiga_change_window_shape(c->window, catching ? c->catch_region : c->normal);
        return previous == expected || (!catching && previous == c->normal);
    }
    /* It covers only the plate, where the dock hides every window: in front
       of the other programs' windows there (behind the dock's own). Behind
       them, a drop over a window under the dock went to that window: a
       Directory Opus lister (AmiKit), a Workbench drawer that started
       copying the dropped disk into itself (AmigaOS 3.2.3). Over the whole
       box a window in the headroom was cut and showed stale bands through
       PiP (Coffin). Under the plate a window shows as it was when the
       receiver armed until it steps back. */
    if (catching) {
        const struct Rectangle *r = &c->armed_at;
        int busy;
        ChangeWindowBox(c->window, r->MinX, r->MinY, r->MaxX-r->MinX+1, r->MaxY-r->MinY+1);
        foreign_front(c, r, 0, &busy, c->window);
        if (busy) WindowToBack(c->window);
    } else ChangeWindowBox(c->window, 0, 0, 1, 1);
    return 1;
}

AmigaDropCatcher *amiga_drop_catcher_init(struct Screen *screen,
    struct Window *window, struct Region *normal_shape, int left, int top, int width, int height)
{
    AmigaDropCatcher *c;
    struct Rectangle rect;
    if (!screen || !window || width < 1 || height < 1 ||
        width > 32767 || height > 32767) return NULL;
    c = calloc(1, sizeof(*c));
    if (!c) return NULL;
    c->catch_region = NewRegion();
    /* The catching shape covers the box, in the window's own coordinates. */
    rect.MinX = left - window->LeftEdge; rect.MinY = top - window->TopEdge;
    rect.MaxX = rect.MinX + width - 1; rect.MaxY = rect.MinY + height - 1;
    if (!c->catch_region || !OrRectRegion(c->catch_region, &rect)) {
        if (c->catch_region) DisposeRegion(c->catch_region);
        free(c); return NULL;
    }
    c->screen = screen; c->window = window; c->normal = normal_shape;
    c->shaped = normal_shape != NULL;
    c->left = left; c->top = top; c->width = width; c->height = height;
    c->rest = (struct Rectangle){left, top, left+width-1, top+height-1};
    c->backing = AllocBitMap(width,height,GetBitMapAttr(screen->RastPort.BitMap,BMA_DEPTH),
        BMF_MINPLANES,screen->RastPort.BitMap);
    if (!c->backing) { DisposeRegion(c->catch_region); free(c); return NULL; }
    c->backfill.h_Entry = dock_backfill_entry;
    c->backfill.h_Data = c;
    InstallLayerHook(window->WLayer,&c->backfill);
    return c;
}

/* Armed, and the layers on the plate are no longer those the copy shows. */
static int windows_changed(const AmigaDropCatcher *c)
{
    ULONG windows;
    if (!c->armed) return 0;
    windows = box_windows(c, &c->armed_at);
    return windows && windows != c->windows;
}

/* End a catch. The window stays armed unless disarm is set: it may not
   move while Workbench starts a drag, and what it uncovers is redrawn. */
static unsigned int end_catch(AmigaDropCatcher *c, int disarm)
{
    unsigned int flags = c->active ? AMIGA_DROP_RESTORED : 0;
    c->waiting = c->active = 0;
    if (disarm && c->armed) {
        ++c->stats.disarms;
        if (!set_catching(c, 0)) c->failed = 1;
        else {
            c->armed = c->rest_armed = 0;
            c->backing_valid = 0;
            flags |= AMIGA_DROP_RESTORED;
        }
    }
    return flags | (c->failed ? AMIGA_DROP_ERROR : 0);
}

/* The window leaves the dock after each gesture unless it rests: with
   shapes, and on the Directory Opus desktop. There each move away made
   what it uncovered redraw (a flicker after every click), and only
   arm_for_pointer moves it. */
unsigned int amiga_drop_catcher_cancel(AmigaDropCatcher *c)
{
    if (!c) return 0;
    return end_catch(c, !c->rest_armed);
}

/* The window must be on the dock before Workbench starts a drag: it holds
   the layers from the press on, and AmigaOS 3.9 cancels the drag if
   windows move. Without shapes, with the buttons up, arm while the pointer
   is on a Workbench icon or drawer and disarm when it leaves. With shapes
   rest on the plate: taken at each press and given up at each release,
   the shape made Workbench redraw what it uncovered at every drop, even
   one far from the dock (icons beside the plate and in its glass blinked,
   ApolloOS, IMG_2600). */
static unsigned int arm_for_pointer(AmigaDropCatcher *c, uint64_t now, uint32_t frequency)
{
    struct Rectangle want;
    ULONG windows;
    int client;
    if (c->active || c->waiting) return 0;
    if (windows_changed(c)) {
        /* A window on the plate closed or moved while the receiver lay
           there: the copy still shows that window (a band of it stayed
           on the screen for as long as the pointer rested on the dock,
           Coffin). Step back so the desktop redraws, and arm again only
           after it has. */
        c->rearm_at = now+frequency/2;
        return end_catch(c, 1);
    }
    /* A shape neither moves nor reorders the window. */
    if (!c->shaped && c->armed && c->repaint) repaint_corner(c);
    if (!c->shaped && c->armed && now >= c->order_check_at) {
        /* Another program's window came in front of the armed window:
           step back and arm again in front of it. Retry a busy layer
           list on the same schedule as a successful check. */
        int busy;
        c->order_check_at = now+frequency/4;
        if (foreign_front(c, &c->armed_at, 1, &busy, NULL) && !busy) {
            c->rearm_at = now+frequency/4;
            return end_catch(c, 1);
        }
    }
    if (now < c->rearm_at) return 0;
    if (c->shaped) {
        want = c->rest;
        if (!c->armed) goto arm;
        if (!memcmp(&want, &c->armed_at, sizeof(want))) return 0;
        /* The plate changed: arm again on it once the uncovered part has
           redrawn. */
        c->rearm_at = now+frequency/4;
        return end_catch(c, 1);
    }
    /* On the dock itself the pointer is on no drawer: stay as it is. */
    if (inside(c, c->screen->MouseX, c->screen->MouseY)) return 0;
    client = workbench_client_start(c, c->screen->MouseX, c->screen->MouseY);
    if (client < 0) return 0;
    /* The Directory Opus desktop (2) cannot tell its icons from its
       background, so the window rests there whenever the pointer is on it:
       Directory Opus gives a drop only to a window that lay at the release
       point before the press (moved during its drag, the window ended the
       drag). Resting, it only covers the plate, where the dock hides the
       desktop anyway: on the whole box it took the clicks on desktop icons
       beside and above the dock, and Directory Opus could not redraw there
       (half of a moved icon stayed, AmiKit). */
    /* Once Directory Opus is seen as the desktop, stay on the plate for
       its lister icons too: leaving made it erase and redraw every desktop
       icon there (they vanished for a few frames, AmiKit). Workbench never
       gives 2. */
    if (client == 2) c->dopus_desktop = 1;
    want = c->rest;
    if (c->armed) {
        if (client && !memcmp(&want, &c->armed_at, sizeof(want))) return 0;
        /* The desktop and windows redraw where the receiver was: a copy
           taken before they have finished showed a stale rectangle through
           PiP. */
        c->rearm_at = now+frequency/4;
        return end_catch(c, 1);
    }
    if (!client) return 0;
arm:
    /* Taken with the copy: what the copy shows on the box. With the layer
       list busy, arm on a later pass. */
    if (!(windows = box_windows(c, &want))) return 0;
    if (c->shaped && windows != c->windows) {
        /* Resting, the copy stays: take it once the windows on the plate
           have been still a moment, not while one scrolls or moves. */
        c->windows = windows;
        c->rearm_at = now+frequency/2;
        return 0;
    }
    c->windows = windows;
    BltBitMap(c->screen->RastPort.BitMap,want.MinX,want.MinY,
        c->backing,want.MinX-c->left,want.MinY-c->top,
        want.MaxX-want.MinX+1,want.MaxY-want.MinY+1,0xc0,0xff,NULL);
    WaitBlit();
    c->backing_valid = 1;
    c->armed_at = want;
    c->rest_armed = c->shaped || c->dopus_desktop;
    /* The depth change is deferred: check the order once it is done. */
    c->order_check_at = now+frequency/4;
    if (!set_catching(c, 1)) {
        c->backing_valid = 0;
        c->failed = 1;
        return AMIGA_DROP_ERROR;
    }
    c->armed = 1;
    c->repaint = !c->shaped;
    ++c->stats.arms;
    return AMIGA_DROP_STARTED;
}

/* The wheel scrolls the window under the pointer. Where that window lies
   under the plate the armed receiver hid the change: a drawer's old icons
   stayed under the plate while it scrolled (AmigaOS 3.2.3). Step back while
   it scrolls; arm again once the wheel has been still a moment. */
unsigned int amiga_drop_catcher_wheel(AmigaDropCatcher *c, uint64_t now, uint32_t frequency)
{
    const struct Layer *layer;
    const struct Rectangle *r;
    int under = 0;
    if (!c || c->active || c->waiting) return 0;
    if (!AttemptSemaphore(&c->screen->LayerInfo.Lock)) return 0;
    layer = WhichLayer(&c->screen->LayerInfo, c->screen->MouseX, c->screen->MouseY);
    if (layer && layer != c->window->WLayer && layer->Window &&
        !(((const struct Window *)layer->Window)->Flags & WFLG_BACKDROP)) {
        const struct Rectangle *b = &layer->bounds;
        r = &c->rest;
        under = b->MaxX >= r->MinX && b->MinX <= r->MaxX && b->MaxY >= r->MinY && b->MinY <= r->MaxY;
    }
    ReleaseSemaphore(&c->screen->LayerInfo.Lock);
    if (!under) return 0;
    c->rearm_at = now+frequency/4;
    return c->armed ? end_catch(c, 1) : 0;
}

unsigned int amiga_drop_catcher_accepted(AmigaDropCatcher *c)
{
    return amiga_drop_catcher_cancel(c);
}

unsigned int amiga_drop_catcher_update(AmigaDropCatcher *c,
    uint32_t external_serial, struct Screen *external_screen,
    int down_x, int down_y, int external_held, int external_dragged,
    int release_known, int release_x, int release_y, uint16_t qualifiers,
    uint64_t now, uint32_t frequency)
{
    unsigned int flags = 0;
    if (!c) return 0;
    if (c->failed) return AMIGA_DROP_ERROR;
    /* Compare the pointer without walking Intuition's screen list. The
       caller keeps our screen open throughout the receiver's lifetime. */
    if (IntuitionBase->FirstScreen != c->screen || (qualifiers & IEQUALIFIER_RBUTTON)) {
        /* Consume the generation logically so the same held gesture cannot
           rearm after a menu or screen switch. Original events pass through. */
        c->serial = external_serial; c->have_serial = 1;
        invalidate_desktop_icons(c);
        return amiga_drop_catcher_cancel(c);
    }
    /* Workbench can wait for the button to be released inside the
       selection API. Never call it while servicing a press or drag. */
    if (external_dragged && (external_held || c->active || !c->have_serial || external_serial != c->serial))
        invalidate_desktop_icons(c);
    if (!external_held && !c->active) flags |= refresh_desktop_icons(c, now, frequency);
    if (now >= c->next_icons) c->desktop = NULL;
    if (!c->have_serial || external_serial != c->serial) {
        int client = 0;
        flags |= end_catch(c, 0);
        if (flags & AMIGA_DROP_ERROR) return flags;
        if (external_held && external_screen == c->screen) {
            if (c->armed) ++c->stats.presses_armed; else ++c->stats.presses_unarmed;
            /* Armed means the pointer was checked on a Workbench icon with
               the buttons up; Workbench holds the layers from the press on,
               so do not ask again. */
            client = !c->shaped && c->armed ? 1 : workbench_client_start(c, down_x, down_y);
            if (client < 0) return flags;
        }
        c->serial = external_serial; c->have_serial = 1;
        /* Only an armed window can catch: moving or shaping it now would
           wait for Workbench's layers, or cancel the drag. */
        if (client && c->armed) { c->active = 1; flags |= AMIGA_DROP_STARTED; }
    }
    if (c->waiting) {
        if (now >= c->deadline) flags |= amiga_drop_catcher_cancel(c);
        return flags;
    }
    if (!external_held && !c->active && !c->waiting) flags |= arm_for_pointer(c, now, frequency);
    else if (c->shaped && external_held && !c->active) {
        /* A held press that is not a Workbench drag, such as a window
           dragged by its title bar: the resting receiver hid the window
           under the plate until the release (ApolloOS). Step back as soon
           as the windows there change, and arm again only once the press
           has been over a moment, after they have redrawn. A shape may
           change now: no Workbench drag holds the layers (box_windows
           found them free). */
        if (windows_changed(c)) flags |= end_catch(c, 1);
        if (!c->armed) c->rearm_at = now+frequency/2;
    }
    if (!c->active || external_held) return flags;
    if (release_known && (!inside_catch(c, release_x, release_y) ||
        (!external_dragged && release_x == down_x && release_y == down_y)))
        return flags | amiga_drop_catcher_cancel(c);
    c->waiting = 1;
    {
        uint64_t grace = (uint64_t)frequency * 3 / 10;
        c->deadline = now + (grace ? grace : 1);
    }
    return flags;
}

AmigaDropCatcherStats amiga_drop_catcher_stats(const AmigaDropCatcher *c)
{
    AmigaDropCatcherStats none = {0};
    return c ? c->stats : none;
}

int amiga_drop_catcher_active(const AmigaDropCatcher *c)
{
    return c && c->active;
}

int amiga_drop_catcher_armed(const AmigaDropCatcher *c)
{
    return c && c->armed;
}

void amiga_drop_catcher_destroy(AmigaDropCatcher *c)
{
    if (!c) return;
    WaitBlit();
    FreeBitMap(c->backing);
    DisposeRegion(c->catch_region);
    free(c);
}
