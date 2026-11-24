# Handover: Folder Stacks for every pinned drawer, no divider, PR 1

FINAL PR - no further kickoff.

## Implemented scope

Plan tasks/plan-folder-stacks.md §3, D1-D17:
- D1 `DockSettings.end_gap`, `extra_gap`, `draw_divider`, `add_divider` and
  `STACK_DIVIDER_GAP` removed; `grep -rn end_gap src include tests` shows only
  `end_gap(app)` and its definition; `grep -rn stack_index src` is empty.
- D2 `dock_prefs_migrate` (`dock_prefs.h/.c`); `"folder"` read, never
  written; `read_config` sets `app->prefs_migrated`; startup saves once when
  migrated and no pin is missing.
- D3 `drawer_is_stack` in `pin_path` (directory, not a soft link, not a
  path ending in `:`), `ENV:Sys/def_drawer` fallback.
- D4 `Item.prefetch/prefetch_turns`, `App.open_stack_pin/stack_next_pin/
  prefs_migrated`, `ProfileStats.prefetch_reads`.
- D5 `stack_index`, `exchange_stack`, `prepare_stack` deleted; append at
  the end; `item_path_index` and `save_config` treat stacks as pins.
- D6 `DOCK_INPUT_FIXED`/`DockGesture.fixed` and the stack exclusions gone.
- D7 close rectangle, tip and morph follow `open_stack_pin`.
- D8 click on another stack: dismiss and open it once the box has gone;
  cleared by a new press, `open_menu`, `remove_pin`.
- D9 stacks go to the status worker with kind 0: no dot, no Close.
- D10 menu rows Show Contents / Open in Workbench (`DOCK_UI_OPEN_DRAWER`).
- D11 Settings without the Folder Stack section, ASL, blocking hook,
  `STRINGIDCMP`, keys Tab/b/c; height `4*row+3*MARGIN+2*GAP`.
- D12 close button built when the first stack is dropped.
- D13 `prefetch[DOCK_MAX_ITEMS]` slots, `folder_grid`/`folder_fit_count`
  split from `show_folder`, `stack_anchor`, round robin nearest first.
- D14 512-entry cache, FNV-1a key, no eviction by the prefetch,
  `dock_ui_folder_cache_stats`.
- D15 profile line `Folder Stack prefetch: stacks= pending= reads= cache=
  evicted=`.
- D16 host tests (goldens filtered per §4.2, `test_folder_migration`).
- D17 architecture.md, DESIGN.md, README.md, header comments, draft
  "1.0 beta 6" release notes, todo, lessons.

## Deviations

- D4/D15: `ProfileStats` also holds `cache_used`, `cache_capacity`,
  `cache_evicted`, taken right before `dock_ui_destroy`: the UI and its
  cache are freed before `profile_report` prints, so the report cannot ask
  the UI.
- D13: a prefetch slot also starts over when the anchor (x, y) differs, not
  only path, icon size or edge (Codex P2: a lower Scale moved the anchor and
  could enlarge the first page; a finished slot kept the old page).
- `remove_pin` dismisses the open box when its own stack is removed (self
  review; before, stacks could not be removed).

## Assumptions

Plan §8 A1-A9 as written. A5 was decided by the owner after the first
report: a migrated Folder Stack whose drawer is missing at start is dropped
silently (as in 1.0 beta 5) and the save removes it from the file; only a
pin the migration appended is treated so (App.migrated_pin).

## Verification

- Host: `make host-test`, `make host-sanitize` green;
  `DOCK_PRINT_GOLDENS=1 build/test_core` prints exactly the filtered table.
- Build: `make amiga` with no warnings; reviewed build is build 533.
- ApolloOS R9.5 matrix (§4.4): all nine steps passed (details and builds in
  tasks/todo.md, section "Folder Stacks for every pinned drawer").
- ApolloOS performance, old (main 989cfa7) / new (533) / old:

  | run | stack | first paint ms | entrance ms |
  | --- | --- | --- | --- |
  | old 1 | Downloads | 110.5 | 148.3 |
  | new | Downloads | 106.3 | 132.2 |
  | new | Games | 109.9 | 149.9 |
  | new | Prefs (smaller page) | 70.3 | 153.4 |
  | old 2 | Downloads | 110.2 | 174.7 |

  New prefetch: stacks=3 pending=0 reads=81 cache=81/512 evicted=0.
  `--bench-sweep 8` render_us_avg old/new/old 6800/6802/6793 and
  7091/6967/7106, stalled 0 in all runs.

## Reviews

- Opus round 1: no P0/P1; P2 = A5, later changed by the owner's decision (see Assumptions).
- Codex round 1: P2 prefetch slot and anchor, fixed in 3f1fe03. Round 2:
  no findings. Codex wrote its own section into tasks/todo.md (reverted)
  and built in /tmp/dockawesome-review-obj (left there; outside the repo).
- Fable: no code blocker; merge blocked only by the hardware evidence below.

## Hardware not run

- §4.4 matrix on Coffin, AmigaOS 3.2.3, 3.9, 3.1.4.1, AmiKit XE: needs a
  reboot into each; the agent can drive all steps except on AmiKit.
- §4.5 first open on AmiKit XE (old/new/old) and hover on Coffin and AmiKit
  XE: needs those boots; on AmiKit the user does the clicks and the sweep.

## Installed builds and leftovers

- ApolloOS: build 533 in SYS:WBStartup/DockAwesome, running; the user's
  ENVARC:DockAwesome.json is migrated (no "folder", Personal:Downloads last).
- Delete SYS:WBStartup/Disabled/DockAwesome-locked-delete (0.527, in use)
  after the next ApolloOS boot. C:ApolloExplorerSrv started this session
  stays until reboot. RAM: is clean.
- Other boots still run their earlier builds.

## Delivery

One push of folder-stacks; PR opened as a draft and left a draft with a
"Blocked on hardware" checklist. Not merged.
