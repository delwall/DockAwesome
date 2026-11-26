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

## Follow-up: AmiKit XE, agent-driven (2026-11-25)

- Agent input works on AmiKit: the agent's warp lands at screen = 2*w-4,
  so earlier clicks pressed beside the dock. Clicks, drags and menus at
  ((X+4)/2, (Y+4)/2) reach the dock (input.device spy: the dock captures
  the agent's button events). tasks/lessons.md, "AmiKit: agent clicks land
  at twice the coordinates".
- Build 544 (HEAD 4ed4e32), --config RAM: copies, agent: Open in Workbench
  on Personal:Audio opens a DOpus lister without the DOS error (fix
  4ed4e32 verified); Archives: pin opens a lister; stack switch, program
  pin with a box open, Sort by rows and check mark, reorder, drag out,
  Settings Save all pass (tasks/todo.md, "AmiKit XE: agent gestures").
  Not run: the box's Open in Workbench tile, the no-dot check (PiP only).
- §4.5 first open, Pictures 584x527, v1.0b5 / 544 alternating, 4 runs
  each: first paint new 238.7-285.8 ms vs old 239.2-363.5 (pass);
  entrance new 132.5-174.2 vs old 134.3-159.4: FAILS the "not above the
  old maximum" rule in 2 of 4 runs (4-frame entrances with a late last
  frame, as in old o1). Not fixed; open for the owner.
- Pre-existing finding (also v1.0b5): the first click after the dock idles
  can be lost (4 of 12 fresh-start runs); a diagnostic build showed the
  hit snapshot withdrawn at the press. Open for the owner.
- Compose finding closed: same binary gives 3.8 ms or 5.3 ms per run with
  code and canvas at the same addresses; clean runs 544 3820/3837 vs
  v1.0b5 3778-3793 us. Run-level interference, not a code regression.
- End state AmiKit: installed 544 running (Run from a Stack 400000 script),
  the only dock; ENVARC:DockAwesome.json migrated by it; RAM: clean;
  scratch worktrees removed. No code change in this follow-up.

## Follow-up: ApolloOS rerun with build 550 (2026-11-25)

- Boot: ApolloOS R9.5 (Kickstart 51.3, core 12856, 16-bit 1280x720),
  freshly rebooted, agent input. Build 550 = HEAD 2f42fad rebuilt clean
  (build/ held a 549 from the AmiKit pad runs); no warnings; no code
  change. Details: tasks/todo.md, "ApolloOS rerun with build 550".
- The old Disabled/DockAwesome-locked-delete, -2, -3 were not locked and
  are deleted. 550 installed in place; the running 538's file stayed in
  use, so it is now Disabled/DockAwesome-locked-delete (delete after the
  next ApolloOS boot).
- §4.4 checks the fixes touch, all passed: --caps identical to v1.0b5 and
  538; migration and a second start; no divider (41 px pitch); stack
  switch both ways with quick clicks and 1 s holds; Editor started by a
  quick click with a box open (2 of 2, fix 32e645f now verified on
  ApolloOS); Archives: pin opens a window, Close closes it; stack menu row
  and box tile open Workbench windows, no Close; Sort by rows, saving,
  heading, persistence across a restart, Name removes "sorts"; A5.
- §4.5 first open, v1.0b5 / 550 alternating (o1 n1 o2 n2 o3): Downloads
  first paint 107.6 / 104.6 / 107.8 / 105.1 / 108.1 ms, entrance 130.9 /
  133.9 / 131.7 / 141.2 / 149.8 ms; Games 107.7 / 107.4 and 146.1 /
  145.7; Prefs (smaller box) 70.3 / 149.4. All new values under the old
  maximum (108.1 / 149.8); prefetch evicted=0. A fifth entrance frame
  (131 vs 141-150 ms) happens in both builds.
- §4.5 hover agent sweep, same 15-pin row, ChangeTaskPri -30, old/new
  alternating: render_us_avg old 10211 / 9185 / 9118 / 9077, new 9174 /
  9236 / 9185; stalled old 0 / 0 / 0 / 0, new 0 / 1 / 0. The one new
  stall is a 542 pacing defer after a late rest frame (retire_defers=1),
  which the old path would spend in WaitTOF instead; noted for the owner,
  not a code defect.
- End state ApolloOS: 550 running from SYS:WBStartup (WBRun, the user's
  ENVARC config, not written by the tests), the only dock; RAM: clean;
  scratch worktree removed. C:ApolloExplorerSrv runs until reboot.
- Not run on ApolloOS this time: drops, reorder/drag-out, Settings,
  --bench-sweep (unchanged paths; run with 533/538).

## Follow-up: "Click to" Show Stack / Open Folder per stack (2026-11-25)

- Owner request: the stack menu's action rows Show Contents / Open in
  Workbench become options under a muted "Click to" heading (after the
  Sort by group and the hairline): Show Stack (default, the box) or Open
  Folder (a left click opens the drawer as a Workbench window, a DOpus
  lister when DOpus is the desktop), as macOS "Display as Stack / Folder".
  The box's Open in Workbench tile stays; no Close row is added back.
- Code: `DockClick` and `"clicks": {path: "folder"}` in dock_prefs (the
  same path-map reader/writer now serves "sorts" and "clicks"; nothing
  written for Show Stack; old files read as Show Stack), `Item.click`,
  `stack_box()`, `click_menu_item` (save at once, revert on a failed save,
  re-arm the read-ahead on Show Stack), `DOCK_UI_CLICK_STACK/_FOLDER`
  replace `DOCK_UI_SHOW_CONTENTS/_OPEN_DRAWER`, popup check marks as a row
  mask.
- Open Folder click path: `launch_pin(path,1)` + `status_changed`, the
  launch code of 1.0 beta 5 drawer pins and of the old menu row (through
  `launch_object` and the unquoted DOpus `lister new` from 4ed4e32 when
  Workbench is not the desktop). No `record_launch`: a stack has no
  running state, so no app may claim it.
- Plan decision 3 stands: an Open Folder stack has no running dot and no
  Close row; it still goes to the status worker as kind 0 like every stack.
- Open Folder stacks are not read ahead (prefetch and `stacks=` in the
  profile count only Show Stack stacks); Show Stack first opens unchanged.
- Verification: tasks/todo.md, "Folder Stack "Click to"": host tests
  (test_clicks), v1.0b5 and build 550 read a file with "clicks", ApolloOS
  menu / window / box / persistence / restart / reorder / drag-out /
  migration, first open and hover.
- Opus review: no P0/P1/P2.
- Installed: build 551 in ApolloOS SYS:WBStartup/DockAwesome, running
  (WBRun, the user's ENVARC config, not written by the tests). Delete
  SYS:WBStartup/Disabled/DockAwesome-locked-delete (538) and -2 (550)
  after the next ApolloOS boot. RAM: clean.
- Not run: the other boots; Open Folder as a DOpus lister on AmiKit (same
  launch_pin path the old menu row used there); the no-dot look (PiP).

## Follow-up: Coffin rerun with build 551 (2026-11-25)

- Boot: Coffin r65 (Kickstart 45.57, Workbench 45.5), agent input. Build
  551 = HEAD 5cc7184, not rebuilt (no source newer than the binary); no
  code change. Details: tasks/todo.md, "Coffin rerun with build 551".
- §4.4 for what changed since 541, all passed with --config RAM: copies:
  --caps identical to 541 and v1.0b5; migration and a second start; A5;
  no divider (37 px pitch); stack switch both ways with quick clicks and
  1 s holds; EditPad started by a quick click with a box open; Archives:
  pin opens a Workbench window, Close closes it; Sort by rows, check and
  persistence; Click to: Open Folder opens a Workbench window and no box,
  no Close row, no running dot (--pip-dump: a dot under the Archives:
  volume only, none under the two stacks with their windows open), Show
  Stack brings the box back, both survive a restart, reorder to slot 0
  and drag-out keep each stack's "clicks"/"sorts"; Settings without the
  Folder Stack section, Save keeps "clicks".
- §4.5 hover, same 20-pin row, ChangeTaskPri -30: render_us_avg v1.0b5 /
  551 / v1.0b5 13966 / 9284 / 10526, stalled 11 / 4 / 6: pass. The old
  runs show the 3.2.3 pacing cascade at the x 950 rest; 551 does not
  (retire_waits=0, retire_defers=2). First check of 98bbf1a on Coffin.
- §4.5 first open, v1.0b5 x4 / 551 x4 (+ 541 and 542 controls): old
  max 90.0 / 150.3 ms. 551 n3/n4 entrance 144.4 / 143.8 (Games 131.3 /
  130.3) pass, first paint 90.2 / 90.8 (0.2-0.8 ms over); n1/n2 first
  paint 85-86 pass, but n2 entrance 168.6 (one 60 ms last frame) and n1/n2
  Games 153.8 / 153.4 are over. Not reproduced in n3/n4 or the 541/542
  controls (130-137 ms); the open and entrance code is the same in 542
  and 551. Not fixed; open for the owner.
- Installed on Coffin: 551 in SYS:WBStartup/DockAwesome, running (WBRun,
  the user's ENVARC config, not written by the tests), the only dock.
  Delete SYS:WBStartup/Disabled/DockAwesome-locked-delete (541, in use)
  after the next Coffin boot; the older 0.527 one is deleted. RAM: clean;
  scratch worktrees removed.
- Not run on Coffin: drops (pins from config), the box's Open in
  Workbench tile, --bench-sweep.

## Follow-up: plain AmigaOS 3.9 with build 551 (2026-11-25)

- Boot: AmigaOS 3.9 (Kickstart 40.68 ROM, Workbench 45.5; graphics 40,
  intuition 40, icon 51.4, workbench 45.132, so the dock's V39/V44
  requirements hold), core 12856, 24-bit 1280x720, no shaped windows, agent
  input. Before: release 1.0 beta 4 installed and running. amiagent did not
  start at boot; the user started it by hand. Build 551 = HEAD 1fb8556, not
  rebuilt; no code change. Details: tasks/todo.md, "AmigaOS 3.9 matrix and
  performance with build 551".
- §4.4 all passed with --config RAM: copies: --caps identical to v1.0b5
  and 1.0 beta 4; migration and a second start; A5; no divider (41 px
  pitch, last two 5 px apart); drawer drop -> stack, volume drop ->
  window; stack switch both ways with quick clicks and 1 s holds; Editor
  started by a quick click with a box open; Archives: window and its
  Close; Sort by rows, heading, check and persistence; the box's Open in
  Workbench tile; Click to Open Folder (window, no box, no Close row, no
  running dot by --pip-dump), Show Stack back, both after a restart;
  reorder and drag-out keep each stack's "clicks"/"sorts"; Settings
  without the Folder Stack section, Save keeps "clicks".
- §4.5 first open, v1.0b5 / 551 alternating (o1 n1 o2 n2 o3 n3):
  Downloads first paint 114.4 / 97.4 / 110.4 / 97.8 / 100.6 / 97.7 ms,
  entrance 145.6 / 137.6 / 144.7 / 137.2 / 134.5 / 137.3 ms; Games
  96.4-96.9 / 138.1-138.8; Prefs (smaller box) 87.6-88.2 ms, entrance
  131.5 / 130.9 and once 152.1 (n1: a one-frame entrance, 6.5 ms over the
  old maximum 145.6; not repeated in n2/n3). Open for the owner, like the
  Coffin n2 outlier. Prefetch stacks=3 pending=0 evicted=0.
- §4.5 hover agent sweep (15 pins, -30): render_us_avg v1.0b5 / 551 /
  v1.0b5 9460 / 9540 / 9561, stalled 0 / 0 / 1: pass.
- Installed on 3.9: 551 in SYS:WBStartup/DockAwesome, running (WBRun, the
  user's ENVARC config migrated by it, not written by the tests), the only
  dock. Delete SYS:WBStartup/Disabled/DockAwesome-locked-delete (1.0 beta
  4, in use) after the next 3.9 boot; the older one (509) is deleted. RAM:
  clean; scratch worktree removed. C:ApolloExplorerSrv runs until reboot.
- Not run on 3.9: --bench-sweep; DOpus lister (Workbench is the desktop).
- 3.1.4.1 (2026-11-25): booted; Kickstart 46.143, Workbench 45.194,
  graphics 45.27, intuition 40.87, icon 51.4, workbench 45.194 (the dock's
  requirements hold), core 12856, 24-bit 1280x720. --caps of 551, v1.0b5
  and the installed 1.0 beta 4 identical. Then a defect, also in v1.0b5: a
  Break within a second of a test dock's start hit libnix's __chkabort in
  a write (save_config's migration save), which exits past main's cleanup;
  the input handler, the PiP2 lease and the receiver window leaked and
  Intuition took no more input. Fixed by an empty `__chkabort` in
  amiga_main.c (552); after the Opus review (no blocking finding) 553 also
  clears a Break that --bench-sweep or an early exit never took. Proven by
  running the dock in-process with Ctrl-C pending (CtrlCRun): v1.0b5 and
  551 print only "***Break" (rc 20) for --caps, 552 and 553 the full
  output (rc 0; 553 leaves no Ctrl-C pending).
- 3.1.4.1 after the user's reboot, build 553 (agent input lands 1:1):
  553 started from WBStartup and migrated the user's ENVARC file at boot
  (tests never wrote it); the 1.0 beta 4 leftover is deleted. Break
  checks: Ctrl-C pending through a whole start, a Break right after a
  start and Breaks during --bench-sweep all end with the full profile and
  no PiP2 lease left; the pointer keeps moving. §4.4 all passed (--caps,
  migration and a second start, A5, no divider 41 px pitch with the last
  two 5 px apart, stack switch both ways by quick click and 1 s hold,
  Editor started from a box, Archives: window and Close, Sort by, Click to
  Open Folder and back, no running dot under stacks, persistence after a
  restart, box tile, reorder, drag-out, drawer and volume drops,
  Settings). §4.5: every first paint under the old maximum (101.4 ms);
  entrance not a clean pass (Downloads up to 0.6 ms over 135.9, Games
  once 143.3 and once 153.5, Prefs 142.5-146.4), open for the owner like
  the Coffin and 3.9 outliers; hover 9529 / 9610 / 10719 us (pass);
  --bench-sweep 8 6753 / 6440 / 6736 us, stalled 41 / 41 / 43 (pass).
  Installed: 553, running (WBRun), the only dock; RAM: clean; scratch
  worktree removed. Not run: DOpus lister (Workbench is the desktop).
  Details: tasks/todo.md, "AmigaOS 3.1.4.1 with builds 551 and 553".

## Follow-up: AmigaOS 3.2.3 rerun with build 553 (2026-11-25)

- Boot: AmigaOS 3.2.3 (Kickstart 47.115, Workbench 47.5), core 12856,
  24-bit 1280x720, agent input. Build 553 = HEAD 24573c0, not stale, no
  code change. Details and tables: tasks/todo.md, "AmigaOS 3.2.3 rerun
  with build 553".
- Break checks with 553: Ctrl-C pending through a whole start, a Break
  right after a start, Breaks 3 s and 6 s into --bench-sweep 8: full
  profile every time, no PiP2 lease left, no leaked window, the pointer
  moves.
- §4.4 all passed with --config RAM: copies: --caps identical to v1.0b5
  and 542; migration and a second start; A5; no divider (41 px pitch);
  stack switch both ways by quick click and 1 s hold; TextEdit started by
  a quick click with a box open; Archives: window and Close; Sort by
  (heading, Date Modified saved and kept); Click to Open Folder (window,
  no box, no Close row, no dot under stacks by --pip-dump), Show Stack
  back, both after a restart; box tile; reorder and drag-out keep
  "sorts"/"clicks"; drawer and volume drops; Settings Save.
- §4.5 first open, v1.0b5 / 553 alternating, 8 runs each with per-frame
  data: entrance v1.0b5 130.0-201.1 ms (median 148.6), 553 143.6-147.0
  (median 144.4); first paint 96.4-112.3 against 92.8-95.9 and once 116.2
  (a slow screen capture, bimodal in both builds). Verdict on the open
  entrance finding: noise. The motion code is byte-identical to v1.0b5;
  the length is 4 or 5 frames in both builds; the session's first three
  553 runs had one 43-47 ms frame (the 3/4-fade frame) that the same
  binary did not have with a 32 KB memory shift (19.7 ms) or later in the
  session (28.6 ms); diagnostic builds of both versions: 25-27.5 ms. The
  memory-state cause of the 43 ms frame was not found.
- §4.5 hover (same 15-pin row, -30): v1.0b5 / 553 / v1.0b5 13374 / 10378
  / 13868 us, stalled 9 / 4 / 8: pass.
- §4.5 --bench-sweep 8: stalled 41-42 both, but render_us_avg 553 7432-
  8018 against v1.0b5 6481-7035 (compose +0.9 ms, frame for frame on the
  same damage). Same compose code; the cost follows the binary's layout
  (v1.0b5 + an unrelated diagnostic 5154 us compose, 553 variants 4943-
  6597 us; alignment flags do not pin it; on 3.1.4.1 553 was faster than
  v1.0b5). Open for the owner, not fixed.
- Installed on 3.2.3: 553 in SYS:WBStartup/DockAwesome, running (WBRun),
  the only dock; 542 deleted (not locked); ENVARC:DockAwesome.json not
  written by the tests. RAM: clean; scratch worktrees removed.
- Not run on 3.2.3: DOpus lister (Workbench is the desktop).

## Follow-up: AmiKit XE rerun with build 553 (2026-11-25)

- Boot: AmiKit XE 11.5 (KS 47.96, Directory Opus 5 desktop), core 12856,
  24-bit 1280x720, agent input at ((X+4)/2, (Y+4)/2). Build 553 = HEAD
  f47f5c5, not stale, no warnings; no code change. Details and tables:
  tasks/todo.md, "AmiKit XE rerun with build 553".
- Installed in place over 544 (not locked). --caps identical to v1.0b5 and
  544.
- Break checks: Ctrl-C pending through a whole start, a Break right after a
  start, Breaks 3 s and 6 s into --bench-sweep 8: full profile each time,
  no PiP2 lease left, no leaked window, the pointer moves.
- §4.4 all passed with --config RAM: copies: migration and a second start;
  A5; stack switch both ways by quick click and 1 s hold; Shell and Clock
  started by a quick click with a box open; Archives: opens a DOpus
  lister; Sort by (heading, Date Modified, Kind, Name removes "sorts",
  persistence); Click to Open Folder opens a DOpus lister on the drawer
  with no box and no DOS error, also for Personal:Audio (the old "dos
  device not mounted" path); box open + click on an Open Folder stack;
  restart; Show Stack back; the box's Open in Workbench tile (DOpus
  lister); no running dot under stacks (--pip-dump: one dot, under
  Archives:); drawer and volume drops; reorder and drag-out keep
  "clicks"; Settings Save keeps "sorts". This closes the last open AmiKit
  items (box tile, no-dot, Click to).
- Agent artifact noted: a click warped from a zoomed row straight to a far
  pin can press where the drawn row has no icon; a move to the pin first
  (as a mouse does) always works. Not a dock defect.
- §4.5 first open, 8 runs each, alternating: Pictures entrance v1.0b5
  131.7-174.3 (median 169.4) against 553 133.8-141.1 (median 134.3);
  first paint 264.0-356.1 against 245.3-285.0 and once 383.2 (a slow
  screen capture, one stage at 159 ms; v1.0b5 has single-stage spikes of
  the same size). Downloads first paint 131.0-148.8 and once 262.1 (old)
  against 121.5-134.3 (new); entrance 137.3-161.3 against 137.4-152.1 and
  once 246.5 (one present waited 7 fields). Judged noise; the earlier 544
  entrance failure does not repeat. Prefetch evicted=0; every first click
  after idle opened its box (24 of 24).
- §4.5 hover (same 10-pin row, -30): v1.0b5 12506 / 10502 / 7578 us,
  stalled 2 / 3 / 3; 553 6960 / 7005, stalled 4 / 1: pass.
  --bench-sweep 8: v1.0b5 6221 / 6214 / 7039, 553 5865 / 5873, stalled
  38-40 both: pass.
- Seen, cause not found: the three DOpus listers open at the start were
  closed by the end of the tests; reopened by ARexx at the same places.
- End state AmiKit: 553 running (Run from a Stack 400000 script), the only
  dock, the user's ENVARC config untouched by the tests (424 bytes,
  17:49:26); RAM: clean; scratch worktree removed.
- All six systems now have their hardware runs. Open for the owner (none
  blocks a merge decision by itself): single-run first-open outliers on
  Coffin, 3.9, 3.1.4.1 and AmiKit (3.2.3 analysis: noise); the 3.2.3
  --bench-sweep compose gap that follows the binary's layout; the P2 about
  a press during the box motion; the lost first press after idle seen with
  544 on AmiKit (0 of 24 this time).

## Follow-up in the same PR: Open Folder tile (2026-11-25)

- Scope (owner's decision): the box's last tile reads **Open Folder** on
  one line, its ring is about 72% of the icon size at the same centre with
  the old ring weight, and ring and label use the muted heading colour.
  The action, cell widths and every other cell are unchanged; the wrap row
  is gone, so at small sizes the box is one text line shorter.
- `--pip-dump` also writes `prefix-folder.ppm` for each settled box page.
- Verified on AmiKit XE with build 556: before/after dumps at sizes 36, 48
  and 72, the tile opens a DOpus lister, first open and hover within the
  553 spread (tasks/todo.md, "Open Folder tile in the Folder Stack box").
- Installed: 556 on AmiKit XE; the other boots still run 553.

## Follow-up: Open Folder tile on AmigaOS 3.2.3 (2026-11-25)

- Build 556 (HEAD 3ea4b09, not stale, no warnings) installed in place on
  3.2.3 (553 deleted, not locked), started with WBRun, the only dock.
- Box dump (Downloads, size 36): smaller muted ring, one-line "Open
  Folder". The tile opens a Workbench window; quick stack switch and a
  program pin with a box open still work.
- First open (5 runs): first paint 90.1-103.6 ms and once 181.9 (one
  backdrop-copy stage at 94.5 ms), entrance 136.0-149.6 ms; 553 had
  92.8-95.9 / 143.6-147.0. Hover sweep 9626 us, stalled 2 (553 10378, 4).
- Owner's "window behind the dock": the position is the drawer icon's own
  snapshot (1034,599 169x96 in Personal:Downloads.info); v1.0b5 opens it at
  the same place; --pip-dump shows the row ends at x 965 even zoomed, so
  only the open Downloads box overlaps the window's corner. No code change;
  decision left to the owner. Details: tasks/todo.md, "Open Folder tile on
  AmigaOS 3.2.3 with build 556".
- Installed now: 556 on AmigaOS 3.2.3 and AmiKit XE; 553 on 3.1.4.1; 551
  on ApolloOS, Coffin, 3.9.

## Follow-up: centred box buttons and the parent-drawer arrow (AmigaOS 3.2.3, 2026-11-26)

- Owner's report: the back button's "<" was off centre in its circle.
  Measured on 556 (`--pip-dump`, sizes 36/41/48/72, control 19 px at every
  size): every chevron's pixel box sat 0.50 px toward its open side (ink
  centroid 0.70-0.73 px), vertical 0.00. Cause: the end points were shifted
  radius/16.
- Fix: button geometry in dock_geometry.c (`dock_button_glyph`,
  `dock_button_coverage`, `dock_button_pixels`, host-tested in
  test_geometry): glyph ink box centred on the disc; disc and glyph 5/6 of
  the control (owner: "a bit smaller"), stroke scaled with it; hit
  rectangles, layout and actions unchanged. After (563/564): 0.00 px both
  axes for the page buttons and the header button.
- The header button (to the parent drawer) now shows a stemmed up arrow
  (candidate A). An up chevron and a "Back"/"BACK" pill were tried and
  rejected by the owner; the final glyph is the owner's choice from the
  contact sheet (scratchpad `design/parent-candidates-sheet.png`, A-E).
- First open and hover with 559 within the 556 spread (tasks/todo.md,
  "Folder Stack box buttons").
- Installed then: 564 on AmigaOS 3.2.3 (WBRun, the only dock); 556 on AmiKit
  XE; 553 on 3.1.4.1; 551 on ApolloOS, Coffin, 3.9.
- Owner chose candidate C: the parent-drawer button shows a return arrow
  (up, then left), one pixel lines on a one pixel grid at the 19 px
  control, placed one pixel below the disc centre so it reads centred (box
  +1.00 px, ink centroid 0.15 px down / 0.29 px right). Installed: 567 on
  AmigaOS 3.2.3 (WBRun, the only dock).
- Owner's correction (2026-11-26): every control glyph centred exactly by
  its box (the arrow is no longer one pixel low; the Open Folder ring sat
  half a pixel off its icon cell and is now exact) and one shared control
  tone FOLDER_GLYPH #dee2e9 (15% less contrast than the old #f2f4f8; the
  tile was #aeb8c7). File names keep #f2f4f8. Installed: 569 on AmigaOS
  3.2.3 (WBRun, the only dock).

## Follow-up: AmigaOS 3.1.4.1 with build 569 (2026-11-26)

- Build 569 (HEAD d175ad7, not stale, no warnings) installed in place on
  3.1.4.1 (553 was locked by WBRun and waits in
  SYS:WBStartup/Disabled/DockAwesome-locked-delete until reboot), started
  with WBRun, the only dock.
- Box dumps at 36/48/72 (nested drawer, page 2): parent arrow and page
  buttons centred 0.00 px by their box, the Open Folder ring on its icon
  box (662.00, 522.00 at 36), every control #dee2e9, file names #f2f4f8:
  the same pixels as on 3.2.3. One 48 px ">" lies over the backdrop logo
  and does not measure.
- Tile, parent arrow, page buttons, quick switch, program pin with a box
  open, Sort by and Click to all work. First open (4 runs): 94.0-94.3 /
  139.1-140.8 ms (553: 98.9-99.5 / 135.6-136.5, same field pattern);
  hover sweep within the old builds' spread.
- No defect; no code change. RAM:BtnTest stayed locked by an unknown
  holder (not reproduced with the dock); two stuck runs of a diagnostic
  tool and RAM:cd.txt stay until reboot. Details: tasks/todo.md,
  "AmigaOS 3.1.4.1 with build 569".
- Installed now: 569 on AmigaOS 3.2.3 and 3.1.4.1; 556 on AmiKit XE; 551
  on ApolloOS, Coffin, 3.9.

## Follow-up: AmiKit XE with build 569 (2026-11-26)

- Build 569 (08de37b; HEAD 5fead28 changed only docs; not stale, no
  warnings) installed in place of 556 on AmiKit XE, started from a
  `Run >NIL: Stack 400000 +` script (WBRun does not work under DOpus),
  the only dock, stack 400000, one PiP2 lease.
- Box dumps at 36/48/72 (nested drawer, page 2, over an even CON window so
  the desktop icons do not show through the glass): parent arrow and page
  buttons 0.00 px by their box (one -0.05 reading = glass gradient), the
  Open Folder ring centred on its icon box (662.00, 522.00 at 36); every
  #dee2e9 pixel at exactly the 3.2.3 positions (0 differ at all three
  sizes); file names #f2f4f8.
- Open Folder cell opens a DOpus lister with no DOS error (RAM:BtnTest,
  Personal:Audio); parent arrow, page buttons, quick switch, a program pin
  with a box open and Click to work.
- First open (4 runs, 556 controls alternated): Pictures 284.4 / 168.2 ms
  medians (556 283.7-284.6 / 134.6-174.4), Downloads 126.5 / 153.9 (556
  121.3-121.4 / 137.6-137.8; 553 121.5-134.3 / 137.4-246.5). One 569
  Pictures entrance waited 8 fields (214.4). The layer step is 7-9 ms
  slower in code unchanged since 556 (placement, not code; not settled by
  a rebuild). Hover sweep 6957 us, stalled 2 (556 7283 / 2). No regression
  shown; no code change.
- Pre-existing finding seen again (also 556): the first agent click after
  idle is often lost when the pointer was parked on the DOpus desktop (the
  receiver rests on the plate there). Open for the owner as before.
- ENVARC:DockAwesome.json untouched (424 bytes, 17:49:26). RAM: clean. No
  DOpus listers were open at the start (rebooted machine), none left open.
- Installed now: 569 on AmigaOS 3.2.3, 3.1.4.1 and AmiKit XE; 551 on
  ApolloOS, Coffin, 3.9.
