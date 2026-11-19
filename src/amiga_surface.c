/* The common surface steps around the backends (SurfaceOps in
   amiga_app.h): placement, the display layer, canvas and layout, the queued
   drops, and the choice of backend from the probed profile. */
#include "amiga_app.h"

/* Pending AppMessages stay queued. accept_drops handles them on the next
   surface, and main replies to any that remain at exit. */
/* Queued drops are relative to the receiver window, which goes away with
   the surface. Make them screen-relative while no surface is open:
   accept_drops takes them relative to the new one, so a drop queued across
   Save or R lands where it was made. */
static void hold_drops(App *app)
{
    struct Node *node;
    int index = 0;
    if (!app->drop_port) return;
    Forbid();
    for (node = app->drop_port->mp_MsgList.lh_Head; node->ln_Succ; node = node->ln_Succ, ++index) {
        struct AppMessage *msg = (struct AppMessage *)node;
        if (index < app->held_drops) continue;
        msg->am_MouseX += app->window->LeftEdge;
        msg->am_MouseY += app->window->TopEdge;
    }
    app->held_drops = index;
    Permit();
}

void close_surface(App *app)
{
    if (app->window) hold_drops(app);
    app->label_pin_id = 0;
    label_slots_free(app);
    amiga_drag_preview_close(app->drag_preview); app->drag_preview = NULL;
    drag_bob_close(app);
    interaction_cancel(app);
    dock_gesture_reset(&app->gesture);
    input_invalidate(app);
    animation_cancel(app);
    live_cancel(app);
    /* PiP releases its hardware, native window and buffers. No surface is
       set before the first select_backend. */
    if (app->surface) app->surface->close(app);
    free(app->check_canvas.pixels); memset(&app->check_canvas,0,sizeof(app->check_canvas));
    dock_renderer_release(&app->check_renderer);
    app->settling = 0;
    amiga_display_close(app->display); app->display = NULL;
    free(app->canvas.pixels); free(app->backdrop.pixels); dock_renderer_release(&app->renderer);
    memset(&app->canvas, 0, sizeof(app->canvas)); memset(&app->backdrop, 0, sizeof(app->backdrop));
}

int open_surface(App *app)
{
    /* SAGA cannot start a PiP at the screen's left edge: at x 0 no
       alignment showed the image in place (it sat a few pixels right; the
       ApolloOS 1280x720 probe), from x 3 the usual one did. A left dock's
       canvas starts there, where its desktop gap ends: nothing moves. */
    enum { RIGHT_PIP_TAIL = 8, PIP_LEFT_INSET = DOCK_DESKTOP_GAP };
    int reserve = app->count, wanted, limit = viewport_limit(app,app->edge);
    int cross = app->settings.max_size+2*app->settings.padding+12;
    int available_height = app->screen->Height-app->screen->BarHeight-2;
    /* Keep the right PiP raster's keyed tail even where it crosses the screen edge. */
    int edge_padding = app->edge == DOCK_EDGE_RIGHT ? RIGHT_PIP_TAIL-DOCK_DESKTOP_GAP :
        app->edge == DOCK_EDGE_LEFT ? -PIP_LEFT_INSET : 0;
    int panel_cross = dock_panel_thickness(&app->settings)+DOCK_DESKTOP_GAP+edge_padding;
    /* SAGA shows the last columns of each PiP row at the start of the next
       one: a right dock keeps them past the screen edge, a left dock keyed
       on its far side. Without them a pin dragged out of a left dock (on
       AmigaOS 3.2.3) left icon pixels at the screen's left edge and lost
       its own last columns: an invisible edge. */
    app->pip_tail = app->edge == DOCK_EDGE_LEFT ? RIGHT_PIP_TAIL : 0;
    /* Each attempt reports its own error. Keep the text of an earlier failure. */
    if (!app->failed) app->surface_error[0] = 0;
    app->settings.edge_padding = edge_padding;
    if (cross < panel_cross) cross = panel_cross;
    /* Pinning can use this spare room without closing and opening the surface. */
    if (reserve && app->settings.max_size > app->settings.base_size) {
        reserve += 2;
        if (reserve > DOCK_MAX_ITEMS) reserve = DOCK_MAX_ITEMS;
    }
    wanted = dock_viewport_length(&app->settings,reserve);
    if (wanted > limit) wanted = limit;
    if (wanted < 1) return 0;
    app->width = app->edge == DOCK_EDGE_BOTTOM ? wanted : cross+app->pip_tail;
    app->width = (app->width+31)&~31;
    if (app->width > app->screen->Width) app->width = app->screen->Width&~31;
    app->height = app->edge == DOCK_EDGE_BOTTOM ? cross : wanted;
    /* The hover label beside a zoomed icon is drawn in the dock's own
       image: room for it past the zoom headroom, above the row on the
       bottom edge, inside a side dock for its widest label (capped; a
       longer one is shortened). The rows and columns grow on the side
       away from the screen edge, where the layout is not anchored. */
    app->label_max_width = 0;
    app->label_room = 0;
    {
        /* Measured on a row of at least one icon: a pin added to an
           empty dock has its label too. */
        int label = dock_ui_label_height(app->ui), icons = app->count ? app->count : 1;
        DockPlacement rest[DOCK_MAX_ITEMS];
        if (label > 0 && dock_edge_layout(&app->settings,app->edge,icons,app->width,app->height,0,0,0,rest)) {
            int need;
            if (app->edge == DOCK_EDGE_BOTTOM) {
                int headroom = rest[0].y+rest[0].size-app->settings.max_size;
                need = DOCK_UI_LABEL_GAP+label-headroom;
                if (need > 0) { app->height += need; app->label_room = need; }
                app->label_max_width = app->screen->Width/2 < pip_visible_width(app) ?
                    app->screen->Width/2 : pip_visible_width(app);
            } else {
                int column = app->screen->Width/4 < 240 ? app->screen->Width/4 : 240, widest = 0, i;
                int headroom = app->edge == DOCK_EDGE_RIGHT ?
                    rest[0].x+rest[0].size-app->settings.max_size :
                    pip_visible_width(app)-(rest[0].x+app->settings.max_size);
                char text[PATH_CAP];
                for (i = 0; i < app->count; ++i) {
                    int w = pin_label_text(app,i,text) ? dock_ui_label_width(app->ui,text,column) : 0;
                    if (w > widest) widest = w;
                }
                need = DOCK_UI_LABEL_GAP+widest-headroom;
                if (need > 0) app->label_room = (need+31)&~31;
                app->width += app->label_room;
                if (app->width > app->screen->Width) {
                    app->label_room -= app->width-(app->screen->Width&~31);
                    app->width = app->screen->Width&~31;
                }
                app->label_max_width = column;
            }
        }
    }
    if (app->height > available_height) return 0;
    app->left = app->edge == DOCK_EDGE_LEFT ? PIP_LEFT_INSET : app->edge == DOCK_EDGE_RIGHT ?
        app->screen->Width-app->width+edge_padding : (app->screen->Width-app->width)/2;
    app->top = app->edge == DOCK_EDGE_BOTTOM ? app->screen->Height-app->height :
        app->screen->BarHeight+2+(available_height-app->height)/2;
    app->renderer.edge = app->edge;
    app->renderer.center_panel = 0;
    app->dialog_clip = (DockRect){0};
    app->drop_preview_slot = app->displayed_drop_slot = -1;
    app->screen_left = app->screen->LeftEdge; app->screen_top = app->screen->TopEdge;
    app->display = amiga_display_open(app->screen, app->width, app->height);
    if (!app->display || !image_alloc(&app->canvas, app->width, app->height) ||
        !image_alloc(&app->backdrop, app->width, app->height)) return 0;
    amiga_display_set_converters(app->display,app->renderer.pixels->rgba_to_rgb24,
        app->renderer.pixels->decode565,app->renderer.pixels->encode565);
    app->active = 0; app->hovered = -1;
    memset(app->placement, 0, sizeof(app->placement));
    if (!dock_edge_layout(&app->settings, app->edge, app->count, app->width, app->height,
                     0, 0, 0, app->placement)) return 0;
    memcpy(app->target, app->placement, sizeof(app->target));
    dock_animation_reset(&app->animation);
    if (app->damage_check) {
        if (!image_alloc(&app->check_canvas, app->width, app->height)) return 0;
        app->check_renderer.pixels = app->renderer.pixels;
        app->check_renderer.coverage = app->check_coverage;
    }
    /* The backdrop, the native window and AppWindow, and present's needs. */
    if (!app->surface->open(app)) return 0;
    stack_button_build(app);
    return 1;
}

/* Follow the (new) Workbench screen. features.error says why PiP, which
   the dock needs, cannot run there. */
void select_backend(App *app)
{
    amiga_caps_probe_screen(&app->caps,app->screen);
    app->features = dock_choose_profile(&app->caps);
    app->use_ammx = app->features.pixels == DOCK_PIXELS_AMMX;
    app->surface = &amiga_surface_pip;
}

/* Pixel kernels for use_ammx, shared with the controls. */
void use_pixel_backend(App *app)
{
    app->renderer.pixels = dock_backend_portable();
#ifdef DOCK_AMMX
    if (app->use_ammx) app->renderer.pixels = dock_backend_ammx();
#endif
    dock_ui_set_converters(app->ui,app->renderer.pixels->rgba_to_rgb24,app->renderer.pixels->decode565,
        app->renderer.pixels->encode565,app->renderer.pixels->mix_half);
}
