#include "amiga_drag_preview.h"
#include "dock_compose.h"
#include "amiga_display.h"
#include <graphics/gfxbase.h>
#include <graphics/regions.h>
#include <intuition/intuitionbase.h>
#include <intuition/screens.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <stdlib.h>
#include <string.h>

#ifndef WA_ShapeRegion
#define WA_ShapeRegion (WA_Dummy + 65)
#endif

struct AmigaDragPreview {
    struct Screen *screen;
    struct Window *window;
    struct Region *shape;
    int width, height, icon_height, left, top;
    int shaped;
    int stale; /* moved, the save-under not yet shown there */
    /* The window keeps a copy of what lies under it (a save-under, as for
       a BOB) and draws the picture over it: the Remove label's glass and
       the icon's soft edges blend with the screen, as in the dock. */
    AmigaDisplay *display;
    DockImage icon, picture, background, scratch, frame;
    unsigned long stamp;
};

static void position(const AmigaDragPreview *p, int x, int y, int *left, int *top)
{
    *left = dock_clamp(x-p->width/2, 0, p->screen->Width-p->width);
    *top = dock_clamp(y-p->icon_height/2, 0, p->screen->Height-p->height);
}

static int add_rect(struct Region *shape, int left, int top, int right, int bottom)
{
    struct Rectangle rect = {left, top, right, bottom};
    return OrRectRegion(shape, &rect);
}

/* The picture's outline, row by row. */
static int build_shape(AmigaDragPreview *p, const DockImage *picture)
{
    int x, y;
    p->shape = NewRegion();
    if (!p->shape) return 0;
    for (y = 0; y < picture->height; ++y) {
        const unsigned char *row = picture->pixels+(size_t)y*picture->width*4;
        int first = picture->width, last = -1;
        for (x = 0; x < picture->width; ++x) if (row[x*4+3] >= 24) {
            if (first == picture->width) first = x;
            last = x;
        }
        if (last >= first && !add_rect(p->shape,first,y,last,y)) return 0;
    }
    return 1;
}

static int alloc_image(DockImage *image, int width, int height)
{
    image->width = width; image->height = height;
    image->pixels = malloc((size_t)width*height*4);
    return image->pixels != NULL;
}

/* The picture over the save-under. */
static int render(AmigaDragPreview *p)
{
    memcpy(p->frame.pixels,p->background.pixels,(size_t)p->width*p->height*4);
    dock_blend(&p->frame,&p->picture,0,0);
    return amiga_display_present(p->display,p->window,&p->frame);
}

AmigaDragPreview *amiga_drag_preview_open(struct Screen *screen, const DockImage *icon,
    const DockImage *picture, int shaped, int x, int y)
{
    AmigaDragPreview *p;
    if (!screen || !icon || !icon->pixels || !picture || !picture->pixels ||
        picture->width < icon->width || picture->height < icon->height) return NULL;
    p = calloc(1,sizeof(*p));
    if (!p) return NULL;
    p->screen = screen;
    p->shaped = shaped;
    p->icon_height = icon->height;
    p->width = picture->width;
    p->height = picture->height;
    if (p->width > screen->Width || p->height > screen->Height) goto fail;
    if (shaped && !build_shape(p,picture)) goto fail;
    /* Private copies: the item's cache can be rebuilt during the drag. */
    if (!alloc_image(&p->icon,icon->width,icon->height) ||
        !alloc_image(&p->picture,p->width,p->height) || !alloc_image(&p->frame,p->width,p->height))
        goto fail;
    memcpy(p->icon.pixels,icon->pixels,(size_t)icon->width*icon->height*4);
    memcpy(p->picture.pixels,picture->pixels,(size_t)p->width*p->height*4);
    { static unsigned long stamps; p->stamp = ++stamps; }
    position(p,x,y,&p->left,&p->top);
    if (!(p->display = amiga_display_open(screen,p->width,p->height))) goto fail;
    if (!alloc_image(&p->background,p->width,p->height) ||
        !alloc_image(&p->scratch,p->width,p->height) ||
        !amiga_display_capture(p->display,&p->background,p->left,p->top)) goto fail;
    p->window = OpenWindowTags(NULL, WA_CustomScreen, (ULONG)screen,
        WA_Left, p->left, WA_Top, p->top, WA_Width, p->width, WA_Height, p->height,
        WA_Borderless, TRUE, WA_Activate, FALSE, WA_SmartRefresh, TRUE, WA_NoCareRefresh, TRUE,
        shaped ? WA_ShapeRegion : TAG_IGNORE, (ULONG)p->shape, TAG_DONE);
    if (!p->window || !render(p)) goto fail;
    p->left = p->window->LeftEdge; p->top = p->window->TopEdge;
    return p;
fail:
    amiga_drag_preview_close(p);
    return NULL;
}

/* The save-under for a move: the screen at the new place, except where the
   window still covers it, which the old save-under already holds. */
static int save_under(AmigaDragPreview *p, int left, int top)
{
    int x0 = left > p->left ? left : p->left, y0 = top > p->top ? top : p->top;
    int x1 = left+p->width < p->left+p->width ? left+p->width : p->left+p->width;
    int y1 = top+p->height < p->top+p->height ? top+p->height : p->top+p->height;
    int y;
    DockImage swap;
    if (!amiga_display_capture(p->display,&p->scratch,left,top)) return 0;
    for (y = y0; y < y1 && x0 < x1; ++y)
        memcpy(p->scratch.pixels+((size_t)(y-top)*p->width+(x0-left))*4,
            p->background.pixels+((size_t)(y-p->top)*p->width+(x0-p->left))*4,(size_t)(x1-x0)*4);
    swap = p->background; p->background = p->scratch; p->scratch = swap;
    return 1;
}

static int landed(const AmigaDragPreview *p)
{
    return p->window->LeftEdge == p->left && p->window->TopEdge == p->top;
}

int amiga_drag_preview_move(AmigaDragPreview *p, int x, int y)
{
    int left, top;
    if (!p) return 0;
    /* Intuition queues MoveWindow (on AmigaOS 3.2.3 it lands with the next
       input event, ~120 ms without one). Keep at most one move in flight.
       Without shapes the save-under for the new place is shown once the
       move has landed: drawn before, the window showed it at the old place
       and the background around the icon lagged. */
    if (!landed(p)) return 0;
    if (amiga_drag_preview_settle(p)) return 1;
    position(p,x,y,&left,&top);
    if (left == p->left && top == p->top) return 0;
    if (!save_under(p,left,top)) return 0;
    MoveWindow(p->window,left-p->left,top-p->top);
    p->left = left; p->top = top;
    p->stale = 1;
    /* A shaped window's move lands at once (ApolloOS): draw it now. */
    amiga_drag_preview_settle(p);
    return 1;
}

int amiga_drag_preview_settle(AmigaDragPreview *p)
{
    if (!p || !p->stale || !landed(p)) return 0;
    p->stale = 0;
    render(p);
    return 1;
}

unsigned long amiga_drag_preview_stamp(const AmigaDragPreview *p)
{
    return p ? p->stamp : 0;
}

const DockImage *amiga_drag_preview_icon(const AmigaDragPreview *p, int *x, int *y)
{
    if (!p) return NULL;
    /* Shaped (ApolloOS) moves land at once: where it is going. Without
       shapes, where it is now, so the dock's half stays with the window's. */
    *x = (p->shaped ? p->left : p->window->LeftEdge)+(p->width-p->icon.width)/2;
    *y = p->shaped ? p->top : p->window->TopEdge;
    return &p->icon;
}

int amiga_drag_preview_owns(const AmigaDragPreview *p, const struct Window *window)
{
    return p && p->window == window;
}

void amiga_drag_preview_close(AmigaDragPreview *p)
{
    if (!p) return;
    if (p->window) CloseWindow(p->window);
    if (p->shape) DisposeRegion(p->shape);
    amiga_display_close(p->display);
    free(p->icon.pixels); free(p->picture.pixels);
    free(p->background.pixels); free(p->scratch.pixels); free(p->frame.pixels);
    free(p);
}
