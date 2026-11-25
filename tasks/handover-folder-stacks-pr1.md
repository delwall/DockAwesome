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

## Follow-up in the same PR: per-stack Sort by (2026-11-25)

- Scope: the stack menu gains a muted "Sort by" heading, Name / Date
  Modified / Kind with a check mark, a separator, then Show Contents /
  Open in Workbench (macOS Dock stack menu; no Date Added/Created, no
  Display as / View content as / Options).
- Code: `dock_sort.c/h` (host-tested ordering), `amiga_folder_read(path,
  sort,...)`, `DockPrefs.sorts` + `"sorts": {path: key}` (only non-name
  stacks written), `Item.sort`, `sort_menu_item` (save at once, revert on a
  failed save, re-arm that stack's read-ahead), prefetch slot keyed by
  sort, `FolderAnchor.sort` (drawers opened inside the box keep it),
  variable-height popup rows (`popup_top`, inert heading and separator).
- Decision: keyed by path, not by index, so a hand-edited or reordered
  `"paths"` cannot move a sort to another stack; 1.0 beta 5 skips the key
  (its own `dock_prefs.c` read a file written by this build on the host).
- The menu and the box share one window, so the box is always closed
  while the menu is up: a new sort shows at the next opening (no open box
  to refresh).
- Verification: tasks/todo.md, section "Folder Stack Sort by per
  stack": host tests, ApolloOS menu/order/persistence/reorder/removal/
  migration, first-open table per sort.
- Reviews: Opus one P2 (inert row kept an earlier press armed), fixed
  ad454f3 and checked on ApolloOS; Codex (base main) no findings.
- Installed: build 538 in ApolloOS SYS:WBStartup/DockAwesome, running.
  Delete SYS:WBStartup/Disabled/DockAwesome-locked-delete, -2 and -3
  after the next ApolloOS boot.
- Not run: the other boots (as above).

## Delivery

One push of folder-stacks; PR opened as a draft and left a draft with a
"Blocked on hardware" checklist. Not merged.

## Follow-up: Coffin matrix, performance and a release-hit fix (2026-11-25)

- Boot: Coffin r65 (Kickstart 45.57, Workbench 45.5, core 12856, 24-bit
  1280x720), agent input. Details and tables: tasks/todo.md, section
  "Coffin hardware matrix and performance".
- §4.4 steps 1-8, Sort by and A5 passed (step 1-2 with build 539, the rest
  with 540); all destructive runs used --config RAM: copies.
- Defect found and fixed (also in 1.0 beta 5): with a box open, a quick
  click on another pin only closed the box. The press started the box's
  closing motion, `input_publish` withdrew the handler's snapshot, and the
  release came with hit_id 0. The handler now flags such a release
  `INPUT_HIT_UNRESOLVED` and the main task hit-tests the row it drew
  (`src/amiga_input_handler.c`, `src/amiga_main.c`, `src/amiga_app.h`).
  Verified on Coffin with builds 540/541 (stack switch both ways, EditPad
  started); not rerun on ApolloOS.
- §4.5 Coffin, v1.0b5 / 540 / v1.0b5: first paint Downloads 101.4 / 85.1
  / 89.6 ms, entrance 135.9 / 134.5 / 130.3 ms (Games 86.4 / 133.2; Prefs,
  larger page, 141.1 / 154.0); prefetch evicted=0. Hover agent sweep
  render_us_avg 10400 / 9410 / 10525, stalled 3 / 3 / 4.
- Opus review of the fix: no P0/P1; P2s fixed in build 541 (message()
  cancels a held press; comment). Open P2 for the owner: a press during
  the ~130 ms box motion is not taken by the dock and reaches what lies
  under it (older behaviour, same root at the press).
- Installed on Coffin: build 541 in SYS:WBStartup/DockAwesome, running
  (WBRun); ENVARC:DockAwesome.json migrated by it. Delete
  SYS:WBStartup/Disabled/DockAwesome-locked-delete (0.527) after the next
  Coffin boot. RAM: clean.
- Still not run: AmigaOS 3.2.3, plain 3.9, 3.1.4.1, AmiKit XE (matrix,
  AmiKit first open and hover); ApolloOS with build 540.

## Follow-up: AmigaOS 3.2.3 matrix and performance (2026-11-25)

- Boot: AmigaOS 3.2.3 (Kickstart 47.115, Workbench 47.5, core 12856,
  24-bit 1280x720), agent input. Build 541 (HEAD e6f433b, no rebuild
  needed). Details and tables: tasks/todo.md, section "AmigaOS 3.2.3
  hardware matrix and performance".
- §4.4 steps 1-9, Sort by and A5 passed with --config RAM: copies; the
  quick-click fix (32e645f) passed: stack switch both ways with quick
  clicks and 1 s holds, TextEdit started from an open box.
- §4.5 first open v1.0b5 / 541 / v1.0b5 / 541 / v1.0b5: Downloads first
  paint 98.7 / 93.3 / 99.0 / 94.3 / 110.2 ms, entrance 134.3 / 145.4 /
  137.6 / 134.7 / 146.8 ms; Games 92.7-92.9, Prefs (smaller box) 72.8;
  every new value under the old maximum; prefetch evicted=0.
- Hover --bench-sweep 8 (ChangeTaskPri -30): render_us_avg 6799 / 6602 /
  6804 / 7707 / 7889, stalled 41-44; pass.
- Hover agent sweep FAILS on 3.2.3: render_us_avg 12049 / 14718 / 12087
  (new) against 9549-10076 (old), stalled 6-14 against 0-1. It comes from
  the rest at x 800 beside the mid-row Games and Prefs stacks; the same
  build without those two stacks is normal. The time is the PiP
  back-buffer retirement wait (WaitTOF in amiga_pip_back_buffer, keyed to
  GfxBase->VBCounter; unchanged since 1.0 beta 5), not CPU (task sampler,
  diagnostic counters). Trigger not found, not fixed: open for the owner.
  The control had one stack, last, as in the user's own file.
- Installed on 3.2.3: build 541 in SYS:WBStartup/DockAwesome, running
  (WBRun); the 1.0 beta 5 binary deleted; ENVARC:DockAwesome.json migrated
  by it; the tests only copied it. RAM: clean; scratch worktrees removed.
- Still not run: plain 3.9, 3.1.4.1, AmiKit XE; ApolloOS with build 540+.

## Follow-up: hover pacing on AmigaOS 3.2.3 (2026-11-25)

- Root cause of the failed 3.2.3 hover sweep: a pacing cascade present
  since 1.0 beta 5, not the stacks. Once one frame's present lands after
  the next VBlank, each later frame of the motion starts on the already
  pending field signal and blocks 9-30 ms in the PiP back-buffer WaitTOF
  until the row rests. The new evenly spaced row puts larger icons under
  the x 800 / 950 rests (compose 10-12.5 ms), which triggers it; v1.0b5
  with the same 15-pin row fails the same way (13508 / 13561 / 10679 us,
  stalled 10 / 10 / 4). Details: tasks/todo.md, "Hover pacing on AmigaOS
  3.2.3".
- Fix (build 542): `amiga_pip_back_busy()`; such a frame is drawn at the
  next field instead (once; the WaitTOF stays as the safeguard). Profile
  `Frame pacing:` adds `retire_waits` and `retire_defers`.
- 3.2.3 results: hover sweep 542 9695 / 9697 / 9711 us, stalled 2 / 3 / 2,
  retire_waits 0 (541: 14096, stalled 9; v1.0b5 po: 10931, stalled 3);
  first open 95.8 / 95.5 ms, entrance 139.5 / 138.6 ms (541 95.4 / 144.5,
  v1.0b5 98.1 / 138.8); --bench-sweep 8 6836 / 6582 / 6826, stalled 41 /
  41 / 42. host-test, host-sanitize green, make amiga no warnings; Opus
  review no P0/P1/P2.
- Installed on 3.2.3: build 542 in SYS:WBStartup/DockAwesome, running
  (WBRun); 541 deleted; RAM: clean; ENVARC:DockAwesome.json not written by
  the tests.
- Still not run: Coffin and ApolloOS with 542; plain 3.9, 3.1.4.1, AmiKit
  XE.


## Follow-up: AmiKit "dos device not mounted" (2026-11-25)

- Report (user, gesture batch on AmiKit XE with 542): Personal:Audio
  dropped and clicked gave a window "dos device not mounted".
- Personal:Audio is an ordinary directory on the mounted Personal: (DH12)
  with a WBDRAWER PNG icon; it opened as a stack (profile stacks=6).
- The window was a Directory Opus 5 lister `DOS error: device (or volume)
  is not mounted` on `"Personal:Audio"/`: the dock sent `lister new
  "<path>"`, and DOpus 5 keeps the quotes as part of the path. Reproduced
  with ARexx alone; unquoted paths (with a space, with a quote) open.
  Present since 1.0 beta 2 (050f9c8), the same in v1.0b5; not a DOS
  requester and not the prefetch. This PR makes it more visible: Open in
  Workbench of every stack goes through it.
- Fix (build 544): `dock_dopus_lister_new` sends `lister new <path>`
  unquoted (host test in tests/test_text.c). Not run end to end through
  the dock (agent clicks do not reach it on AmiKit); the user checks Open
  in Workbench during the batch.
- AmiKit state: build 544 in SYS:WBStartup/DockAwesome; the test dock runs
  from it with the batch's arguments (--config RAM:ak.json --profile
  >RAM:ak.prof); the profile before the restart is RAM:T/ak-batch1.prof.
  The batch and its cleanup are still open (tasks/todo.md).
