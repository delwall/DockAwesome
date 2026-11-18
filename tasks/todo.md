# DockAwesome rename and cleanup

- [x] Rename the application, build and package references, and set the dialog title to About.
- [x] Use a 3 px desktop gap on every edge and preserve the right PiP transport.
- [x] Trim redundant comments and archive old artifacts outside the checkout.
- [x] Verify both builds, portable tests and the Vampire configuration.
- [x] Rename the checkout and initialize Git without creating a commit.

## Review

Host tests and sanitizers passed. Both Amiga HUNK builds and the distribution
package built successfully. The Vampire ran DockAwesome080 0.115. The
original JSON was moved to the new default path with identical content.
Menus opened on all three edges and the About window reported the exact
title. Exported PiP frames measured a 3 px gap on all three edges.
Git reports an empty main branch with no staged files. The ignore rules
exclude builds, the local revision counter and runtime preferences.

## Trim standalone tools

- [x] Archive the four optional diagnostics and remove their build targets and documentation references.
- [x] Build both variants and the retained AMMX/PiP probes, run host tests and verify Git remains empty.

Both Amiga variants and all four retained probes built in an isolated copy.
Host tests passed, and Make rejected each removed target. Git has no staged
files or commits.

## Whole-repository architecture and performance review

- [x] Capture a baseline and inventory the portable core, native integrations, and build tooling.
- [x] Review lifecycle, concurrency, input, persistence, rendering, and shared code paths.
- [x] Fix confirmed defects, consolidate compatible logic, and measure optimizations.
- [x] Compare behavior against the baseline with portable tests, both Amiga builds, and targeted Vampire checks.
- [x] Record verified changes, measurements, and open findings.
- [x] Complete an interactive PiP smoke test after the Vampire is restarted.

### Review results

- Saved rows that exceed the screen or cache budget now fail before any
  preference write. If a saved icon cannot load, the reduced dock stays
  usable and preference writes report that the change could not be saved.
  On Vampire, early admission preserved all six paths in the isolated
  recovery backup and created no replacement JSON.
- Divider coordinates remain in 64-bit arithmetic until clipped. The
  original UBSan reproducer passed after the fix. Screen/cache admission
  and RGB24 decoding now each have one shared implementation.
- Both Amiga builds, the portable tests and sanitizers, and the native AMMX
  probe passed. A 360-case render comparison produced identical RGBA and
  coverage output. The PiP probes compiled.
- A synthetic EClock composition test on Vampire used 13 icons, 40 frames per
  pass and six passes per build and orientation. Median Bottom time fell
  from 17.70 to 17.32 ms per frame. Median Right time fell from 19.51 to
  19.18 ms. Row scanning and an alternate alpha calculation were slower and
  were discarded.
- The baseline recovery trial left an older Dock process waiting on
  Intuition with the PiP lease held. The live configuration was untouched.
  After the restart, 0.117 passed PiP hover tests on Bottom and Right and
  opened About. An isolated config saved an additional pin. The running
  dock retained all 12 original pins and the original preferences.
- End-to-end hover measurements did not establish a frame-time improvement.
  The missing-icon save guard was reviewed separately from the native
  over-budget recovery and successful-save cases.

## US English copy

- [x] Audit documentation, comments, and program messages.
- [x] Use short, plain US English and keep key technical details.
- [x] Verify both Amiga builds and check the text.

Both Amiga builds and the probes compiled. Host tests and packaged Markdown
links passed. C code matched the baseline after excluding comments and
string contents. Git has no staged files or commits.

## Sharper icon scaling trial

- [x] Build a sharper cache filter with the same icon sizes and alpha handling.
- [x] Compare both filters on Vampire at rest and at full zoom.
- [x] Check the original icon and saved preferences after the test.

Both Amiga builds, host tests, and sanitizers passed. Both filters ran in
PiP on Vampire with a separate config. Demos kept 32 x 28 px of artwork at
rest and 62 x 54 px at full zoom. The original icon and live config match
their pretest bytes.

## WBDock icon copy

- [x] Read the icon path from WBDock and copy that icon to RAM.
- [x] Compare the copy with the existing pin on Vampire.
- [x] Run the sharper build beside WBDock without changing live preferences.

DH11 is the Archives volume. The two icon paths and the RAM copy had
identical bytes. DockAwesome 0.119 produced identical full PiP frames
from either icon at rest and full zoom. The copy runs in a test dock on
the left. WBDock remains at the bottom.

## Faint icon shadows

- [x] Apply one rule to all icons with partial alpha.
- [x] Compare Demos on Vampire at rest and full zoom.
- [x] Verify that edge pixels, icon size, and the original file are preserved.

The icon cache clears faint dark pixels away from denser pixels. The tests
cover soft edges, colored pixels, and fully translucent icons. On Vampire,
0.123 removed the shadow under e, m, and o in both frames and kept the line
under D. The artwork bounds, original icon, and live preferences matched.

## Review fixes for icon scaling and shadows

- [x] Clear faint shadows once on a cropped source copy, before scaling.
- [x] Sharpen only axes that scale up. Keep linear weights when scaling down.
- [x] Keep `sharp_fraction` within 0..255.
- [x] Test upscaled soft edges and shadow clearing at every level.
- [x] Run host tests, sanitizers and both Amiga builds.
- [x] Review the whole codebase as a senior engineer and record findings.

## Bicubic icon scaling

- [x] Replace the 2x2 bilinear cache filter with separable bicubic resampling.
- [x] Compare kernels on the 11 decoded pins at rest (36) and full zoom (70).
- [x] Measure cache builds on Vampire with the real pins and configuration.
- [x] Run 0.126 on Vampire with a copy of the live configuration.
- [x] Decide between bicubic quality and build time after viewing PiP output. (Keep bicubic.)

Keys bicubic (a = -0.75) with half-ratio widening for reductions. It is exact
at 1:1, keeps flat areas flat, and averages fine stripes. Against the 0.123
smoothstep filter, mean edge contrast on the real pins rose 6% at rest and
10% at full zoom. Catmull-Rom (a = -0.5) gave -4% and +6%. Vampire cache
builds for the 11 pins (36 -> 70 px, 33 levels): 0.123 took 107 ms per icon,
bicubic 239 ms (setup 26, vertical 83, horizontal 130). Moving tap tables to
32-bit math saved 10%. Register accumulation, transparent-pixel skips and a
reciprocal color divide did not change the time. Host tests, sanitizers and
both Amiga builds passed. 0.126 runs from RAM: with RAM:DockAwesome-test.json;
0.123 remains at RAM:DockAwesome080 for rollback.

### Senior review findings (open)

High
- amiga_main.c:1600/1780: worker stack frames of ~17.5 KB each on a 32 KiB
  stack; the icon-discovery chain reaches ~22 KB before icon.library runs.

Medium
- amiga_main.c:3405/3211: fatal-error requesters run while PiP, the input
  handler and the drop receiver are still live.
- amiga_main.c:1845-1918: Intuition calls and a blocking DoPkt inside Forbid.
- amiga_display.c:216: ReadPixelArray8 uses the UWORD NDK inline.
- amiga_ui.c:1738: ASL folder requester blocks main without suspending PiP.
- amiga_display.c:69-83: 8-bit pen table (1M iterations) rebuilt per open.
- dock_core.c: cleared shadows still count toward artwork bounds, shifting
  icons up to 3 px (rest) and 7 px (zoom). Kept by design; needs a decision.
- dock_core.c: the shadow rule erases dark translucent art enclosed by solid
  pixels; a flood fill from the crop border would keep it.
- dock_core.c: plate glass is recomputed every frame though it is constant.

Low
- Wake-ups on every mouse event while another screen is in front; full-surface
  compare on every animated frame; blocking LockLayerInfo in snapshot hover;
  drops lost during rebuild; save leftovers; unbounded exit loop; JSON errors
  invisible from Workbench; folder scans without pr_WindowPtr; drag preview
  has a window title; folder icons build a redundant second level; animation
  push can leave the surface with levels=2.

Resolved by the bicubic filter: large reductions lost fine detail, and
resampled colors were rounded down.

## Fix all review findings

- [x] Core: visible-artwork bounds after shadow clearing; flood-fill shadow
      rule; animated rows kept inside the surface; 32-bit mix_half; uint32
      weight math; big-endian RGB24 test; presenter hook removed;
      single-level dock_icon_build for folder icons.
- [x] amiga_main.c: all 12 findings (worker stack, deferred fatal
      requesters, IBase revalidation, DoPkt on the worker, handler wake
      filter, partial compare, AttemptSemaphore hover, drops across rebuild,
      save cleanup, bounded exit loop, Workbench JSON error).
- [x] amiga_ui/display/folder/drag_preview: findings 1-8; ASL blocking hook
      wired in main.
- [x] Host tests, big-endian tests, sanitizers, fuzz harness, both Amiga
      builds (0.135), AMMX probe on Vampire.
- [x] Program icon: assets/DockAwesome.info (tools/make-icon.py), added to
      `make package`; icon.library decodes it byte-exact on Vampire.
- [x] Independent review of the native diff; its five findings fixed in 0.140:
      exit waits for EndDockStatusNotify instead of exiting while Intuition
      can still signal the task; kept backfill hooks point at persistent
      code (no-op or forwarder) instead of DockAwesome's segment; console
      windows are probed only while the pointer rests on a running Shell
      pin, not on every status poll; Quit Shell is offered only for targets
      probed in the last 2 s; drops queued across a surface rebuild are
      translated to the new surface.
- [ ] Visual and interactive check of 0.140 on the physical display.

On Vampire, ApolloOS console windows have no UserPort and each console has
its own handler task, so window ownership cannot be checked through the
UserPort. 0.135 exited cleanly on Ctrl-C; 0.140 runs from RAM: with
RAM:DockAwesome-test.json. 0.123 remains at RAM:DockAwesome080.

Results: the shadow and glass repros now keep 400/400 glass pixels and
equal margins; all 11 real pins render byte-identical to before. The
levels=2 overflow repro fell from 65 px to 0; the fuzz harness reports 0
target and 0 animated out-of-surface frames. The C half blend fell from
4,686,175 to 2,150,417 EClock ticks per 16.4M pixels (AMMX unchanged, all
probe checks pass). Worker stack frames fell from ~17.5 KB to 1.3 KB.

Deferred, with reasons:
- Tinted-plate cache (kept glass) and visible-span icon blending (runs) were
  implemented in c41bfa5; see "Large-icon zoom speed".
- Partial compare (main finding 6) is implemented (frame_check and the
  plate-only check in live_capture); its cost is capture_compare_us_avg in
  the --profile report.
- Folder-reveal hook: if another program chains our backfill hook and the
  original was the default, a pass-through stays installed while the dock
  runs. Closing it needs a permanent code stub.
- Refactors: split amiga_main.c, narrow App, one ApolloOS ABI header, merge
  the two layout mappings, collapse the render wrapper stack, validate
  settings once per swap.

## Install in WBStartup

- [x] Move WBDock and WBDock.info to ApolloOS:WBStartup/Disabled.
- [x] Copy DockAwesome080 0.140 as ApolloOS:WBStartup/DockAwesome with the
      build's .info (assets/DockAwesome-48.info: WBTOOL, 32 KB stack).
- [x] Start it through Workbench (WBRun) with the live ENVARC configuration.
- [x] Remove staging files from RAM:.

WBRun started the installed copy; FindTask reports DockAwesome running and
WBDock not running. Roll back by moving WBDock and WBDock.info back from
Disabled and removing DockAwesome and DockAwesome.info from WBStartup.

## Workbench-style program icon

- [x] Measure the previous icon: 48x48 canvas but 40x22 px of artwork, while
      the pinned icons fill about 44x44.
- [x] Icon concept: a magnifying glass over the dock (user's choice of three
      bold concepts after the scene-style drafts were rejected). The lens
      shows the dock plate, two glossy icons and a Boing ball enlarged; the
      metal rim and handle fill the 48x48 canvas.
- [x] Add a selected image like ApolloOS icons: darker, with the ball turned
      half a check.
- [x] tools/make-icon.py writes assets/DockAwesome-48.info (WBTOOL, 32 KB
      stack, frameless); installed as ApolloOS:WBStartup/DockAwesome.info.

The previous icon is saved outside the checkout in the session scratchpad.

Icon handed over to Codex (2026-11-08): it replaces assets/DockAwesome-48.info
and the WBStartup icon. Do not change the icon files from this session.

## Green CRT program icon

- [x] Package the approved 48x48 artwork with a selected image.
- [x] Verify both packages use the new icon.
- [x] Replace the Workbench icon on Vampire and verify the installed file.

Both builds package the same icon. The WBStartup file matches it byte for
byte, and a Vampire screenshot shows the new icon.

## Simplify without losing performance

- [x] Resample: drop the register path, transparent skip and reciprocal
      divide (measured no gain on Vampire); keep the 32-bit tap tables.
- [x] Remove the four unused render wrappers and dock_edge_slot.
- [x] Validate settings once per call chain instead of in every layer.
- [x] One shared ChangeWindowShape wrapper.
- [x] Group profiling counters in one struct.
- [x] Verify: host tests, sanitizers, byte-identical render and cache output
      on the real pins, both Amiga builds, Vampire cache benchmark.

WBStartup fix: after a reboot Workbench waited on DockAwesome ("has not yet
returned"), so AmiMCP never started. The icon lacked DONOTWAIT (WBDock has
it). tools/make-icon.py now adds it; icon.library reads it from the
installed ApolloOS:WBStartup/DockAwesome.info (type 3, stack 32768).

## Sharper green CRT icon

- [x] Sharpen the 48x48 source with its color tones and alpha intact.
- [x] Verify the rebuilt Workbench icon and package output.
- [x] Compare the original and sharpened icon in WBStartup on Vampire.

Both packages and WBStartup use the sharpened icon. The before and after
captures match their source images at 1:1. The DONOTWAIT tool type is intact.

Simplification results: the resample keeps only the unrolled four-tap sum
(a generic register loop was slower than streaming: 279 vs 245 ms) and drops
the transparent skip and reciprocal. Vampire cache builds for the 11 pins:
241 ms before, 217-220 ms after. Output: 3 of 68,156 cache pixels and 1 of
72,000 rendered pixels differ by one level (exact division again). Removed
four render wrappers, dock_edge_slot and dock_interaction_slot (tests only),
and the duplicate validation in dock_renderer_draw; other validations guard
array bounds at API boundaries and stay. ChangeWindowShape lives in
include/amiga_abi.h (LVO calls unchanged). 47 profiling counters moved into
ProfileStats. Host, big-endian and sanitizer tests and the fuzz harness pass.

## Temporary icon jump

- [x] Cause: a program that is not pinned (ApolloBoot's ReqList window) gets
      a temporary icon; when it quits, program_poll_result snapped the whole
      row to the new rest layout, so the centered dock jumped.
- [x] Fix: like drops and unpins, the new icon appears in its slot and the
      rest glide to the new row (0.148, installed in WBStartup).
- [ ] User check: open and close ApolloBoot's window.

The 0.140 binary was locked after the dock quit, so it was renamed to
WBStartup/Disabled/DockAwesome-0140.old (since deleted).

## Running indicators for scripts and detached programs

Probed on Vampire with ApolloBoot's requester and HippoPlayer open:
- ApolloBoot (project, default tool IconX): the window "  ApolloBoot  " is
  owned by the CLI process BOOT:C/ReqList-Short; cli_CommandFile is empty and
  Workbench lists only ApolloOS:C/IconX. It became a temporary ReqList icon.
- HippoPlayer detaches: its window owner is task "HippoPlayer" without a CLI
  and it is missing from Workbench's program list, so it was not discovered.

- [x] A window titled exactly with a document pin's file name (blanks
      trimmed) marks that pin running and adds no temporary icon.
- [x] A window owner without a CLI is discovered as its task name in its
      home directory (lock duplicated in Forbid after a liveness check).
- [x] 0.150 installed in WBStartup and started through Workbench.
- [ ] User check: dot under ApolloBoot with its window open and no extra
      icon; HippoPlayer appears as a temporary icon with a dot.

0148.old in WBStartup/Disabled was still locked by Workbench; delete it later.

## General running-program detection (no per-program rules)

Requirement: every program with a window on screen appears, background
processes do not, and nothing is special-cased per program.
- [x] An app is a process owning a visible window: a layer, not a backdrop,
      at least 16x16, with a title, close gadget or drag bar.
- [x] Attribution order: earlier attribution, pinned program by path, a new
      app that appears within 10 s of a dock launch (that pin), a temporary
      icon from its program file, else the default tool icon labeled with
      the task name.
- [x] Attributions persist while the task lives and still shows a window,
      in every poll, so document pins keep their dot.
- [x] Clicking a running icon focuses its attributed task's window.
- [x] Remove the title-name and sole-tool heuristics.
- [ ] Verify on Vampire: ApolloBoot from the dock (dot, no extra icon),
      HippoPlayer from Workbench (temporary icon), background tasks absent.

0.156 (installed in WBStartup) implements the above. Apps without a program
file cannot be kept as pins. Disabled/ holds locked old binaries to delete.

## Test release 1.0 beta 1

- [x] Release versions: RELEASE replaces the build counter in About and
      $VER ("DockAwesome 1.0 (08.11.2026) beta 1"); About credits Reine
      "ZyBeR" Delwall. 68080 only.
- [x] make release stages DockAwesome, its icon, ReadMe.txt and README.md.
- [x] Packed with LhA 2.15 on the Amiga; Lhasa tests it and every file
      matches the staged build. ~/Downloads/DockAwesome-1.0b1.lha
      (103,621 bytes, SHA-256 02412cd2...0d0cfdf).
- [x] 1.0 beta 1 installed as ApolloOS:WBStartup/DockAwesome (same bytes
      as the archive) and started through Workbench. Disabled/ holds the
      locked DockAwesome-0156.old; delete it after a reboot.

## About typography and speed

- [x] Vera Sans through diskfont made the first About after a reboot take
      ~11 s (measured: open fonts 11,243 ms, window 3 ms, measure 0 ms,
      draw 5 ms). Later opens were fast only while the fonts stayed cached.
- [x] tools/make-about-font.py renders Vera Sans (Bold 20, 12, 11, 13 px,
      ASCII 32-126) into src/about_font.c; About composes lines from it and
      blits them with BltTemplateAlpha. No font files are needed at run
      time; without cybergraphics alpha it falls back to the screen font.
- [x] Credit on two lines with spacing; the name is regular weight.
- [x] About redesigned and prerendered; installed for the user's check.

## Settings limits, Save feedback and Workbench resets

- [x] Size slider maximum follows the largest Size that fits the draft's
      edge, Scale and Folder Stack (main's fit hook, same dock_items_fit as
      Save); Scale stops at the 192 px zoom. The icon cache budget is a
      quarter of free memory (at least 16 MB) instead of a fixed 16 MB.
- [x] Save shows a busy pointer and progress in the Settings title. Measured
      for 11 pins: icon loading 267 ms, cache 36->70 2,250 ms, 64->125
      6,444 ms; the cache build is the cost.
- [x] Workbench resets (resolution changes): Intuition on ApolloOS R9.5 sent
      no BEFORE_CLOSEWB (probe: 0 messages), so the dock is a
      screennotify.library Workbench client (type 3; FALSE before close,
      TRUE after open, also after a failed close). It releases its windows,
      UI and screen lock, waits, then reopens with the backend chosen for
      the new screen and lowers Size if needed.
- [x] Vampire: CloseWorkbench() from a separate process succeeded, Workbench
      reopened, and the dock reopened on the new screen with no requester.
- [x] User check: a real resolution change in ScreenMode prefs works.

Release archive rebuilt: ~/Downloads/DockAwesome-1.0b1.lha (119,699 bytes,
SHA-256 111bc02f...8afed359), packed with LhA on the Amiga; every file
matches build/release/DockAwesome.

## Large-icon zoom speed and plate padding

Vampire, Size 72, 13 icons, 1280x168 canvas (framebench, compose only):
58.7 ms before.
- [x] Plate glass: the plate color mixed with the backdrop is kept per row
      and mixed again only after the backdrop changes (PiP capture, drag
      reference, snapshot). 58.7 -> 46.3 ms; a backdrop that changes every
      frame costs +3.4 ms over before.
- [x] Icon blend without a divide, two channels per multiply, and
      precomputed per-row runs (transparent skipped, opaque copied) built
      with the cache (~3% of pixel bytes). -> 28.5 ms.
- [x] Backdrop copied only outside the plate interior. -> 24.5 ms.
- [x] Output byte-identical to HEAD: 300 render scenarios (3 edges, flat and
      captured, coverage, pressed, centered, divider), with and without
      glass; 200,000 random blends; new unit tests for glass and runs.
- [x] Real dock with --pip --profile: render 52.9 -> 40.8 ms (compose 40.6
      -> 28.5) before the backdrop-span change.
- [x] Real dock after all compose changes: compose 24.1 ms, conversion
      12.3 ms (render about 36 ms, from 52.9).
- [x] Rejected: a fused AMMX encode+key kernel. Validated bit-exact on the
      Vampire and 13% faster cache-hot, 8.1 vs 11.7 ms in framebench, but in
      the real dock writing the PiP buffer it was slower in every form: row
      spans 15.3-16.4 ms, full rows 13.4 ms, via a fast-RAM row and CopyMem
      14.7 ms, against 12.3 ms for the separate passes. Code removed.
- [x] Plate thickness: margin grows by an eighth of the artwork above Size 32
      (Size 72: 20 -> 12 px per side; user asked for slightly less than 14). Unchanged at Size 32 and below.
- [x] User check of the new padding on the Vampire (asked for slightly
      less than 14 px; 12 px approved).
- [x] Installed 1.0 beta 1 in ApolloOS:WBStartup (old binaries are in
      WBStartup/Disabled as DockAwesome-*.old; Workbench holds locks on
      them until a reboot, then delete them). RAM: cleaned.

Release archive rebuilt as 1.0 beta 1 (beta 1 was never published):
~/Downloads/DockAwesome-1.0b1.lha (122,219 bytes, SHA-256
75693347...ffb8eefe), packed with LhA on the Amiga; every file matches
build/release/DockAwesome.

## Damage-region rendering (plan-damage-rendering.md, PR 1)

Vampire, 12 icons, `--pip --bench-sweep`, baseline 34dae0a against the
damage build, run alternately per speed in one batch. Times in ms.

| Size, speed | Render before | Render after | Compose | Conversion | Damage (compose / conversion) |
| --- | --- | --- | --- | --- | --- |
| 72, 3 | 35.9 | 33.0 (-8%) | 24.1 -> 23.3 | 11.8 -> 9.6 | 52% / 59% |
| 72, 12 | 35.7 | 27.5 (-23%) | 23.8 -> 19.2 | 11.8 -> 8.2 | 52% / 57% |
| 72, 25 | 35.3 | 31.0 (-12%) | 23.5 -> 21.5 | 11.7 -> 9.4 | 59% / 66% |
| 42, 12 | 15.5 | 12.2 (-22%) | 10.0 -> 8.5 | 5.5 -> 3.6 | 42% / 49% |

- [x] Goldens (384 scenarios) unchanged; damage fuzz 20,400 frames; PiP
      double-buffer test; host-test and host-sanitize green.
- [x] `--damage-check`: sweep at Size 72 speed 12, 167 frames, 0 compose
      and 0 conversion mismatches; manual sessions on all three edges
      (hover, Workbench drag and drop, reorder, popup, Settings over the
      dock) 246 + 64 + 51 frames, 0 mismatches.
- [ ] Physical scanout: not observed by a person; the damage check compares
      the PiP back buffer with a full conversion before every present.
- Why the gain is smaller than the damaged area: a host replay of the same
  sweep shows 86-95% of icon pixels redrawn per frame, because moving the
  pointer shifts the whole row by a pixel or more. Icon blending dominates
  compose, so damage saves mostly plate and background. Speed 3 also has
  about twice the rectangles per frame (9 vs 4.5), which adds per-row
  call overhead in conversion. Next lever: keep distant icons still during
  the wave (layout), then merge row spans in the conversion.

## Next: zoom wave matching macOS (requested 2026-11-08)

Measured from the user's macOS screenshot (right edge, 2x pixels, rest
96 px, hovered 187 px) against `dock_layout` (Size 48, 195%, pointer on an
icon centre). Share of the full zoom per icon from the pointer:

| Steps from the pointer | macOS | DockAwesome |
| --- | --- | --- |
| 0 | 1.00 (1.95x) | 1.00 (1.96x) |
| 1 | 0.84-0.87 | 0.83 |
| 2 | 0.47-0.49 | 0.50 |
| 3 | ~0.04 | 0.15 |
| 4 | 0 | 0 |

- [x] Make the third icon from the pointer grow ~0.04 of the zoom instead
      of 0.15, keeping steps 0-2; add a geometry test for the profile.
      Catmull-Rom wave; at Size 42, 195%: 1.00/0.88/0.45/0.03/0.
- [x] Vampire, 0.178 against 0.180 (alternated, `--bench-sweep`, 12 icons):
      render Size 72 speed 3 33.1 -> 22.9 ms, speed 12 27.6 -> 23.9, speed 25
      31.2 -> 25.8; Size 42 speed 12 12.2 -> 10.3. Damage share at Size 72
      speed 3 52% -> 38% (distant icons no longer move). Entry settle at Size
      42 171 -> 136 ms, Size 72 329 -> 274 ms. `--damage-check` sweep: 166
      frames, 0 mismatches. 0.180 installed for the user to try (0.178 in
      WBStartup/Disabled). The profile itself is verified on the host; PiP
      does not show in screenshots, so the visual match is the user's check.
- [x] Entry: cubic ease-out over the same 8 ticks (user: macOS feels
      snappier). Note: the macOS video in docs/macos-reference.md shows an
      S-curve of about 150 ms, which the old smoothstep matched at Size 42
      (158 ms measured); the ease-out is faster at the start than that.
- Also reported: the entry feels less snappy than macOS. At Size 72 the
  8-tick entry took 332 ms before damage rendering (36 ms frames on a
  20 ms tick); re-measure after damage rendering before changing the curve.

## Smoother zoom and new size limits (2026-11-08)

- [x] Settings: Size up to 72 px, Scale up to 2.50x, zoom at most 128 px
      (the macOS limit). Measured at Size 42 x 2.50 (105 px): 13.0 ms per
      frame, entry 148 ms; Size 72 x 1.78 (128 px): 23.1 ms.
- [x] Display refresh measured: 50.00 Hz (150 WaitTOF in 2.9997 s), so 50
      frames per second is the ceiling; the dock already reached it.
- [x] Animation ticks synced to the vertical blank. Same pointer motion,
      two runs each: stalled frames (2+ fields) 40 of 250 with the timer,
      10 of 181 synced. Remaining stalls are frames over 20 ms (backdrop
      re-capture when the hover label opens, first ease-out frame).
- [x] Review fixes: sync only while fields measure 19-21 ms (60 Hz modes
      keep the timer), timer fallback when fields stop. Final build 184,
      same motion: 131 even gaps, 3 stalls (2.2%).
- [x] Build 184 installed in WBStartup (build 180 in
      WBStartup/Disabled/DockAwesome-build180.old). The live config keeps
      Scale 1.95x; raise it in Settings to try 2.50x.

## Round hover labels (2026-11-08)

- [x] Labels are pills (semicircular ends) with antialiased rim and tip,
      instead of a radius-7 circle on a 17 px body that showed as an
      octagon with stepped edges.
- [x] Text centred on its ink: on the Vampire (build 186, "Files") the rim
      runs x 12-60 and rows 8-24, the ink x 22-50 and capitals rows 13-19:
      the same centre column and row, 9 px of surface each side (was 1.5 px
      left and 1 px high).
- [x] Codex: tall fonts (ttCourier 25) clipped an underscore; the padding
      now grows equally above and below for accents, descenders and the
      label's own ink. Build 186 installed (build 184 in
      WBStartup/Disabled/DockAwesome-build184.old).

## Folder Stack motion and close button (2026-11-08)

- [x] Open in Workbench glyph drawn with 4x4 antialiasing.
- [x] The stack grows from its pin with a strip reveal (open 160 ms, close
      130 ms, cubic ease-out); measured on the Vampire at open 176 ms in 8
      frames and close 140 ms in 6, about 1 ms per frame.
- [x] While open, the pin morphs into a flat close button with a chevron
      toward the screen edge, and back when it closes.
- [x] Codex review fixes: no clicks in the unrevealed part, the close button
      consumes its press in PiP mode, closes (not reopens) in native modes,
      and keeps its release across shutdown. Final Codex review clean.
- [x] Build 207 installed in WBStartup (build 186 in
      WBStartup/Disabled/DockAwesome-build186.old); click on the pin opens,
      click on the close button closes and stays closed. Old rollbacks
      0.178, build180 and build184 are locked by Workbench until a reboot.

## Glass close button and faster Folder Stack start (2026-11-09)

- [x] Close button: smoked glass (surface alpha 176, rim alpha 220) at 90%
      of an icon's extent via dock_cache_build_scaled (the cache fits any
      artwork to the box, so an inset in the source was undone), thin
      chevron (2.6% stroke) in soft white (212).
- [x] The pin morphs as far as the box is out (dock_ui_folder_progress),
      opening and closing; opening uses the closing motion reversed, 130 ms.
- [x] First frame after the click on the Vampire (299x315 box): 89-95 ms
      before, 60 ms now. Glass over the backdrop is finished per revealed
      strip (15-20 ms moved into the frames, about 2.7 ms each), the glass
      tint writes whole words (16-24 ms with a memcpy per pixel, 8 ms now)
      and runs after the motion. Remaining stages (ms): read 12.5, layer 9,
      labels 8.6, buffers 6, capture 4.6, first present 4.8, window 3.8.
- [x] Build 215 installed in WBStartup (build 207 in
      WBStartup/Disabled/DockAwesome-build207.old). The 90% button and the
      tint guard came after it, from the Codex review.

## /simplify across the codebase (2026-11-09)

- [x] Makefile: pattern rules for host tests, sanitize and probes (183 to
      139 lines); `make -n -B` gives the same commands.
- [x] Core: dock_convert565_damage returns the pixels converted (replacing
      the profile copy), dock_image_opaque_at shared by both input hit
      tests, DOCK_DRAG_THRESHOLD, the plate rect from dock_edge_panel_rect,
      the empty plate normalized once, a derivable renderer field removed.
- [x] Amiga: one backend selection and snapshot fallback, full_frame,
      buttons_engaged, zoom_levels, ticks_us, the v50 Intuition LVOs in
      amiga_abi.h, popup shape via make_folder_shape, a shared coverage
      blend, never-written profile counters and write-only fields removed.
- [x] Host tests, sanitizer and the 68080 build pass with no warnings.
- [ ] Not run on the Vampire yet: needs a real-machine check of PiP,
      snapshot fallback, popups and drag before installing.

## Simplify follow-ups and platform architecture (2026-11-09)

Plan: ~/.claude/plans/rosy-frolicking-lynx.md. Vampire stays the target;
the architecture must allow other platforms later at no performance cost
(per-frame or startup dispatch only, measured against the baseline).

Baseline, build 219 (simplify commit), Size 36, 12 pins, 800x96 PiP canvas:
- PiP sweep 8 px: render 7.8-7.9 ms (compose 5.6, conversion 2.2-2.3).
- Snapshot sweep 8 px: render 13.8 ms (compose 4.6, conversion 8.3 at
  100% of the canvas, portable kernels: AMMX is PiP-only today).
- Window moved behind the PiP dock: 21 changes, all full frames,
  render 24.6 ms (compose 16.3, conversion 8.3), decode 2.1, compare 1.3.
- --damage-check: 0 compose and 0 convert mismatches.

- [x] 1. Backdrop change damages only the changed box (PiP hot path).
      Window behind the dock: render 24.6 -> 13.6 ms, 1 of 22 frames full,
      changed box 42% of the canvas; --damage-check 0.
- [x] 2. Snapshot/opaque present only damage; AMMX kernels in every mode.
      Snapshot sweep: render 13.8 -> 5.9 ms, conversion 8.3 -> 0.64 ms;
      a window passing over the dock repaints fully; --damage-check 0.
- [x] 3. Cache stamps replace address-based invalidation; one morph buffer.
      Folder Stack open/close and reorder: --damage-check 0 over 204 frames.
- [x] 4. Folder Stack close button rebuilt only when its inputs change.
- [x] 5. Bugs: stale dock_screen after Workbench reopen; slider vs Save rules.
      Slider checked on the Vampire; a screen mode change is not yet tested.
- [x] 6. Compositor in dock_compose.c; shared clamp helpers. Every m68k
      function is instruction-identical; Vampire sweep unchanged.
- [x] 7. Hover label, Size slider, popup rows, drag mask (labels, popup row
      hover and the slider checked on the Vampire). Unmasked PiP conversion keys
      coverage in place: 2.17-2.24 -> 1.82 ms per sweep frame, 3 A/B pairs.
- [x] 8a. DockCaps probe (amiga_caps.c), pure dock_choose_profile with
      host tests, --caps. ApolloOS R9.5: cpu_080 yes (AttnFlags $a47f),
      core 12398, profile pip/ammx/glass. Sweep A/B within noise.
- [x] 8b. SurfaceOps (pip, window) chosen once per surface; amiga_main.c
      split into input handler, status, workbench and surface modules
      (4787 -> 2359 lines). Hot functions instruction-identical or proven
      equivalent; Vampire sweep: conversion 1.81 -> 1.73 ms, compose same;
      --damage-check 0 in PiP and snapshot; popup, Folder Stack, R checked.
- [x] 9. Build 225 installed in SYS:WBStartup (build 218 in
      SYS:WBStartup/Disabled/DockAwesome-build218.old). RAM: test files
      removed.

Review (2026-11-09):
- Measured on the Vampire against build 219:
  PiP sweep 7.8-7.9 -> 7.4-7.5 ms (conversion 2.2 -> 1.73);
  snapshot sweep 13.8 -> 5.9 ms; window moved behind the dock
  24.6 -> 13.6 ms per change, 21 of 21 -> 1 of 22 frames full.
- Not yet tested: a Workbench screen mode change (step 5 input fix and the
  depth re-probe), opaque mode with an empty dock, an 8-bit screen.
- Next measured costs: PiP compose 5.6 ms per sweep frame; a backdrop box
  still remixes whole glass rows; GCC places draw/event_loop in
  .text.unlikely (size-optimized), worth an A/B with __attribute__((hot)).

## Launching pins without Workbench (AmiKit / Directory Opus)

Report: on AmiKit for Vampire every pin click shows "Workbench could not
open this object." AmiKit runs Directory Opus 5 Magellan as the desktop
instead of Workbench.

Cause, from the DOpus 5 source (github.com/BlitterStudio/dopus5,
source/Library/wb.c): DOpus patches OpenWorkbenchObjectA only on OS4
(`#ifdef __amigaos4__`). On 68k the call goes to workbench.library, which
fails without the Workbench process. DOpus's own OS4 patch does what we
need: `FindPort("WORKBENCH") ? original : launch ourselves`. On 68k DOpus
does patch WorkbenchControlA: WBCTRLA_DuplicateSearchPath returns its path
list; GetProgramList and GetOpenDrawerList are unsupported. Its ARexx port is
DOPUS.1, and `lister new <path>` opens a drawer.

Plan (ApolloOS keeps today's path unchanged):
- [x] 1. caps: `dopus` (FindPort "DOPUS.1"), printed by --caps; test rows.
- [x] 2. src/amiga_launch.c: a Workbench-style launch for tools and projects,
      used when there is no WORKBENCH port (or with --self-launch).
      - Icon via GetIconTagList (DefIcons fallback). WBTOOL runs the path;
        WBPROJECT runs its default tool with the document as argument 1.
      - A bare default tool name is searched next to the project, then in
        the WBCTRLA_DuplicateSearchPath list (Workbench or DOpus), then C:,
        SYS:Utilities, SYS:Tools, SYS:System.
      - LoadSeg, CreateNewProc with the larger icon stack (at least 4096;
        Workbench itself uses the project's, RKM Libraries ch. 14), TOOLPRI
        from the opened icon, home and current directory = the tool's drawer; a WBStartup
        with a lock per argument. Icons set to "Start from Shell" (CLI) or "Start from
        ARexx" (REXX) run through SystemTags with an AUTO output window, and a
        $STACK: string in the program raises the stack, as workbench.library
        45+ does (amiga-refs/amigaos-3.2, wb release notes).
      - Replies come to launch_port in the event loop; the pin shows a dot
        (RUNNING_SELF) until then. On quit the dock closes its windows and
        waits for the programs it started.
      Vampire, ApolloOS, --snapshot --self-launch: Calculator and Scout
      started (Calculator pri 0; ApolloOS gives every process 40960 bytes
      of stack, so the icon stack is not visible there); RiVA.guide opened
      through its default tool; RAM:test.txt (DefIcons, no .info) opened in
      Editor; dots on while running, off when closed; the drawer pin shows
      "Could not open / ApolloOS:Utilities / object is not of required
      type"; Ctrl-C with two programs running closed the dock window and the
      process ended once both were closed. Graphics memory back to the
      value before the test.
- [ ] 3. Drawers and disks without Workbench, through ARexx to DOPUS.1
      (built, not yet run on hardware): open with `lister new "<path>"`
      (sent without waiting; a failing reply becomes a notice); the status
      worker reads `lister query all` and `lister query <handle> path` and
      matches drawers with SameLock (dots); Close sends
      `lister close <handle>`. Without DOpus a drawer pin says "object is not
      of required type". Scalos waits until someone can check its ARexx
      interface.
- [x] 4. The failure text says "Could not open" plus path and reason, not
      "Workbench".
- [x] 3b. Directory Opus 4.17 (ApolloOS, Coffin) also names its port
      DOPUS.1 but has no lister commands: the probe and every ARexx send
      now also need dopus5.library in memory. With DOpus 4 running,
      --caps says dopus no and a drawer pin gives "object is not of
      required type" without talking to it (Vampire, 2026-11-09).
- [ ] 5. Verify on the Vampire with --self-launch: tool, tool with a large
      stack (Status in a Shell), project with .info, DefIcons document, a
      CLI tooltype, a script, Folder Stack open, the dock quit while a
      started program runs, PiP sweep unchanged. Then a test build for the
      AmiKit user with --caps.

Vampire, ApolloOS R9.5, --self-launch, test tools in RAM: (2026-11-09):
- TOOLPRI=-5 on a tool icon: the process ran at -5. A project with no
  TOOLPRI started its tool at 0, as Workbench does.
- $STACK:200000 in a tool with an 8192 icon: 200000 bytes.
- Project (stack 100000, default tool "plain" next to it): plain started
  with 100000 bytes and WBStartup args plain + doc in RAM:lt.
- CLI tooltype: ran from a Shell, argc 1, current dir the icon's drawer.
  The command got the Shell's default stack (40960 here), not the icon's
  50000: SystemTags' NP_StackSize sets the Shell process, and Directory
  Opus 5 launches CLI icons the same way.
- REXX tooltype: RX "<path>" ran the path as ARexx source (error 41);
  fixed with RX "call '<path>'". hello.rexx and "it's a script.rexx" then
  ran from the dock in their drawer.
- Lock check: a drawer stayed locked after its tool ended, but Workbench's
  own OpenWorkbenchObjectA of the same libnix test tool leaves the same
  lock, and a tool with no C startup leaves none with any combination of
  NP_CurrentDir/NP_HomeDir. Not caused by the dock's launcher.
- Not run: DOpus 5 listers (only DOpus 4.17 here); needs AmiKit.

Not possible under DOpus: a list of running programs. Its launch list is
private (LibData.launch_list) and neither ARexx nor dopus5.library exposes
it, so program dots come from our own launches and the window pass.

## PiP on AmigaOS 3.x on a Vampire (Coffin, AmiKit) (2026-11-09)

Goal: the ApolloOS experience (live see-through dock, windows dragged
under it) where the Vampire runs AmigaOS 3.9/3.2 with P96. The probe on
branch pip-os3-probe (b535291) showed it works on Coffin r65 at 16 bits:
SAGA PiP2 and its colour key work under P96, the dock shows, zoom works,
and a window dragged under it updates without lag once a busy layer list
no longer stops the capture.

PiP2 itself is RGB565 only (SAGA formats: 8/15/16-bit, YUV), so the dock's
own pixels are 16-bit; the desktop around and through it keeps its depth.

Plan (branch pip-os3 from simplify-followups):
- [x] 1. Profile: PiP needs AMMX and core >= 11900 only. Window shapes and
      the 16-bit screen stop being requirements; apollo_pointer only on
      ApolloOS Intuition (shaped_windows). Update test_profile rows
      (AmigaOS 3.2/3.9 and 24/32-bit screens now choose PiP).
- [x] 2. Receiver window without shapes: a 1x1 window (no backfill) while
      no drag is on; visible width from the canvas, not the window.
- [x] 3. Live capture keeps going when the layer list is busy (only the
      Z-order check needs it). Verify on ApolloOS before merging.
- [x] 4. Drops without shapes. Moving the receiver at the press failed:
      Workbench 3.9 stalls input and cancels the drag (receiver in place
      at 3 of 3 releases, no AppMessage). It now arms ahead: buttons up and
      the pointer on a Workbench icon or drawer puts the receiver on the
      dock box (front, painting the desktop saved then); leaving puts it
      back at 1x1. Coffin 16-bit, by the agent: Scout dropped from a drawer
      was added; drags inside a drawer still work; a window moved under the
      plate afterwards was followed (5 changes).
- [ ] 5. (built, not run at 24/32 bits) 24- and 32-bit screens: capture converts RGB24/BGR24/ARGB32/
      BGRA32/RGBA32 rows to RGB565 for the same compare and decode path
      (the overlay is RGB565 anyway). Measure the capture cost against
      the 16-bit 4.0 ms.
- [ ] 6. Verify: Coffin 16 and 24 bit (show at start, zoom, drag under,
      click, right-click menu, drop an icon, Folder Stack, Settings);
      ApolloOS unchanged (PiP sweep, drops, --damage-check); --caps rows.
      Coffin 16-bit so far: shows at start, zoom, window dragged under it
      without lag (user), clicks and right-click menu, drop from a drawer.
- [ ] Output of a dock started with Run >file is lost on Coffin (the
      foreground run with > keeps it); look into it.

### Floating parts on PiP1 (no window shapes) (2026-11-09)
The save-under drag preview lags (Intuition moves windows asynchronously)
and labels have square corners without shapes. SAGA's second overlay,
PiP1, shows beside the dock's PiP2 under Coffin (probe: both visible).
- [x] 1. amiga_pip: choose the unit (PiP1 0xdff3d0, PiP2 0xdff3b0, own
      lease each) and move an open overlay.
- [x] 2. amiga_float: an ARGB image shown in PiP1, alpha 0 as the key
      colour, width padded to 32; opened once and kept on (keyed clear
      when idle), reopened only to grow; cut at the screen edge.
- [x] 3. Dragged pin on PiP1 when PiP has no shapes: the same picture as
      the dock's (PiP2 lies over PiP1), redrawn on each size change,
      Remove as a pill once armed; window preview when PiP1 is taken.
- [x] 4. Hover labels on PiP1.
- [x] 5. Verified on Coffin 24-bit by the user (videos IMG_2477-2479):
      drag in/out, Remove, labels. ApolloOS: not yet rechecked.

### Coffin polish (2026-11-10) — verified on Coffin, user approved
- [x] A Folder Stack pin cannot be dragged (all modes and OSes).
- [x] Workbench drag over the PiP dock: the drag image is painted into
      PiP2 over the whole surface (holes lagged Workbench's image).
- [x] Folder Stack and menus without window shapes: glass over the
      captured screen, rounded edges and tip, AA glyph, entrance motion.
- [x] rgb_rastport and alpha_text probed by drawing (both false on Coffin).
- [x] Folder Stack grows from its tip and fades, both ways; quarter-step
      fade via the 50% mix; 5-6 frames in 130 ms at 50 Hz.

### Coffin as daily dock (2026-11-10) — user approved
- [x] Pins from Coffin's WBDock (SYS:WBStartup/WBDock), bottom edge, size 32.
- [x] PiP1 floats only beside a narrow dock (scrambled the 992 px dock).
- [x] Icon-less scripts run with Execute (ShapeShifter Launch, WOMyFiles).
- [x] Quit for running apps on AmigaOS 3.x (posted IDCMP_CLOSEWINDOW).
- [x] No drag preview past the screen-edge side of the dock.
- [x] Installed on Coffin 2026-11-10: SYS:WBStartup/DockAwesome (+ .info, DONOTWAIT), ENVARC:DockAwesome.json, WBDock moved to SYS:WBStartup/Disabled/. Build db01deb. Rollback: move WBDock back, delete the two DockAwesome files.

### Before merging pip-os3
- [x] ApolloOS regression, build 315 (2026-11-10 night, by agent):
      --caps: rgb_rastport yes (after the RGB565-precision probe fix,
      653280b), alpha_text yes, ui_glass yes; --damage-check 80 frames, 0
      mismatches; shaped label (tip, glass), menu (glass, AA text), Folder
      Stack (scale + fade: entrance 6-7 frames in ~150 ms, dismissal 6 in
      ~140 ms), shaped drag preview with Remove, Workbench drop added a pin
      (drag image painted 8 frames); config restored afterwards.
- [x] Installed build 315 on ApolloOS: SYS:WBStartup/DockAwesome; the old
      build 225 (black rectangle after boot) is Disabled/DockAwesome-build225.old.
      Older .old builds removed; DockAwesome-1.0b1.old kept.
- [x] The user rebooted ApolloOS: build 315 came up from WBStartup (dock
      window there). Whether the black rectangle is gone: ask the user.
- [x] Drag out on ApolloOS (shaped): the window opens past the surface, not
      only when Remove arms (4f6891f); the dock draws the window's own picture
      where the window is, and the window reopens only on Remove changes
      (9c28c08). User: "Allt funkar nu".
- [x] Folder Stack: default icons (DefIcons) for entries without a .info
      (5e8d5ed); round antialiased back and page buttons (5e4e0ec). Seen on
      ApolloOS; back button tested.
- [x] Installed build 320 on ApolloOS; rollback Disabled/DockAwesome-build318.old.
- [ ] Delete SYS:WBStartup/Disabled/DockAwesome-build315.old after the next
      ApolloOS boot (Workbench holds it since it started it at boot).
- [x] Folder Stack icons kept between openings and read ahead while idle
      (4272be2): 22 icons 940 ms -> first opening 132 ms, later 12 ms.
- [x] AmigaOS 3.2.3 (Vampire): installed build 344 in WBStartup with its own
      ENVARC config (WBDock's programs, bottom), WBDock moved to
      WBStartup/Disabled, rollback Disabled/DockAwesome-build326.old. Fixed
      there: stale band from the drop receiver (now behind windows), window
      preview lag (pins drawn on the screen, dock_bob), tearing on a side
      dock (PiP2 flips in the VBlank), labels in windows (PiP1 only with
      --pip1), left dock keyed tail. User: "Funkar perfekt!".
- [x] ApolloOS: installed build 344 (rollback Disabled/DockAwesome-build320.old;
      315 and 318 removed). --caps unchanged; shaped drag-out with Remove
      checked with the button held.
- [ ] ApolloOS with the user: drag-out seam now that PiP2 flips in the
      VBlank (the shaped window moves at once), tooltips, Folder Stack.
- [x] Coffin: installed build 344 (rollback Disabled/DockAwesome-build325.old;
      314 removed). Drawn drag pin with the round Remove pill checked with
      the button held; the screen came back clean. User: "Allt funkar på
      Coffin också!" (said on 325; 344 installed after).
- [x] Installed build 325 on Coffin (rollback Disabled/DockAwesome-build314.old).
- [ ] Coffin with the user: drag out of the bottom dock (the window path now
      draws the window's picture), default icons, native back button, icon
      fill speed.
- Note: Coffin starts AmiMCP from S:User-Startup (run amiagent TOKEN=amiga),
  ApolloOS from SYS:WBStartup/AmiMCP (IconX script waiting for AROSTCP).
- [ ] A crash after starting a second dock (PiP2 is already in use, OK):
      unknown whether the new or the broken old dock; reproduce with the user
      present. Exit path reviewed, nothing found.
- [ ] Run >file loses profile output on Coffin (use an Execute script).
- [ ] Merge pip-os3 (user's call); push only when asked.
- [x] Build 352 (PiP off at start and on Ctrl-Amiga-Amiga, --pip-off, menus
      step aside only when they can reach the dock) installed on AmigaOS
      3.2.3 and Coffin; rollbacks Disabled/DockAwesome-build344.old there.
- [x] ApolloOS: installed build 352 (rollback Disabled/DockAwesome-build350.old).
- [ ] With the user: reboot with Ctrl-Amiga-Amiga into another system and
      check that no previous dock frame (or black box) shows at boot; offer
      `DockAwesome --pip-off` early in S:User-Startup for other reboots.


## AmiKit and more AmigaOS versions (2026-11-10)
- [x] AmiKit (DOpus 5 desktop): drops from desktop and listers, selection box,
      Clock's dot, dragged icon without blinking, clean quit. Branch
      amikit-dopus, build 367 (2a808c8), not pushed, no release.
- [x] Build 367 installed in SYS:WBStartup on AmiKit XE, AmigaOS 3.9 and
      AmigaOS 3.1.4.1; WBDock moved to SYS:WBStartup/Disabled there. Prefs
      in ENVARC:DockAwesome.json (the WBDock pins on 3.9 and 3.1.4.1).
- [ ] AmigaOS 3.1 (Kickstart 40): `DockAwesome --caps` hangs. Not installed,
      WBDock left as is. Find the call that needs V44+ before installing.
- [ ] Not retested with build 367: ApolloOS, Coffin, AmigaOS 3.2.3.
- [ ] Illegal instruction (#80000004) reported on 3.2.3 (not a 68080?): safe
      CPU check at start.
- [ ] Reports from others on 1.0b2 (ApolloOS): window part kept behind the
      dock, grey PiP rectangle. Likely no colour key on their core; ask for
      --caps.
- [ ] Same report on AmigaOS 3.2.3, core STANDALONE_12846_x14_16, 1.0b2
      (photo 2026-11-15): a window is cut where it meets the dock's box,
      the wallpaper shows instead. Live PiP keys those pixels to the
      scanout, so the dock likely runs in snapshot mode: saved PiP prefs
      fall back to snapshot without a word when PiP fails to open
      (amiga_main.c, fall_back_to_snapshot), and the snapshot window covers
      the zoom room above the dock. Ask for `DockAwesome --caps` and, with
      the dock quit, `DockAwesome --pip` (prints the reason). Then: report
      the fallback reason, and keep the snapshot window at rest height.
- [ ] PiP survives ApolloBoot's BOOT:C/Reboot (direct RESET): message to the
      ApolloBoot author and the Apollo team drafted.

## CPU check before any 68080 code (2026-11-15)
Illegal instruction (#80000004) was reported on 3.2.3. AttnFlags bit 10 is
set on the Vampire under ApolloOS ($a47f) and AmigaOS 3.2.3 ($847f), core
12856 (measured). The startup and libnix are 68000 code (multilib ".").
- [x] src/amiga_cpu_guard.c, built with -m68020: an ADD2INIT at priority
      -128 (the first libnix constructor) checks bit 10 and exits with 20
      after a Shell line or an EasyRequest from Workbench.
- [x] objdump: the check is plain 68020 code (btst #2,296(a6) on
      AttnFlags); its init-list entry has priority 0, the lowest.
- [x] 3.2.3, build 370: --caps and --version as before. A test build that
      requires bit 11: from the Shell the line and return code 20; from
      Workbench (WBRun) the requester, and the process ended after OK.
      Test files removed from RAM:.

## PiP only (2026-11-15)
The CPU check admits only the 68080, and a dock without PiP (snapshot or
opaque window) cut windows behind its zoom room (3.2.3 report). Decision
(user): PiP is the only backend; when PiP cannot run, say why and quit.
- [x] Profile: no backend choice. dock_choose_profile reports why PiP
      cannot run (no AMMX, core, screen); the start shows that reason.
- [x] Start: open failures show surface_error (pixel format, alignment,
      PiP2 taken); no fall_back_to_snapshot. A Workbench reopen on a screen
      PiP cannot use leaves the dock closed until the next change.
- [x] Removed amiga_surface_window.c, app->snapshot, app->pip_mode,
      requested_backend, the window-only IDCMP button and refresh paths,
      the layer check in pointer_inside and the input handler, and the
      fields only they used.
- [x] Prefs: "snapshot" and "pip" still read (old files load) and ignored;
      "pip": true is still written so an older build keeps PiP on rollback.
- [x] CLI: --snapshot and --opaque go (ignored like unknown options); --pip
      and --ammx are accepted. --caps prints "pip yes/no" instead of the
      backend.
- [x] Tests (test_profile, test_prefs), docs (README, PRODUCT,
      architecture, pip-backend, hardware, release notes).
- [x] 3.2.3, build 371 from RAM:: --caps (pip yes); started while the
      installed dock ran: requester "PiP2 is already in use.", process
      ended after OK, no crash. Installed dock quit through its menu, the
      new build ran as the dock: zoom sweep, menu, RAM Disk dropped from
      Workbench (saved), dragged out (removed), Quit. Config restored from
      the backup, installed dock restarted with WBRun, RAM: files removed.
- [x] ApolloOS R9.5 (shaped windows), final build: --caps all yes; installed
      dock quit through its menu; new build with --profile: shaped label,
      zoom, RAM Disk dropped from Workbench (messages=1 added=1) and dragged
      out (downs=1 ups=1, screen clean), menu Quit. A second dock showed
      "PiP2 is already in use." and ended after OK. Config restored,
      installed dock restarted with WBRun, RAM: test files removed.
- [x] Coffin (24-bit) and AmiKit (24-bit): start, --caps, zoom, refusal of a
      second dock (Coffin), drop and drag out (Coffin). AmiKit desktop drops
      fail with build 367 too, see below.
- [ ] A screen PiP cannot use (8-bit): not run on hardware, only in
      test_profile.

## 24-bit PiP capture cost (2026-11-15)
User on Coffin (24-bit): the part behind the dock lags a window dragged
sideways. --bench-sweep: one forced capture of the 1056x100 box took 31.6
ms (16-bit: 4.0 ms). Each changed row read video memory three or four
times, the conversion byte by byte.
- [x] Read each row once into a Fast RAM row (memcpy), compare and convert
      from there; one conversion loop per pixel format.
- [x] Convert and copy only the changed span; scan for it a long at a time.
- [x] Matched run on Coffin, core 12856, 24-bit: dock with --profile from an
      Execute script, Archives window moved 70 x 10 px under the dock by
      ARexx CHANGEWINDOW (delay 2), Break. 58-60 polls, 37-38 changes each:
      capture_compare_us_avg 27333 (before) -> 16663 (Fast RAM row) ->
      14031 (long scan) -> 12072 (span copy). Forced capture 31561 -> 18087.
- [x] User: "Bättre men laggar lite fortfarande".
- [x] Poll spacing with a held button (new profile counter motion_polls,
      motion_interval_us_avg; ARexx move plus a held desktop drag): 20 ms
      requested gave 28.6 ms. Starting the timer before the capture: 28.1
      ms, no gain, reverted. Asking for 10 ms: 17.1 ms (timer.device rounds
      up), capture 6.2 ms average.
- [x] User with the 10 ms build: "Microskopiskt mindre". Floor: the glass
      shows a copy of the screen taken after Workbench drew the window, then
      composed and shown at the next field (one to two fields). SAGA PiP has
      a colour key, no alpha blend, so translucent glass needs the copy.
      Stopped here; left open: VBlank-paced motion polls, AMMX row compare,
      or a design change (less translucent glass while the source moves).
- [x] Coffin cleaned: test docks quit, Archives window closed, ENVARC: and
      ENV: DockAwesome.json restored from the backup, installed dock started
      with WBRun, RAM: test files removed. A second C:ApolloExplorerSrv that
      I started (one already ran) stays until the next reboot.

## AmiKit: drops from the Directory Opus desktop fail (2026-11-15)
User on AmiKit XE (24-bit, DOpus 5 desktop, core 12856): an icon dragged
from the desktop "glitches" over the dock and is not added. Same with the
installed build 367 (2a808c8), started from the Shell and from a lister,
so not from today's changes. tasks/todo.md had desktop and lister drops
working on 2026-11-10.
- Profiles (new build / 367): drag image painted over the dock (110 / 31
  frames), receiver armed late on the drag image (arms=2 / 1), presses
  unarmed (2 / 1), and no AppMessage at all (messages=0): Directory Opus
  did not deliver the drop to the receiver window.
- [ ] Find what changed since 2026-11-10 (screen depth? DOpus settings?) and
      whether lister drops still work. Agent input does not reach the dock
      on AmiKit: each try needs the user's drag.
- Cleaned: config restored, test files removed from RAM:, installed dock
  running (started by the user from a lister). A C:ApolloExplorerSrv that I
  started (none ran at boot) stays until the next reboot.


## Code review fixes (2026-11-17)
Whole-codebase review (five slices, checked against ApolloKnowledgeBase and
amiga-refs). User: "Fixa rubbet!". Branch review-fixes, commits local, not
pushed. Build 388.
Bugs (built; host tests and sanitizer pass; hardware only where said):
- [x] B1 launch: a document without .info runs as a script only with the
      script bit; a dock pin also when it is text and not a program (the
      ShapeShifter Launch case). Never from the Folder Stack, never binary.
      Not run on hardware.
- [x] B2 dock_rgb565_refresh: edge scans bounded (test with a lying equal).
- [x] B3 Apollo bit (SR bit 11) set at the start of main. Ran on ApolloOS;
      not run on 3.2.3 or Coffin, where it matters.
- [x] B4 Delete key hit test uses screen coordinates. Not run.
- [x] B5 native-menu branch defers its Workbench and status signals and
      keeps the live tick. Not run (needs MagicMenu on Coffin).
- [x] B6 unreplied close message gives its signal bit back. Not run.
- [x] B7 status worker: Directory Opus gets two seconds per ARexx call,
      then is not asked again. program_poll_close still waits for the
      worker (its code must stay loaded). Not run (needs AmiKit).
- [x] B8 reset signal served while Workbench is closed; PiP and the float
      layer go off before RESETHANDLERDONE. Not run. Open: a reset while
      an EasyRequest is up still waits for keyboard.device's timeout.
- [x] B9 presses pass through while a requester is up. Not run.
- [x] B10 prefs: unknown keys are skipped, a byte order mark is accepted
      (tests/test_prefs.c).
- [x] B11 new temporary item index with a Folder Stack.
- [x] B12 About releases openurl.library and its pen when the window fails.
- [x] B13 pip_coexist_probe matches "already in use".
- [x] Minor: lister path with a quote refused, 1024-byte handle list,
      Makefile (.DELETE_ON_ERROR, release rebuilds all objects, package
      clears build/dist), fill_rounded, NULL guards in three UI getters.
Performance, measured on ApolloOS R9.5, core 12856, 16-bit, 800x96, 12 pins:
- [x] P1 AMMX row kernels for 24/32-bit capture (ARGB, BGRA, RGB24, BGR24;
      RGBA uses the existing kernel). ammx_probe: all counts and alignments
      pass; RGB24 row 1,220,421 -> 150,797 ticks per 16,384,000 pixels (105
      -> 13 ns per pixel). The dock's capture on a 24-bit screen (Coffin)
      is not measured yet: this machine runs 16-bit.
- [x] P2+P3 blend_pixel inlined with two multiplies, opaque runs as longs
      or memcpy. --bench-sweep 4, two matched runs each: compose_us_avg
      5625/5655 -> 5329/5336 (5.5 %), render 7412/7415 -> 7085/7096.
      --damage-check: 180 frames, 0 compose and 0 convert mismatches.
      Smaller than the review's estimate: compose is not dominated by
      antialiased edges. Next: find where the 5.3 ms goes before more work.
- [x] P4 fast live rate only with the dock's screen in front. Not measured.
Second round (user: "Kör vidare med alla punkter"), build 393:
- [x] Overlay stamps: a rebuilt drag picture or preview icon at a reused
      address is redrawn (DockOverlay.stamp, size compare).
- [x] Status attribution keeps a hash of the task's name beside its
      address.
- [x] Folder Stack entrance shows the icons already in the cache. ApolloOS,
      Personal:Downloads (22 entries): entrance 4-5 frames, second opening
      "0 read, 22 cached", dismissal 6 frames; first paint 80-82 ms (the
      layer stage is 13.5-16.9 ms with the icons blended in).
- [x] Folder Stack dismissal under another window hides by strips instead
      of shrinking a picture of that window. Not seen on hardware (needs a
      window over the open stack); the plain dismissal still animates.
- [x] Quit: PiP goes off and presses pass through before the status worker
      is waited for. A reset while a requester is up is answered
      (BuildEasyRequest + SysReqHandler). Not run.
- [x] src/dock_text.c: title matching, alias extraction, Shell quoting and
      text detection moved out of the Amiga files, with tests/test_text.c.
      The tests found that "Editor - RAM:Copy/Notes.txt" never gave the
      alias "RAM:Copy/Notes.txt" (the volume search ran across the label);
      fixed.
- [x] tests/bench_core.c times dock_renderer_draw (glass, coverage, damage)
      beside whole frames.
- [x] --profile prints "Idle passes" (woken, nothing drawn, no live tick).
Measured and rejected:
- Handler signal filter: 537 idle passes, 377 us each (ApolloOS). At 60
  mouse events a second that is 2.3 % of the CPU, only while the pointer
  moves on the dock's screen; the drop receiver and labels need those
  events. Not worth the risk.
- Aligned pixel buffers: ammx_probe, decode to a destination 4 mod 8 and
  0 mod 8: 60,434 and 60,374 ticks; half blend 187,961 and 186,637. No
  difference on core 12856.
- Capture compare before copy: the Fast RAM row was a measured win on
  Coffin (27.3 -> 16.7 ms).
- Folder Stack listing kept by the drawer's date: FAT volumes (the SD card
  here) do not change a drawer's date when its contents change.
Open:
- [ ] Menu glass over a just-closed label: not reproduced. On ApolloOS the
      label is in PiP1, so closing it exposes nothing; needs a system
      where the label is a window.
- [ ] .bak recovery and save_config have no host test (they are DOS calls).
- [ ] Run on Coffin/3.2.3/AmiKit: B3-B9 above and the 24-bit capture.
- [ ] The Games drawer window on the test machine moved from 351,409 to
      364,420 during my tests.
Coffin (AmigaOS 3.9, 24-bit, no shaped windows, core 12856), 2026-11-17:
- [x] ammx_probe: every kernel passes, also the new screen-row kernels.
- [x] --bench-sweep 4, build 395 against the installed 352-era build:
      compose 5028/4799 -> 5016/5482 us (no gain here, noisy);
      --damage-check 262 frames, 0 compose and 0 convert mismatches. The
      Apollo bit is set in these runs.
- [x] 24-bit capture, matched (baseline 7a74a3d and the new build with the
      same new counter, one script: Archives window moved in 38 steps of
      10 px over the dock's box, 2 ticks apart): a poll that finds a change
      10735 -> 7899 us (second pair 11052 -> 8146); a poll that finds none
      about 3 ms in both (2751-3677). New profile line "PiP2 capture:
      changed/unchanged".
- [x] Compare in place before copying the row: changed polls 8146 against
      7899 us, unchanged 2888 against 2751-3677. No gain; reverted, the
      measurement is in the comment. Kept: a row redrawn back between
      compare and copy is skipped instead of giving an inverted box.
- [x] The 3 ms of an unchanged poll: LockBitMap. A diagnostic build timed
      94 locks at 2.6 ms each (245 ms of 93 polls); the plate compare
      passed in 75 of 76 unchanged polls. Now a poll with a check box looks
      at the screen's memory without the lock, at the address the last lock
      gave; a change, or every tenth poll, takes the lock. Dock at rest,
      84-87 polls each, old/new/old in one script: 4340 -> 2202 us a poll.
      What is left is the compare itself (memory-bound) and one lock a
      second. The same path serves 16-bit screens; not run on ApolloOS.
- [x] The band: the receiver without shapes, armed behind the other windows
      while the pointer is on a drawer or icon, holds a copy of the screen.
      A window over the dock's box that closed uncovered the receiver,
      which Layers filled from the copy: the closed window, or garbage.
      With the pointer then resting on the dock it never stepped back. Now
      it remembers which layers lay on the box when the copy was taken
      (box_windows) and steps back when they differ, arming again half a
      second later. The first try, a time window after arming, failed: the
      window closed within it. Same sequence as the report (right-click at
      640,717, Esc), build 405: clean. Pointer parked on a desktop icon
      for 12 s: arms=1 disarms=0. Window closed by ARexx while armed, build
      406 installed: clean.
- My diagnostic build crashed at exit (it read the EClock after
  profile_close had cleared TimerBase) and kept the PiP2 lease: the user
  rebooted Coffin. The installed builds never held that code.
- Build 406: --damage-check 262 frames, 0 and 0; installed in
  SYS:WBStartup (399 kept as Disabled/DockAwesome-build399.old).
- [x] Independent review of the whole branch diff (one reviewer, with
      host harnesses): no crash or data loss; seven defects in today's
      changes, all fixed in build 407:
      a reset answered inside a requester let the loop show PiP again
      (app->resetting); the Folder Stack's first still frame lacked the
      preloaded icons; the alias search no longer found a path whose
      drawer name holds " - "; dock_shell_quote wrote one byte past size
      with two bytes left (moved code, ASan); the menu branch ran the fast
      tick; a busy layer list at arming stored 0 and stepped back at once;
      popup_covered released with UnlockLayerInfo after AttemptSemaphore.
      Also: PiP1 goes off at quit before the worker is waited for.
      Confirmed by the reviewer: VPERM constants, exact kernel ranges,
      blend bit-identical (200 million triples), peek bounds and
      invalidation, prefs skip_value, loaded[] ownership.
- [x] Build 407 on Coffin: --damage-check twice, 262 frames, 0 and 0; the
      band sequence with the dock running: clean, arms=disarms; installed
      in SYS:WBStartup. Disabled/DockAwesome-build406.old could not be
      deleted (Workbench still holds a WBRun binary); delete after a
      reboot. Disabled/DockAwesome-build399.old is the rollback.
AmigaOS 3.2.3 (Kickstart 47.13, 24-bit, no shaped windows), build 407,
2026-11-17:
- [x] ammx_probe: every kernel passes. --damage-check 194 frames, 0 and 0
      (Apollo bit set).
- [x] --bench-sweep 4 against the installed build (272452 bytes): compose
      5363/5355 -> 5229/4905 us.
- [x] Dock at rest, 95-100 polls: capture 4766 -> 1513 us a poll.
- [x] Band: receiver armed (pointer on the RAM Disk icon, window at
      224,624 832x96), Archives window over the box closed by ARexx, dock
      running: clean; arms=disarms=5.
- [ ] Not run: the dock's menu (a right-click at 640,717 opened none with
      the installed build either), Delete, drops, B5-B9.
- Left: RexxMast started by me (was not running); RAM:DockAwesome (407)
  for the user; installed dock restarted with WBRun, other files removed.

ApolloOS R9.5 (16-bit, shaped windows), build 407, 2026-11-17:
- [x] The reviewer's doubt about the peek: a probe (LockBitMapTags, unlock,
      WriteRGBPixel with no lock, read at the remembered address): the
      memory changed with the pixel, was restored with it, and a second
      lock gave the same address. Live screen memory between locks.
- [x] --damage-check 180 frames, 0 and 0. Dock at rest, 94 polls each:
      capture 787 (build 395) and 781 us a poll: the lock is cheap here,
      no gain and no loss.
- [x] Installed in SYS:WBStartup (395 kept as
      Disabled/DockAwesome-build395.old); Folder Stack opened with its
      icons from the installed dock.
- The cross compiler at ~/ApolloCrossDevLocal was gone when I came to build
  the probe (not removed by me); the probe was built with
  ~/GitRepos/ApolloCrossDev/Compilers/GCC-6.50-Dev. `make amiga` fails
  until the path is back or APOLLO_ROOT points elsewhere. Build 407 was
  already built.

- [ ] Reviewer's notes left open: a hover label
      or popup of the dock's own over the box counts as a window change
      (one step back, half a second without arming); rexx_call polls with
      Delay(1), 20 ms a call; apply_attributions matches by address only.
- [ ] Not run on Coffin: the dock's own menu (my right-click at 640,717
      missed the plate), Delete key, Folder Stack, B5-B9.
- Cleaned: test docks quit, installed dock restarted with WBRun, RAM: test
  files removed. RAM:DockAwesome (build 399) left for the user to try.

Smoke test, builds 388 and 393 from RAM: on ApolloOS: hover, plate menu,
About, Quit through the menu, Folder Stack, profile printed. Installed dock (WBStartup) stopped with
a FindTask/Signal Ctrl-C tool and restarted with WBRun; RAM: test files
removed (BreakTask too). A C:ApolloExplorerSrv that I started stays until
the next reboot.

Folder Stack entrance hitch, build 408 (2026-11-17, ApolloOS):
- [x] The icons that grew with the box blinked out for a few ms at the end
      of the entrance: the first still frame was presented before they were
      blended into the artwork. They are now blended in first
      (load_folder_icon(ui,0)), then the still frame is painted once.
- [x] From RAM: with --profile, Personal:Downloads (22 entries), four
      openings: entrance 4-6 frames, 133-145 ms, all icons in place.
- [x] Installed in SYS:WBStartup (407 kept as
      Disabled/DockAwesome-build407.old). Disabled/DockAwesome-build395.old
      is held by Workbench (WBRun binary); delete after a reboot. 352 deleted.

The dock stays live while a Folder Stack is open, builds 410-412
(2026-11-17, ApolloOS). Reference: the user's macOS screencast; the box
stays put, its tip slides along the edge to point at the stack button's
center every frame while the dock zooms.
- [x] dock_ui_holds_dock: a dialog, a menu or a moving box holds the row;
      a still box does not. Hover, zoom and presses on the dock follow it
      (pointer_inside, update_target, input_publish, sync_input_ui,
      message). The pointer over the box never counts as dock hover.
- [x] A click on another pin closes the stack and goes on as a click
      (at the launch, not the press: the press would clear the snapshot
      and the release would miss the pin). CLI pin with the stack open:
      stack closed, new Shell.
- [x] dock_ui_point_folder_tip: draw() points the tip at the stack pin's
      center; the band by the tip is retinted from the clean backdrop and
      presented. A page change in the same window keeps the tip.
- [x] Build 411 changed the window shape per tip move: Workbench redrew
      the desktop behind each time, dock frames 10 -> 24 ms, settle
      avg/max 184/312 -> 351/876 ms, 168 stalled frames. Build 412 shapes
      the whole tip strip once and paints its backdrop: settle 174/319 ms,
      23 stalled, entrance 5 frames.
- [x] Measured tip on screen: pointer on the stack x=870, left of it
      x=898, pointer off the dock x=871; old place repainted cleanly.
- [ ] Not seen: the dock itself (PiP) beside the tip on the monitor;
      side docks (left/right) not run.
- [x] Installed build 412 in SYS:WBStartup (408 kept as
      Disabled/DockAwesome-build408.old); the stack opened and its tip
      moved to x=898 with the pointer left of the pin. RAM: test files
      removed. Disabled/DockAwesome-build407.old and -build395.old are held
      by Workbench (WBRun binaries); delete after a reboot.

Smoother animations: Folder Stack box and dock zoom, builds 413-427
(2026-11-17, ApolloOS). Plan: measure where frames drop, fix the box's
frame rate, let the user pick the zoom curve.
- [x] Profile: fields between box frames (fields=), each box frame's time
      (frame_us), dock frames during the box's motion, and the first
      stalled dock frames (only frames that continue a motion).
- [x] Zoom alone drops no frames: 60 of 60 one field apart. The earlier
      "stalled" count included a new motion starting after the last.
- [x] Box entrance dropped one frame of 6 (fields=01121, 5 frames):
      1. PiP converted the whole canvas while a popup was open (12 ms dock
         frames). Now the popup's rectangle and the one in that buffer's
         last frame; --damage-check 418 frames, 0 and 0.
      2. The pin's morph blend: bytes and three divides per pixel, 5 ms.
         Now words, one divide per alpha pair (max 1 level from the old
         result), quarter steps kept in three slots with runs, made when
         the stack opens: morph frames 8.5 -> 1.7-3.4 ms.
      3. The box's own large frames (21 ms at near full size): runs of
         consecutive source columns copied whole when they average 4 px
         or more. Largest frame 21 -> 16 ms.
      Entrance now 6 frames, fields=011111, in all five runs.
- [ ] Dismissal: 6 frames as before, but fields=021111: a 7.4 ms dock
      frame (27426 px, the backdrop under the moved tip) lands with the
      first box frame. Not fixed.
- [x] Zoom curve: DOCK_ZOOM_TICKS (default 8). RAM:Dock8 (build 426),
      Dock10 (424), Dock12 (425) for the user to compare on the monitor.

Regression report: the zoomed row slid left and lost the pointer
(all three zoom variants, 2026-11-18).
- [x] Cause: the toolchain, not the zoom change. ~/ApolloCrossDevLocal
      (GCC-6.50-Stable) is gone; builds since 408 use GCC-6.50-Dev
      (6.5.0b). Clean builds of the committed 412 source show the same
      shift; the installed 412 still held old-compiler objects. With
      dock_core.c (or only dock_layout) at -O1 the frame matches 412.
- [x] logical_positions: focus*span/rest_pitch in 32 bits. A layout
      harness on the Vampire (12 icons, 36 px, 190%, end gap 12, every x
      0-799) now matches the host exactly; at -O2 before, 463 of 800 x
      differed (row pinned to the left padding).
- [x] Host tests cross-compiled at -O2 on the Vampire: geometry, prefs,
      profile, bob, text pass.
- [ ] test_core ran over 17 minutes without finishing (1.6 s on the Mac);
      test_interaction hung earlier (possibly the 4 KB stack). The Amiga
      stopped answering after ChangeTaskPri on the stuck test: needs a
      reset. Unknown whether more Dev miscompiles remain.
- [ ] The fixed dock itself not yet checked with --pip-dump. Built:
      Dock8 (430), Dock10 (428), Dock12 (429) in the scratchpad.

Compiler fixed, 2026-11-18. The zoomed row slid left in every build made
with GCC-6.50-Dev: bbb's opt_elim_dead_assign deleted a reload because
track_var::invalidate_mem kept a register's cached MEM over a DImode store
to the same slot (inverted offset test, store attributes required).
- [x] Patch bbb-invalidate-mem-fix.patch in ApolloCrossDev (Dev scripts),
      PR WDrijver/ApolloCrossDev#30. Toolchain rebuilt with the script:
      ~/GitRepos/ApolloCrossDev/Compilers/GCC-6.50-Dev is the only one.
- [x] dock_core.c back to the original int64_t expression; Makefile and
      README default to the ApolloCrossDev toolchain.
- [x] Layout harness on the Vampire with the installed compiler and the
      original code: 800 of 800 pointer positions equal to the host.
- [x] 720 compilations (373 C files, -O2/-O3/-Os): 703 identical, 17 keep
      loads the old pass had replaced with register copies.
- [x] All seven test suites cross-compiled with the fixed compiler at -O2
      -m68080 pass on the Vampire, test_core in full (20400 fuzz frames,
      about 27 minutes there; test_interaction about 10).
- [x] Build 434 (fixed compiler, original layout code) installed in
      SYS:WBStartup; build 412 is the one rollback
      (Disabled/DockAwesome-build412.old). Zoom curve variants for the user:
      Disabled/DockAwesome-zoom10-build432 and -zoom12-build433 (build 434
      is the 8-tick curve). 408/407/395/1731 backups deleted.
- [x] The night's "freezes" were starvation: tests at -5 starved SDIO0/1
      (-21). Build 431 had been waiting on a hidden "PiP2 is already in
      use" requester the whole time.
- [x] Only compiler left: ~/GitRepos/ApolloCrossDev/Compilers/GCC-6.50-Dev
      (rollback copy and scratch cc1 builds removed). The toolchain build
      needs a PATH without spaces (VMware Fusion's entry broke the port
      libraries' configure).
- [x] Stable scripts: same bbb-opts.c code, fix applies unchanged. A Stable
      cc1 before/after over the same 720 compilations: 709 identical, 11
      only add reloads of stack slots; no wrong code found, so the defect is
      latent there. PR WDrijver/ApolloCrossDev#31 (patch file identical to
      #30, plus the step in GCC-6.50-Stable-MacOS.sh/-Linux.sh).
- [x] Zoom curve: 8, 10 and 12 ticks looked the same to the user (the
      cubic ease-out differs by about 40 ms). Following divisors 1/4 and
      1/5 (40 -> 60/80 ms lag) gave no visible gain either. Kept 8 ticks
      and 1/3: build 434 stays installed; all variants deleted from
      SYS:WBStartup/Disabled (412 rollback kept).
- [x] Filmed hitch (IMG_2570.MOV): every ~15th camera frame unchanged, then
      a double step, at each icon crossing. A smooth input.device sweep
      (scratch Sweep tool) with per-section loop timing found the name
      label window: open 26 ms (glass 9, OpenWindow 13), close 13 ms, in
      the main loop. Build 457: label in its own task; capture and window
      check skip a poll while it works; redraws between fields wait for
      the next field. Label stage 0.1 ms, 13 ms poll waits 10 -> 2 per
      sweep, longest frame 45 -> 18 ms. Menu, quit with a label: fine.
      Installed; rollback Disabled/DockAwesome-build446.old.
- [x] Label without delay as a window still came late (user: worse than
      457). Build 465: on the bottom edge the label is an overlay in the
      dock canvas (+12 px), shown at once, following the zoom; labels made
      while idle (~11 ms each). Sweep: stalled=0, longest frame 16.5 ms,
      no backdrop recaptures; damage-check 472 frames clean. Installed;
      rollback Disabled/DockAwesome-build457.old.
- [x] Side docks: overlay label beside the icon (canvas +96 px inward for
      the widest label, capped 240 px). Right/left tested with RAM test
      configs: label placement, sweep 1 late frame, damage-check clean.
      Build 473 installed; rollback Disabled/DockAwesome-build465.old.
- [x] /simplify of today's work: label task removed (only the fallback
      used it), label outline 3x faster (caps sampled once, identical
      bytes), slots by pin, one placement and one build function. Build
      479 installed; rollback Disabled/DockAwesome-build473.old.
