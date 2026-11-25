#include "amiga_ui.h"
#include <inline/macros.h>
#include "dock_compose.h"
#include "amiga_display.h"
#include "amiga_float.h"
#include "amiga_font_aa.h"
#include "about_font.h"
#include "amiga_folder.h"
#include "amiga_text.h"
#include "amiga_version.h"
#include <exec/libraries.h>
#include <exec/memory.h>
#include <graphics/clip.h>
#include <graphics/gfxbase.h>
#include <graphics/regions.h>
#include <graphics/text.h>
#include <intuition/intuitionbase.h>
#include <intuition/screens.h>
#include <graphics/layers.h>
#include <devices/inputevent.h>
#include <devices/timer.h>
#include <libraries/gadtools.h>
#include <proto/diskfont.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/layers.h>
#include <proto/timer.h>
#include <proto/utility.h>
#include <utility/hooks.h>
#define GADTOOLS_BASE_NAME ui->gadtools
#include <proto/gadtools.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef WA_ShapeRegion
#define WA_ShapeRegion (WA_Dummy + 65)
#endif
#ifndef RPTAG_PenMode
#define RPTAG_PenMode 0x80000080UL
#endif
#ifndef RPTAG_FgColor
#define RPTAG_FgColor 0x80000081UL
#endif

enum { GID_CANCEL = 1, GID_SIZE, GID_SCALE, GID_POSITION, GID_SAVE };
enum { SIZE_SLIDER, SCALE_SLIDER, SLIDER_COUNT };
static const int slider_min[] = {DOCK_MIN_ICON_SIZE,DOCK_SCALE_ONE};
static const char *const settings_positions[] = {"Left","Bottom","Right",NULL};
static const DockEdge settings_edges[] = {DOCK_EDGE_LEFT,DOCK_EDGE_BOTTOM,DOCK_EDGE_RIGHT};
/* Size stops at 72 px and Scale at 2.50x, and the zoomed icon at 128 px
   like the macOS Dock: larger zooms blur the 48 px icons and take more
   than a 20 ms frame. */
enum { SIZE_SLIDER_MAX = 72, SCALE_SLIDER_MAX = 5*DOCK_SCALE_ONE/2, ZOOM_CEILING = 128 };
/* Hover labels: a pill with semicircular ends; the text's ink sits
   LABEL_PADDING px from each end and its capitals LABEL_VPAD px from the
   top and bottom. */
enum { POPUP_RADIUS = 6, LABEL_GAP = DOCK_UI_LABEL_GAP, LABEL_MARGIN = 4, LABEL_TIP = 4, LABEL_PADDING = 10,
    LABEL_VPAD = 5 };
enum {
    MENU_SURFACE, MENU_RIM, MENU_HOVER, MENU_TEXT, MENU_HOVER_TEXT,
    FOLDER_MUTED, MENU_COLORS
};
static const ULONG menu_colors[MENU_COLORS] = {
    0xff1b1f27UL, 0xff4b515cUL, 0xff374153UL, 0xfff2f4f8UL, 0xfff2f4f8UL,
    0xffaeb8c7UL
};
typedef struct { const char *label; int action; } MenuItem;
static const MenuItem menu_items[] = {
    {"Settings...", DOCK_UI_SETTINGS}, {"About...", DOCK_UI_ABOUT}, {"Quit", DOCK_UI_QUIT}
};
static const MenuItem close_item = {"Close", DOCK_UI_CLOSE_ITEM};
/* A Folder Stack's menu, after the macOS Dock's: its order (a muted
   heading, the active one checked), a separator, then its box or the
   drawer as a window. A NULL label is the separator; DOCK_UI_NONE rows
   do not react. */
static const MenuItem drawer_items[] = {
    {"Sort by", DOCK_UI_NONE}, {"Name", DOCK_UI_SORT_NAME},
    {"Date Modified", DOCK_UI_SORT_DATE}, {"Kind", DOCK_UI_SORT_KIND}, {NULL, DOCK_UI_NONE},
    {"Show Contents", DOCK_UI_SHOW_CONTENTS}, {"Open in Workbench", DOCK_UI_OPEN_DRAWER}
};
enum { POPUP_MAX_ROWS = sizeof(drawer_items)/sizeof(*drawer_items),
    POPUP_SEPARATOR = 7, /* the separator row's height; its line is in the middle */
    POPUP_INERT = -2 };  /* popup_hit: a heading or separator */
static const MenuItem quit_item = {"Quit", DOCK_UI_QUIT_ITEM};
static const MenuItem temporary_items[] = {
    {"Keep in Dock", DOCK_UI_KEEP_ITEM}, {"Quit", DOCK_UI_QUIT_ITEM}
};
static const MenuItem shell_items[] = {
    {"New Shell", DOCK_UI_NEW_SHELL}, {"Show Shell", DOCK_UI_SHOW_SHELL},
    {"Quit", DOCK_UI_QUIT_SHELL}
};
/* About typography: Vera Sans, prerendered and antialiased (about_font.c),
   because opening it through diskfont takes about 11 s after a reboot.
   gap: space after the line; the larger gaps set off the credit and
   where new releases are. */
static const struct { const char *text; int style, gap; } about_lines[] = {
    {"DockAwesome",ABOUT_TITLE,8}, {dock_build_version,ABOUT_BODY,3},
    {dock_build_date,ABOUT_SMALL,16}, {"Created by",ABOUT_SMALL,3},
    {"Reine \"ZyBeR\" Delwall",ABOUT_NAME,14},
    {"github.com/delwall/DockAwesome/releases",ABOUT_SMALL,0}
};
enum { ABOUT_LINES = sizeof(about_lines)/sizeof(about_lines[0]), ABOUT_MARGIN = 18,
    ABOUT_LINK = ABOUT_LINES-1 };
/* Opened in the default browser through openurl.library when it is there;
   without it the line is plain text. */
static const char about_url[] = "https://github.com/delwall/DockAwesome/releases";
enum {
    FOLDER_COLS = 12, FOLDER_ROWS = 10,

    FOLDER_OPEN_MS = 130, FOLDER_CLOSE_MS = 130,
    FOLDER_TIP = 7, FOLDER_TIP_HALF = 9, FOLDER_BACKDROP_LAYERS = 16,
    FOLDER_GAP = 10, /* between the box and the anchor at the dock */
    FOLDER_TINT_SHIFT = 3, /* the glass passes an eighth of the backdrop */
    FOLDER_OUTSIDE = -1, FOLDER_BACK = -2, FOLDER_PREV = -3, FOLDER_NEXT = -4
};
enum { FOLDER_STILL, FOLDER_OPENING, FOLDER_CLOSING };
typedef struct {
    DockEdge edge;
    int x, y;
    int icon_size, profile;
    DockUIReadIcon read_icon;
    const void *context;
    DockSort sort;
} FolderAnchor;
typedef struct {
    struct Layer *layer;
    struct Rectangle bounds;
    WORD scroll_x, scroll_y;
} FolderBackdropLayer;
typedef struct {
    FolderAnchor anchor;
    char path[DOCK_PREFS_PATH_CAP];
    DockFolderEntry *entries;
    size_t count, page, root_length;
    int cols, rows, cell_width, cell_height, header, footer, read_ok;
    int body_left, body_width, body_height, tip;
    int pad, label_gap, action_wrap;
    int next_icon, artwork_ready, needs_repaint;
    /* --profile: the icons of this page, EClock ticks from the first load
       to the last, and per icon read. */
    ULONG fill_start, read_sum, read_max;
    int read_count, cache_hits, fill_reported;
    int motion, motion_frames, handoff;
    /* Reveal from the tip side: pixels composed or shown along the axis
       away from the dock, those already on screen, and the extent at the
       start of this motion. */
    int reveal_shown, reveal_presented, reveal_from;
    ULONG motion_start, motion_ticks;
    ULONG motion_prepare_sum, motion_present_sum; /* --profile: EClock ticks per stage */
    ULONG motion_field; /* --profile: fields between frames, as digits */
    UWORD motion_frame_us[12]; /* --profile: compose+present per frame */
    char motion_fields[24];
    DockImage motion_backdrop;
    FolderBackdropLayer backdrop_layers[FOLDER_BACKDROP_LAYERS];
    int backdrop_layer_count;
    struct BitMap *handoff_bitmap;
    DockImage motion_scene, motion_frame;
    DockImage motion_out; /* the frame: the scaled, faded box over the backdrop */
    /* The box grows from (and shrinks to) the tip: its scale in 1/256 now
       and at the start of this motion, and the window rectangle it covers
       on screen. motion_frame holds the full-size box over the backdrop. */
    int motion_scale, scale_from;
    DockRect motion_rect;
    DockImage artwork;
    DockImage loaded[FOLDER_COLS*FOLDER_ROWS];
} FolderView;

/* Folder Stack icons as built, kept between openings: reading one takes
   15-50 ms on ApolloOS (DefIcons and PNG icons), so a page of 23 took half
   a second each time. An entry is current while the file's and its .info's
   dates and the size match. 512 hold the first pages of many stacks (about
   4.7 MB of pixels at Size 48); key is the path's hash, compared first. */
enum { FOLDER_ICON_CACHE = 512 };
typedef struct {
    unsigned long key;
    char path[DOCK_PREFS_PATH_CAP];
    unsigned long stamp[4];
    DockImage icon;
} FolderCachedIcon;
/* dock_ui_prefetch_folder, one per slot: a drawer's first page (for the
   page size the box would have at the anchor edge,x,y), and how far it got. */
typedef struct {
    char path[DOCK_PREFS_PATH_CAP];
    int icon_size, x, y;
    DockEdge edge;
    DockSort sort;
    DockFolderEntry *entries;
    size_t count, next;
} FolderPrefetch;

typedef struct {
    struct Gadget *slider[SLIDER_COUNT], *readout[SLIDER_COUNT], *position;
    struct Gadget *cancel, *save;
    int limit[SLIDER_COUNT]; /* What fits now. */
    int cap[SLIDER_COUNT];   /* Slider range, raised for larger saved values. */
    int zoom_cap;            /* ZOOM_CEILING, raised for a larger saved zoom. */
    int shown_max[SLIDER_COUNT]; /* GTSL_Max the sliders were given. */
    int moving, moving_level;    /* Slider reporting its knob level now, or -1. */
    char value[SLIDER_COUNT][16];
    /* The fit hook ignores Size: its result for this edge and scale,
       while fit_valid. */
    int fit_valid, fit_size, fit_edge, fit_scale;
} SettingsView;

typedef struct FolderReveal FolderReveal;


struct DockUI {
    struct Screen *screen;
    struct Library *gadtools;
    APTR visual;
    struct DrawInfo *draw;
    struct TextAttr font;
    struct TextFont *folder_font;
    AmigaFontAA *folder_aa;
    AmigaFontAA *chrome_aa;
    struct Library *text_alpha;
    struct Window *popup, *dialog;
    struct Region *popup_shape;
    /* The popup is drawn rounded (with the Folder Stack's tip): by its
       shape, or over its captured glass where windows cannot be shaped. */
    int popup_round;
    /* The font's ink boxes of "H" and of the tall reference string. */
    DockRect label_caps, label_tall;
    int label_metrics; /* 0 unmeasured, 1 measured, 2 measured with label_tall */
    const MenuItem *popup_items;
    int popup_rows;
    /* Each row's top in the window and the bottom of the last; the labels'
       left edge (past the check column in a stack's menu); the checked
       row, or -1. */
    int popup_top[POPUP_MAX_ROWS+1];
    int popup_text_left, popup_checked;
    AmigaDisplay *popup_display;
    DockConvertRGB24 encode;
    DockConvertRGB565 encode565;
    DockConvertRGB565 decode;
    DockMixHalf mix_half; /* the renderer's 50% mix (AMMX on the 68080) */
    DockImage popup_glass;
    int glass_tint_shift; /* nonzero: popup_glass still holds the bare backdrop */
    FolderView folder;
    /* FOLDER_ICON_CACHE entries: free ones first, then (only for the open
       box) round robin; used counts live entries, evicted the live entries
       of another path the box overwrote. */
    FolderCachedIcon *icon_cache;
    int icon_cache_next, icon_cache_used;
    unsigned long icon_cache_evicted;
    FolderPrefetch prefetch[DOCK_MAX_ITEMS];
    FolderReveal *folder_reveal;
    char folder_open_path[DOCK_PREFS_PATH_CAP];
    DockImage backdrop_carry;
    int carry_left, carry_top;
    DockUIFit fit;
    const volatile ULONG *fields; /* --profile: the display's field count */
    void *fit_context;
    char dialog_title[64], busy_title[96];
    struct Gadget *gadgets;
    DockUIPrefs draft;
    SettingsView settings;
    int font_height, font_width, popup_row, popup_selected, popup_pressed;
    int window_shapes, popup_rgb, rgb_pens, showing_about;
    struct Library *openurl; /* while About is open, when installed */
    LONG link_pen;           /* a blue for the link, -1 without one */
    DockRect about_link;     /* the link line in the About window */
    UWORD popup_pen[MENU_COLORS];
};

static int text_length32(struct RastPort *rp, const char *text)
{
    return amiga_text_length(rp,text,(ULONG)strlen(text));
}

static void text32(struct RastPort *rp, const char *text)
{
    amiga_text_draw(rp,text,(ULONG)strlen(text));
}

static int ready(DockUI *ui)
{
    if (!ui->gadtools) ui->gadtools = OpenLibrary((STRPTR)"gadtools.library", 37);
    if (!ui->gadtools) return 0;
    if (!ui->visual) ui->visual = GetVisualInfo(ui->screen, TAG_DONE);
    return ui->visual != NULL;
}

static void free_popup_glass(DockUI *ui)
{
    amiga_display_close(ui->popup_display);
    ui->popup_display = NULL;
    free(ui->popup_glass.pixels);
    ui->popup_glass = (DockImage){0};
    ui->glass_tint_shift = 0;
}

static void free_folder_icons(FolderView *folder)
{
    int i;
    for (i = 0; i < FOLDER_COLS*FOLDER_ROWS; ++i) {
        free(folder->loaded[i].pixels);
        folder->loaded[i] = (DockImage){0};
    }
}

struct FolderBackfillMessage {
    struct Layer *layer;
    struct Rectangle bounds;
    LONG offset_x, offset_y;
};

typedef struct {
    struct Hook hook;
    struct Hook *original;
    struct BitMap *source, *screen;
    struct Rectangle bounds;
    FolderBackdropLayer target;
    FolderReveal *reveal;
    int restored;
} FolderBackfill;

struct FolderReveal {
    FolderBackfill fills[FOLDER_BACKDROP_LAYERS];
    AmigaDisplay *display;
    volatile ULONG deadline;
    volatile int closing, expired;
    int count, kept;
};

extern ULONG dock_folder_backfill_entry(void);

/* Layers supplies a screen-clipped rectangle. Never reenter a layer-aware
   graphics call from its own backfill callback. */
ULONG dock_folder_backfill(struct Hook *hook, struct RastPort *port,
    const struct FolderBackfillMessage *message)
{
    FolderBackfill *fill = hook->h_Data;
    const struct Rectangle *r = &message->bounds;
    const struct Layer *layer = message->layer;
    int x = r->MinX, y = r->MinY;
    int w = r->MaxX-x+1, h = r->MaxY-y+1;
    int preserve = fill->reveal->closing;
    if (!preserve && !fill->reveal->expired && layer && (layer->Flags&LAYERUPDATING)) {
        struct EClockVal now;
        ReadEClock(&now);
        preserve = (LONG)(now.ev_lo-fill->reveal->deadline) < 0;
    }
    if (preserve && layer == fill->target.layer &&
        layer->Scroll_X == fill->target.scroll_x && layer->Scroll_Y == fill->target.scroll_y &&
        !memcmp(&layer->bounds,&fill->target.bounds,sizeof(layer->bounds)) &&
        port->BitMap == fill->screen && x >= fill->bounds.MinX && y >= fill->bounds.MinY &&
        r->MaxX <= fill->bounds.MaxX && r->MaxY <= fill->bounds.MaxY) {
        BltBitMap(fill->source,x-fill->bounds.MinX,y-fill->bounds.MinY,
            port->BitMap,x,y,w,h,0xc0,0xff,NULL);
    } else if (fill->original == LAYERS_BACKFILL) {
        BltBitMap(port->BitMap,0,0,port->BitMap,x,y,w,h,0,0xff,NULL);
    } else if (fill->original != LAYERS_NOBACKFILL) {
        CallHookPkt(fill->original,port,(APTR)message);
    }
    return 0;
}

static int folder_backdrop_layers(struct Layer *layer, const struct Rectangle *bounds,
    FolderBackdropLayer *result)
{
    int n = 0;
    for (; layer; layer = layer->back) {
        const struct Rectangle *r = &layer->bounds;
        if (r->MaxX < bounds->MinX || r->MinX > bounds->MaxX ||
            r->MaxY < bounds->MinY || r->MinY > bounds->MaxY) continue;
        if (n == FOLDER_BACKDROP_LAYERS) return -1;
        result[n++] = (FolderBackdropLayer){layer,*r,layer->Scroll_X,layer->Scroll_Y};
    }
    return n;
}

/* Hook code that must outlive the dock: a foreign layer owner may call a hook
   we could not remove after DockAwesome has quit. Allocated once, never freed.
   [0] moveq #0,d0; rts           no-op for the default backfill choices
   [2] movea.l h_Data(a0),a0; move.l h_Entry(a0),-(sp); rts
                                   forward to the original hook in h_Data */
static const UWORD *persistent_hook_code(void)
{
    static UWORD *code;
    static const UWORD words[] = {0x7000,0x4e75,0x2068,0x0010,0x2f28,0x0008,0x4e75};
    if (!code && (code = AllocVec(sizeof(words),MEMF_PUBLIC))) {
        CopyMem((APTR)words,code,sizeof(words));
        CacheClearU();
    }
    return code;
}

/* --profile: EClock at each stage of an opening, printed with its motion. */
enum { FOLDER_STAGES = 13 };
static ULONG folder_stage[FOLDER_STAGES];
static int folder_stages;
static void folder_stamp(int profile, int first)
{
    struct EClockVal now;
    if (!profile) return;
    if (first) folder_stages = 0;
    if (folder_stages >= FOLDER_STAGES) return;
    ReadEClock(&now);
    folder_stage[folder_stages++] = now.ev_lo;
}

static void finish_folder_reveal(DockUI *ui, int wait)
{
    FolderReveal *reveal = ui->folder_reveal;
    struct Layer_Info *info = &ui->screen->LayerInfo;
    struct Layer *layer;
    int i, busy = 0;
    if (!reveal) return;
    reveal->expired = 1;
    if (wait) LockLayerInfo(info);
    else if (!AttemptSemaphore(&info->Lock)) return;
    for (i = 0; i < reveal->count; ++i) {
        FolderBackfill *fill = &reveal->fills[i];
        if (fill->restored) continue;
        for (layer = info->top_layer; layer && layer != fill->target.layer; layer = layer->back) {}
        if (!layer) { fill->restored = 1; continue; }
        if (wait) LockLayer(0,layer);
        else if (!AttemptSemaphore(&layer->Lock)) { busy = 1; continue; }
        if (layer->BackFill == &fill->hook) InstallLayerHook(layer,fill->original);
        else {
            /* The owner installed its own hook and may restore or chain ours
               later, even after we quit. Keep this hook allocated and point
               it at persistent code: forward to the original, or do nothing
               for the default backfill choices. */
            const UWORD *code = persistent_hook_code();
            int real = fill->original != LAYERS_BACKFILL && fill->original != LAYERS_NOBACKFILL;
            Forbid();
            if (code) {
                fill->hook.h_SubEntry = NULL;
                fill->hook.h_Data = real ? (APTR)fill->original : NULL;
                fill->hook.h_Entry = (APTR)(real ? code+2 : code);
            } else if (real) {
                fill->hook.h_SubEntry = fill->original->h_SubEntry;
                fill->hook.h_Data = fill->original->h_Data;
                fill->hook.h_Entry = fill->original->h_Entry;
            }
            Permit();
            reveal->kept = 1;
        }
        fill->restored = 1;
        if (wait) UnlockLayer(layer);
        else ReleaseSemaphore(&layer->Lock);
    }
    if (wait) UnlockLayerInfo(info);
    else ReleaseSemaphore(&info->Lock);
    if (busy) return;
    amiga_display_close(reveal->display);
    reveal->display = NULL;
    if (!reveal->kept) FreeVec(reveal);
    ui->folder_reveal = NULL;
}

static void close_folder_window(DockUI *ui, int preserve)
{
    FolderView *f = &ui->folder;
    struct Window *w = ui->popup;
    struct Layer_Info *info = &ui->screen->LayerInfo;
    struct Rectangle bounds = {w->LeftEdge,w->TopEdge,
        w->LeftEdge+w->Width-1,w->TopEdge+w->Height-1};
    FolderBackdropLayer current[FOLDER_BACKDROP_LAYERS];
    /* Public memory: a foreign layer owner may keep a pointer to a hook. */
    FolderReveal *reveal = preserve && f->handoff_bitmap && f->backdrop_layer_count > 0 ?
        AllocVec(sizeof(*reveal),MEMF_PUBLIC|MEMF_CLEAR) : NULL;
    int n, i;
    if (reveal) {
        reveal->closing = 1;
        LockLayers(info);
        n = folder_backdrop_layers(w->WLayer->back,&bounds,current);
        if (n == f->backdrop_layer_count) {
            for (i = 0; i < n; ++i) {
                const FolderBackdropLayer *a = &current[i], *b = &f->backdrop_layers[i];
                if (a->layer != b->layer || a->scroll_x != b->scroll_x || a->scroll_y != b->scroll_y ||
                    memcmp(&a->bounds,&b->bounds,sizeof(a->bounds))) break;
            }
            if (i == n) for (i = 0; i < n; ++i) {
                struct Layer *layer = current[i].layer;
                FolderBackfill *fill = &reveal->fills[reveal->count];
                if (!layer->Window || !(layer->Flags&LAYERSIMPLE)) continue;
                fill->hook.h_Entry = dock_folder_backfill_entry;
                fill->hook.h_Data = fill;
                fill->bounds = bounds;
                fill->target = current[i];
                fill->reveal = reveal;
                fill->source = f->handoff_bitmap;
                fill->screen = ui->screen->RastPort.BitMap;
                fill->original = InstallLayerHook(layer,&fill->hook);
                ++reveal->count;
            }
        }
        UnlockLayers(info);
    }
    CloseWindow(w);
    if (reveal && reveal->count) {
        struct EClockVal now;
        ULONG hz = ReadEClock(&now);
        /* The exposed Simple Refresh windows can repaint after CloseWindow
           returns. Keep their backdrop available during that refresh only. */
        reveal->deadline = now.ev_lo+hz*140/1000;
        reveal->closing = 0;
        reveal->display = ui->popup_display;
        ui->popup_display = NULL;
        ui->folder_reveal = reveal;
    } else if (reveal) FreeVec(reveal);
}

static void close_popup(DockUI *ui, int preserve)
{
    struct IntuiMessage *msg;
    finish_folder_reveal(ui,1);
    if (ui->popup) {
        while ((msg = (struct IntuiMessage *)GetMsg(ui->popup->UserPort))) ReplyMsg((struct Message *)msg);
        close_folder_window(ui,preserve);
        ui->popup = NULL;
    }
    if (ui->popup_shape) {
        DisposeRegion(ui->popup_shape);
        ui->popup_shape = NULL;
    }
    ui->popup_round = 0;
    free_popup_glass(ui);
    free_folder_icons(&ui->folder);
    free(ui->folder.entries);
    free(ui->folder.artwork.pixels);
    free(ui->folder.motion_backdrop.pixels);
    free(ui->folder.motion_scene.pixels);
    free(ui->folder.motion_frame.pixels);
    free(ui->folder.motion_out.pixels);
    ui->folder = (FolderView){0};
    ui->popup_selected = ui->popup_pressed = -1;
}

void dock_ui_close_dialog(DockUI *ui)
{
    struct IntuiMessage *msg;
    if (!ui) return;
    if (ui->dialog) {
        while ((msg = GT_GetIMsg(ui->dialog->UserPort))) GT_ReplyIMsg(msg);
        CloseWindow(ui->dialog);
        ui->dialog = NULL;
    }
    if (ui->gadgets) FreeGadgets(ui->gadgets);
    ui->gadgets = NULL;
    memset(&ui->settings,0,sizeof(ui->settings));
    if (ui->openurl) CloseLibrary(ui->openurl);
    ui->openurl = NULL;
    if (ui->link_pen >= 0) ReleasePen(ui->screen->ViewPort.ColorMap,(ULONG)ui->link_pen);
    ui->link_pen = -1;
    ui->showing_about = 0;
}

DockUI *dock_ui_create(struct Screen *screen, int window_shapes, int glass, int rgb_pens, int alpha_text)
{
    DockUI *ui;
    struct Library *DiskfontBase;
    struct TextAttr folder_face = {(STRPTR)"Helvetica.font",9,FS_NORMAL,0};
    if (!screen || !screen->Font) return NULL;
    ui = (DockUI *)calloc(1, sizeof(*ui));
    if (!ui) return NULL;
    ui->screen = screen;
    ui->link_pen = -1;
    ui->window_shapes = window_shapes;
    ui->popup_rgb = glass;
    ui->rgb_pens = glass && rgb_pens;
    ui->draw = GetScreenDrawInfo(screen);
    if (!ui->draw || !ui->draw->dri_Font) {
        if (ui->draw) FreeScreenDrawInfo(screen,ui->draw);
        free(ui); return NULL;
    }
    ui->font = *screen->Font;
    ui->font_height = ui->draw->dri_Font->tf_YSize;
    ui->font_width = ui->draw->dri_Font->tf_XSize;
    if (ui->font_height < 8) ui->font_height = 8;
    if (ui->font_width < 6) ui->font_width = 6;
    DiskfontBase = OpenLibrary((STRPTR)"diskfont.library",37);
    if (DiskfontBase) {
        ui->folder_font = OpenDiskFont(&folder_face);
        CloseLibrary(DiskfontBase);
    }
    if (ui->folder_font && ui->folder_font->tf_YSize > folder_face.ta_YSize) {
        CloseFont(ui->folder_font);
        ui->folder_font = NULL;
    }
    if (!ui->folder_font) {
        folder_face.ta_Name = (STRPTR)"topaz.font";
        folder_face.ta_YSize = 8;
        ui->folder_font = OpenFont(&folder_face);
    }
    if (ui->popup_rgb && ui->folder_font) ui->folder_aa = amiga_font_aa_create(ui->folder_font);
    /* The hover label's text, drawn in software into the dock's image: on
       every system. */
    ui->chrome_aa = amiga_font_aa_create(ui->draw->dri_Font);
    /* amiga_caps_probe checked V51 BltTemplateAlpha (antialiased text in
       windows); keep the base open. */
    if (alpha_text) ui->text_alpha = OpenLibrary((STRPTR)"cybergraphics.library",51);
    ui->popup_selected = ui->popup_pressed = -1;
    return ui;
}

void dock_ui_destroy(DockUI *ui)
{
    int i;
    if (!ui) return;
    close_popup(ui,0);
    dock_ui_close_dialog(ui);
    if (ui->visual) FreeVisualInfo(ui->visual);
    if (ui->gadtools) CloseLibrary(ui->gadtools);
    amiga_font_aa_free(ui->folder_aa);
    amiga_font_aa_free(ui->chrome_aa);
    if (ui->text_alpha) CloseLibrary(ui->text_alpha);
    if (ui->folder_font) CloseFont(ui->folder_font);
    for (i = 0; i < DOCK_MAX_ITEMS; ++i) free(ui->prefetch[i].entries);
    if (ui->icon_cache) {
        for (i = 0; i < FOLDER_ICON_CACHE; ++i) free(ui->icon_cache[i].icon.pixels);
        free(ui->icon_cache);
    }
    FreeScreenDrawInfo(ui->screen, ui->draw);
    free(ui);
}

ULONG dock_ui_signal_mask(const DockUI *ui)
{
    ULONG mask = 0;
    if (!ui) return 0;
    if (ui->popup) mask |= 1UL << ui->popup->UserPort->mp_SigBit;
    if (ui->dialog) mask |= 1UL << ui->dialog->UserPort->mp_SigBit;
    return mask;
}

int dock_ui_owns_window(const DockUI *ui, const struct Window *window)
{
    return ui && window && (window == ui->popup || window == ui->dialog);
}

int dock_ui_busy(const DockUI *ui)
{
    return ui && (ui->popup || ui->dialog);
}

int dock_ui_holds_dock(const DockUI *ui)
{
    return ui && (ui->dialog ||
        (ui->popup && !(ui->folder.path[0] && ui->folder.motion == FOLDER_STILL)));
}

static int popup_hit(const DockUI *ui, int x, int y)
{
    int i;
    if (x < 3 || x >= ui->popup->Width-3 || y < ui->popup_top[0] || y >= ui->popup_top[ui->popup_rows])
        return -1;
    for (i = 0; y >= ui->popup_top[i+1]; ++i) {}
    return ui->popup_items[i].action ? i : POPUP_INERT;
}

static int rounded_inset(int radius, int row)
{
    int inset = 0, dy = 2*(radius-row)-1;
    if (row >= radius) return 0;
    while (inset < radius) {
        int dx = 2*(radius-inset)-1;
        if (dx*dx+dy*dy <= 4*radius*radius) break;
        ++inset;
    }
    return inset;
}

static void popup_span(const FolderView *folder, int width, int height, int radius,
    int row, int *left, int *right)
{
    int body_height = folder ? folder->body_height : height;
    int opposite = body_height-1-row;
    if (folder && row >= body_height) {
        int half = (height-1-row)*FOLDER_TIP_HALF/FOLDER_TIP+1;
        *left = folder->tip-half;
        *right = folder->tip+half+1;
    } else {
        int inset = rounded_inset(radius,row < opposite ? row : opposite);
        *left = inset+(folder ? folder->body_left : 0);
        *right = (folder ? folder->body_left+folder->body_width : width)-inset;
        if (folder && folder->anchor.edge != DOCK_EDGE_BOTTOM) {
            int distance = abs(row-folder->tip);
            int tip = distance < FOLDER_TIP_HALF ? FOLDER_TIP-distance*FOLDER_TIP/FOLDER_TIP_HALF : 0;
            if (folder->anchor.edge == DOCK_EDGE_LEFT) *left -= tip;
            else *right += tip;
        }
    }
}

static void popup_inner_span(const FolderView *folder, int width, int height, int radius,
    int row, int lo, int hi, int *inner_lo, int *inner_hi)
{
    int above_lo, above_hi, below_lo, below_hi;
    popup_span(folder,width,height,radius,row ? row-1 : row,&above_lo,&above_hi);
    popup_span(folder,width,height,radius,row+1 < height ? row+1 : row,&below_lo,&below_hi);
    *inner_lo = dock_clamp(above_lo,lo+1,hi);
    if (*inner_lo < below_lo) *inner_lo = below_lo;
    *inner_hi = dock_clamp(above_hi,lo,hi-1);
    if (*inner_hi > below_hi) *inner_hi = below_hi;
    if (!row || row+1 == height || *inner_hi < *inner_lo) *inner_lo = *inner_hi = lo;
}

void dock_ui_popup_span(const DockUI *ui, int row, int *left, int *right)
{
    int width = ui->popup->Width, height = ui->popup->Height;
    if (!ui->popup_round) { *left = 0; *right = width; return; }
    popup_span(ui->folder.path[0] ? &ui->folder : NULL,
        width,height,POPUP_RADIUS,row,left,right);
}

/* The outline in the window's span; with a glass band, the strip the tip
   slides in is whole: the tip follows the stack pin by repainting it from
   the backdrop, without a shape change (each one had Workbench redraw the
   desktop behind and took dock frames from 10 to 24 ms on the 68080). */
static void folder_shape_span(const FolderView *folder, int width, int height, int band,
    int row, int *left, int *right)
{
    popup_span(folder,width,height,POPUP_RADIUS,row,left,right);
    if (!folder || !band) return;
    if (folder->anchor.edge == DOCK_EDGE_BOTTOM) {
        if (row >= folder->body_height) { *left = 0; *right = folder->body_width; }
    } else if (folder->anchor.edge == DOCK_EDGE_LEFT) *left = 0;
    else *right = width;
}

static struct Region *make_folder_shape(const FolderView *folder, int width, int height, int band)
{
    struct Region *shape = NewRegion();
    int start = 0, left, right, row;
    if (!shape) return NULL;
    folder_shape_span(folder,width,height,band,0,&left,&right);
    for (row = 1; row <= height; ++row) {
        int next_left = 0, next_right = 0;
        if (row < height)
            folder_shape_span(folder,width,height,band,row,&next_left,&next_right);
        if (left != next_left || right != next_right) {
            struct Rectangle rect = {left,start,right-1,row-1};
            if (!OrRectRegion(shape,&rect)) { DisposeRegion(shape); return NULL; }
            start = row;
            left = next_left;
            right = next_right;
        }
    }
    return shape;
}

static void prepare_popup_colors(DockUI *ui)
{
    static const UWORD fallback[MENU_COLORS] = {
        BACKGROUNDPEN, SHADOWPEN, FILLPEN, TEXTPEN, FILLTEXTPEN,
        TEXTPEN
    };
    struct ColorMap *map = ui->screen->ViewPort.ColorMap;
    int i, last_pen;
    last_pen = map && map->Count ? (map->Count < 256 ? map->Count-1 : 255) : -1;
    for (i = 0; i < MENU_COLORS; ++i) {
        ULONG rgb = menu_colors[i];
        LONG pen = last_pen >= 0 ? FindColor(map,((rgb>>16)&255)*0x01010101UL,
            ((rgb>>8)&255)*0x01010101UL,(rgb&255)*0x01010101UL,last_pen) : -1;
        ui->popup_pen[i] = pen >= 0 ? (UWORD)pen : ui->draw->dri_Pens[fallback[i]];
    }
    if (ui->popup_pen[MENU_TEXT] == ui->popup_pen[MENU_SURFACE]) {
        ui->popup_pen[MENU_SURFACE] = ui->draw->dri_Pens[BACKGROUNDPEN];
        ui->popup_pen[MENU_TEXT] = ui->draw->dri_Pens[TEXTPEN];
    }
    if (ui->popup_pen[MENU_HOVER] == ui->popup_pen[MENU_SURFACE] ||
        ui->popup_pen[MENU_HOVER] == ui->popup_pen[MENU_HOVER_TEXT]) {
        ui->popup_pen[MENU_HOVER] = ui->draw->dri_Pens[FILLPEN];
        ui->popup_pen[MENU_HOVER_TEXT] = ui->draw->dri_Pens[FILLTEXTPEN];
    }
}

static void popup_color(const DockUI *ui, struct RastPort *rp, int role)
{
    if (ui->rgb_pens) {
        const struct TagItem tags[] = {{RPTAG_PenMode, FALSE},
            {RPTAG_FgColor, menu_colors[role]}, {TAG_DONE, 0}};
        SetRPAttrsA(rp,tags);
    } else SetAPen(rp,ui->popup_pen[role]);
}

static void paint_ui_text(DockUI *ui, struct RastPort *rp, AmigaFontAA *font,
    const char *text, int x, int top, int color)
{
    popup_color(ui,rp,color);
    if (!amiga_font_aa_blit(font,ui->text_alpha,rp,text,x,top)) {
        Move(rp,x,top+rp->TxBaseline);
        text32(rp,text);
    }
}

static void fill_rounded(struct RastPort *rp, int x, int y, int width, int height, int radius)
{
    int row, right = x+width-1, bottom = y+height-1;
    /* RectFill needs ymax >= ymin: an even height of two radii has no middle. */
    if (bottom-radius >= y+radius) RectFill(rp,x,y+radius,right,bottom-radius);
    for (row = 0; row < radius; ++row) {
        int inset = rounded_inset(radius,row);
        RectFill(rp,x+inset,y+row,right-inset,y+row);
        RectFill(rp,x+inset,bottom-row,right-inset,bottom-row);
    }
}

static AmigaDisplay *capture_glass(const DockUI *ui, DockImage *glass,
    int left, int top, int width, int height)
{
    AmigaDisplay *display;
    if (!ui->popup_rgb) return NULL;
    display = amiga_display_open(ui->screen,width,height);
    if (!display) return NULL;
    amiga_display_set_converters(display,ui->encode,ui->decode,ui->encode565);
    *glass = (DockImage){width,height,malloc((size_t)width*height*4)};
    if (glass->pixels && amiga_display_capture(display,glass,left,top)) return display;
    free(glass->pixels);
    *glass = (DockImage){0};
    amiga_display_close(display);
    return NULL;
}

static uint32_t glass_tint(int shift)
{
    uint32_t tint = 255;
    int c;
    for (c = 0; c < 3; ++c) {
        unsigned int channel = (menu_colors[MENU_SURFACE] >> (16-8*c)) & 255;
        unsigned int base = ((1U<<shift)-1)*channel+(1U<<(shift-1));
        tint |= (base>>shift)<<(24-8*c);
    }
    return tint;
}

/* Copy the previous view's clean backdrop where it overlaps the new view.
   Simple Refresh windows behind the old view have not repainted yet. */
static void apply_backdrop_carry(const DockUI *ui, DockImage *glass, int left, int top)
{
    const DockImage *carry = &ui->backdrop_carry;
    int x0 = left > ui->carry_left ? left : ui->carry_left;
    int y0 = top > ui->carry_top ? top : ui->carry_top;
    int x1 = left+glass->width, y1 = top+glass->height, y;
    if (!carry->pixels) return;
    if (x1 > ui->carry_left+carry->width) x1 = ui->carry_left+carry->width;
    if (y1 > ui->carry_top+carry->height) y1 = ui->carry_top+carry->height;
    if (x1 <= x0 || y1 <= y0) return;
    for (y = y0; y < y1; ++y)
        memcpy(glass->pixels+((size_t)(y-top)*glass->width+(x0-left))*4,
            carry->pixels+((size_t)(y-ui->carry_top)*carry->width+(x0-ui->carry_left))*4,
            (size_t)(x1-x0)*4);
}

/* Tint the captured backdrop into the popup's glass, with its rim. */
static void tint_popup_glass(DockUI *ui)
{
    DockImage *glass = &ui->popup_glass;
    const FolderView *folder = ui->popup_round && ui->folder.path[0] ? &ui->folder : NULL;
    int radius = ui->popup_round ? POPUP_RADIUS : 0, tint_shift = ui->glass_tint_shift;
    int width = glass->width, height = glass->height, y;
    const uint32_t rim = (menu_colors[MENU_RIM]<<8)|255;
    uint32_t mask, tint;
    if (!tint_shift || !glass->pixels) return;
    ui->glass_tint_shift = 0;
    mask = (255U>>tint_shift)*UINT32_C(0x01010100);
    tint = glass_tint(tint_shift);
    for (y = 0; y < height; ++y) {
        int lo, hi, inner_lo, inner_hi, x;
        uint32_t *row = (uint32_t *)glass->pixels+(size_t)y*width;
        popup_span(folder,width,height,radius,y,&lo,&hi);
        popup_inner_span(folder,width,height,radius,y,lo,hi,&inner_lo,&inner_hi);
        /* Native RGBA bytes are a big-endian word. Independent lanes keep
           the background contribution and tint from spilling across channels.
           Whole words: a memcpy per pixel took 16-24 ms on the 68080. */
        for (x = lo; x < inner_lo; ++x) row[x] = rim;
        for (; x < inner_hi; ++x) row[x] = ((row[x]>>tint_shift)&mask)+tint;
        for (; x < hi; ++x) row[x] = rim;
    }
}

/* tint_popup_glass for the rectangle [x0,x1) x [y0,y1) of a folder's
   glass, from the clean backdrop under it: outside the outline the
   backdrop, then rim and tinted glass as the outline now runs. */
static void retint_folder_glass(const FolderView *f, DockImage *glass, const DockImage *under,
    int x0, int y0, int x1, int y1)
{
    int width = glass->width, height = glass->height, y;
    const uint32_t rim = (menu_colors[MENU_RIM]<<8)|255;
    const uint32_t mask = (255U>>FOLDER_TINT_SHIFT)*UINT32_C(0x01010100);
    const uint32_t tint = glass_tint(FOLDER_TINT_SHIFT);
    for (y = y0; y < y1; ++y) {
        int lo, hi, inner_lo, inner_hi, x;
        uint32_t *row = (uint32_t *)glass->pixels+(size_t)y*width;
        const uint32_t *clean = (const uint32_t *)under->pixels+(size_t)y*width;
        popup_span(f,width,height,POPUP_RADIUS,y,&lo,&hi);
        popup_inner_span(f,width,height,POPUP_RADIUS,y,lo,hi,&inner_lo,&inner_hi);
        for (x = x0; x < x1; ++x)
            row[x] = x < lo || x >= hi ? clean[x] :
                x >= inner_lo && x < inner_hi ? ((clean[x]>>FOLDER_TINT_SHIFT)&mask)+tint : rim;
    }
}

static void capture_popup_glass(DockUI *ui, int left, int top, int width, int height, int tint_shift)
{
    DockImage *glass = &ui->popup_glass;
    const FolderView *folder = ui->popup_round && ui->folder.path[0] ? &ui->folder : NULL;
    int profile = folder && ui->folder.anchor.profile;
    ui->popup_display = capture_glass(ui,glass,left,top,width,height);
    if (!ui->popup_display) return;
    folder_stamp(profile,0);
    apply_backdrop_carry(ui,glass,left,top);
    if (folder) {
        size_t bytes = (size_t)width*height*4;
        DockImage *under = &ui->folder.motion_backdrop;
        *under = (DockImage){width,height,malloc(bytes)};
        if (under->pixels) {
            struct Layer_Info *info = &ui->screen->LayerInfo;
            struct Rectangle bounds = {left,top,left+width-1,top+height-1};
            memcpy(under->pixels,glass->pixels,bytes);
            LockLayers(info);
            ui->folder.backdrop_layer_count =
                folder_backdrop_layers(info->top_layer,&bounds,ui->folder.backdrop_layers);
            UnlockLayers(info);
        }
    }
    folder_stamp(profile,0);
    /* An opening Folder Stack shows its own frames until the motion ends:
       tint then (8 ms on the 68080), not before the first frame. */
    ui->glass_tint_shift = tint_shift;
    if (!folder || ui->folder.motion != FOLDER_OPENING) tint_popup_glass(ui);
}

static void paint_popup_surface(DockUI *ui, const DockImage *surface)
{
    struct RastPort *rp = ui->popup->RPort;
    int width = ui->popup->Width, height = ui->popup->Height;
    SetFont(rp, ui->draw->dri_Font);
    SetDrMd(rp, JAM1);
    if (!surface->pixels || !amiga_display_present(ui->popup_display,ui->popup,surface)) {
        popup_color(ui,rp,MENU_RIM);
        RectFill(rp,0,0,width-1,height-1);
        popup_color(ui,rp,MENU_SURFACE);
        fill_rounded(rp,1,1,width-2,height-2,ui->popup_round ? POPUP_RADIUS-1 : 0);
    }
}

/* A check mark in the check column, two pixels thick, sized by the font:
   a short stroke down to the foot, a long one up to the right. */
static void paint_check(DockUI *ui, struct RastPort *rp, int top, int color)
{
    int h = (ui->font_height*3+2)/4, x = 10, y = top+(ui->popup_row-h)/2, t;
    popup_color(ui,rp,color);
    for (t = 0; t < 2; ++t) {
        Move(rp,x+t,y+h/2);
        Draw(rp,x+t+h/3,y+h-1);
        Draw(rp,x+t+h,y);
    }
}

static void paint_popup_item(DockUI *ui, int i)
{
    struct RastPort *rp = ui->popup->RPort;
    const MenuItem *item = &ui->popup_items[i];
    int top = ui->popup_top[i], height = ui->popup_top[i+1]-top;
    int color = i == ui->popup_selected ? MENU_HOVER_TEXT : item->action ? MENU_TEXT : FOLDER_MUTED;
    if (!item->label) {
        popup_color(ui,rp,MENU_RIM);
        /* Inset like the labels on both sides: a hairline between groups. */
        RectFill(rp,10,top+height/2,ui->popup->Width-11,top+height/2);
        return;
    }
    if (i == ui->popup_selected) {
        popup_color(ui,rp,MENU_HOVER);
        fill_rounded(rp,3,top,ui->popup->Width-6,height,4);
    }
    if (i == ui->popup_checked) paint_check(ui,rp,top,color);
    paint_ui_text(ui,rp,ui->chrome_aa,item->label,ui->popup_text_left,
        top+(height-ui->font_height)/2,color);
}

static void paint_popup(DockUI *ui)
{
    int i;
    paint_popup_surface(ui,&ui->popup_glass);
    for (i = 0; i < ui->popup_rows; ++i) paint_popup_item(ui,i);
}

/* A hover change repaints only the rows it touches. Rows touch and each
   one's highlight and text stay inside its band, so restoring the band's
   full width from the tinted glass (rim and corners included) and drawing
   that row again gives the pixels of a full repaint. Antialiased text
   blends, so the band is restored first. Without RTG glass, repaint all. */
static void paint_popup_hover(DockUI *ui, int previous)
{
    int rows[2], count = 0, i;
    if (!ui->popup_glass.pixels || ui->glass_tint_shift ||
        ui->popup_glass.width != ui->popup->Width || ui->popup_glass.height != ui->popup->Height) {
        paint_popup(ui); return;
    }
    if (previous >= 0 && previous < ui->popup_rows) rows[count++] = previous;
    if (ui->popup_selected >= 0 && ui->popup_selected < ui->popup_rows &&
        ui->popup_selected != previous) rows[count++] = ui->popup_selected;
    for (i = 0; i < count; ++i)
        if (!amiga_display_present_tile(ui->popup_display,ui->popup,&ui->popup_glass,
                0,ui->popup_top[rows[i]],ui->popup->Width,
                ui->popup_top[rows[i]+1]-ui->popup_top[rows[i]])) {
            paint_popup(ui); return;
        }
    SetFont(ui->popup->RPort,ui->draw->dri_Font);
    SetDrMd(ui->popup->RPort,JAM1);
    for (i = 0; i < count; ++i) paint_popup_item(ui,rows[i]);
}

static size_t folder_page_size(const FolderView *f)
{
    return f->cols*f->rows-1; /* Reserve one tile for opening the drawer in Workbench. */
}

static size_t folder_last_page(const FolderView *f)
{
    return f->count ? (f->count-1)/folder_page_size(f) : 0;
}

static int folder_visible_files(const FolderView *f)
{
    size_t remaining = f->count-f->page*folder_page_size(f);
    return remaining < folder_page_size(f) ? remaining : folder_page_size(f);
}

static int folder_nested(const FolderView *f)
{
    return strlen(f->path) > f->root_length;
}

static struct TextFont *folder_font(const DockUI *ui)
{
    return ui->folder_font ? ui->folder_font : ui->draw->dri_Font;
}

/* The box's cells for the anchor's icon size and the folder font, and
   the most columns (f->cols) and rows (returned) that fit beside the
   anchor, before the drawer is read; *max_height and *extra_bottom bound
   the box's height. */
static int folder_grid(const DockUI *ui, FolderView *f, int *max_height, int *extra_bottom)
{
    struct RastPort measure;
    const FolderAnchor *anchor = &f->anchor;
    int font_height = folder_font(ui)->tf_YSize;
    int max_width, control = font_height+10;
    int icon = anchor->icon_size, text_width;
    int screen_top = ui->screen->BarHeight+2;
    max_width = ui->screen->Width;
    *max_height = ui->screen->Height-screen_top;
    if (anchor->edge == DOCK_EDGE_BOTTOM) *max_height = anchor->y-screen_top-FOLDER_GAP;
    else if (anchor->edge == DOCK_EDGE_LEFT) max_width -= anchor->x+FOLDER_GAP;
    else max_width = anchor->x-FOLDER_GAP;
    InitRastPort(&measure);
    SetFont(&measure,folder_font(ui));
    f->pad = dock_clamp((icon+2)/5,6,24);
    f->label_gap = dock_clamp((icon+11)/22,2,8);
    f->cell_width = (icon*246+87)/174;
    text_width = text_length32(&measure,"Workbench 0")+6;
    if (f->cell_width < text_width) f->cell_width = text_width;
    f->action_wrap = text_length32(&measure,"Open in Workbench")+6 > f->cell_width;
    f->cell_height = icon+f->label_gap+font_height+dock_clamp(icon/6,4,16);
    f->header = control+dock_clamp(icon/5,8,22);
    f->footer = control+8;
    *extra_bottom = (f->action_wrap ? font_height : 0)+f->pad;
    f->cols = dock_clamp((max_width-2*f->pad)/f->cell_width,0,FOLDER_COLS);
    return dock_clamp((*max_height-f->header-f->footer-*extra_bottom)/f->cell_height,0,FOLDER_ROWS);
}

/* After the drawer is read: fewer columns for few entries, and the rows
   (at most rows) its entries take. */
static void folder_fit_count(FolderView *f, int rows)
{
    while (f->cols > 3 && (size_t)(f->cols-1)*(f->cols-1) > f->count &&
        (size_t)(f->cols-1)*rows > f->count) --f->cols;
    f->rows = f->count/(size_t)f->cols >= (size_t)rows ? rows : (int)(f->count/f->cols)+1;
}

static void fit_folder_text(struct RastPort *rp, const char *text, int width, char *label)
{
    int length = strlen(text);
    const char *extension = strrchr(text,'.');
    int suffix = extension && extension != text ? (int)strlen(extension) : 1;
    int lo = 2, hi = length-1;
    strcpy(label,text);
    if (text_length32(rp,text) <= width) return;
    strcpy(label,"...");
    while (lo <= hi) {
        char candidate[DOCK_PREFS_PATH_CAP+3];
        int keep = (lo+hi)/2;
        int tail = (keep+1)/2;
        if (tail < suffix && suffix < 7) tail = suffix;
        if (tail >= keep) tail = keep-1;
        int head = keep-tail;
        memcpy(candidate,text,head);
        memcpy(candidate+head,"...",3);
        strcpy(candidate+head+3,text+length-tail);
        if (text_length32(rp,candidate) <= width) {
            strcpy(label,candidate);
            lo = keep+1;
        } else hi = keep-1;
    }
}


static void centered_folder_text(DockUI *ui, struct RastPort *rp, DockImage *target,
    const char *text, int center, int top, int width)
{
    char label[DOCK_PREFS_PATH_CAP+3];
    int measured;
    fit_folder_text(rp,text,width,label);
    measured = text_length32(rp,label);
    if (measured > width) return;
    if (target) {
        amiga_font_aa_draw(ui->folder_aa,target,label,center-measured/2,top,menu_colors[MENU_TEXT]);
    } else {
        popup_color(ui,rp,MENU_TEXT);
        Move(rp,center-measured/2,top+rp->TxBaseline);
        text32(rp,label);
    }
}

/* The two parts of the arrow each cover one span per row. Coordinates are
   relative to the ring center in units of 1/64 of its radius. */
static const BYTE arrow_tail[][2] = {
    {4,-15}, {-8,-14}, {-17,-11}, {-24,-6}, {-29,2}, {-32,12},
    {-33,22}, {-31,25}, {-28,24}, {-24,18}, {-19,13}, {-12,10}, {4,8}
};
static const BYTE arrow_head[][2] = {
    {1,-28}, {4,-31}, {7,-30}, {33,-6}, {35,-3}, {33,1},
    {7,24}, {4,25}, {1,23}
};
static void fill_arrow_part(struct RastPort *rp, int x, int y, int radius,
    const BYTE (*outline)[2], int count)
{
    int row;
    for (row = -radius/2; row <= radius/2; ++row) {
        int sample = row*64/radius;
        int left = 64, right = -64, i, previous;
        for (i = 0, previous = count-1; i < count; previous = i++) {
            int from_y = outline[previous][1], to_y = outline[i][1];
            if ((sample < from_y) != (sample < to_y)) {
                int from_x = outline[previous][0], to_x = outline[i][0];
                int hit = from_x+(to_x-from_x)*(sample-from_y)/(to_y-from_y);
                if (hit < left) left = hit;
                if (hit > right) right = hit;
            }
        }
        if (left <= right)
            RectFill(rp,x+left*radius/64,y+row,x+right*radius/64,y+row);
    }
}

static void folder_open_glyph(DockUI *ui, struct RastPort *rp, int x, int y, int radius)
{
    int thickness = dock_clamp((radius+6)/13,1,radius), inset;
    popup_color(ui,rp,FOLDER_MUTED);
    for (inset = 0; inset < thickness; ++inset)
        DrawEllipse(rp,x,y,radius-inset,radius-inset);
    fill_arrow_part(rp,x,y,radius,arrow_tail,sizeof(arrow_tail)/sizeof(arrow_tail[0]));
    fill_arrow_part(rp,x,y,radius,arrow_head,sizeof(arrow_head)/sizeof(arrow_head[0]));
}

/* The span [*left,*right] where a sample row y crosses an outline given in
   1/64 of the radius, as fill_arrow_part fills it; positions in 1/16 px. */
static int outline_span(const BYTE (*outline)[2], int count, int radius, int y, int *left, int *right)
{
    int i, previous, found = 0;
    for (i = 0, previous = count-1; i < count; previous = i++) {
        int y1 = outline[previous][1]*radius/4, y2 = outline[i][1]*radius/4;
        if ((y1 > y) != (y2 > y)) {
            int x1 = outline[previous][0]*radius/4, x2 = outline[i][0]*radius/4;
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

/* folder_open_glyph antialiased into an RGBA target: 4x4 samples per pixel.
   Each sample row is resolved to spans once (the ring, the arrow's tail and
   head), then the samples only compare positions. */
static void folder_open_glyph_aa(DockImage *target, int cx, int cy, int radius, uint32_t color)
{
    unsigned int red = (color>>16)&255, green = (color>>8)&255, blue = color&255;
    /* The ring: one native pixel plus half a pixel wide, so it keeps its
       weight once antialiased. Radii and positions in 1/16 px. */
    int thickness = dock_clamp((radius+6)/13,1,radius);
    int outer = radius*16+8, inner = outer-thickness*16-8;
    unsigned char covered[2*DOCK_MAX_ICON_SIZE+4];
    int x, y, size = 2*radius+3;
    if (radius < 4 || size > (int)sizeof(covered) || !target || !target->pixels) return;
    for (y = 0; y < size; ++y) {
        int py = cy-radius-1+y, sy;
        if (py < 0 || py >= target->height) continue;
        memset(covered,0,(size_t)size);
        for (sy = 0; sy < 4; ++sy) {
            int dy = (py-cy)*16+2+4*sy-8, ring_out = -1, ring_in = -1, sx;
            int tail_l = 0, tail_r = -1, head_l = 0, head_r = -1;
            if (dy*dy <= outer*outer) ring_out = isqrt(outer*outer-dy*dy);
            if (dy*dy < inner*inner) ring_in = isqrt(inner*inner-dy*dy);
            if (!outline_span(arrow_tail,sizeof(arrow_tail)/sizeof(arrow_tail[0]),radius,dy,&tail_l,&tail_r)) tail_r = tail_l-1;
            if (!outline_span(arrow_head,sizeof(arrow_head)/sizeof(arrow_head[0]),radius,dy,&head_l,&head_r)) head_r = head_l-1;
            for (x = 0; x < size; ++x) for (sx = 0; sx < 4; ++sx) {
                int dx = (x-radius-1)*16+2+4*sx-8, ax = dx < 0 ? -dx : dx;
                if ((ax <= ring_out && ax >= ring_in) || (dx >= tail_l && dx <= tail_r) ||
                    (dx >= head_l && dx <= head_r)) ++covered[x];
            }
        }
        for (x = 0; x < size; ++x) {
            int px = cx-radius-1+x;
            if (!covered[x] || px < 0 || px >= target->width) continue;
            amiga_blend_coverage(target->pixels+((size_t)py*target->width+px)*4,
                red,green,blue,(covered[x]*255+8)/16);
        }
    }
}

static void folder_glyph(DockUI *ui, struct RastPort *rp, int x, int y, int drawer, int open)
{
    const FolderView *f = &ui->folder;
    int size = f->anchor.icon_size;
    int half = size/2-2, radius = size/16;
    if (open) {
        folder_open_glyph(ui,rp,x,y,half);
    } else if (drawer) {
        popup_color(ui,rp,MENU_RIM);
        fill_rounded(rp,x-half,y-half*3/4,half,half/2,radius/2);
        popup_color(ui,rp,FOLDER_MUTED);
        fill_rounded(rp,x-half,y-half/2,half*2,half*5/4,radius);
    } else {
        int width = half*3/2, fold = half/2;
        popup_color(ui,rp,MENU_TEXT);
        fill_rounded(rp,x-width/2,y-half,width,half*2,radius);
        popup_color(ui,rp,MENU_RIM);
        Move(rp,x+width/2-fold,y-half);
        Draw(rp,x+width/2-fold,y-half+fold);
        Draw(rp,x+width/2-1,y-half+fold);
    }
}

enum { FOLDER_PAINT_LABELS = 1, FOLDER_PAINT_GLYPHS = 2 };

/* The back and page buttons: a round face with a chevron pointing left
   (direction -1) or right (1). Natively a disc and a two-pixel chevron. */
static void folder_button(DockUI *ui, struct RastPort *rp, int direction,
    int x, int y, int width, int height)
{
    int size = width < height ? width : height, arm = size/5, i;
    x += (width-size)/2; y += (height-size)/2;
    popup_color(ui,rp,MENU_HOVER);
    fill_rounded(rp,x,y,size,size,size/2);
    popup_color(ui,rp,MENU_TEXT);
    x += size/2-direction*arm/2; y += size/2;
    for (i = 0; i < 2; ++i) {
        Move(rp,x-direction*(arm-i),y-arm);
        Draw(rp,x+direction*i,y);
        Draw(rp,x-direction*(arm-i),y+arm);
    }
}

/* Squared distance from (px,py) to the segment (x0,y0)-(x1,y1), all in
   1/16 px. */
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

/* folder_button antialiased into an RGBA target, 4x4 samples per pixel:
   a faint disc of the text colour over the glass and a round-capped
   chevron stroke, the weight of the Open in Workbench ring. */
static void folder_button_aa(DockImage *target, int direction, int x, int y, int width, int height)
{
    ULONG color = menu_colors[MENU_TEXT];
    unsigned int red = (color>>16)&255, green = (color>>8)&255, blue = color&255;
    int size = width < height ? width : height;
    int radius = size*8, cx = x*16+width*8, cy = y*16+height*8;
    int arm = radius*3/8, reach = radius*3/16, half = 13;
    int tip = cx+direction*(reach-radius/16), back = cx-direction*(reach+radius/16);
    int px, py;
    if (!target || !target->pixels || size < 6) return;
    for (py = y; py < y+height; ++py) {
        if (py < 0 || py >= target->height) continue;
        for (px = x; px < x+width; ++px) {
            int disc = 0, stroke = 0, sx, sy;
            unsigned char *pixel;
            if (px < 0 || px >= target->width) continue;
            for (sy = 0; sy < 4; ++sy) for (sx = 0; sx < 4; ++sx) {
                int qx = px*16+2+4*sx, qy = py*16+2+4*sy;
                int dx = qx-cx, dy = qy-cy;
                if (dx*dx+dy*dy <= radius*radius) ++disc;
                if (segment_distance2(qx,qy,back,cy-arm,tip,cy) <= half*half ||
                    segment_distance2(qx,qy,back,cy+arm,tip,cy) <= half*half) ++stroke;
            }
            pixel = target->pixels+((size_t)py*target->width+px)*4;
            if (disc) amiga_blend_coverage(pixel,red,green,blue,(unsigned int)disc*3);
            if (stroke) amiga_blend_coverage(pixel,red,green,blue,((unsigned int)stroke*255+8)/16);
        }
    }
}

/* A button where the labels go: antialiased with them into the target, or
   natively in the pass that draws native labels. */
static void folder_control(DockUI *ui, struct RastPort *rp, DockImage *target, int parts,
    int direction, int x, int y, int width, int height)
{
    if ((parts&FOLDER_PAINT_LABELS) && target) folder_button_aa(target,direction,x,y,width,height);
    else if ((parts&FOLDER_PAINT_LABELS) && (parts&FOLDER_PAINT_GLYPHS))
        folder_button(ui,rp,direction,x,y,width,height);
}

static void paint_folder_content(DockUI *ui, struct RastPort *rp, int parts, DockImage *target)
{
    FolderView *f = &ui->folder;
    int font_height = folder_font(ui)->tf_YSize;
    int i, width = f->body_width, offset = f->body_left, control = font_height+10;
    int visible = folder_visible_files(f), icon = f->anchor.icon_size;
    size_t first = f->page*folder_page_size(f);
    const char *title = (const char *)FilePart((STRPTR)f->path);
    int labels = parts&FOLDER_PAINT_LABELS, glyphs = parts&FOLDER_PAINT_GLYPHS;
    if (!title[0]) title = f->path;

    if (!target) SetDrMd(rp,JAM1);
    SetFont(rp,folder_font(ui));
    if (labels)
        centered_folder_text(ui,rp,target,title,offset+width/2,(f->header-font_height)/2,width-4*control);
    if (folder_nested(f))
        folder_control(ui,rp,target,parts,-1,offset+f->pad,(f->header-control)/2,control,control);
    if (labels && !f->count) {
        centered_folder_text(ui,rp,target,f->read_ok ? "Empty drawer" : "Cannot read drawer",
            offset+width/2,f->header,width-2*f->pad);
    }
    for (i = 0; i <= visible; ++i) {
        int open = i == visible;
        int center = offset+(f->count ? f->pad+(i%f->cols)*f->cell_width+f->cell_width/2 : width/2);
        int top = f->header+(i/f->cols)*f->cell_height+(f->count ? 0 : font_height+6);
        int label_top = top+icon+f->label_gap;
        /* The open glyph is antialiased wherever the labels are (into an
           RGBA target); a native pass draws it only with native labels. */
        if (open && labels && target)
            folder_open_glyph_aa(target,center,top+icon/2,icon/2-2,menu_colors[FOLDER_MUTED]);
        else if (glyphs && (open ? labels : !f->loaded[i].pixels))
            folder_glyph(ui,rp,center,top+icon/2,!open && f->entries[first+i].drawer,open);
        if (!labels) continue;
        if (open && f->action_wrap) {
            centered_folder_text(ui,rp,target,"Open in",center,label_top,f->cell_width-6);
            centered_folder_text(ui,rp,target,"Workbench",center,label_top+font_height,f->cell_width-6);
        } else {
            centered_folder_text(ui,rp,target,open ? "Open in Workbench" : f->entries[first+i].name,
                center,label_top,f->cell_width-6);
        }
    }
    if (folder_last_page(f)) {
        int y = f->body_height-f->footer;
        if (labels) {
            char page[36];
            snprintf(page,sizeof(page),"%lu / %lu",(unsigned long)f->page+1,
                (unsigned long)folder_last_page(f)+1);
            centered_folder_text(ui,rp,target,page,offset+width/2,y+(control-font_height)/2,width);
        }
        if (f->page) folder_control(ui,rp,target,parts,-1,offset+f->pad,y,control,control);
        if (f->page < folder_last_page(f))
            folder_control(ui,rp,target,parts,1,offset+width-f->pad-control,y,control,control);
    }
}

static void paint_folder_labels(DockUI *ui, DockImage *target)
{
    struct RastPort measure;
    InitRastPort(&measure);
    paint_folder_content(ui,&measure,FOLDER_PAINT_LABELS,target);
}

static int prepare_folder_artwork(DockUI *ui);

static const DockImage *folder_background(DockUI *ui)
{
    tint_popup_glass(ui);
    return ui->folder.artwork_ready ? &ui->folder.artwork : &ui->popup_glass;
}

/* Final glass: alpha 0 adds an eighth of the backdrop, alpha 255 is
   artwork, partial alpha scales the glass. */
static uint32_t folder_glass(uint32_t pixel, uint32_t under)
{
    unsigned int transmission = 255-(pixel&255);
    if (transmission) {
        uint32_t glass = (under>>3)&UINT32_C(0x1f1f1f00);
        if (transmission < 255) {
            uint32_t rb = ((glass>>8)&UINT32_C(0x00ff00ff))*transmission+UINT32_C(0x00800080);
            uint32_t g = ((glass>>16)&255)*transmission+128;
            glass = (rb&UINT32_C(0xff00ff00))+((g&0xff00)<<8);
        }
        pixel += glass;
    }
    return pixel|255;
}

/* The box grows out of the dock: rows from the tip upward for a bottom
   dock, columns away from a side dock. */
static int folder_reveal_extent(const FolderView *f, int width, int height)
{
    return f->anchor.edge == DOCK_EDGE_BOTTOM ? height : width;
}

/* The window rectangle of reveal positions [from,to), counted from the tip. */
static void folder_reveal_rect(const FolderView *f, int width, int height, int from, int to,
    int *x, int *y, int *w, int *h)
{
    if (f->anchor.edge == DOCK_EDGE_BOTTOM) { *x = 0; *w = width; *y = height-to; *h = to-from; }
    else if (f->anchor.edge == DOCK_EDGE_LEFT) { *y = 0; *h = height; *x = from; *w = to-from; }
    else { *y = 0; *h = height; *x = width-to; *w = to-from; }
}

/* Put the reveal strip [from,to) of source on screen. A frame is skipped
   rather than waiting while Workbench holds the layers (a desktop click);
   the next one covers the whole difference. Returns 1 when it was shown. */
static int present_folder_strip(DockUI *ui, const DockImage *source, int from, int to)
{
    FolderView *f = &ui->folder;
    struct Layer *layer = ui->popup->WLayer;
    int x, y, w, h, shown;
    if (from > to) { int t = from; from = to; to = t; }
    if (from == to || !source->pixels) return 1;
    folder_reveal_rect(f,ui->popup->Width,ui->popup->Height,from,to,&x,&y,&w,&h);
    if (!AttemptLockLayerRom(layer)) return 0;
    shown = amiga_display_present_tile(ui->popup_display,ui->popup,source,x,y,w,h);
    UnlockLayerRom(layer);
    return shown;
}

enum { FOLDER_SCALE_START = 26 }; /* the first frame's scale, 1/256 (about 10%) */

/* The point the box grows from: its tip toward the dock icon. */
static void folder_anchor(const FolderView *f, int width, int height, int *x, int *y)
{
    if (f->anchor.edge == DOCK_EDGE_BOTTOM) { *x = f->tip; *y = height-1; }
    else if (f->anchor.edge == DOCK_EDGE_LEFT) { *x = 0; *y = f->tip; }
    else { *x = width-1; *y = f->tip; }
}

static DockRect folder_scaled_rect(const FolderView *f, int width, int height, int scale)
{
    int ax, ay;
    folder_anchor(f,width,height,&ax,&ay);
    return (DockRect){ax-ax*scale/256, ay-ay*scale/256,
        ax+1+(width-1-ax)*scale/256, ay+1+(height-1-ay)*scale/256};
}

/* The full-size box for the entrance: the layer's glass over the backdrop
   inside the outline, the backdrop outside it. Composed once (about 15 ms
   for 304x282 on the 68080), so each frame only samples it: glass per
   frame took 14 ms and left 4 frames. */
static void finish_folder_box(FolderView *f)
{
    int width = f->motion_scene.width, height = f->motion_scene.height, row;
    for (row = 0; row < height; ++row) {
        size_t offset = (size_t)row*width;
        const uint32_t *scene = (const uint32_t *)f->motion_scene.pixels+offset;
        const uint32_t *under = (const uint32_t *)f->motion_backdrop.pixels+offset;
        uint32_t *out = (uint32_t *)f->motion_frame.pixels+offset;
        int lo, hi, i;
        popup_span(f,width,height,POPUP_RADIUS,row,&lo,&hi);
        memcpy(out,under,(size_t)width*4);
        for (i = lo; i < hi; ++i) out[i] = folder_glass(scene[i],under[i]);
    }
}

/* Compose the box at scale/256 around its tip, faded in by alpha/256 over
   the backdrop, into motion_out, and set the window rectangle to present:
   the scaled box and what it covered last frame. Each pixel samples the
   full-size box in motion_frame (nearest). The fade has quarter steps of
   the renderer's 50% mix (a general blend in C took 12 ms a frame on the
   68080). Returns 0 without memory. */
static int compose_folder_scale(FolderView *f, DockMixHalf mix, int width, int height,
    int scale, unsigned int alpha, DockRect *present)
{
    DockRect box = folder_scaled_rect(f,width,height,scale), all = f->motion_rect;
    int ax, ay, y, x, quarters = (int)(alpha+32)/64, runs = 0, use_runs;
    uint32_t *row_box;
    int *column, *run_x, *run_source;
    if (!f->motion_out.pixels) {
        f->motion_out = (DockImage){width,height,malloc((size_t)width*height*4)};
        if (!f->motion_out.pixels) { f->motion_out = (DockImage){0}; return 0; }
    }
    row_box = malloc((size_t)width*(sizeof(*row_box)+3*sizeof(*column))+sizeof(*column));
    if (!row_box) return 0;
    column = (int *)(row_box+width);
    run_x = column+width;
    run_source = run_x+width+1;
    folder_anchor(f,width,height,&ax,&ay);
    for (x = box.left; x < box.right; ++x) {
        column[x] = dock_clamp(ax+(x-ax)*256/scale,0,width-1);
        /* Runs of consecutive source columns, copied whole: near full size
           they are about 16 px long, and a per-pixel gather took a large
           frame past one 20 ms field on the 68080. */
        if (x == box.left || column[x] != column[x-1]+1) {
            run_x[runs] = x; run_source[runs] = column[x]; ++runs;
        }
    }
    run_x[runs] = box.right;
    use_runs = runs*4 <= box.right-box.left;
    if (all.right <= all.left || all.bottom <= all.top) all = box;
    else {
        if (box.left < all.left) all.left = box.left;
        if (box.top < all.top) all.top = box.top;
        if (box.right > all.right) all.right = box.right;
        if (box.bottom > all.bottom) all.bottom = box.bottom;
    }
    for (y = all.top; y < all.bottom; ++y) {
        size_t offset = (size_t)y*width;
        const uint32_t *under = (const uint32_t *)f->motion_backdrop.pixels+offset;
        uint32_t *out = (uint32_t *)f->motion_out.pixels+offset;
        const uint32_t *source;
        uint32_t *dst;
        int sy, lo, hi, a, b, w = box.right-box.left;
        if (y < box.top || y >= box.bottom || !quarters) {
            memcpy(out+all.left,under+all.left,(size_t)(all.right-all.left)*4);
            continue;
        }
        if (all.left < box.left) memcpy(out+all.left,under+all.left,(size_t)(box.left-all.left)*4);
        if (box.right < all.right) memcpy(out+box.right,under+box.right,(size_t)(all.right-box.right)*4);
        sy = dock_clamp(ay+(y-ay)*256/scale,0,height-1);
        popup_span(f,width,height,POPUP_RADIUS,sy,&lo,&hi);
        /* The columns sample left to right: [a,b) lies inside the outline. */
        for (a = box.left; a < box.right && column[a] < lo; ++a) {}
        for (b = box.right; b > a && column[b-1] >= hi; --b) {}
        source = (const uint32_t *)f->motion_frame.pixels+(size_t)sy*width;
        dst = quarters == 4 ? out : row_box;
        if (a > box.left) memcpy(dst+box.left,under+box.left,(size_t)(a-box.left)*4);
        if (use_runs) {
            int r;
            for (r = 0; r < runs; ++r) {
                int x0 = run_x[r], x1 = run_x[r+1], from = run_source[r];
                if (x1 <= a) continue;
                if (x0 >= b) break;
                if (x0 < a) { from += a-x0; x0 = a; }
                if (x1 > b) x1 = b;
                if (x1-x0 >= 8) memcpy(dst+x0,source+from,(size_t)(x1-x0)*4);
                else for (x = x0; x < x1; ++x) dst[x] = source[from++];
            }
        } else for (x = a; x < b; ++x) dst[x] = source[column[x]];
        if (b < box.right) memcpy(dst+b,under+b,(size_t)(box.right-b)*4);
        if (quarters == 4) continue;
        out += box.left; under += box.left; dst += box.left;
        memcpy(out,quarters == 3 ? dst : under,(size_t)w*4);
        mix((unsigned char *)out,(const unsigned char *)(quarters == 3 ? under : dst),(unsigned long)w);
        if (quarters == 1) mix((unsigned char *)out,(const unsigned char *)under,(unsigned long)w);
        if (quarters == 3) mix((unsigned char *)out,(const unsigned char *)dst,(unsigned long)w);
    }
    free(row_box);
    *present = all;
    f->motion_rect = box;
    return 1;
}

static int present_folder_rect(DockUI *ui, const DockImage *image, int x, int y, int w, int h)
{
    struct Layer *layer = ui->popup->WLayer;
    int ok;
    if (w <= 0 || h <= 0) return 1;
    if (!AttemptLockLayerRom(layer)) return 0;
    ok = amiga_display_present_tile(ui->popup_display,ui->popup,image,x,y,w,h);
    UnlockLayerRom(layer);
    return ok;
}

static void paint_folder(DockUI *ui)
{
    FolderView *f = &ui->folder;
    const DockImage *background;
    struct RastPort *buffer;
    int parts = FOLDER_PAINT_GLYPHS|FOLDER_PAINT_LABELS;
    if (f->motion && f->handoff) {
        if (f->handoff_bitmap) amiga_display_end_frame(ui->popup_display,ui->popup);
        else amiga_display_present(ui->popup_display,ui->popup,&f->motion_backdrop);
        return;
    }
    if (f->motion == FOLDER_OPENING && f->motion_frame.pixels) {
        /* Show the backdrop; the next motion frame draws the box from
           scratch. */
        amiga_display_present(ui->popup_display,ui->popup,&f->motion_backdrop);
        f->motion_rect = (DockRect){0,0,0,0};
        return;
    }
    /* A repaint while closing shows the whole box; the next motion frame
       hides what should be hidden. */
    if (f->motion == FOLDER_CLOSING) {
        f->reveal_presented = folder_reveal_extent(f,ui->popup->Width,ui->popup->Height);
        f->motion_rect = (DockRect){0,0,ui->popup->Width,ui->popup->Height};
    }
    if (ui->folder_aa && ui->popup_glass.pixels && (f->artwork_ready || prepare_folder_artwork(ui)))
        parts = FOLDER_PAINT_GLYPHS;
    background = folder_background(ui);
    buffer = amiga_display_begin_frame(ui->popup_display,background);
    if (buffer) {
        paint_folder_content(ui,buffer,parts,NULL);
        amiga_display_end_frame(ui->popup_display,ui->popup);
    } else {
        paint_popup_surface(ui,background);
        paint_folder_content(ui,ui->popup->RPort,parts,NULL);
    }
}

static void clear_folder_motion(FolderView *f)
{
    free(f->motion_scene.pixels);
    free(f->motion_frame.pixels);
    free(f->motion_out.pixels);
    f->motion_scene = f->motion_frame = f->motion_out = (DockImage){0};
    f->motion = FOLDER_STILL;
    f->handoff = 0;
    f->handoff_bitmap = NULL;
}

static void load_folder_icon(DockUI *ui, int present);

void dock_ui_finish_folder_motion(DockUI *ui)
{
    FolderView *f = &ui->folder;
    finish_folder_reveal(ui,1);
    if (f->motion == FOLDER_CLOSING) {
        /* End on the desktop pixels while the layer still owns them. */
        if (!f->handoff) amiga_display_present(ui->popup_display,ui->popup,&f->motion_backdrop);
        close_popup(ui,0);
    } else if (f->motion == FOLDER_OPENING) {
        f->motion = FOLDER_STILL;
        /* The icons that grew with the box go into the still picture
           before it is shown: drawn one by one after it, they blinked out
           for a few ms at the end of the entrance. */
        while (f->count && f->next_icon < folder_visible_files(f) && f->loaded[f->next_icon].pixels)
            load_folder_icon(ui,0);
        paint_folder(ui);
        clear_folder_motion(f);
    }
}

static int folder_icon_left(const FolderView *f, int slot)
{
    return f->body_left+f->pad+(slot%f->cols)*f->cell_width+f->cell_width/2-f->anchor.icon_size/2;
}

static int folder_icon_top(const FolderView *f, int slot)
{
    return f->header+(slot/f->cols)*f->cell_height;
}

/* RGB is the panel's contribution over black. Alpha is the opacity of
   artwork over the glass, with zero for unobstructed glass. */
static void finish_folder_layer(FolderView *f)
{
    DockImage *layer = &f->motion_scene;
    int width = layer->width, height = layer->height, y, i;
    const uint32_t tint = glass_tint(3)&UINT32_C(0xffffff00);
    const uint32_t rim = (menu_colors[MENU_RIM]<<8)|255;
    for (y = 0; y < height; ++y) {
        int lo, hi, inner_lo, inner_hi, x;
        uint32_t *row = (uint32_t *)layer->pixels+(size_t)y*width;
        popup_span(f,width,height,POPUP_RADIUS,y,&lo,&hi);
        popup_inner_span(f,width,height,POPUP_RADIUS,y,lo,hi,&inner_lo,&inner_hi);
        for (x = lo; x < inner_lo; ++x) row[x] = rim;
        for (; x < inner_hi; ++x)
            if (row[x] == UINT32_C(0xff00ffff)) row[x] = tint;
        for (; x < hi; ++x) row[x] = rim;
    }
    for (i = 0; i < folder_visible_files(f); ++i) {
        const DockImage *icon = &f->loaded[i];
        int x, top, left;
        if (!icon->pixels) continue;
        left = folder_icon_left(f,i);
        top = folder_icon_top(f,i);
        dock_blend(layer,icon,left,top);
        for (y = 0; y < icon->height; ++y) {
            unsigned char *out = layer->pixels+((size_t)(top+y)*width+left)*4;
            const unsigned char *in = icon->pixels+(size_t)y*icon->width*4;
            for (x = 0; x < icon->width; ++x, out += 4, in += 4)
                out[3] = in[3];
        }
    }
}

static int prepare_folder_scene(DockUI *ui)
{
    FolderView *f = &ui->folder;
    const DockImage *background = &f->motion_backdrop;
    static const struct TagItem key_color[] = {
        {RPTAG_PenMode,FALSE}, {RPTAG_FgColor,0xffff00ffUL}, {TAG_DONE,0}
    };
    struct RastPort *buffer;
    size_t bytes = (size_t)background->width*background->height*4;
    if (!background->pixels) return 0;
    if (f->motion_scene.pixels) return 1;
    f->motion_scene = (DockImage){background->width,background->height,malloc(bytes)};
    f->motion_frame = (DockImage){background->width,background->height,malloc(bytes)};
    if (!f->motion_scene.pixels || !f->motion_frame.pixels) {
        clear_folder_motion(f);
        return 0;
    }
    /* Native text and glyphs are opaque. A reserved color marks untouched
       pixels, even when the actual backdrop matches a glyph's color. With
       pens only, the color is written as an image (into motion_frame,
       filled before its first use below). */
    folder_stamp(f->anchor.profile,0);
    if (ui->rgb_pens) {
        buffer = amiga_display_begin_frame(ui->popup_display,NULL);
        if (buffer) {
            SetDrMd(buffer,JAM1);
            SetRPAttrsA(buffer,key_color);
            RectFill(buffer,0,0,background->width-1,background->height-1);
        }
    } else {
        uint32_t *key = (uint32_t *)f->motion_frame.pixels;
        size_t i, count = (size_t)background->width*background->height;
        for (i = 0; i < count; ++i) key[i] = UINT32_C(0xff00ffff);
        buffer = amiga_display_begin_frame(ui->popup_display,&f->motion_frame);
    }
    if (!buffer) {
        clear_folder_motion(f);
        return 0;
    }
    SetDrMd(buffer,JAM1);
    paint_folder_content(ui,buffer,FOLDER_PAINT_GLYPHS|
        (ui->folder_aa ? 0 : FOLDER_PAINT_LABELS),NULL);
    folder_stamp(f->anchor.profile,0);
    if (amiga_display_capture_frame(ui->popup_display,&f->motion_scene)) {
        folder_stamp(f->anchor.profile,0);
        finish_folder_layer(f);
        folder_stamp(f->anchor.profile,0);
        if (ui->folder_aa) paint_folder_labels(ui,&f->motion_scene);
        return 1;
    }
    clear_folder_motion(f);
    return 0;
}

static void start_folder_clock(FolderView *f, int duration)
{
    struct EClockVal now;
    f->motion_ticks = ReadEClock(&now)*duration/1000;
    f->motion_start = now.ev_lo;
    f->motion_frames = 0;
    f->motion_prepare_sum = f->motion_present_sum = 0;
    f->motion_fields[0] = 0;
}

static unsigned long folder_path_key(const char *path);
static FolderCachedIcon *cached_folder_icon(const DockUI *ui, unsigned long key, const char *path,
    const unsigned long *stamp, int size);
static int copy_image(DockImage *out, const DockImage *in);

static void reset_folder_page(DockUI *ui)
{
    FolderView *f = &ui->folder;
    f->next_icon = f->artwork_ready = f->needs_repaint = 0;
    f->fill_start = f->read_sum = f->read_max = 0;
    f->read_count = f->cache_hits = f->fill_reported = 0;
    free_folder_icons(f);
    if (f->motion == FOLDER_OPENING) {
        /* Icons already in the cache (the prefetch read them) grow with
           the box; the rest show their placeholder until they are read. */
        int i, size = f->anchor.icon_size;
        for (i = 0; i < folder_visible_files(f); ++i) {
            char path[DOCK_PREFS_PATH_CAP];
            const DockFolderEntry *entry = &f->entries[f->page*folder_page_size(f)+i];
            const FolderCachedIcon *cached;
            strcpy(path,f->path);
            if (AddPart((STRPTR)path,(STRPTR)entry->name,sizeof(path)) &&
                (cached = cached_folder_icon(ui,folder_path_key(path),path,entry->stamp,size)))
                copy_image(&f->loaded[i],&cached->icon);
        }
        /* Start from the bare backdrop; the box grows out of the tip. */
        if (prepare_folder_scene(ui)) {
            folder_stamp(f->anchor.profile,0);
            finish_folder_box(f);
            f->reveal_shown = f->reveal_presented = f->reveal_from = 0;
            f->motion_scale = FOLDER_SCALE_START;
            f->motion_rect = (DockRect){0,0,0,0};
        } else clear_folder_motion(f);
    }
    paint_folder(ui);
    if (f->motion == FOLDER_OPENING) {
        folder_stamp(f->anchor.profile,0);
        start_folder_clock(f,FOLDER_OPEN_MS);
    }
}

/* Another window lies over the popup: its pixels on the screen are not
   the popup's own. Unknown (the layers are locked elsewhere) reads as no. */
static int popup_covered(const DockUI *ui)
{
    struct Layer_Info *info = &ui->screen->LayerInfo;
    const struct Layer *own = ui->popup->WLayer, *layer;
    int covered = 0;
    if (!own || !AttemptSemaphore(&info->Lock)) return 0;
    for (layer = own->front; layer && !covered; layer = layer->front)
        covered = layer->bounds.MinX <= own->bounds.MaxX && layer->bounds.MaxX >= own->bounds.MinX &&
            layer->bounds.MinY <= own->bounds.MaxY && layer->bounds.MaxY >= own->bounds.MinY;
    ReleaseSemaphore(&info->Lock);
    return covered;
}

void dock_ui_dismiss_popup(DockUI *ui)
{
    FolderView *f = &ui->folder;
    if (f->motion == FOLDER_CLOSING) return;
    if (!ui->popup || !f->path[0] || !f->motion_backdrop.pixels || !ui->popup_display) {
        close_popup(ui,0);
        return;
    }
    /* Hide strips back toward the tip from wherever the box is: only the
       backdrop is needed, so closing starts at once. An opening in
       progress closes from the part already shown. */
    if (f->motion != FOLDER_OPENING) {
        f->reveal_shown = folder_reveal_extent(f,ui->popup->Width,ui->popup->Height);
        f->motion_scale = 256;
        f->motion_rect = (DockRect){0,0,ui->popup->Width,ui->popup->Height};
        /* The box as shown, to shrink and fade: read back from the screen.
           Under another window that would shrink a picture of that window;
           the strips hide the box instead. */
        if (!f->motion_frame.pixels && !popup_covered(ui)) {
            DockImage *view = &f->motion_frame;
            *view = (DockImage){ui->popup->Width,ui->popup->Height,
                malloc((size_t)ui->popup->Width*ui->popup->Height*4)};
            if (!view->pixels || !amiga_display_capture(ui->popup_display,view,
                    ui->popup->LeftEdge,ui->popup->TopEdge)) {
                free(view->pixels); *view = (DockImage){0};
            }
        }
    } else f->reveal_shown = f->reveal_presented;
    f->scale_from = f->motion_scale > FOLDER_SCALE_START ? f->motion_scale : FOLDER_SCALE_START;
    f->reveal_from = f->reveal_presented = f->reveal_shown;
    f->motion = FOLDER_CLOSING;
    ui->popup_pressed = FOLDER_OUTSIDE;
    start_folder_clock(f,FOLDER_CLOSE_MS);
}

static int advance_folder_motion(DockUI *ui)
{
    FolderView *f = &ui->folder;
    struct EClockVal now;
    ULONG hz = ReadEClock(&now), elapsed = now.ev_lo-f->motion_start;
    int closing = f->motion == FOLDER_CLOSING;
    int extent = folder_reveal_extent(f,ui->popup->Width,ui->popup->Height);
    if (f->handoff) {
        close_popup(ui,1);
        return 1;
    }
    if (elapsed < f->motion_ticks) {
        /* Quadratic ease-out for the size, linear for the fade: at 50 Hz
           the motion has 6-7 frames, and a cubic curve spent most of the
           way in the first one. */
        int t = (int)(elapsed*256/f->motion_ticks), rest = 256-t;
        int weight = 256-rest*rest/256;
        int scale = closing ? f->scale_from+(FOLDER_SCALE_START-f->scale_from)*weight/256 :
            FOLDER_SCALE_START+(256-FOLDER_SCALE_START)*weight/256;
        int source = f->motion_frame.pixels != NULL;
        ULONG composed;
        DockRect rect = {0,0,0,0};
        /* Grow from the tip and fade in while opening; the reverse while
           closing. Without a box to scale, closing hides strips. */
        int composed_ok = source && compose_folder_scale(f,ui->mix_half ? ui->mix_half : dock_mix_half_c,
            ui->popup->Width,ui->popup->Height,scale,(unsigned int)(closing ? rest : t),&rect);
        if (!composed_ok && closing) {
            int target = f->reveal_from*(256-weight)/256;
            if (target < f->reveal_shown) f->reveal_shown = target;
        }
        ReadEClock(&now); composed = now.ev_lo;
        if (composed_ok) {
            if (present_folder_rect(ui,&f->motion_out,rect.left,rect.top,
                    rect.right-rect.left,rect.bottom-rect.top)) {
                f->motion_scale = scale;
                f->reveal_shown = f->reveal_presented = extent*scale/256;
            }
        } else if (closing && present_folder_strip(ui,&f->motion_backdrop,f->reveal_presented,f->reveal_shown))
            f->reveal_presented = f->reveal_shown;
        ++f->motion_frames;
        if (f->anchor.profile && f->motion_frames <= 12) {
            ULONG done;
            ReadEClock(&now); done = now.ev_lo;
            f->motion_frame_us[f->motion_frames-1] =
                (UWORD)((uint64_t)(done-f->motion_start-elapsed)*1000000/hz);
        }
        if (f->anchor.profile && ui->fields) {
            /* Fields since the previous frame (since the start for the first). */
            ULONG field = *ui->fields;
            size_t used = strlen(f->motion_fields);
            if (f->motion_frames == 1) f->motion_field = field;
            if (used+2 < sizeof(f->motion_fields)) {
                ULONG gap = field-f->motion_field;
                f->motion_fields[used] = (char)('0'+(gap > 9 ? 9 : gap));
                f->motion_fields[used+1] = 0;
            }
            f->motion_field = field;
        }
        ReadEClock(&now);
        f->motion_prepare_sum += composed-f->motion_start-elapsed;
        f->motion_present_sum += now.ev_lo-composed;
        return 0;
    }
    if (f->anchor.profile && hz) {
        int frames = f->motion_frames ? f->motion_frames : 1;
        printf("Folder Stack %s: %lu us, %d frames, icon %d px, font %d px, "
            "compose_us_avg=%lu present_us_avg=%lu box=%dx%d fields=%s\n",
            closing ? "dismissal" : "entrance",(unsigned long)((uint64_t)elapsed*1000000/hz),
            f->motion_frames,f->anchor.icon_size,folder_font(ui)->tf_YSize,
            (unsigned long)((uint64_t)f->motion_prepare_sum*1000000/hz/frames),
            (unsigned long)((uint64_t)f->motion_present_sum*1000000/hz/frames),
            ui->popup->Width,ui->popup->Height,f->motion_fields);
        {
            int i;
            printf("  frame_us:");
            for (i = 0; i < f->motion_frames && i < 12; ++i) printf(" %u",f->motion_frame_us[i]);
            printf("\n");
        }
        if (!closing && folder_stages == FOLDER_STAGES) {
            /* read, shape, capture, backdrop copy, tint, window, buffers,
               text, read back, layer, labels, first present */
            int i;
            printf("Folder Stack prepare_us:");
            for (i = 1; i < FOLDER_STAGES; ++i)
                printf(" %lu",(unsigned long)((uint64_t)(folder_stage[i]-folder_stage[i-1])*1000000/hz));
            printf("\n");
        }
        fflush(stdout);
    }
    if (closing) {
        struct RastPort *buffer = amiga_display_begin_frame(ui->popup_display,&f->motion_backdrop);
        f->handoff_bitmap = buffer ? buffer->BitMap : NULL;
        f->handoff = 1;
        paint_folder(ui);
    } else dock_ui_finish_folder_motion(ui);
    return 0;
}

static int prepare_folder_artwork(DockUI *ui)
{
    FolderView *f = &ui->folder;
    DockImage *art = &f->artwork;
    int width = ui->popup->Width, height = ui->popup->Height;
    size_t pixels = (size_t)width*height;
    tint_popup_glass(ui);
    if (!ui->popup_display) ui->popup_display = amiga_display_open(ui->screen,width,height);
    if (!ui->popup_display) return 0;
    amiga_display_set_converters(ui->popup_display,ui->encode,ui->decode,ui->encode565);
    if (!art->pixels) *art = (DockImage){width,height,malloc(pixels*4)};
    if (!art->pixels) return 0;
    if (ui->popup_glass.pixels) {
        memcpy(art->pixels,ui->popup_glass.pixels,pixels*4);
        if (ui->folder_aa) paint_folder_labels(ui,art);
    } else {
        int x, y;
        for (y = 0; y < height; ++y) {
            int lo, hi;
            dock_ui_popup_span(ui,y,&lo,&hi);
            for (x = 0; x < width; ++x) {
                unsigned char *p = art->pixels+((size_t)y*width+x)*4;
                ULONG color = menu_colors[y && y+1 < height && x > lo && x+1 < hi ?
                    MENU_SURFACE : MENU_RIM];
                p[0] = color>>16; p[1] = color>>8; p[2] = color; p[3] = 255;
            }
        }
    }
    f->artwork_ready = 1;
    return 1;
}

int dock_ui_folder_open(const DockUI *ui)
{
    return ui && ui->popup && ui->folder.path[0] && ui->folder.motion != FOLDER_CLOSING;
}

int dock_ui_folder_progress(const DockUI *ui)
{
    const FolderView *f;
    int extent;
    if (!ui || !ui->popup || !ui->folder.path[0]) return 0;
    f = &ui->folder;
    if (f->motion == FOLDER_STILL) return 256;
    if (f->handoff) return 0;
    extent = folder_reveal_extent(f,ui->popup->Width,ui->popup->Height);
    return extent > 0 ? f->reveal_presented*256/extent : 256;
}

int dock_ui_folder_moving(const DockUI *ui)
{
    return ui && ui->popup && ui->folder.path[0] && ui->folder.motion != FOLDER_STILL;
}

int dock_ui_folder_pending(const DockUI *ui)
{
    if (!ui) return 0;
    const FolderView *f = &ui->folder;
    return ui->folder_reveal ||
        (ui->popup && (f->motion || (f->count && f->next_icon < folder_visible_files(f))));
}

int dock_ui_folder_loading(const DockUI *ui)
{
    return ui && !ui->folder_reveal && !ui->folder.motion && dock_ui_folder_pending(ui);
}

ULONG dock_ui_folder_frame_delay(const DockUI *ui)
{
    return ui->folder.motion && !ui->folder.handoff ? 4000 : 20000;
}

/* FNV-1a of the path's bytes. */
static unsigned long folder_path_key(const char *path)
{
    unsigned long hash = 2166136261UL;
    for (; *path; ++path) hash = ((hash^(unsigned char)*path)*16777619UL)&0xffffffffUL;
    return hash;
}

static FolderCachedIcon *cached_folder_icon(const DockUI *ui, unsigned long key, const char *path,
    const unsigned long *stamp, int size)
{
    int i;
    for (i = 0; ui->icon_cache && i < FOLDER_ICON_CACHE; ++i) {
        FolderCachedIcon *c = &ui->icon_cache[i];
        if (c->key == key && c->icon.pixels && c->icon.width == size &&
            !memcmp(c->stamp,stamp,sizeof(c->stamp)) && !strcmp(c->path,path)) return c;
    }
    return NULL;
}

static int copy_image(DockImage *out, const DockImage *in)
{
    size_t bytes = (size_t)in->width*in->height*4;
    *out = (DockImage){in->width,in->height,malloc(bytes)};
    if (!out->pixels) return 0;
    memcpy(out->pixels,in->pixels,bytes);
    return 1;
}

/* Keep a copy of a built icon, in place of an older one of the same path,
   else in a free entry. Without one, evict takes the oldest (round robin);
   otherwise nothing is kept. */
static void cache_folder_icon(DockUI *ui, unsigned long key, const char *path,
    const unsigned long *stamp, const DockImage *icon, int evict)
{
    FolderCachedIcon *c = NULL, *empty = NULL;
    int i;
    if (strlen(path) >= sizeof(c->path)) return;
    if (!ui->icon_cache && !(ui->icon_cache = calloc(FOLDER_ICON_CACHE,sizeof(*ui->icon_cache))))
        return;
    for (i = 0; i < FOLDER_ICON_CACHE && !c; ++i) {
        FolderCachedIcon *e = &ui->icon_cache[i];
        if (!e->icon.pixels) { if (!empty) empty = e; }
        else if (e->key == key && !strcmp(e->path,path)) c = e;
    }
    if (!c) c = empty;
    if (!c) {
        if (!evict) return;
        c = &ui->icon_cache[ui->icon_cache_next];
        ui->icon_cache_next = (ui->icon_cache_next+1)%FOLDER_ICON_CACHE;
        ++ui->icon_cache_evicted;
    }
    if (c->icon.pixels) --ui->icon_cache_used;
    free(c->icon.pixels);
    c->icon = (DockImage){0};
    if (!copy_image(&c->icon,icon)) return;
    ++ui->icon_cache_used;
    c->key = key;
    strcpy(c->path,path);
    memcpy(c->stamp,stamp,sizeof(c->stamp));
}

/* Read and build an entry's icon at size and keep a copy (evict: as
   cache_folder_icon). */
static int read_folder_icon(DockUI *ui, unsigned long key, const char *path, const unsigned long *stamp,
    int size, DockUIReadIcon read_icon, const void *context, DockImage *icon, int evict)
{
    DockImage source = {0};
    void *requesters = amiga_requesters_off();
    int ok = read_icon(context,path,&source) && dock_icon_build(icon,&source,size);
    amiga_requesters_restore(requesters);
    free(source.pixels);
    if (ok) cache_folder_icon(ui,key,path,stamp,icon,evict);
    return ok;
}

void dock_ui_folder_cache_stats(const DockUI *ui, int *used, int *capacity, unsigned long *evicted)
{
    *used = ui ? ui->icon_cache_used : 0;
    *capacity = FOLDER_ICON_CACHE;
    *evicted = ui ? ui->icon_cache_evicted : 0;
}

/* Load the page's next icon into the artwork, and show it if present:
   from the cache while the entry's dates match, else read and built (and
   kept). */
static void load_folder_icon(DockUI *ui, int present)
{
    FolderView *f = &ui->folder;
    char path[DOCK_PREFS_PATH_CAP];
    DockImage icon = {0};
    const FolderCachedIcon *cached = NULL;
    int i = f->next_icon++, size = f->anchor.icon_size, ok = 0;
    const DockFolderEntry *entry = &f->entries[f->page*folder_page_size(f)+i];
    struct EClockVal before, after;
    unsigned long key;
    strcpy(path,f->path);
    if (!AddPart((STRPTR)path,(STRPTR)entry->name,sizeof(path))) return;
    key = folder_path_key(path);
    if (f->loaded[i].pixels) {
        /* Already taken from the cache for the entrance. */
        icon = f->loaded[i]; ok = 1;
        ++f->cache_hits;
    } else if ((cached = cached_folder_icon(ui,key,path,entry->stamp,size))) {
        ok = copy_image(&icon,&cached->icon);
        ++f->cache_hits;
    } else {
        if (f->anchor.profile) ReadEClock(&before);
        ok = read_folder_icon(ui,key,path,entry->stamp,size,f->anchor.read_icon,f->anchor.context,&icon,1);
        if (f->anchor.profile) {
            ULONG ticks;
            ReadEClock(&after);
            ticks = after.ev_lo-before.ev_lo;
            f->read_sum += ticks; ++f->read_count;
            if (ticks > f->read_max) f->read_max = ticks;
        }
    }
    if (!ok) return;
    if (!f->artwork_ready && !prepare_folder_artwork(ui)) {
        free(icon.pixels); f->loaded[i] = (DockImage){0};
        return;
    }
    {
        int x = folder_icon_left(f,i), y = folder_icon_top(f,i);
        dock_blend(&f->artwork,&icon,x,y);
        f->loaded[i] = icon;
        if (present && (!ui->popup_glass.pixels ||
            !amiga_display_present_tile(ui->popup_display,ui->popup,&f->artwork,x,y,size,size)))
            f->needs_repaint = 1;
    }
}

/* Icons load for up to this long per step (one at least), then the event
   loop runs again; one icon per step took ~1 s for a page of 23. */
enum { FOLDER_LOAD_BUDGET_US = 12000 };

int dock_ui_folder_step(DockUI *ui)
{
    FolderView *f = &ui->folder;
    struct EClockVal start, now;
    ULONG hz, budget;
    if (ui->folder_reveal) {
        ReadEClock(&now);
        if ((LONG)(now.ev_lo-ui->folder_reveal->deadline) >= 0) finish_folder_reveal(ui,0);
        return 0;
    }
    if (!dock_ui_folder_pending(ui)) return 0;
    if (f->motion) return advance_folder_motion(ui);
    hz = ReadEClock(&start);
    budget = (ULONG)((uint64_t)hz*FOLDER_LOAD_BUDGET_US/1000000);
    if (!f->next_icon) f->fill_start = start.ev_lo;
    do {
        load_folder_icon(ui,1);
        ReadEClock(&now);
    } while (f->next_icon < folder_visible_files(f) && now.ev_lo-start.ev_lo < budget);
    if (dock_ui_folder_pending(ui)) return 0;
    if (f->needs_repaint) {
        f->needs_repaint = 0;
        paint_folder(ui);
    }
    if (f->anchor.profile && !f->fill_reported && hz) {
        f->fill_reported = 1;
        printf("Folder Stack icons: %d read, %d cached, in %lu us, read_us_avg=%lu read_us_max=%lu\n",
            f->read_count,f->cache_hits,(unsigned long)((uint64_t)(now.ev_lo-f->fill_start)*1000000/hz),
            (unsigned long)(f->read_count ? (uint64_t)f->read_sum*1000000/hz/f->read_count : 0),
            (unsigned long)((uint64_t)f->read_max*1000000/hz));
        fflush(stdout);
    }
    return 0;
}

int dock_ui_prefetch_folder(DockUI *ui, int slot, DockEdge edge, int x, int y,
    const char *path, int sort, int icon_size, DockUIReadIcon read_icon, const void *context)
{
    FolderPrefetch *p;
    if (!ui || slot < 0 || slot >= DOCK_MAX_ITEMS || !path[0] ||
        strlen(path) >= sizeof(ui->prefetch[slot].path)) return 0;
    p = &ui->prefetch[slot];
    /* A moved anchor (another Scale, a reordered pin) can change the page,
       another sort its entries. */
    if (strcmp(p->path,path) || p->sort != (DockSort)sort || p->icon_size != icon_size ||
        p->edge != edge || p->x != x || p->y != y) {
        /* The first page the box would show at (edge,x,y): show_folder's
           grid, in its order. */
        FolderView grid = {0};
        int rows, max_height, extra_bottom;
        free(p->entries);
        memset(p,0,sizeof(*p));
        strcpy(p->path,path);
        p->icon_size = icon_size;
        p->edge = edge;
        p->sort = (DockSort)sort;
        p->x = x; p->y = y;
        grid.anchor = (FolderAnchor){edge,x,y,icon_size,0,read_icon,context,(DockSort)sort};
        rows = folder_grid(ui,&grid,&max_height,&extra_bottom);
        if (!grid.cols || !rows || !amiga_folder_read(path,(DockSort)sort,&p->entries,&p->count)) {
            p->entries = NULL; p->count = 0;
            return 0;
        }
        grid.count = p->count;
        folder_fit_count(&grid,rows);
        if (p->count > folder_page_size(&grid)) p->count = folder_page_size(&grid);
    }
    if (ui->popup) return p->next < p->count;
    /* Entries already kept cost a compare; stop after one read. The
       prefetch never evicts: with the cache full it stops here. */
    while (p->next < p->count) {
        char entry_path[DOCK_PREFS_PATH_CAP];
        const DockFolderEntry *entry = &p->entries[p->next];
        DockImage icon = {0};
        unsigned long key;
        strcpy(entry_path,path);
        if (!AddPart((STRPTR)entry_path,(STRPTR)entry->name,sizeof(entry_path))) { ++p->next; continue; }
        key = folder_path_key(entry_path);
        if (cached_folder_icon(ui,key,entry_path,entry->stamp,icon_size)) { ++p->next; continue; }
        if (ui->icon_cache_used >= FOLDER_ICON_CACHE) return 0;
        ++p->next;
        read_folder_icon(ui,key,entry_path,entry->stamp,icon_size,read_icon,context,&icon,0);
        free(icon.pixels);
        break;
    }
    if (p->next < p->count) return 1;
    free(p->entries); p->entries = NULL;
    return 0;
}

void dock_ui_set_busy(DockUI *ui, const char *status)
{
    if (!ui || !ui->dialog) return;
    if (status) {
        strncpy(ui->busy_title,status,sizeof(ui->busy_title)-1);
        ui->busy_title[sizeof(ui->busy_title)-1] = 0;
    }
    SetWindowTitles(ui->dialog,(STRPTR)(status ? ui->busy_title : ui->dialog_title),(UBYTE *)~0);
    SetWindowPointer(ui->dialog,WA_BusyPointer,status != NULL,WA_PointerDelay,FALSE,TAG_DONE);
    if (ui->settings.save) {
        /* Nothing can change while the draft is applied. */
        SettingsView *s = &ui->settings;
        struct Gadget *const gadgets[] = {s->slider[SIZE_SLIDER], s->slider[SCALE_SLIDER],
            s->position, s->cancel, s->save};
        size_t i;
        for (i = 0; i < sizeof(gadgets)/sizeof(gadgets[0]); ++i)
            if (gadgets[i]) GT_SetGadgetAttrs(gadgets[i],ui->dialog,NULL,GA_Disabled,status != NULL,TAG_DONE);
    }
}

void dock_ui_set_field_counter(DockUI *ui, const volatile ULONG *fields)
{
    if (ui) ui->fields = fields;
}

void dock_ui_set_fit_hook(DockUI *ui, DockUIFit hook, void *context)
{
    if (!ui) return;
    ui->fit = hook;
    ui->fit_context = context;
    ui->settings.fit_valid = 0;
}

void dock_ui_set_converters(DockUI *ui, DockConvertRGB24 encode, DockConvertRGB565 decode,
    DockConvertRGB565 encode565, DockMixHalf mix_half)
{
    ui->mix_half = mix_half;
    ui->encode565 = encode565;
    ui->encode = encode;
    ui->decode = decode;
}

/* Inside the label outline moved inward by inset px: a pill of height body
   with semicircular ends, plus a tip of tip rows under tip_x. In 1/16 px. */
static int label_inside(int x, int y, int width, int body, int tip, int tip_x, int inset)
{
    int r = body*8-inset*16, left = body*8, right = width*16-body*8;
    int dx = x < left ? left-x : x > right ? x-right : 0, dy = y-body*8;
    if (r > 0 && dx*dx+dy*dy <= r*r) return 1;
    if (tip && y >= body*16-24) {
        /* 45-degree sides; moving them inward 1 px lowers the apex by sqrt 2. */
        int half = (body+tip)*16-inset*23-y;
        if (half > 0 && abs(x-(tip_x*16+8)) <= half) return 1;
    }
    return 0;
}

/* Area coverage (0-255) of pixel (px,py), from 8x8 samples where the
   outline crosses it. */
static int label_coverage(int px, int py, int width, int body, int tip, int tip_x, int inset)
{
    int sx, sy, inside = 0, x = px*16, y = py*16;
    if (py >= body && abs(px-tip_x) > tip+2) return 0;
    if (label_inside(x,y,width,body,0,0,inset) && label_inside(x+16,y,width,body,0,0,inset) &&
        label_inside(x,y+16,width,body,0,0,inset) && label_inside(x+16,y+16,width,body,0,0,inset))
        return 255; /* The pill is convex: all four corners inside means all of it. */
    for (sy = 0; sy < 8; ++sy) for (sx = 0; sx < 8; ++sx)
        inside += label_inside(x+1+2*sx,y+1+2*sy,width,body,tip,tip_x,inset);
    return (inside*255+32)/64;
}

/* Ink box of text that paint_ui_text draws with its top-left at (0,0). */
static int label_ink(DockUI *ui, struct RastPort *rp, const char *text, DockRect *ink)
{
    struct TextExtent extent;
    if (ui->chrome_aa && ui->text_alpha)
        return amiga_font_aa_ink(ui->chrome_aa,text,&ink->left,&ink->top,&ink->right,&ink->bottom);
    amiga_text_extent(rp,text,strlen(text),&extent);
    if (extent.te_Extent.MaxX < extent.te_Extent.MinX) return 0;
    ink->left = extent.te_Extent.MinX; ink->right = extent.te_Extent.MaxX+1;
    ink->top = rp->TxBaseline+extent.te_Extent.MinY;
    ink->bottom = rp->TxBaseline+extent.te_Extent.MaxY+1;
    return 1;
}

typedef struct {
    char text[128];
    int width, body, text_x, text_top;
} LabelGeometry;

/* The label's text (shortened with "..." to fit max_width), the pill's
   width and body height, and where the text goes in it. */
static int label_geometry(DockUI *ui, const char *text, int max_width, LabelGeometry *g)
{
    struct RastPort measure;
    char *label = g->text;
    size_t length = strlen(text);
    int text_width;
    DockRect ink, caps;
    InitRastPort(&measure); SetFont(&measure,ui->draw->dri_Font);
    if (length < sizeof(g->text)) strcpy(label,text);
    else {
        length = sizeof(g->text)-4;
        memcpy(label,text,length); strcpy(label+length,"...");
    }
    text_width = text_length32(&measure,label);
    if (text_width+2*LABEL_PADDING > max_width) {
        if (length > sizeof(g->text)-4) length = sizeof(g->text)-4;
        do {
            strcpy(label+--length,"...");
            text_width = text_length32(&measure,label);
        } while (length && text_width+2*LABEL_PADDING > max_width);
    }
    if (text_width <= 0 || text_width+2*LABEL_PADDING > max_width) return 0;
    /* Centre the ink, not the advance box: equal ink margins at both ends,
       and the capitals of the font (measured on "H") centred vertically. */
    if (!label_ink(ui,&measure,label,&ink)) ink = (DockRect){0,0,text_width,ui->font_height};
    if (!ui->label_metrics) {
        /* The font and its antialiasing are fixed for the UI's life. */
        static const char reference[] = "\xc5\xc4\xd6gjpqy_|";
        if (!label_ink(ui,&measure,"H",&ui->label_caps))
            ui->label_caps = (DockRect){0,0,1,ui->font_height};
        ui->label_metrics = label_ink(ui,&measure,reference,&ui->label_tall) ? 2 : 1;
    }
    caps = ui->label_caps;
    {
        /* Accents, descenders and the label's own ink must stay inside the
           rim: grow the padding on both sides alike, so the capitals stay
           centred and every label of one font has the same height. */
        const DockRect tall = ui->label_tall;
        int pad = LABEL_VPAD;
        if (ui->label_metrics == 2) {
            if (caps.top-tall.top+2 > pad) pad = caps.top-tall.top+2;
            if (tall.bottom-caps.bottom+2 > pad) pad = tall.bottom-caps.bottom+2;
        }
        if (caps.top-ink.top+2 > pad) pad = caps.top-ink.top+2;
        if (ink.bottom-caps.bottom+2 > pad) pad = ink.bottom-caps.bottom+2;
        g->body = caps.bottom-caps.top+2*pad;
        g->text_top = pad-caps.top;
    }
    g->width = ink.right-ink.left+2*LABEL_PADDING;
    if (g->width < g->body) g->width = g->body;
    g->text_x = (g->width-(ink.right-ink.left))/2-ink.left;
    return 1;
}

/* label_outline sampled at every pixel. */
static unsigned char *label_outline_sampled(int width, int height, int body, int tip, int tip_x)
{
    unsigned char *outer = malloc((size_t)width*height*2), *inner;
    int x, y;
    if (!outer) return NULL;
    inner = outer+(size_t)width*height;
    for (y = 0; y < height; ++y) for (x = 0; x < width; ++x) {
        /* The inset outline lies inside the outer one: no outer coverage,
           no inner coverage. */
        int o = label_coverage(x,y,width,body,tip,tip_x,0);
        outer[y*width+x] = (unsigned char)o;
        inner[y*width+x] = (unsigned char)(o ? label_coverage(x,y,width,body,tip,tip_x,1) : 0);
    }
    return outer;
}

/* The outline coverage (outer) and the inset rim's (inner), each
   width*height bytes in one block the caller frees. The caps and middle
   column depend only on body, so they are sampled once per label and
   copied across; only the tip is sampled where it lies (sampling every
   pixel took ~10 ms of a label's making). */
static unsigned char *label_outline(int width, int height, int body, int tip, int tip_x)
{
    int half = (body+1)/2, cols = half+1, x, y, k;
    unsigned char *out, *inner, *cap;
    if (width < 2*half+1) return label_outline_sampled(width,height,body,tip,tip_x);
    out = malloc((size_t)width*height*2);
    cap = malloc((size_t)cols*body*2);
    if (!out || !cap) { free(out); free(cap); return NULL; }
    inner = out+(size_t)width*height;
    /* Columns 0..half-1 of the left cap, then the middle; any width of at
       least 2*half+1 gives the same coverage. */
    for (k = 0; k < cols; ++k) for (y = 0; y < body; ++y) {
        int o = label_coverage(k,y,2*half+1,body,0,0,0);
        cap[(k*body+y)*2] = (unsigned char)o;
        cap[(k*body+y)*2+1] = (unsigned char)(o ? label_coverage(k,y,2*half+1,body,0,0,1) : 0);
    }
    for (y = 0; y < height; ++y) for (x = 0; x < width; ++x) {
        int col = x < half ? x : x >= width-half ? width-1-x : half;
        size_t i = (size_t)y*width+x;
        out[i] = y < body ? cap[(col*body+y)*2] : 0;
        inner[i] = y < body ? cap[(col*body+y)*2+1] : 0;
    }
    free(cap);
    if (tip) for (y = body > 2 ? body-2 : 0; y < height; ++y)
        for (x = tip_x-tip-3; x <= tip_x+tip+3; ++x) {
            size_t i = (size_t)y*width+x;
            int o;
            if (x < 0 || x >= width) continue;
            o = label_coverage(x,y,width,body,tip,tip_x,0);
            out[i] = (unsigned char)o;
            inner[i] = (unsigned char)(o ? label_coverage(x,y,width,body,tip,tip_x,1) : 0);
        }
    return out;
}

/* The label's height with its tip, for an overlay label; 0 without
   antialiased text (the label is then a window). */
int dock_ui_label_height(DockUI *ui)
{
    LabelGeometry g;
    if (!ui || !ui->chrome_aa) return 0;
    return label_geometry(ui,"H",ui->screen->Width,&g) ? g.body+LABEL_TIP : 0;
}

/* The label's width for text within max_width; 0 if it does not fit. */
int dock_ui_label_width(DockUI *ui, const char *text, int max_width)
{
    LabelGeometry g;
    if (!ui || max_width < 2*LABEL_PADDING+16 || !label_geometry(ui,text,max_width,&g)) return 0;
    return g.width;
}

/* The label as straight-alpha RGBA for the dock's own image: the window
   label's glass, back*(255-o) + rim*(o-n) + (4*surface+back)/5*n, is this
   colour over the backdrop at alpha o-n/5. With tip (the bottom edge) its
   tip points down at tip_x, or at the middle when tip_x < 0. The caller
   frees out->pixels. */
int dock_ui_label_image(DockUI *ui, const char *text, int max_width, int tip, int tip_x, DockImage *out)
{
    ULONG rim = menu_colors[MENU_RIM], surface = menu_colors[MENU_SURFACE];
    LabelGeometry g;
    unsigned char *outer;
    int width, height;
    size_t i, count;
    *out = (DockImage){0};
    if (!ui || !ui->chrome_aa || max_width < 2*LABEL_PADDING+16 ||
        !label_geometry(ui,text,max_width,&g)) return 0;
    tip = tip ? LABEL_TIP : 0;
    width = g.width; height = g.body+tip;
    if (tip_x < 0) tip_x = width/2;
    tip_x = dock_clamp(tip_x,g.body/2,width-g.body/2-1);
    if (!(outer = label_outline(width,height,g.body,tip,tip_x))) return 0;
    count = (size_t)width*height;
    if (!(out->pixels = malloc(count*4))) { free(outer); return 0; }
    out->width = width; out->height = height;
    for (i = 0; i < count; ++i) {
        unsigned int o = outer[i], n = outer[count+i], a = o-n/5, c;
        unsigned char *pixel = out->pixels+i*4;
        pixel[3] = (unsigned char)a;
        for (c = 0; c < 3; ++c) {
            unsigned int shift = 16-8*c;
            pixel[c] = a ? (unsigned char)((((rim>>shift)&255)*(o-n)+((surface>>shift)&255)*n*4/5+a/2)/a) : 0;
        }
    }
    free(outer);
    amiga_font_aa_draw(ui->chrome_aa,out,g.text,g.text_x,g.text_top,menu_colors[MENU_TEXT]);
    return 1;
}

/* The pill as RGBA for an overlay: rim and surface as glass over the
   screen at left, top when glass is set (soft edges blend with it), else
   flat with hard edges; then the text. Alpha is 255 inside the outline and
   0 outside. out->pixels is the caller's to free. */
static int label_pill(DockUI *ui, const LabelGeometry *g, int height, const unsigned char *outer,
    int glass, int left, int top, DockImage *out)
{
    const unsigned char *inner = outer+(size_t)g->width*height;
    ULONG rim = menu_colors[MENU_RIM], surface = menu_colors[MENU_SURFACE];
    struct BitMap *bitmap = NULL;
    struct RastPort rp;
    AmigaDisplay *display;
    size_t i, count = (size_t)g->width*height;
    int ok = 0;
    *out = (DockImage){g->width,height,malloc(count*4)};
    if (!out->pixels) return 0;
    display = amiga_display_open(ui->screen,g->width,height);
    if (!display) goto done;
    if (!glass || !amiga_display_capture(display,out,left,top))
        for (i = 0; i < count; ++i) {
            out->pixels[i*4] = (unsigned char)(surface>>16); out->pixels[i*4+1] = (unsigned char)(surface>>8);
            out->pixels[i*4+2] = (unsigned char)surface; out->pixels[i*4+3] = 255;
        }
    for (i = 0; i < count; ++i) {
        unsigned int o = outer[i], n = inner[i], c;
        unsigned char *pixel = out->pixels+i*4;
        if (!glass) { o = o >= 128 ? 255 : 0; n = n >= 128 ? 255 : 0; }
        if (!o) continue;
        for (c = 0; c < 3; ++c) {
            unsigned int back = pixel[c], shift = 16-8*c;
            unsigned int glass_tint = glass ? (4*((surface>>shift)&255)+back+2)/5 : (surface>>shift)&255;
            pixel[c] = (unsigned char)((back*(255-o)+((rim>>shift)&255)*(o-n)+glass_tint*n+127)/255);
        }
    }
    bitmap = AllocBitMap(g->width,height,GetBitMapAttr(ui->screen->RastPort.BitMap,BMA_DEPTH),
        BMF_MINPLANES,ui->screen->RastPort.BitMap);
    if (!bitmap) goto done;
    InitRastPort(&rp); rp.BitMap = bitmap;
    if (!amiga_display_write_rgba(display,&rp,out)) goto done;
    prepare_popup_colors(ui);
    SetFont(&rp,ui->draw->dri_Font);
    SetDrMd(&rp,JAM1);
    paint_ui_text(ui,&rp,ui->chrome_aa,g->text,g->text_x,g->text_top,MENU_TEXT);
    if (!amiga_display_read_rgba(display,&rp,out)) goto done;
    for (i = 0; i < count; ++i) out->pixels[i*4+3] = outer[i] && (glass || outer[i] >= 128) ? 255 : 0;
    ok = 1;
done:
    if (bitmap) { WaitBlit(); FreeBitMap(bitmap); }
    amiga_display_close(display);
    if (!ok) { free(out->pixels); out->pixels = NULL; }
    return ok;
}

int dock_ui_pill_image(DockUI *ui, const char *text, DockImage *out)
{
    LabelGeometry g;
    unsigned char *outer;
    int ok;
    *out = (DockImage){0};
    if (!ui || !label_geometry(ui,text,ui->screen->Width/2,&g)) return 0;
    if (!(outer = label_outline(g.width,g.body,g.body,0,0))) return 0;
    ok = label_pill(ui,&g,g.body,outer,0,0,0,out);
    free(outer);
    return ok;
}


static int window_bounds(const struct Window *window, DockRect *bounds)
{
    if (!window) return 0;
    *bounds = (DockRect){window->LeftEdge, window->TopEdge,
        window->LeftEdge+window->Width, window->TopEdge+window->Height};
    return 1;
}

int dock_ui_popup_bounds(const DockUI *ui, DockRect *bounds)
{
    if (!ui) return 0;
    return window_bounds(ui->popup,bounds);
}

int dock_ui_dialog_bounds(const DockUI *ui, DockRect *bounds)
{
    if (!ui) return 0;
    return window_bounds(ui->dialog,bounds);
}

static int open_popup_window(DockUI *ui, int left, int top, int width, int height,
    int report_mouse, int capture_backdrop)
{
    int profile = ui->folder.path[0] && ui->folder.anchor.profile;
    const FolderView *folder = ui->folder.path[0] ? &ui->folder : NULL;
    if (ui->window_shapes)
        ui->popup_shape = make_folder_shape(folder,width,height,ui->popup_rgb && capture_backdrop);
    ui->popup_round = ui->popup_shape || ui->popup_rgb;
    prepare_popup_colors(ui);
    folder_stamp(profile,0);
    if (capture_backdrop) capture_popup_glass(ui,left,top,width,height,report_mouse ? 2 : FOLDER_TINT_SHIFT);
    if (ui->popup_shape && folder && !ui->popup_glass.pixels) {
        /* No glass to paint the strip's backdrop: the outline alone. */
        struct Region *outline = make_folder_shape(folder,width,height,0);
        if (outline) { DisposeRegion(ui->popup_shape); ui->popup_shape = outline; }
    }
    folder_stamp(profile,0);
    ui->popup = OpenWindowTags(NULL, WA_CustomScreen, (ULONG)ui->screen,
        WA_Left, left, WA_Top, top, WA_Width, width, WA_Height, height,
        WA_Borderless, TRUE, WA_Activate, TRUE, WA_RMBTrap, TRUE,
        WA_ReportMouse, report_mouse, WA_SimpleRefresh, TRUE,
        WA_BackFill, (ULONG)LAYERS_NOBACKFILL,
        ui->popup_shape ? WA_ShapeRegion : TAG_IGNORE, (ULONG)ui->popup_shape,
        WA_IDCMP, IDCMP_MOUSEBUTTONS|IDCMP_RAWKEY|IDCMP_REFRESHWINDOW|IDCMP_INACTIVEWINDOW|
            (report_mouse ? IDCMP_MOUSEMOVE : 0), TAG_DONE);
    if (!ui->popup) { close_popup(ui,0); return 0; }
    if (ui->popup->LeftEdge != left || ui->popup->TopEdge != top) free_popup_glass(ui);
    ui->popup_selected = ui->popup_pressed = -1;
    return 1;
}

int dock_ui_open_popup(DockUI *ui, DockEdge edge, int anchor_x, int anchor_y,
    const struct Window *native_dock,
    int item_action, int can_keep, int drawer, int sort)
{
    struct RastPort measure;
    DockRect dock;
    int i, width = 0, height, left, top, screen_top, covers_native_dock;
    enum { POINTER_GAP = 4 };
    if (!ui || ui->dialog) return 0;
    close_popup(ui,0);
    ui->popup_checked = -1;
    ui->popup_text_left = 10;
    if (drawer) {
        ui->popup_items = drawer_items;
        ui->popup_rows = POPUP_MAX_ROWS;
        for (i = 0; i < ui->popup_rows; ++i)
            if (drawer_items[i].action == DOCK_UI_SORT_NAME+sort) ui->popup_checked = i;
        /* Every label past the check column, as in a native menu with a
           checked item. */
        ui->popup_text_left = 10+(ui->font_height*3+2)/4+6;
    } else if (item_action >= DOCK_UI_NEW_SHELL && item_action <= DOCK_UI_QUIT_SHELL) {
        ui->popup_items = shell_items;
        ui->popup_rows = item_action-DOCK_UI_NEW_SHELL+1;
    } else {
        ui->popup_items = can_keep ? temporary_items : item_action == DOCK_UI_CLOSE_ITEM ? &close_item :
            item_action == DOCK_UI_QUIT_ITEM ? &quit_item : menu_items;
        ui->popup_rows = can_keep ? 1+(item_action == DOCK_UI_QUIT_ITEM) :
            item_action ? 1 : sizeof(menu_items)/sizeof(*menu_items);
    }
    InitRastPort(&measure); SetFont(&measure, ui->draw->dri_Font);
    ui->popup_row = ui->font_height+10;
    ui->popup_top[0] = 3;
    for (i = 0; i < ui->popup_rows; ++i) {
        const char *label = ui->popup_items[i].label;
        int label_width = label ? text_length32(&measure,label) : 0;
        if (label_width > width) width = label_width;
        ui->popup_top[i+1] = ui->popup_top[i]+(label ? ui->popup_row : POPUP_SEPARATOR);
    }
    width += ui->popup_text_left+14;
    height = ui->popup_top[ui->popup_rows]+3;
    screen_top = ui->screen->BarHeight+2;
    if (width > ui->screen->Width || height > ui->screen->Height-screen_top) return 0;
    if (edge == DOCK_EDGE_BOTTOM) {
        left = anchor_x-width/2;
        top = anchor_y-height-POINTER_GAP;
    } else {
        left = edge == DOCK_EDGE_LEFT ? anchor_x+POINTER_GAP : anchor_x-width-POINTER_GAP;
        top = anchor_y-3-ui->popup_row/2;
    }
    left = dock_clamp(left, 0, ui->screen->Width-width);
    top = dock_clamp(top, screen_top, ui->screen->Height-height);
    /* A native Dock is part of the screen bitmap, unlike the PiP overlay. */
    covers_native_dock = window_bounds(native_dock,&dock) &&
        left < dock.right && left+width > dock.left && top < dock.bottom && top+height > dock.top;
    if (!open_popup_window(ui,left,top,width,height,1,!covers_native_dock)) return 0;
    paint_popup(ui);
    return 1;
}

static int show_folder(DockUI *ui, FolderAnchor anchor, const char *path, size_t root_length, int entrance)
{
    FolderView next = {0};
    int font_height = folder_font(ui)->tf_YSize;
    int width, height, left, top, max_height, rows;
    /* Without window shapes the tip is drawn over the captured glass. */
    int tip_size = ui->window_shapes || ui->popup_rgb ? FOLDER_TIP : 0;
    int extra_bottom;
    int screen_top = ui->screen->BarHeight+2;

    next.anchor = anchor;
    next.motion = entrance && ui->popup_rgb && tip_size ? FOLDER_OPENING : FOLDER_STILL;
    next.root_length = root_length;
    strcpy(next.path,path);
    rows = folder_grid(ui,&next,&max_height,&extra_bottom);
    if (!next.cols || !rows) return 0;
    folder_stamp(anchor.profile && entrance,1);
    next.read_ok = amiga_folder_read(path,anchor.sort,&next.entries,&next.count);
    folder_stamp(anchor.profile && entrance,0);
    folder_fit_count(&next,rows);
    if (next.count && !folder_page_size(&next)) {
        free(next.entries);
        return 0;
    }
    width = 2*next.pad+next.cols*next.cell_width;
    height = next.header+next.rows*next.cell_height+
        (folder_last_page(&next) ? next.footer : 0)+extra_bottom+(next.count ? 0 : font_height+6);
    if (height > max_height) {
        free(next.entries);
        return 0;
    }
    next.body_width = width;
    next.body_height = height;
    if (anchor.edge == DOCK_EDGE_BOTTOM) {
        left = dock_clamp(anchor.x-width/2,0,ui->screen->Width-width);
        top = anchor.y-FOLDER_GAP-height;
        next.tip = dock_clamp(anchor.x-left,POPUP_RADIUS+FOLDER_TIP_HALF,
            width-POPUP_RADIUS-FOLDER_TIP_HALF-1);
        height += tip_size;
    } else {
        left = anchor.edge == DOCK_EDGE_LEFT ? anchor.x+FOLDER_GAP : anchor.x-FOLDER_GAP-width;
        top = dock_clamp(anchor.y-height/2,screen_top,ui->screen->Height-height);
        next.tip = dock_clamp(anchor.y-top,POPUP_RADIUS+FOLDER_TIP_HALF,
            height-POPUP_RADIUS-FOLDER_TIP_HALF-1);
        if (anchor.edge == DOCK_EDGE_LEFT) {
            left -= tip_size;
            next.body_left = tip_size;
        }
        width += tip_size;
    }
    if (ui->folder.path[0] && ui->popup->Width == width && ui->popup->Height == height &&
        ui->popup->LeftEdge == left && ui->popup->TopEdge == top) {
        next.artwork = ui->folder.artwork;
        /* The glass and shape were made for the tip where it points now. */
        next.tip = ui->folder.tip;
        free(ui->folder.entries);
        next.motion_backdrop = ui->folder.motion_backdrop;
        next.backdrop_layer_count = ui->folder.backdrop_layer_count;
        memcpy(next.backdrop_layers,ui->folder.backdrop_layers,sizeof(next.backdrop_layers));
        free_folder_icons(&ui->folder);
        free(ui->folder.motion_scene.pixels);
        free(ui->folder.motion_frame.pixels);
        free(ui->folder.motion_out.pixels);
        ui->folder = next;
    } else {
        const DockImage *old = &ui->folder.motion_backdrop;
        int opened;
        if (ui->popup && ui->folder.path[0] && old->pixels &&
            old->width == ui->popup->Width && old->height == ui->popup->Height) {
            ui->backdrop_carry = *old;
            ui->carry_left = ui->popup->LeftEdge;
            ui->carry_top = ui->popup->TopEdge;
            ui->folder.motion_backdrop = (DockImage){0};
        }
        close_popup(ui,0);
        ui->folder = next;
        opened = open_popup_window(ui,left,top,width,height,0,1);
        free(ui->backdrop_carry.pixels);
        ui->backdrop_carry = (DockImage){0};
        if (!opened) return 0;
    }
    folder_stamp(anchor.profile && entrance,0);
    ui->popup_pressed = FOLDER_OUTSIDE;
    reset_folder_page(ui);
    return 1;
}

int dock_ui_open_folder(DockUI *ui, DockEdge edge, int x, int y, const char *path, int sort,
    int icon_size, DockUIReadIcon read_icon, const void *context, int profile)
{
    return ui && !ui->dialog && *path &&
        show_folder(ui,(FolderAnchor){edge,x,y,icon_size,profile,read_icon,context,(DockSort)sort},
            path,strlen(path),1);
}

void dock_ui_point_folder_tip(DockUI *ui, int x, int y)
{
    FolderView *f;
    DockImage *glass;
    int width, height, tip, old, low, high, x0, y0, x1, y1;
    if (!ui || !ui->popup) return;
    f = &ui->folder;
    glass = &ui->popup_glass;
    width = ui->popup->Width; height = ui->popup->Height;
    /* Only a still box with its tinted glass and the clean backdrop. */
    if (!f->path[0] || f->motion != FOLDER_STILL || ui->glass_tint_shift || !glass->pixels ||
        !f->motion_backdrop.pixels || glass->width != width || glass->height != height ||
        f->motion_backdrop.width != width || f->motion_backdrop.height != height) return;
    if (f->anchor.edge == DOCK_EDGE_BOTTOM)
        tip = dock_clamp(x-ui->popup->LeftEdge,POPUP_RADIUS+FOLDER_TIP_HALF,
            f->body_width-POPUP_RADIUS-FOLDER_TIP_HALF-1);
    else tip = dock_clamp(y-ui->popup->TopEdge,POPUP_RADIUS+FOLDER_TIP_HALF,
            f->body_height-POPUP_RADIUS-FOLDER_TIP_HALF-1);
    if (tip == f->tip) return;
    old = f->tip;
    f->tip = tip;
    /* The band by the edge facing the dock that both tips lie in: the tip
       and the rim rows (or columns) next to it, three deep into the box. */
    low = (old < tip ? old : tip)-FOLDER_TIP_HALF-2;
    high = (old > tip ? old : tip)+FOLDER_TIP_HALF+3;
    if (f->anchor.edge == DOCK_EDGE_BOTTOM) {
        x0 = low; x1 = high; y0 = f->body_height-3; y1 = height;
    } else {
        y0 = low; y1 = high;
        x0 = f->anchor.edge == DOCK_EDGE_LEFT ? 0 : f->body_left+f->body_width-3;
        x1 = f->anchor.edge == DOCK_EDGE_LEFT ? f->body_left+3 : width;
    }
    x0 = dock_clamp(x0,0,width); x1 = dock_clamp(x1,x0,width);
    y0 = dock_clamp(y0,0,height); y1 = dock_clamp(y1,y0,height);
    retint_folder_glass(f,glass,&f->motion_backdrop,x0,y0,x1,y1);
    if (f->artwork_ready && f->artwork.pixels) {
        int row;
        for (row = y0; row < y1; ++row)
            memcpy(f->artwork.pixels+((size_t)row*width+x0)*4,
                glass->pixels+((size_t)row*width+x0)*4,(size_t)(x1-x0)*4);
    }
    amiga_display_present_tile(ui->popup_display,ui->popup,folder_background(ui),
        x0,y0,x1-x0,y1-y0);
}

static struct Gadget *gadget(DockUI *ui, struct Gadget *previous, ULONG kind,
                             int id, const char *label, int x, int y, int w, int h,
                             ULONG flags, const struct TagItem *tags)
{
    struct NewGadget ng;
    if (!previous) return NULL;
    memset(&ng, 0, sizeof(ng));
    ng.ng_LeftEdge = x; ng.ng_TopEdge = y; ng.ng_Width = w; ng.ng_Height = h;
    ng.ng_GadgetText = (CONST_STRPTR)label; ng.ng_TextAttr = &ui->font;
    ng.ng_GadgetID = id; ng.ng_Flags = flags; ng.ng_VisualInfo = ui->visual;
    return CreateGadgetA(kind, previous, &ng, tags);
}

static struct Window *open_dialog(DockUI *ui, const char *title, int width, int height, ULONG idcmp)
{
    int left, top;
    if (width+ui->screen->WBorLeft+ui->screen->WBorRight > ui->screen->Width ||
        height+ui->screen->WBorTop+ui->font_height+2 > ui->screen->Height-ui->screen->BarHeight-2) return NULL;
    left = (ui->screen->Width-width)/2;
    top = (ui->screen->Height-height)/2;
    if (top < ui->screen->BarHeight+2) top = ui->screen->BarHeight+2;
    return OpenWindowTags(NULL, WA_CustomScreen, (ULONG)ui->screen,
        WA_Left, left, WA_Top, top, WA_InnerWidth, width, WA_InnerHeight, height,
        WA_Title, (ULONG)title, WA_DragBar, TRUE, WA_DepthGadget, TRUE,
        WA_CloseGadget, TRUE, WA_Activate, TRUE, WA_RMBTrap, TRUE,
        WA_SimpleRefresh, TRUE, WA_NotifyDepth, TRUE,
        WA_IDCMP, idcmp|IDCMP_CLOSEWINDOW|IDCMP_REFRESHWINDOW|IDCMP_RAWKEY|IDCMP_CHANGEWINDOW,
        TAG_DONE);
}

static void format_slider(char *text, int slider, int value)
{
    if (slider == SIZE_SLIDER) sprintf(text,"%d px",value);
    else sprintf(text,"%d.%02dx",value/DOCK_SCALE_ONE,value%DOCK_SCALE_ONE);
}

static int *slider_value(DockUI *ui, int slider)
{
    return slider == SIZE_SLIDER ? &ui->draft.size : &ui->draft.scale_percent;
}

static int position_index(int edge)
{
    return edge == DOCK_EDGE_LEFT ? 0 : edge == DOCK_EDGE_RIGHT ? 2 : 1;
}

static void update_limits(DockUI *ui, int live);

int dock_ui_open_settings(DockUI *ui, const DockUIPrefs *initial)
{
    static const char *const slider_labels[] = {"_Size","Sc_ale"};
    SettingsView *s;
    struct Window *w;
    struct RastPort measure;
    struct Gadget *g;
    int width, height, line, row, left, right, x, y, readout, save, cancel, i;
    enum { MARGIN = 12, GAP = 6 };

    if (!ui || !initial || !ready(ui)) return 0;
    if (ui->dialog) { WindowToFront(ui->dialog); ActivateWindow(ui->dialog); return 1; }
    close_popup(ui,0);
    s = &ui->settings;
    ui->draft = *initial;
    ui->draft.size = dock_clamp(initial->size,DOCK_MIN_ICON_SIZE,DOCK_MAX_ICON_SIZE);
    ui->draft.scale_percent = dock_clamp(initial->scale_percent,DOCK_SCALE_ONE,DOCK_MAX_SCALE);
    if (initial->edge != DOCK_EDGE_LEFT && initial->edge != DOCK_EDGE_RIGHT)
        ui->draft.edge = DOCK_EDGE_BOTTOM;
    s->cap[SIZE_SLIDER] = ui->draft.size > SIZE_SLIDER_MAX ? ui->draft.size : SIZE_SLIDER_MAX;
    s->cap[SCALE_SLIDER] = ui->draft.scale_percent > SCALE_SLIDER_MAX ? ui->draft.scale_percent : SCALE_SLIDER_MAX;
    s->zoom_cap = dock_scaled_size(ui->draft.size,ui->draft.scale_percent);
    if (s->zoom_cap < ZOOM_CEILING) s->zoom_cap = ZOOM_CEILING;
    s->fit_valid = 0;
    s->moving = -1;
    update_limits(ui,0);

    line = ui->font_height > ui->font.ta_YSize ? ui->font_height : ui->font.ta_YSize;
    row = line+8;
    if (row < 18) row = 18;
    width = (ui->screen->Width < 366 ? ui->screen->Width-12 : 354)-
        ui->screen->WBorLeft-ui->screen->WBorRight;
    height = 4*row+3*MARGIN+2*GAP;
    if (width < 300) return 0;
    strcpy(ui->dialog_title,"Dock Settings");
    ui->dialog = w = open_dialog(ui,ui->dialog_title,width,height,
        BUTTONIDCMP|CYCLEIDCMP|SLIDERIDCMP|IDCMP_VANILLAKEY);
    if (!w) return 0;

    InitRastPort(&measure);
    SetFont(&measure,ui->draw->dri_Font);
    left = w->BorderLeft+MARGIN;
    right = w->Width-w->BorderRight-MARGIN;
    x = left+text_length32(&measure,"Position")+MARGIN;
    y = w->BorderTop+MARGIN;
    format_slider(s->value[SIZE_SLIDER],SIZE_SLIDER,s->cap[SIZE_SLIDER]);
    format_slider(s->value[SCALE_SLIDER],SCALE_SLIDER,s->cap[SCALE_SLIDER]);
    readout = text_length32(&measure,s->value[SIZE_SLIDER]);
    i = text_length32(&measure,s->value[SCALE_SLIDER]);
    readout = (readout > i ? readout : i)+8;
    save = text_length32(&measure,"Save")+24;
    cancel = text_length32(&measure,"Cancel")+24;
    if (right-x-readout < 64 || right-left-save-cancel < GAP) goto fail;

    g = CreateContext(&ui->gadgets);
    for (i = 0; i < SLIDER_COUNT; ++i) {
        int value = *slider_value(ui,i);
        const struct TagItem slider_tags[] = {
            {GTSL_Min,slider_min[i]}, {GTSL_Max,s->limit[i]}, {GTSL_Level,value},
            {GT_Underscore,'_'}, {TAG_DONE,0}
        };
        const struct TagItem readout_tags[] = {
            {GTTX_Text,(ULONG)s->value[i]}, {GTTX_Justification,GTJ_RIGHT}, {TAG_DONE,0}
        };
        g = s->slider[i] = gadget(ui,g,SLIDER_KIND,GID_SIZE+i,slider_labels[i],
            x,y,right-x-readout-GAP,row,PLACETEXT_LEFT,slider_tags);
        s->shown_max[i] = s->limit[i];
        format_slider(s->value[i],i,value);
        g = s->readout[i] = gadget(ui,g,TEXT_KIND,0,NULL,right-readout,
            y+(row-line)/2,readout,line,0,readout_tags);
        y += row+GAP;
    }
    {
        const struct TagItem position_tags[] = {
            {GTCY_Labels,(ULONG)settings_positions},
            {GTCY_Active,position_index(ui->draft.edge)},
            {GT_Underscore,'_'}, {TAG_DONE,0}
        };
        g = s->position = gadget(ui,g,CYCLE_KIND,GID_POSITION,"_Position",x,y,
            text_length32(&measure,"Bottom")+48,row,PLACETEXT_LEFT,position_tags);
    }
    y += row+MARGIN;
    g = s->cancel = gadget(ui,g,BUTTON_KIND,GID_CANCEL,"Cancel",right-save-cancel-GAP,y,cancel,row,PLACETEXT_IN,NULL);
    g = s->save = gadget(ui,g,BUTTON_KIND,GID_SAVE,"Save",right-save,y,save,row,PLACETEXT_IN,NULL);
    if (!g) goto fail;
    AddGList(w,ui->gadgets,(UWORD)-1,-1,NULL);
    RefreshGList(ui->gadgets,w,NULL,-1);
    GT_RefreshWindow(w,NULL);
    return 1;
fail:
    dock_ui_close_dialog(ui);
    return 0;
}

/* Prerendered lines need cybergraphics alpha; otherwise use the screen
   font, with a soft bold for the title. */
static int about_prerendered(const DockUI *ui)
{
    return ui->text_alpha != NULL;
}

static int about_height(const DockUI *ui, int line)
{
    return about_prerendered(ui) ? about_font[about_lines[line].style].height :
        ui->draw->dri_Font->tf_YSize;
}

/* Extent of a prerendered line: *left is the first covered column
   relative to the pen start. */
static int about_extent(int style, const char *text, int *left)
{
    const AboutFace *face = &about_font[style];
    const unsigned char *c;
    int pen = 0, lo = 0, hi = 0;
    for (c = (const unsigned char *)text; *c; ++c) {
        const AboutGlyph *glyph;
        if (*c < ABOUT_FIRST_CHAR || *c > ABOUT_LAST_CHAR) continue;
        glyph = &face->glyphs[*c-ABOUT_FIRST_CHAR];
        if (glyph->width) {
            if (pen+glyph->bearing < lo) lo = pen+glyph->bearing;
            if (pen+glyph->bearing+glyph->width > hi) hi = pen+glyph->bearing+glyph->width;
        }
        pen += glyph->advance;
    }
    *left = lo;
    return hi-lo;
}

static int about_width(const DockUI *ui, struct RastPort *rp, int line)
{
    int left;
    if (about_prerendered(ui)) return about_extent(about_lines[line].style,about_lines[line].text,&left);
    return text_length32(rp,about_lines[line].text);
}

static void about_draw(DockUI *ui, struct RastPort *rp, int line, int center, int top)
{
    const char *text = about_lines[line].text;
    if (about_prerendered(ui)) {
        const AboutFace *face = &about_font[about_lines[line].style];
        const unsigned char *c;
        int left, width = about_extent(about_lines[line].style,text,&left), pen = -left, y, x;
        unsigned char *alpha = width > 0 ? calloc((size_t)width*face->height,1) : NULL;
        if (!alpha) return;
        for (c = (const unsigned char *)text; *c; ++c) {
            const AboutGlyph *glyph;
            if (*c < ABOUT_FIRST_CHAR || *c > ABOUT_LAST_CHAR) continue;
            glyph = &face->glyphs[*c-ABOUT_FIRST_CHAR];
            for (y = 0; y < face->height; ++y) {
                const unsigned char *src = face->coverage+glyph->offset+(size_t)y*glyph->width;
                unsigned char *dst = alpha+(size_t)y*width+pen+glyph->bearing;
                for (x = 0; x < glyph->width; ++x) if (src[x] > dst[x]) dst[x] = src[x];
            }
            pen += glyph->advance;
        }
        amiga_alpha_template_blit(ui->text_alpha,rp,alpha,width,face->height,center-width/2,top);
        free(alpha);
        return;
    }
    SetSoftStyle(rp,about_lines[line].style == ABOUT_TITLE ? FSF_BOLD : FS_NORMAL,FSF_BOLD);
    Move(rp,center-text_length32(rp,text)/2,top+ui->draw->dri_Font->tf_Baseline);
    text32(rp,text);
    SetSoftStyle(rp,FS_NORMAL,FSF_BOLD);
}

static void paint_about(DockUI *ui)
{
    struct Window *w = ui->dialog;
    struct RastPort *rp = w->RPort;
    int i, y = w->BorderTop+ABOUT_MARGIN;
    SetFont(rp,ui->draw->dri_Font);
    SetDrMd(rp,JAM1);
    SetAPen(rp,ui->draw->dri_Pens[TEXTPEN]);
    for (i = 0; i < ABOUT_LINES; ++i) {
        int link = i == ABOUT_LINK && ui->openurl;
        if (link) SetAPen(rp,ui->link_pen >= 0 ? (ULONG)ui->link_pen : ui->draw->dri_Pens[TEXTPEN]);
        about_draw(ui,rp,i,w->Width/2,y);
        if (link) {
            struct RastPort measure;
            int width, bottom = y+about_height(ui,i);
            InitRastPort(&measure); SetFont(&measure,ui->draw->dri_Font);
            width = about_width(ui,&measure,i);
            ui->about_link = (DockRect){w->Width/2-width/2,y,w->Width/2-width/2+width,bottom+1};
            RectFill(rp,ui->about_link.left,bottom,ui->about_link.right-1,bottom);
            SetAPen(rp,ui->draw->dri_Pens[TEXTPEN]);
        }
        y += about_height(ui,i)+about_lines[i].gap;
    }
}

/* openurl.library: URL_OpenA(url, tags), the first function. */
static void open_about_link(DockUI *ui)
{
    struct Library *base = ui->openurl;
    if (!base) return;
    LP2(0x1e, ULONG, URL_OpenA, STRPTR, (STRPTR)about_url, a0, struct TagItem *, NULL, a1, , base);
}

int dock_ui_open_about(DockUI *ui)
{
    struct RastPort measure;
    struct Gadget *previous;
    int i, width = 240, height = 2*ABOUT_MARGIN, button_width, button_height;
    if (!ui || ui->dialog || !ready(ui)) return 0;
    close_popup(ui,0);
    InitRastPort(&measure); SetFont(&measure,ui->draw->dri_Font);
    for (i = 0; i < ABOUT_LINES; ++i) {
        int line_width = about_width(ui,&measure,i)+2*ABOUT_MARGIN+16;
        if (line_width > width) width = line_width;
        height += about_height(ui,i)+about_lines[i].gap;
    }
    button_width = ui->font_width*10;
    button_height = ui->font_height+12;
    height += button_height+10; /* 16 px between the credit and the button. */
    ui->openurl = OpenLibrary((STRPTR)"openurl.library",1);
    ui->about_link = (DockRect){0};
    /* A link blue (FILLPEN read as pale grey on ApolloOS); the text pen
       underlined without one. */
    ui->link_pen = ui->openurl ? ObtainBestPen(ui->screen->ViewPort.ColorMap,
        0x20202020UL,0x50505050UL,0xb0b0b0b0UL,OBP_Precision,PRECISION_GUI,TAG_DONE) : -1;
    ui->dialog = open_dialog(ui,"About",width,height,BUTTONIDCMP|IDCMP_MOUSEBUTTONS);
    if (!ui->dialog) { dock_ui_close_dialog(ui); return 0; }
    previous = CreateContext(&ui->gadgets);
    if (!previous || !gadget(ui,previous,BUTTON_KIND,GID_CANCEL,"OK",
            (ui->dialog->Width-button_width)/2,ui->dialog->BorderTop+height-button_height-12,
            button_width,button_height,PLACETEXT_IN,NULL)) {
        dock_ui_close_dialog(ui);
        return 0;
    }
    ui->showing_about = 1;
    AddGList(ui->dialog,ui->gadgets,(UWORD)-1,-1,NULL);
    RefreshGList(ui->gadgets,ui->dialog,NULL,-1);
    GT_RefreshWindow(ui->dialog,NULL);
    paint_about(ui);
    return 1;
}

static int folder_hit(const DockUI *ui, int x, int y)
{
    const FolderView *f = &ui->folder;
    int font_height = folder_font(ui)->tf_YSize;
    int width = f->body_width, control = font_height+10;
    int top = (f->header-control)/2;
    int col, row, slot;

    if (f->motion == FOLDER_OPENING) {
        /* Only the part already on screen can be hit. */
        int rx, ry, rw, rh;
        folder_reveal_rect(f,ui->popup->Width,ui->popup->Height,0,f->reveal_presented,&rx,&ry,&rw,&rh);
        if (x < rx || x >= rx+rw || y < ry || y >= ry+rh) return FOLDER_OUTSIDE;
    }
    x -= f->body_left;
    if (folder_nested(f) && y >= top && y < top+control &&
        x >= f->pad && x < f->pad+control) return FOLDER_BACK;
    top = f->body_height-f->footer;
    if (folder_last_page(f) && y >= top && y < top+control) {
        if (f->page && x >= f->pad && x < f->pad+control) return FOLDER_PREV;
        if (f->page < folder_last_page(f) && x >= width-f->pad-control &&
            x < width-f->pad) return FOLDER_NEXT;
    }
    y -= f->header+(f->count ? 0 : font_height+6);
    x -= f->count ? f->pad : (width-f->cell_width)/2;
    if (x < 0 || y < 0) return FOLDER_OUTSIDE;
    col = x/f->cell_width;
    row = y/f->cell_height;
    if (col >= f->cols || row >= f->rows) return FOLDER_OUTSIDE;
    slot = row*f->cols+col;
    return slot <= folder_visible_files(f) ? slot : FOLDER_OUTSIDE;
}

static int folder_select(DockUI *ui, int hit, char *open_path)
{
    FolderView *f = &ui->folder;
    char path[DOCK_PREFS_PATH_CAP];
    size_t entry;
    if (hit == FOLDER_PREV || hit == FOLDER_NEXT) {
        if (hit == FOLDER_PREV && f->page) --f->page;
        else if (hit == FOLDER_NEXT && f->page < folder_last_page(f)) ++f->page;
        else return DOCK_UI_NONE;
        if (f->motion) clear_folder_motion(f);
        reset_folder_page(ui);
        return DOCK_UI_REFRESH;
    }
    strcpy(path,f->path);
    if (hit == FOLDER_BACK) {
        char *end = (char *)FilePart((STRPTR)path);
        if (!folder_nested(f)) return DOCK_UI_NONE;
        if ((size_t)(end-path) > f->root_length && end[-1] == '/') --end;
        *end = '\0';
    } else {
        if (hit == folder_visible_files(f)) {
            strcpy(open_path,path);
            return DOCK_UI_OPEN_PATH;
        }
        entry = f->page*folder_page_size(f)+hit;
        if (!AddPart((STRPTR)path,(STRPTR)f->entries[entry].name,sizeof(path))) return DOCK_UI_NONE;
        if (!f->entries[entry].drawer) {
            strcpy(open_path,path);
            return DOCK_UI_OPEN_PATH;
        }
    }
    show_folder(ui,f->anchor,path,f->root_length,0);
    return DOCK_UI_REFRESH;
}

static int popup_poll(DockUI *ui, DockUIAction *result)
{
    struct IntuiMessage *msg;
    int action = DOCK_UI_NONE, close = 0;
    while (ui->popup && (msg = (struct IntuiMessage *)GetMsg(ui->popup->UserPort))) {
        ULONG cls = msg->Class;
        UWORD code = msg->Code;
        int folder = ui->folder.path[0] != 0;
        int hit = folder ? folder_hit(ui,msg->MouseX,msg->MouseY) : popup_hit(ui,msg->MouseX,msg->MouseY);
        if (!close && cls == IDCMP_REFRESHWINDOW) {
            BeginRefresh(ui->popup);
            if (folder) paint_folder(ui); else paint_popup(ui);
            EndRefresh(ui->popup,TRUE);
        }
        ReplyMsg((struct Message *)msg);
        if (close || ui->folder.motion == FOLDER_CLOSING) continue;
        if (cls == IDCMP_INACTIVEWINDOW || (cls == IDCMP_RAWKEY && code == 0x45)) close = 1;
        else if (folder) {
            int selected = FOLDER_OUTSIDE;
            if (cls == IDCMP_MOUSEBUTTONS && code == MENUDOWN) close = 1;
            if (cls == IDCMP_MOUSEBUTTONS && code == SELECTDOWN) ui->popup_pressed = hit;
            if (cls == IDCMP_MOUSEBUTTONS && code == SELECTUP) {
                if (hit == ui->popup_pressed) selected = hit;
                ui->popup_pressed = FOLDER_OUTSIDE;
            }
            if (cls == IDCMP_RAWKEY) {
                if (code == 0x41) selected = FOLDER_BACK;
                else if (code == 0x4f) selected = FOLDER_PREV;
                else if (code == 0x4e) selected = FOLDER_NEXT;
            }
            if (selected != FOLDER_OUTSIDE) {
                action = folder_select(ui,selected,result->path);
                if (action == DOCK_UI_OPEN_PATH) {
                    dock_ui_dismiss_popup(ui);
                    if (ui->popup) {
                        strcpy(ui->folder_open_path,result->path);
                        return DOCK_UI_REFRESH;
                    }
                }
                if (action) return action;
            }
        }
        else if (cls == IDCMP_MOUSEMOVE && (hit < 0 ? -1 : hit) != ui->popup_selected) {
            int previous = ui->popup_selected;
            ui->popup_selected = hit < 0 ? -1 : hit; paint_popup_hover(ui,previous);
        } else if (cls == IDCMP_MOUSEBUTTONS && hit == POPUP_INERT) {
            /* A heading or separator: the menu stays, as a native one does. */
        } else if (cls == IDCMP_MOUSEBUTTONS && code == SELECTDOWN) {
            if (hit < 0) close = 1;
            else {
                int previous = ui->popup_selected;
                ui->popup_pressed = hit; ui->popup_selected = hit; paint_popup_hover(ui,previous);
            }
        } else if (cls == IDCMP_MOUSEBUTTONS && code == SELECTUP) {
            if (hit >= 0 && hit == ui->popup_pressed) action = ui->popup_items[hit].action;
            close = 1;
        }
        /* Opening RMB release is intentionally ignored. Selection is a
           subsequent left click, with matching DOWN and UP inside one row. */
    }
    if (close) { dock_ui_dismiss_popup(ui); return action ? action : DOCK_UI_CANCEL; }
    return DOCK_UI_NONE;
}

/* Keep the draft within what the dock can show: Size up to the largest that
   fits this edge and scale, and Scale up to the 192 px zoom.
   cap holds the slider ranges, raised for larger saved values. */
static void update_limits(DockUI *ui, int live)
{
    SettingsView *s = &ui->settings;
    const DockUIPrefs *probe = &ui->draft;
    int slider, size_max = s->cap[SIZE_SLIDER], scale_max = s->cap[SCALE_SLIDER];
    if (ui->fit) {
        if (!s->fit_valid || s->fit_edge != probe->edge || s->fit_scale != probe->scale_percent) {
            s->fit_size = ui->fit(ui->fit_context,probe);
            s->fit_edge = probe->edge;
            s->fit_scale = probe->scale_percent;
            s->fit_valid = 1;
        }
        if (s->fit_size < size_max) size_max = s->fit_size;
    }
    if (size_max < slider_min[SIZE_SLIDER]) size_max = slider_min[SIZE_SLIDER];
    if (ui->draft.size > size_max) ui->draft.size = size_max;
    while (scale_max > DOCK_SCALE_ONE &&
        ((unsigned int)(dock_scaled_size(ui->draft.size,scale_max)-1) >= DOCK_MAX_ICON_SIZE ||
         dock_scaled_size(ui->draft.size,scale_max) > s->zoom_cap)) --scale_max;
    if (ui->draft.scale_percent > scale_max) ui->draft.scale_percent = scale_max;
    s->limit[SIZE_SLIDER] = size_max;
    s->limit[SCALE_SLIDER] = scale_max;
    if (!live) return;
    for (slider = 0; slider < SLIDER_COUNT; ++slider) {
        int value = *slider_value(ui,slider), count = 0;
        struct TagItem tags[3];
        char text[sizeof(s->value[0])];
        /* Setting GTSL_Max redraws the slider even when unchanged, and the
           knob being dragged already shows its level. */
        if (s->limit[slider] != s->shown_max[slider]) {
            tags[count++] = (struct TagItem){GTSL_Max,(ULONG)s->limit[slider]};
            s->shown_max[slider] = s->limit[slider];
        }
        if (slider != s->moving || value != s->moving_level)
            tags[count++] = (struct TagItem){GTSL_Level,(ULONG)value};
        tags[count] = (struct TagItem){TAG_DONE,0};
        if (count) GT_SetGadgetAttrsA(s->slider[slider],ui->dialog,NULL,tags);
        format_slider(text,slider,value);
        if (strcmp(text,s->value[slider])) {
            strcpy(s->value[slider],text);
            GT_SetGadgetAttrs(s->readout[slider],ui->dialog,NULL,GTTX_Text,(ULONG)s->value[slider],TAG_DONE);
        }
    }
}

static void update_slider(DockUI *ui, int slider, int value)
{
    SettingsView *s = &ui->settings;
    int *setting = slider_value(ui,slider);
    value = dock_clamp(value,slider_min[slider],s->limit[slider]);
    if (value == *setting) return;
    *setting = value;
    format_slider(s->value[slider],slider,value);
    GT_SetGadgetAttrs(s->readout[slider],ui->dialog,NULL,GTTX_Text,(ULONG)s->value[slider],TAG_DONE);
    update_limits(ui,1);
}

/* A slider reported its knob at level: the knob needs no GTSL_Level. */
static void slider_moved(DockUI *ui, int slider, int level)
{
    SettingsView *s = &ui->settings;
    s->moving = slider;
    s->moving_level = level;
    update_slider(ui,slider,level);
    s->moving = -1;
}

static int settings_key(DockUI *ui, UWORD code)
{
    SettingsView *s = &ui->settings;
    switch (code) {
        case 13: return DOCK_UI_SAVE;
        case 27: return DOCK_UI_CANCEL;
        case 'p': case 'P': {
            int choice = (position_index(ui->draft.edge)+(code == 'p' ? 1 : 2))%3;
            ui->draft.edge = settings_edges[choice];
            GT_SetGadgetAttrs(s->position,ui->dialog,NULL,GTCY_Active,choice,TAG_DONE);
            update_limits(ui,1);
            break;
        }
        case 's': case 'S': case 'a': case 'A': {
            int slider = code == 's' || code == 'S' ? SIZE_SLIDER : SCALE_SLIDER;
            int value = *slider_value(ui,slider);
            update_slider(ui,slider,value+(code == 's' || code == 'a' ? 1 : -1));
            value = *slider_value(ui,slider);
            GT_SetGadgetAttrs(s->slider[slider],ui->dialog,NULL,GTSL_Level,value,TAG_DONE);
            break;
        }
    }
    return DOCK_UI_NONE;
}

static int dialog_poll(DockUI *ui)
{
    struct IntuiMessage *msg;
    int action = DOCK_UI_NONE, moved = 0;
    if (!ui->dialog) return 0;
    while ((msg = GT_GetIMsg(ui->dialog->UserPort))) {
        ULONG cls = msg->Class;
        UWORD code = msg->Code;
        const struct Gadget *g = msg->IAddress;
        int id = (cls == IDCMP_GADGETUP && g) ? g->GadgetID : 0;
        int mx = msg->MouseX, my = msg->MouseY;
        GT_ReplyIMsg(msg);
        if (action) continue;
        if (ui->showing_about && cls == IDCMP_MOUSEBUTTONS && code == SELECTDOWN &&
            mx >= ui->about_link.left && mx < ui->about_link.right &&
            my >= ui->about_link.top && my < ui->about_link.bottom) {
            open_about_link(ui);
            continue;
        }
        if (cls == IDCMP_REFRESHWINDOW) {
            GT_BeginRefresh(ui->dialog);
            if (ui->showing_about) paint_about(ui);
            GT_EndRefresh(ui->dialog,TRUE);
        } else if (cls == IDCMP_CHANGEWINDOW) moved = 1;
        else if (cls == IDCMP_CLOSEWINDOW || id == GID_CANCEL ||
            (cls == IDCMP_RAWKEY && (code == 0x45 || (ui->showing_about && code == 0x44))))
            action = DOCK_UI_CANCEL;
        else if (!ui->showing_about) {
            if (cls == IDCMP_VANILLAKEY) action = settings_key(ui,code);
            else if (cls == IDCMP_MOUSEMOVE || cls == IDCMP_GADGETDOWN || cls == IDCMP_GADGETUP) {
                if (g == ui->settings.slider[SIZE_SLIDER]) slider_moved(ui,SIZE_SLIDER,code);
                else if (g == ui->settings.slider[SCALE_SLIDER]) slider_moved(ui,SCALE_SLIDER,code);
                else if (id == GID_POSITION && code < sizeof(settings_edges)/sizeof(settings_edges[0])) {
                    ui->draft.edge = settings_edges[code];
                    update_limits(ui,1);
                } else if (id == GID_SAVE) action = DOCK_UI_SAVE;
            }
        }
    }
    if (action == DOCK_UI_CANCEL) dock_ui_close_dialog(ui);
    return !action && moved ? DOCK_UI_REFRESH : action;
}

int dock_ui_poll(DockUI *ui, DockUIAction *action)
{
    int type;
    if (!ui || !action) return 0;
    action->type = DOCK_UI_NONE;
    if (!ui->popup && ui->folder_open_path[0]) {
        strcpy(action->path,ui->folder_open_path);
        ui->folder_open_path[0] = '\0';
        type = DOCK_UI_OPEN_PATH;
    } else type = popup_poll(ui,action);
    if (!type) type = dialog_poll(ui);
    if (!type) return 0;
    action->type = type;
    action->prefs = ui->draft;
    return 1;
}
