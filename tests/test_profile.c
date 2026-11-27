#include "dock_profile.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Probed capabilities of the machines DockAwesome meets. Fields in
   DockCaps order: cpu_080, ammx, saga_core, saga_pip2, shaped_windows,
   rgb_rastport, alpha_text, window_action, status_notify, apollo_console,
   screen_notify, screen_depth, workbench_port, scalos, dopus, menu_windows. */
typedef struct {
    const char *name;
    DockCaps caps;
    /* Why PiP cannot run there (the dock then does not start). */
    int pip_error;
    int pixels, ui_shapes, ui_glass, aa_text;
} Machine;

static const Machine machines[] = {
    {"ApolloOS R9.5, Vampire V4, 16-bit SAGA",
        {1,1,12515,1,1,1,1,1,1,1,1,16,1,0,0,1},
        DOCK_PROFILE_OK, DOCK_PIXELS_AMMX, 1, 1, 1},
    {"ApolloOS R9.5, Vampire V4, 24-bit screen",
        {1,1,12515,1,1,1,1,1,1,1,1,24,1,0,0,1},
        DOCK_PROFILE_OK, DOCK_PIXELS_AMMX, 1, 1, 1},
    {"ApolloOS R9.5, Vampire V4, 32-bit screen",
        {1,1,12515,1,1,1,1,1,1,1,1,32,1,0,0,1},
        DOCK_PROFILE_OK, DOCK_PIXELS_AMMX, 1, 1, 1},
    {"ApolloOS R9.5, Vampire V4, 8-bit screen",
        {1,1,12515,1,1,1,1,1,1,1,1,8,1,0,0,1},
        DOCK_PROFILE_PIP_SCREEN, DOCK_PIXELS_AMMX, 1, 0, 0},
    {"ApolloOS, Vampire with a core older than 11900",
        {1,1,11000,0,1,1,1,1,1,1,1,16,1,0,0,1},
        DOCK_PROFILE_PIP_NO_CORE, DOCK_PIXELS_AMMX, 1, 1, 1},
    /* AmigaOS 3.2 has no V50 Intuition or Layers and no AROS RGB tags.
       PiP runs without window shapes there (no ApolloOS pointer events). */
    {"AmigaOS 3.2, Vampire V4, 16-bit SAGA",
        {1,1,12515,1,0,0,0,0,0,0,0,16,1,0,0,0},
        DOCK_PROFILE_OK, DOCK_PIXELS_AMMX, 0, 1, 0},
    {"AmigaOS 3.2, Vampire V4, screennotify installed",
        {1,1,12515,1,0,0,0,0,0,0,1,16,1,0,0,0},
        DOCK_PROFILE_OK, DOCK_PIXELS_AMMX, 0, 1, 0},
    /* AmiKit ships screennotify.library and runs Directory Opus instead of
       Workbench. The glass is RGBA images on any RTG screen; AA text needs
       BltTemplateAlpha. */
    {"AmigaOS 3.9/AmiKit, Vampire V4, P96 16-bit",
        {1,1,12515,1,0,0,0,0,0,0,1,16,0,0,1,0},
        DOCK_PROFILE_OK, DOCK_PIXELS_AMMX, 0, 1, 0},
    {"AmigaOS 3.9/AmiKit, Vampire V4, P96 24-bit, CGX alpha",
        {1,1,12515,1,0,0,1,0,0,0,1,24,0,0,1,0},
        DOCK_PROFILE_OK, DOCK_PIXELS_AMMX, 0, 1, 1},
    /* Coffin r65 (AmigaOS 3.9) on the Vampire, probed by drawing on
       2026-11-10: no RGB RastPort tags (pens), no alpha template text, no
       window shapes. The glass and its rounded edges are drawn over the
       captured screen. */
    {"AmigaOS 3.9 Coffin r65, Vampire V4, 24-bit SAGA",
        {1,1,12398,1,0,0,0,0,0,0,1,24,1,0,0,0},
        DOCK_PROFILE_OK, DOCK_PIXELS_AMMX, 0, 1, 0},
    /* The 68080 binary does not run here; the profile still answers. */
    {"AmigaOS 3.x, 68030 AGA, 8-bit",
        {0,0,0,0,0,0,0,0,0,0,0,8,1,0,0,0},
        DOCK_PROFILE_PIP_NO_AMMX, DOCK_PIXELS_PORTABLE, 0, 0, 0},
};

static void check_machine(const Machine *m)
{
    DockProfile p = dock_choose_profile(&m->caps);
    if (p.error != m->pip_error || p.pixels != m->pixels || p.ui_shapes != m->ui_shapes ||
        p.ui_glass != m->ui_glass || p.aa_text != m->aa_text ||
        p.apollo_pointer != m->caps.shaped_windows) {
        fprintf(stderr,"%s: error %d pixels %d shapes %d glass %d aa %d\n",
            m->name,p.error,p.pixels,p.ui_shapes,p.ui_glass,p.aa_text);
        assert(0);
    }
}

static void test_machines(void)
{
    size_t i;
    for (i = 0; i < sizeof(machines)/sizeof(machines[0]); ++i) check_machine(&machines[i]);
}

/* PiP needs the AMMX build and a 16-, 24- or 32-bit screen (window shapes
   only for ApolloOS pointer events); popup_rgb needs shapes, graphics RGB
   tags and more than 8 bits; text_alpha also needs cybergraphics alpha.
   Every combination must match. */
static void test_previous_rules(void)
{
    unsigned int bits;
    const int depths[] = {1, 8, 15, 16, 24, 32};
    for (bits = 0; bits < 64; ++bits) {
        size_t d;
        for (d = 0; d < sizeof(depths)/sizeof(depths[0]); ++d) {
            DockCaps caps;
            DockProfile p;
            int pip_ready, popup_rgb;
            memset(&caps,0,sizeof(caps));
            caps.cpu_080 = caps.ammx = !!(bits&1);
            caps.saga_pip2 = caps.ammx;
            caps.saga_core = caps.ammx ? 12515 : 0;
            caps.shaped_windows = !!(bits&2);
            caps.rgb_rastport = !!(bits&4);
            caps.alpha_text = !!(bits&8);
            caps.window_action = caps.status_notify = !!(bits&16);
            caps.screen_notify = caps.apollo_console = !!(bits&32);
            caps.screen_depth = depths[d];
            pip_ready = caps.ammx && (caps.screen_depth == 16 || caps.screen_depth == 24 ||
                caps.screen_depth == 32);
            popup_rgb = caps.screen_depth > 8;
            p = dock_choose_profile(&caps);
            assert((p.error == DOCK_PROFILE_OK) == pip_ready);
            assert(p.error == DOCK_PROFILE_OK || dock_profile_error_text(p.error)[0]);
            assert(p.apollo_pointer == caps.shaped_windows);
            assert(p.ui_glass == popup_rgb);
            assert(p.ui_rgb_pens == (popup_rgb && caps.rgb_rastport));
            assert(p.aa_text == (popup_rgb && caps.alpha_text));
            assert(p.ui_shapes == caps.shaped_windows);
        }
    }
}

static void test_names(void)
{
    assert(!dock_profile_error_text(DOCK_PROFILE_OK)[0]);
}

int main(void)
{
    test_machines();
    test_previous_rules();
    test_names();
    puts("profile tests passed");
    return 0;
}
