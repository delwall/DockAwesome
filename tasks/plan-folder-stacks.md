# Plan: Folder Stacks for every pinned drawer, no divider

Feature id `folder-stacks`. Branch `folder-stacks` (exists locally, from main
989cfa7 = tag v1.0b5, plus one todo commit 2884476). Prepared 2026-11-24.
All anchors below were checked against that tree.

## 1. Delivery split

| PR | Scope | Must not touch | Proves it done | Model tier | Review tier |
| --- | --- | --- | --- | --- | --- |
| PR 1 (only, FINAL) | D1-D17 below: divider removed from the core, prefs migration, every pinned drawer a stack, stack menu, Settings without Folder Stack, round-robin prefetch, larger icon cache, host tests, docs, draft "1.0 beta 6" notes | `main`; the reorder gap (`end_gap(app)`, `dock_gap_*`); PiP/surface backends; the input handler's press rules other than the close-button rectangle's source; zoom curves; the Folder Stack box's drawing and motion | §4 obligations green; hardware matrix §4.4 and performance §4.5 run and recorded in `tasks/todo.md`; PR open with the evidence | `opus` | Heavy |

One PR: the user asked for one branch, one push, one PR. Heavy because of two
named risks: (a) a preferences format migration that rewrites the user's file
without `"folder"` (not reversible by a downgrade), and (b) state shared
between the main task and the `input.device` handler (`close_*` rectangle,
gesture ownership) changes its source.

## 2. Verified current-state facts

Core and tests
- F1 `DockSettings.end_gap` (`include/dock_core.h:19`) is the divider gap.
  Uses: `dock_settings_valid` (`src/dock_core.c:18`), `dock_row_length`
  (`:24`), `extra_gap` (`:27`, called at 511, 525, 601, 613, 618, 660, 681,
  682, 693), `dock_layout` (`:604`), `dock_viewport_length`
  (`src/dock_geometry.c:15`), `insertion_slot` (`src/dock_geometry.c:178`),
  `draw_divider` (`src/dock_compose.c:214`, called at `:400`),
  `same_settings` and `add_divider` (`src/dock_renderer.c:182`, `:196`,
  `:264-268`). `dock_settings_default` initialises it as the 7th member
  (`src/dock_core.c:8`).
- F2 `end_gap(App *)` (`src/amiga_app.h:662`) is the REORDER gap reset
  (`dock_gap_reset` + `show_gap`). It is unrelated and stays.
- F3 Tests that set `end_gap`: `tests/test_core.c` 550-585 (wave with and
  without the gap, and the "wider Folder Stack pitch" block), 618-627
  (`config[][4]`, 4th column), 1121 `test_clipped_divider`, 1187-1189
  (`dock_items_fit` with 12), 1457-1520 (goldens: `fixture_scenario` has an
  `end_gap` bit, `FIX_SCENARIOS = 3*2*2*2*2*2*4` = 384 at `:1260`, table
  `render_goldens` at `:1381`), 1653 `test_damage_divider`, 2004, 2069 (fuzz
  event `event < 97` toggles it); `tests/test_geometry.c:315`
  `test_stack_gap` (called `:493`); `tests/test_interaction.c:186`.
- F4 `DOCK_INPUT_FIXED` (`include/dock_interaction.h:15`), `DockGesture.fixed`
  (`:33`), set at `src/dock_interaction.c:31`, read at `:47`; tested by
  `test_fixed_pin` (`tests/test_interaction.c:32`, called `:220`). Its only
  producer is `gesture_point` (`src/amiga_main.c:1803`).

Preferences
- F5 `DockPrefs.folder` (`include/dock_prefs.h:15`); read in `read_object`
  case `FOLDER` (`src/dock_prefs.c`), validated in `valid_prefs`
  (`memchr`), written in `dock_prefs_write` when non-empty.
  `tests/test_prefs.c:30,41` round-trips it; `:60` expects it empty; `:90-91`
  are bad-file cases that must stay rejected.
- F6 `save_config` (`src/amiga_main.c:~455`) writes `paths` from items that
  are neither `temporary` nor `stack` (`:481`) and refuses to save while
  `app->incomplete_pins`. `read_config` returns 1 (file), 2 (backup), 0
  (none), -1 (error). Startup saves when `app->count != initial_count ||
  config_state != 1` (`:~2990`).
- F7 The stored `folder` is canonical (`prepare_stack` uses `NameFromLock`,
  `:361`), as are `paths` (`pin_path` uses `canonical_path`, `:395`).
  `pin_path` returns the existing index for a path already pinned
  (`item_path_index`, `src/amiga_app.h:540`, which today skips `stack` items).

Items and the single stack
- F8 `Item` (`src/amiga_app.h:71`): `running, kind, temporary, stack`.
  `PIN_DRAWER` is set by `load_icon_mode` for icon types `WBDRAWER`,
  `WBDISK`, `WBGARBAGE` (`src/amiga_main.c:276`).
- F9 `stack_index()` (`src/amiga_main.c:33`) returns the last index when the
  last item is the stack, else `app->count`. Uses: 42 (`append_item` inserts
  before the stack), 801, 1126, 1408, 1738, 1753, 1810, 1825, 1953, 2362.
  `exchange_stack` (`:61`) and `prepare_stack` (`:361`) serve Settings and
  startup only (`:2405`, `:2421`, `:2430`, `:2975-2976`).
  `STACK_DIVIDER_GAP = 12` (`:38`) feeds `end_gap` at 1106, 2954, 2973, 2977.
- F10 The stack pin is fixed today: `DOCK_INPUT_FIXED` (`:1803`), `movable`
  (`:1951`), `remove_pin` (`:1786`) and `update_drag_preview` (`:2198`) all
  exclude `stack`.
- F11 Draw (`src/amiga_main.c:1552-1580`): `stack_button_shown`,
  `publish_close_button` (`:1528`), `dock_ui_point_folder_tip` and
  `stack_morph` select the pin by `items[].stack`. `stack_morph` keeps three
  blends in `app->morph_slot[3]`, keyed by level and the icon's and button's
  cache stamps (`:1456`); `open_stack_view` builds them before the box moves
  (`:1930`).
- F12 `stack_button_build` (`:1399`) is called only from `open_surface`
  (`src/amiga_surface.c:156`), frees the button when there is no stack, and
  reuses it while size, zoom, levels, cache limit and edge are unchanged.
- F13 Click handling in `apply_gesture` (`:1996-2011`): a stack click with
  `stack_close_press` dismisses, else `open_stack_view`; a click on another
  pin closes an open stack and launches. With PiP the handler consumes a
  press inside `close_left..close_bottom` (`src/amiga_input_handler.c:197`).
  A still Folder Stack does not hold the dock (`dock_ui_holds_dock`,
  `src/amiga_ui.c:627`); a moving one does.
- F14 Running state: `program_poll_schedule` (`src/amiga_status.c:845`)
  builds `kinds` from every item's `kind` and copies `kind` into
  `poll->pins[]`; drawers are matched by `poll_workbench_list(PIN_DRAWER)`
  or `dopus_drawers` (`src/amiga_launch.c:540`). `draw` adds a dot for every
  item with `running` (`src/amiga_main.c:1579`). `open_menu` (`:2269`)
  offers Close for a running `PIN_DRAWER` and passes `stack` as the popup's
  `drawer` argument.

UI (`src/amiga_ui.c`)
- F15 Stack menu: `drawer_items = {"Show Contents", "Close"}` (`:77`);
  `dock_ui_open_popup` shows 2 rows when `item_action == DOCK_UI_CLOSE_ITEM`,
  else 1 (`:2515`).
- F16 The Folder Stack box already ends every page with an "Open in
  Workbench" tile (`folder_page_size` reserves it, `:971`; painted in
  `paint_folder_content`, `:1276-1289`; `folder_select` returns
  `DOCK_UI_OPEN_PATH` with the shown drawer's path, `:3114`). The main loop
  runs `launch_pin(app,action.path,0)` (`src/amiga_main.c:2579`), which opens
  a Workbench window, or a Directory Opus lister when `self_launching`
  (`launch_object`: `lister new "<path>"`, `src/amiga_launch.c:~337`).
- F17 Icon cache: `FOLDER_ICON_CACHE = 160` (`:166`), `FolderCachedIcon`
  {path, stamp[4], icon}, linear lookup `cached_folder_icon` (`:1945`),
  insert `cache_folder_icon` (`:1968`) replaces the same path or the next
  round-robin entry. Lookups run per visible entry in `reset_folder_page`
  (`:1700`, before first paint) and again in `load_folder_icon` (`:2015`).
- F18 Prefetch: one `FolderPrefetch` (`:173`, `ui->prefetch`);
  `dock_ui_prefetch_folder` (`:2088`) rereads the directory when the path or
  icon size changes, caps the page at `FOLDER_COLS*FOLDER_ROWS` = 120
  entries (not the real page), reads at most one uncached icon per call and
  returns 1 while entries remain. Main calls it from the idle branch of the
  event loop (`src/amiga_main.c:2692`: live tick due, pointer off the dock,
  no gesture, no animation, labels prebuilt first); the live tick is 100 ms
  at rest (`live_schedule`, `:959`). `app->prefetch_pending` is set only in
  `prepare_stack`.
- F19 The real page size comes from `show_folder` (`:2554-2596`): cell
  metrics from the icon size and font, `cols`/`rows` from the anchor (edge,
  x, y) and the screen, then a shrink of `cols` by the entry count; page =
  `cols*rows-1`.
- F20 Settings: `DockUIPrefs.folder` (`include/amiga_ui.h:15`), gadgets
  `GID_PATH`, `GID_BROWSE`, `GID_CLEAR` (`:50`), `SettingsView.path/browse/
  clear/fit_folder`, `read_folder_text`, `set_folder_text`, `browse_folder`
  (ASL, the only user of `ui->asl` and of the blocking hook
  `ui->blocking`), layout block at `:2890-2905`, height
  `5*row+line+4*MARGIN+2*GAP+4` (`:2835`), keys Tab/b/c in `settings_key`
  (`:3270-3277`), `update_limits` (`:3193`), `dock_ui_set_busy` gadget list
  (`:2133`). `ui_blocking` itself is also called directly at
  `src/amiga_main.c:3018` and stays.
- F21 Profile output used as the measuring instrument: `Folder Stack first
  paint: N us` (`src/amiga_main.c:1922`), `Folder Stack opening: N us, F
  frames ...` and `Folder Stack prepare_us:` (`src/amiga_ui.c:1840-1859`),
  `Folder Stack icons: R read, C cached ...` (`:2079`), `Dock frames during
  Folder Stack motion` (`src/amiga_main.c:1262`).

Repository
- F22 No CI workflows exist (`.github/` is absent); `make host-test`,
  `make host-sanitize` and `make amiga` are the whole local checklist.
  `HOST_FLAGS` has `-Werror`; `AMIGA_FLAGS` has `-Wall -Wextra` (warnings do
  not fail the build, so read the output).
- F23 `PRODUCT.md` carries the `impeccable` schema marker; `DESIGN.md:26`
  lists Folder Stack as a Settings control.

## 3. Locked implementation decisions (all PR 1)

The user's decisions (made, not to be re-opened):
1. Remove the Folder Stack setting from Settings. On load, an existing
   `"folder"` becomes a normal pin at the end of `"paths"` (if not already
   pinned); `"folder"` is never written again (still read).
2. Every pinned drawer is a stack. Volume/disk roots (path ending in `':'`)
   stay normal pins that open a window.
3. No running dot for stacks.
4. Remove the divider completely; icons stay where the user places them.
5. A stack offers a way to open the drawer as a window (Workbench, or a
   Directory Opus lister when Directory Opus is the desktop).
Performance requirement: after the dock has idled, the first open of any
stack is not slower than 1.0 beta 5's single stack, and hover/zoom does not
regress (§4.5).

D1-D17 implement them.

D1 Divider out of the core. Remove `DockSettings.end_gap`, `extra_gap`,
`draw_divider`, `add_divider` and every use in F1; `dock_settings_default`
loses its 7th initialiser. `dock_row_length` = `count*base+(count-1)*gap`.
Remove `STACK_DIVIDER_GAP`. After the change `grep -rn end_gap src include
tests` shows only `end_gap(app)` and its definition.

D2 Preferences. `DockPrefs.folder` stays as a read-only legacy field (still
parsed and validated). `dock_prefs_write` never writes `"folder"`. New in
`dock_prefs.h/.c`:

```c
/* 1.0 beta 5 and older kept one Folder Stack apart from the pins: make it
   the last pin unless it is one already. 1 when prefs changed. */
int dock_prefs_migrate(DockPrefs *prefs);
```

Rule: no `folder` -> 0. Else: if no `paths[i]` equals it (ASCII
case-insensitive compare, local helper; no OS call) and `count <
DOCK_MAX_ITEMS`, append it; clear `folder`; return 1 (also when it was a
duplicate or there was no room). `read_config` calls it after a successful
`dock_prefs_read` and stores the result in a new `app->prefs_migrated`.
Startup saves at once when `prefs_migrated && !app->incomplete_pins` (added
to the existing condition in F6), so the file loses `"folder"` on the first
start. Nothing else reads `prefs.folder`.

D3 What is a stack. In `pin_path`, after the icon loads:
`stack = kind == PIN_DRAWER && drawer_is_stack(clean)`, where the new static
`drawer_is_stack(path)` is: the path does not end in `':'`, and `Lock` +
`Examine` say `fib_DirEntryType > 0` and not `ST_SOFTLINK` (the same test
`read_folder` applies). If `load_icon` fails but `drawer_is_stack(clean)`,
fall back to `load_icon(app,"ENV:Sys/def_drawer",...)` with `kind =
PIN_DRAWER` (what `load_stack_icon` did for a drawer without an icon).
`append_item` result gets `item->stack = stack`. Volume roots and
non-directories keep today's drawer behaviour (window, dot, Close).
`load_stack_icon` stays for the resize loop in `apply_preferences`.

D4 State. `Item` gains `int prefetch` (its first page is not read ahead yet)
and `int prefetch_turns`. `App` gains, after `stack_close_press` and before
`surface`: `uint32_t open_stack_pin` (the pin whose box is or was last
open), `uint32_t stack_next_pin` (a stack to open once the current one has
closed), `int prefs_migrated`. `ProfileStats` gains `ULONG prefetch_reads`.

D5 `stack_index`, `exchange_stack`, `prepare_stack` are deleted.
`append_item` appends at `app->count` (no shift). Every remaining
`stack_index(app)` becomes `app->count` (801, 1738, 1753, 1810, 1825, 1953);
`draft_fits` uses `dock_items_fit(s,app->count,...)`. `item_path_index`
no longer skips stacks. `save_config` writes stack items like any pin (drop
`&& !stack` at `:481`). Startup: the `configured.end_gap` lines and the
whole `if (app->prefs.folder[0])` block go; the fit check counts
`app->prefs.count`.

D6 Stacks move like pins. Delete `DOCK_INPUT_FIXED`, `DockGesture.fixed`,
its two uses, and the `stack` exclusions in F10. A stack can be reordered
to any slot and dragged out (Remove); `remove_pin` handles it as any pin.

D7 The open stack. `open_stack_view` sets `app->open_stack_pin` on success.
In `draw`, `publish_close_button` and the tip call, "the stack item" becomes
"the item whose `pin_id == app->open_stack_pin`" (still only while
`stack_button_shown`, and for the morph while `app->morph`). The three morph
slots stay shared; they rebuild when another stack opens (their stamp key
already handles it).

D8 Clicks with several stacks (`apply_gesture`, LAUNCH branch):
- stack clicked, no box open or moving: `open_stack_view`.
- stack clicked, `stack_close_press` and it is `open_stack_pin`: dismiss.
- stack clicked, `stack_close_press` and another stack is open: dismiss and
  set `stack_next_pin = item->pin_id`.
- any other pin: unchanged (closes an open box, then launches).
The event loop opens `stack_next_pin` once `!dock_ui_busy(app->ui) &&
!dock_ui_folder_pending(app->ui)`, if the pin still exists, is a stack and no
gesture is active; then clears it. A new press (`!was_active &&
gesture.active`), `open_menu` and `remove_pin` clear it.

D9 No running state for stacks. In `program_poll_schedule`, a stack item
contributes nothing to `kinds` and is copied with `kind = 0` and `running =
0`; nothing else changes in the worker. `open_menu` therefore never offers
Close for a stack, and `draw` shows no dot.

D10 Open as a window. (a) The existing "Open in Workbench" tile (F16) stays
as is. (b) The stack's right-click menu becomes two fixed rows:
`{"Show Contents", DOCK_UI_SHOW_CONTENTS}, {"Open in Workbench",
DOCK_UI_OPEN_DRAWER}`; `DOCK_UI_OPEN_DRAWER` is a new enum value after
`DOCK_UI_OPEN_PATH`; `dock_ui_open_popup` uses 2 rows whenever `drawer`.
The main loop handles it with `launch_pin(app,item->path,1)` +
`status_changed`. Both routes open a Directory Opus lister when
`self_launching` (existing `launch_pin`).

D11 Settings. Remove everything in F20 that serves the Folder Stack:
`DockUIPrefs.folder`, the three gadgets and their ids, `fit_folder`, the
text helpers, `browse_folder`, `ui->asl` and the ASL includes,
`DockUIBlocking`/`dock_ui_set_blocking_hook`/`ui->blocking*` and its two
callers (`amiga_main.c:2947`, `amiga_workbench.c:71`), keys Tab/b/c,
`STRINGIDCMP`. Dialog height becomes `4*row+3*MARGIN+2*GAP`; the width
guard keeps only the slider and Save/Cancel terms. Nothing else in the
dialog moves. `apply_preferences`, `draft_settings`, `current_ui_prefs` lose
their folder logic; on success after a surface reopen `apply_preferences`
re-arms the prefetch (D13).

D12 Close button. `stack_button_build` keeps its reuse rule; "no stack"
means no item has `stack`. `accept_drops` calls `stack_button_build(app)`
after a drop added a pin (a no-op unless the first stack just arrived).

D13 Prefetch, round robin, nearest first.
```c
/* amiga_ui.h. slot: 0..DOCK_MAX_ITEMS-1, one read-ahead state each.
   (edge,x,y): the anchor the box would open at, for the real page size.
   1 while icons of the first page remain (or a popup is open); 0 when the
   page is read, the drawer cannot be read, or the cache has no free entry. */
int dock_ui_prefetch_folder(DockUI *ui, int slot, DockEdge edge, int x, int y,
    const char *path, int icon_size, DockUIReadIcon read_icon, const void *context);
```
`ui->prefetch` becomes `FolderPrefetch prefetch[DOCK_MAX_ITEMS]`; a slot
resets (and rereads its directory) when path, icon size or edge differ;
`dock_ui_destroy` frees every slot. The page: split `show_folder`'s
geometry (F19) into two statics used by both callers in the same order as
today - the cell metrics with the maximum `cols`/`rows` (before the
directory read) and the `cols` shrink + `rows` (after it) - and cap the
slot's `count` at `cols*rows-1`. `show_folder` must compute exactly what it
does now.
Main: `open_stack_view`'s anchor computation becomes a static
`stack_anchor(app,index,&x,&y)` used by both. `prefetch_stack_icons` picks,
among items with `stack && prefetch`, the one with the fewest
`prefetch_turns`; ties go to the pin whose rest centre is nearest the
pointer on the dock's long axis. It calls the UI with `slot = item index`,
adds a turn and a `prefetch_reads`, clears `prefetch` on 0, and returns
whether any stack is still pending (`app->prefetch_pending`). Armed
(`prefetch = 1`, `prefetch_turns = 0`, `prefetch_pending = 1`) when
`pin_path` creates a stack and for all stacks after `apply_preferences`
reopened the surface. The idle condition in the event loop (F18) is
unchanged: still one read per idle live tick.

D14 Icon cache. `FOLDER_ICON_CACHE = 512`. `FolderCachedIcon` gains
`unsigned long key` (FNV-1a of the path bytes), compared first in
`cached_folder_icon` and `cache_folder_icon`; callers hash once per path.
`cache_folder_icon` takes `int evict`: the prefetch passes 0 and never
overwrites a live entry (it checks for a free entry before reading the
icon and returns 0 without one); the open box passes 1 (round robin as
now) and counts `ui->icon_cache_evicted` when it overwrites a live entry of
another path.
```c
void dock_ui_folder_cache_stats(const DockUI *ui, int *used, int *capacity, unsigned long *evicted);
```

D15 Profile. One new report line beside "Dock frames during Folder Stack
motion": `Folder Stack prefetch: stacks=%d pending=%d reads=%lu cache=%d/%d
evicted=%lu`. Existing lines (F21) keep their text.

D16 Host tests.
- `test_prefs.c`: the round trip sets no `folder` and asserts the JSON has
  no `"folder"`; a file with `"folder"` still parses; new
  `test_folder_migration`: appended last; duplicate in another case not
  appended; no folder -> 0; 32 paths -> count stays 32; `folder` empty
  after each; a write after migrating has no `"folder"`. Bad-file cases
  `:90-91` stay.
- `test_core.c`: wave test runs once (no gap loop) and loses the "wider
  pitch" block; `config` loses its 4th column (keep one row per distinct
  size triple); delete `test_clipped_divider`, `test_damage_divider` and
  the `end_gap = 12` fit lines; goldens per §4.2; fuzz event 96 becomes an
  unchanged frame (keep the `fuzz_next` call sequence).
- `test_geometry.c`: delete `test_stack_gap`.
- `test_interaction.c`: delete `test_fixed_pin`; drop the `end_gap`
  assignment at `:186`.

D17 Documents. `docs/architecture.md` (divider at `:18`, `:168`; Settings
fit at `:224`; add two sentences on stacks = pinned drawers and the
round-robin prefetch), `include/dock_compose.h:3`, `include/dock_core.h`,
`DESIGN.md:26` (Settings: Size, Scale, position), `README.md:10`
(every pinned drawer opens as a Folder Stack), `RELEASE_NOTES.md` draft
"## 1.0 beta 6" on top (only claims with a verification entry in
`tasks/todo.md`; no release, no tag), `tasks/todo.md` section with the
checklist and results, `tasks/lessons.md` for any correction.

## 4. Quality obligations

4.1 Build. `make host-test`, `make host-sanitize`, `make amiga`: all pass;
the Amiga build output shows no warning (read it; warnings do not fail).

4.2 Goldens are filtered, not regenerated. Before touching the renderer,
derive the new 192-entry `render_goldens` from the old table: keep old
index `i` iff `((i>>2)&1) == 0`; its new index is `((i>>3)<<2)|(i&3)`.
`fixture_scenario` drops the `end_gap` factor; `FIX_SCENARIOS =
3*2*2*2*2*4`. After the code change `DOCK_PRINT_GOLDENS=1 build/test_core`
must print exactly that table. A mismatch is a rendering regression for
docks without a divider: fix the code, not the table.

4.3 Design skill. Before the review loop run `impeccable` `polish` on the
two UI surfaces this PR changes (the Settings dialog without the Folder
Stack section; the stack's two-row menu). Its craft bar applies inside the
project's rules: `DESIGN.md`, native GadTools controls, and "never remove
an existing visual/UX element without asking" (only the Folder Stack
section and the divider are approved removals). Where they conflict, the
project's rules win.

4.4 Hardware matrix (Vampire, amiga MCP, 192.168.0.229), on every boot the
user brings up: ApolloOS, Coffin, AmigaOS 3.2.3, 3.9, 3.1.4.1, AmiKit XE.
Identify the boot first (`amiga_system_info`, `Version`). Destructive steps
use `--config RAM:<file>.json`; the user's `ENVARC:DockAwesome.json` is
touched only by the final installed build. Per boot:
1. `--caps` unchanged against the installed build.
2. Migration: a copy of a 1.0b5-format file (with `"folder"`) as the test
   config -> after start and Ctrl-C the file has no `"folder"`, the path is
   last in `"paths"`, a second start changes nothing.
3. No divider: `--pip-dump` shows the last two icons 5 px apart at rest.
4. Drop a drawer -> a stack; click opens, click on its close button closes;
   with two stacks, a click on the other closes one and opens the other.
5. Drop a volume -> a click opens its window; dot and Close as before.
6. Stack: tile "Open in Workbench" and menu row open a window (a lister
   under Directory Opus); no dot appears on the stack.
7. Reorder a stack; drag a stack out (Remove); the saved file follows.
8. Settings opens without a Folder Stack section; Save still works.
9. Clean Ctrl-C; RAM: cleaned; the new build installed in place and
   running (`Status` shows exactly one dock); note the build number.
Agent input does not reach the dock on AmiKit (`Input: downs=0`): there,
steps 4-8 are one request to the user with a numbered gesture list and one
thing to look at. Everywhere else the agent drives them itself.

4.5 Performance (measure, do not assume). Baseline binary: main 989cfa7
built in a separate worktree outside the repo's `build/`, uploaded to RAM:
under another name, deleted after the batch.
- First open, AmiKit XE: old / new / old in one session, each from an
  Execute script with `--profile` and redirected output; idle with the
  pointer off the dock until the new build's prefetch line would read
  `pending=0` (same wait for the old build), then the user opens and closes
  each stack once. Old build: its Folder Stack drawer. New build: that same
  drawer as a stack plus at least two more stacks. Pass: for the same
  drawer, new "first paint" and "opening" are not above the old runs'
  maximum; every other stack whose first page is not larger stays under
  that maximum too; `evicted=0` after the idle. Reference from the brief:
  first paint 109-127 ms, opening 132-150 ms.
- Hover/zoom, Coffin and AmiKit: old / new / old with the same sweep
  (Coffin: agent, relative moves, one per message; AmiKit: the user, in
  the same request). Pass: stalled frames and `render_us_avg` of the new
  run lie within the two old runs' spread.
A failed criterion is fixed (raise the cache, per-pin morph slots, prefetch
order) and measured again; it is not explained away.

4.6 Claims. Release notes and PR text state only what has a verification
entry; anything not run is listed as "Not run".

## 5. Review procedure (Heavy)

Severity: P0 catastrophic (crash, data loss, lost preferences); P1
merge-blocking correctness defect, regression or unmet requirement; P2
concrete non-trivial defect, edge-case failure or missing test that would
let a real defect through. Style, naming, optional refactoring and
unproven theory are not findings.

Order, strictly one at a time (never two reviewers, and never a reviewer
beside a build, Codex or a hardware test run):
1. One self-review of the diff against §3 by the implementing session
   (correctness, missed requirements, regressions, missing tests). Fix.
2. §4.1 green, and an agent-driven smoke run on the booted system.
3. Opus review: `Agent({ subagent_type: "general-purpose", model: "opus",
   prompt: <below> })`, fresh context, adversarial, P0/P1/P2 only with
   file:line and a concrete failure scenario, report capped at about 300
   words plus tables. Fix, commit, run a new fresh Opus round; stop after
   one clean round.
4. Codex: `codex exec review --base main
   --dangerously-bypass-approvals-and-sandbox`. Fix valid findings, rerun
   only if code changed; if tokens are unavailable, skip and say so in the
   handover.
5. §4.4 and §4.5 with the reviewed build. If hardware forces a code change,
   commit it, rerun §4.1 and run one more Opus round on that delta.
6. One Fable final pass: `Agent({ subagent_type: "general-purpose", model:
   "fable", prompt: <below> })`. A blocker is fixed and followed by one new
   Fable pass.
Never `subagent_type: "fork"` and never a model alias as `subagent_type`.

Reviewers re-derive only these claims:
- R1 every former `stack_index` site behaves as before for docks without
  stacks, and stacks reorder/remove like pins (D5, D6);
- R2 the open stack's identity (D7, D8) across reorder, removal, surface
  rebuild and a second stack; the `close_*` rectangle always belongs to the
  open pin;
- R3 migration and saving (D2): no path loses a pin, `"folder"` is never
  written, a failed load of the migrated path cannot overwrite the file;
- R4 prefetch and cache bounds (D13, D14): slot resets, no eviction by the
  prefetch, everything freed in `dock_ui_destroy`;
- R5 the goldens were filtered per §4.2.

Opus prompt: "Review the diff `git diff main...folder-stacks` in
/Users/reinedelwall/GitRepos/DockAwesome against
tasks/plan-folder-stacks.md §3. Re-derive R1-R5 of §5 from the code. Report
only P0/P1/P2 findings (definitions in §5) with file:line and a concrete
failure scenario; no style or refactoring. You may run `make host-test`.
At most 300 words plus tables."

Fable prompt, in this order: (a) "Requirements: tasks/plan-folder-stacks.md
§3 (the user's five decisions, the performance requirement and D1-D17) and
§4"; (b) the full output of `git diff main...folder-stacks` (as a
file path if too large to paste, with the changed-file list); (c) every
Opus and Codex finding with its resolution; then only: "Is this
implementation complete against the original requirements, and is there
anything blocking merge?"

## 6. Delivery procedure

- The branch exists: `git fetch`; confirm `main` has no unpushed commits
  and equals `origin/main` (fast-forward it if behind, then rebase
  `folder-stacks` only if main moved); work on `folder-stacks`. Never
  commit on main.
- Commit after each review round and each hardware batch; push nothing
  until §5 is clean and §4 is recorded.
- Then push once, open the PR as a draft (`gh pr create --draft --base
  main`), check `gh run list --branch folder-stacks` (no workflows exist;
  expect none), and only then `gh pr ready <n>` - not in the same command
  as the push. If required hardware evidence (§4.4 on at least the booted
  system, §4.5 on AmiKit and Coffin) could not be collected because the
  user or a boot was unavailable, leave the PR a draft with a "Blocked on
  hardware" checklist and say so in the handover.
- Push and PR creation are pre-authorized by this workflow.
- Merging is not authorized - never merge. The user merges.

## 7. Standing constraints

- Repo text is English; replies to the user are Swedish, plain words.
- A local build is "build N", never a version.
- `tasks/lessons.md` and the memory files under
  `/Users/reinedelwall/.claude/projects/-Users-reinedelwall-GitRepos-DockAwesome/memory/`
  bind all hardware work. The ones that bite here: transfers with `acp`
  (bounded wait); stop a WBStartup dock with the small FindTask + Ctrl-C
  tool; rename a locked `SYS:WBStartup/DockAwesome` to
  `Disabled/DockAwesome-locked-delete` before copying; no rollback copies
  or variants on the Amiga; clean RAM: after each batch; the dock writes
  only ENVARC: (ENV: is the boot copy); one install step per call; wait a
  few seconds after starting a test dock; screenshots do not show PiP;
  CPU-bound test programs at `ChangeTaskPri -30`; never `Avail FLUSH`;
  when an agent gesture fails, check `downs`/`ambiguous` and rerun on the
  previous build before calling it a regression; every test that stops the
  dock ends with the dock running again.
- One code path for every system: no per-OS branches for this feature.
- Never remove an existing UI element beyond the two approved removals.
- Simplicity first; touch only what the decisions need.

## 8. Assumptions (owner may veto asynchronously)

- A1 The Trashcan (`WBGARBAGE`, a directory) becomes a stack like any
  drawer; only paths ending in `':'` are exempt.
- A2 A click on another stack while one is open closes it and then opens
  the clicked one (as the macOS Dock does).
- A3 "Open in Workbench" keeps its wording under Directory Opus (as the
  tile has in 1.0 beta 5); it opens a lister there.
- A4 The stack menu's second row replaces Close; a window opened from a
  stack is closed in the desktop, not from the dock.
- A5 A migrated Folder Stack whose drawer is missing at start is treated
  like any missing pin (message, saving blocked until it is back); before,
  it was skipped silently.
- A6 The cache holds 512 icons (about 4.7 MB of pixels at Size 48, plus
  0.3 MB of entries); the prefetch stops when it is full rather than evict.
- A7 The prefetch is armed when a stack is pinned and after Settings
  change the surface, not after each closing.
- A8 Going back to 1.0 beta 5 after the migration shows the former stack
  as a plain drawer pin; its Folder Stack setting is empty.
- A9 The new build stays installed on each verified boot, as with earlier
  branches, and migrates the user's real preferences there.

## 9. Handover contract

`tasks/handover-folder-stacks-pr1.md`, facts only, no diary, no repeated
plan text:
- implemented scope (by D number)
- deviations from this plan, each with evidence and rationale
- assumptions made during implementation
- verification commands and results (host, build, per-boot matrix, the
  performance tables old/new/old)
- Opus / Codex / Fable findings and resolutions; Codex skip reason if any
- hardware not run, and what is needed from the user for it
- installed build per boot, and leftovers to delete at the next boot
- delivery state: one push, PR opened as draft, then ready or left draft
  (the handover is committed before the push, so the PR URL itself goes in
  the PR and the session's final report)
- "FINAL PR - no further kickoff"
