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
- [x] Cause (2026-11-19, from DOpus 5 source): DOpus holds LockLayers for
      the whole drag and delivers to the window WhichLayer finds at release,
      so the late-armed receiver was never there. Fix (branch after-1.0b3):
      the receiver is armed in advance whenever the pointer is over the DOpus
      desktop, stays armed through clicks and drops, waits 1/4 s before
      re-arming, and the catch box and insertion gap exclude the label strip.
- [x] Verified on AmiKit (user drags, profile): Workbench drops messages=2
      added=2, receiver arms=3 disarms=2 over a 10-hour session.
- [x] User video (IMG_2580): icons above/beside the dock not selectable, half
      a moved icon left behind, fragments while dragging. Cause: the resting
      receiver covered the whole catch box. Fix (built 19:08): resting on the
      DOpus desktop it covers only the resting plate; Workbench hover arms
      and shapes keep the full box and the 1.0b3 cancel (other systems as
      before). Drop positions now follow the receiver's window.
- [x] Follow-ups from user videos IMG_2581-2586, each fixed and confirmed
      by the user on AmiKit (build 19:46):
      - DOpus lister icons armed the whole box: leaving it made DOpus redraw
        every desktop icon there. Once DOpus is the desktop, every arm rests
        on the plate.
      - A lister under the dock took the drop and its redraw read as drag
        image: the resting receiver lies in front of other programs' windows
        on the plate (MoveWindowInFrontOf), behind the dock's own; it re-arms
        when one comes in front.
      - Stray dot at the plate's corner: ChangeWindowBox carried the 1x1
        window's pixel; repainted from the copy once moved.
      - Icon cut at the grown plate's end: the rest rect covers the plate as
        it grows while making room.
      - Ghost image trailing along the plate's edge, then a line across it:
        the drag image is painted where the dock is opaque and inside the
        receiver, never on clear parts outside it.
      - Shell pin "Could not open": the 3.2 Shell is Shell.info alone; the
        launcher passes drawer and name for icon-only projects.
      Profiles: drops messages=6 added=2 reordered=1, receiver arms=1
      disarms=0 over the session; drag image painted (438 frames).
- [ ] Regression on Coffin, 3.2.3 and ApolloOS (Workbench drops, the gap,
      the drag image, clicks beside the dock): shared changes are the label
      strip exclusion, drop positions from the receiver's window, the corner
      repaint, and painting the drag image only where opaque or inside the
      receiver.
- [x] Lister drops on AmiKit: confirmed in IMG_2582/2583 (drops from a
      lister pinned).
- Crash dropping the GlobalTrash AppIcon (and likely the earlier hang):
  Directory Opus sends an AppIcon with no lock, and accept_drops passed the
  zero lock to NameFromLock. Lockless arguments are now skipped as bad paths
  (build 19:50). [ ] User check: drop GlobalTrash, no crash, nothing pinned.
- [x] Installed on AmiKit: build 503 (ca64844) in place of 1.0b3 at
      SYS:WBStartup/DockAwesome, started from the Shell (WBRun needs
      Workbench). RAM: test files removed. ApolloExplorerSrv left running.
- [x] ApolloOS R9.5 (2026-11-19): build 503 installed at SYS:WBStartup (the
      locked old file moved to Disabled/DockAwesome-locked-delete, delete it
      after the next boot; the two older locked files deleted). Agent drags:
      desktop icon dropped mid-dock pinned at the middle slot (messages=1
      added=1), drag image painted (5 frames), drop on the desktop and a
      click on a desktop icon pinned nothing. Config restored, test files
      removed, dock restarted with WBRun.
- [ ] Coffin and AmigaOS 3.2.3 still to re-run.
- [ ] Old bug (also in 1.0b3, reproduced twice): a second dock started while
      one runs fails with "PiP2 is already in use" and crashes the machine
      after OK, while exiting. No more crash tests on AmiKit (user). Plan:
      detect the running dock at startup and exit quietly before opening
      anything; find the crashing exit step on another system with logging.
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
- [x] Coffin: all 7 suites pass (build 479). Build 482: the label in the
      dock's image on every system (AA font made in software always); the
      window label code removed. Coffin label shown ("RiVA"), sweep 24
      late frames of 635, damage-check 223 clean. Installed in place.
      Coffin Disabled/DockAwesome-locked-delete(-479): delete after reboot.
- [ ] Not verified on Coffin: the dock's right-click menu (my agent click
      showed none, as before 2026-11-17); ApolloOS run of build 482.
- [x] AmigaOS 3.2.3 (24-bit), build 482 installed in place (old 407 and
      Disabled copies deleted): all 7 suites pass incl. full test_core;
      label in the dock's image; sweep 25 late frames of 457;
      damage-check 273 clean. RAM: cleaned.
- [x] ApolloOS: build 482 installed (old copies in Disabled deleted after
      the reboot; 479 locked, Disabled/DockAwesome-locked-delete until the
      next reboot). Label in the dock's image, sweep 0 late frames,
      damage-check 399 clean. RAM: cleaned.

## Release 1.0 beta 3 (2026-11-18)

- [x] PR #11 (review-fixes) merged into main (d7c396e); tag v1.0b3; GitHub
      prerelease "DockAwesome 1.0 beta 3" with DockAwesome-1.0b3.lha
      (167,265 bytes, SHA-256 40c7a442...0fd77bf97), packed with LhA 2.15 on
      ApolloOS; lha t passes and all five files match build/release.
- [x] Release binary (DockAwesome 1.0 (2026-11-18) beta 3) installed on
      ApolloOS. Disabled/DockAwesome-locked-delete and -locked-delete-482:
      delete after the next reboot.

## ApolloOS: left dock alignment and drag flicker (2026-11-19)

- [x] Left dock: windows under the plate showed shifted a few px. SAGA
      cannot start a PiP at x 0; the overlay now starts at x 3 (negative
      edge_padding), the plate stays on the same pixels. pip-dump matched;
      user: "Nu är det rätt!".
- [x] Drag of a desktop icon: the window border under the plate flipped
      from dark to light grey as the drag neared the dock. The press made
      the window inactive (border 128 -> 168 grey); the capture stopped at
      the press but the receiver's copy was used only near the dock. Now it
      is used from the press. User: "Perfekt!".
- [x] Drag on 3.2.3: the press redrew the window under the dock inactive
      and that border was drawn over the icons as drag image; the reference
      is now read from the screen as the gesture starts. User: "inga band".
- [x] Drop on 3.2.3: a disk dropped on the plate over a drawer was copied
      into it. The receiver without shapes arms over the plate in front of
      other programs' windows; it steps back while the wheel scrolls a
      window under the plate. User: "Nu ser det ut att fungera bra".
- [x] Reorder: dragged out and back, the gap opened where the pin last
      left the row (IMG_2593). Off the row the slot follows the pointer
      (dock_edge_row_slot). User: "Bra".
- [x] Reorder at the row ends: far more empty plate than in the middle
      (IMG_2594). The plate reaches the gap by the padding instead of
      mirroring the row. User: "Bättre".
- [x] Reorder (IMG_2595): at an end the plate jumped out in one frame while
      the icons slid, and the gap opened ~135 px left of the plate. The gap
      now animates like an icon (DockGap, also for drop previews) and
      "near" is measured against the row and plate as shown, not the row
      frozen at the press. Host tests: plate end monotonic, first step at
      most half, both ends and all edges alike; outside exactly 24 px from
      the drawn plate on every side. User: "Nu verkar allt fungera".
- [x] Desktop drop on ApolloOS (IMG_2599): a drawer's icons above the plate
      blinked out at every drop on the desktop. The shaped receiver covered
      the whole box while a desktop icon was held, and Workbench redrew
      what it had covered. It now catches over the plate alone, as without
      shapes (rest_plate for every system). Installed. Agent-tested: a
      desktop drop moves the icon, a drop on the plate pins at the centre
      slot (removed again), Ctrl-C quits cleanly. IMG_2600 (right dock):
      the same blink, because a rebuilt surface (another edge) gave the
      shaped receiver no rest_plate; every new receiver now gets one.
      Agent-tested on the right dock: a drop beside the plate lands on the
      desktop, a drop on the plate pins (removed again).
- [x] Still a blink at every desktop drop, beside the dock and in the glass
      (user, same build). Giving up the shape at each release made
      Workbench redraw what it uncovered (simple refresh: Layers always
      adds damage). The shaped receiver now rests on the plate (rest_armed,
      as on the DOpus desktop) and steps back only when a window on the
      plate changes or the wheel scrolls one. A left press on the resting
      receiver between icons is consumed with its release (no focus
      change). Agent-tested on ApolloOS: desktop drop and drop beside the
      plate land on the desktop; a drop on the plate pins (removed again),
      also after a window opened and closed over the plate (no stale band);
      a click on the plate keeps the active window; Ctrl-C quits cleanly.
- [x] User: a window dragged under the dock showed only after the release
      (the resting receiver hid it; stamp checks ran only with the buttons
      up). Now, while a press that is not a Workbench drag is held, the
      receiver steps back as soon as the windows on the plate change, and
      arms again half a second after the release. Agent-tested on the left
      dock: a drop on the plate pins (removed again). User: window drag
      works.
- [x] User: a window scrolled behind the dock did not update there. A
      scroll keeps the layer, and the ApolloOS wheel may scroll without an
      input event. The layer stamp now also covers the scrollers' knobs of
      the windows on the plate, and the shaped receiver arms only once that
      stamp has been still for half a second. Agent-tested (bottom dock):
      dragging Personal's scroller redrew its icons under the plate live
      (the receiver stepped back); a drop on the plate then pinned (removed
      again). User: "Nu är det bra!".
- [ ] Unexplained hang, ApolloOS 2026-11-20: the dock (W 240,612 800x108,
      the pre-install size) ignored drops and presses, and after Ctrl-C
      waited on one signal (wait 04000000, Ctrl-C received). Possibly the
      previous build stuck in remove_input (held/right_held/close_held set
      without their release). Gone after the user's reboot; not reproduced.
- [ ] Personal:1234-5678 (partial copy from the drop bug): ask the user
      before deleting.
- [ ] Delete SYS:WBStartup/Disabled/DockAwesome-locked-delete{,2,3,4} after
      the next ApolloOS reboot, and -locked-delete{,2,3,4} after the next
      3.2.3 boot; RAM:BreakTask and RAM:TaskList when testing is done.
- [ ] ApolloOS: recheck the drag (reference now read from the screen).
- [ ] Quick look at bottom and right docks on Coffin and 3.2.3 before the
      push (edge_padding and drag reference change apply everywhere).
- [x] Remove label (user: "samma som alla andra"): on ApolloOS the shaped
      drag preview drew Remove as a flat box in the screen's pens with
      plain text, over a dark matte; elsewhere it was a flat pill. Every
      path now builds the dragged picture with the hover labels' own
      dock_ui_label_image (glass, rim, AA text; the flat pill only as a
      fallback), and the preview window shows that picture over a
      save-under in both modes, reopening when Remove arms or disarms.
      Agent screenshot mid-drag on ApolloOS: rounded glass pill with AA
      text under the icon; prefs unchanged. User: "Ser kanon ut!".
- [x] AmigaOS 3.2.3 (no shapes), build of a4a3555, 2026-11-20, agent:
      Remove under a held pin is the hover labels' glass pill (BOB path,
      screenshot); a desktop drop moves the icon; a disk icon dropped on
      the plate pinned "1234-5678:" (removed again; prefs unchanged);
      Ctrl-C quits cleanly and WBRun starts again. AppIcons (Stop
      ApolloExplorer) are not sent to AppWindows by Workbench, as before.
- [x] Coffin (AmigaOS 3.9, no shapes), build of a4a3555, 2026-11-20, agent:
      Remove under a held pin is the glass pill (screenshot); the disk
      icon dropped on the plate pinned at the centre slot (removed again;
      prefs unchanged); a desktop drop moves the icon (moved back); Ctrl-C
      quits cleanly and WBRun starts again. -locked-delete (the previous
      build) is in use: delete after the next Coffin boot.
- [x] Coffin (IMG_2601): the plate's glass over a drawer under the dock
      showed noise for a second or two after the drawer was dragged.
      Cause (measured): the screen itself held the noise, in the receiver
      over the drawer, as the receiver armed again after the drag. It grew
      behind the drawer (the press had brought the drawer in front), and
      Layers filled that hidden part in an offscreen bitmap, then copied it
      to the screen once the receiver came in front. The backfill hook
      painted only the screen's bitmap and left that one with old memory
      (instrumented: one foreign-bitmap call, 346x28 at layer offset 210,0,
      exactly the drawer's overlap). The hook now paints any bitmap, placed
      by the message's layer-relative offset. Agent repro (input events:
      hover an icon, press the title bar, drag 150 px, twice): noise in the
      screen capture before, clean after (three runs, installed build).
      Ruled out on the way: the drag image (0 frames), PiP conversion and
      composition (damage check 0 mismatches), the capture. An earlier
      guess (armed press counted only in the client area) was reverted.
      User on Coffin: "Ser ut att funka nu".
- [x] AmigaOS 3.1.4.1 (no shapes), build of 4c6b0d3, 2026-11-21, agent:
      the disk icon dropped on the plate pinned at slot 7 and dragged out
      again (prefs as before; the obsolete "snapshot" key is dropped on
      save, by design since 2270e5a); a desktop drop moves the icon (moved
      back); the Personal drawer dragged under the plate (Intuition's
      outline drag), then the pointer on its icons: the armed receiver
      shows the drawer cleanly, twice. -locked-delete (the 10 Nov build)
      is in use: delete after the next boot.
- [x] AmiKit XE: new build installed (started with Run: WBRun reports no
      memory under the Directory Opus desktop); agent drags do not move
      Directory Opus icons, so the user looked: "Amikit såg ok ut".

## Release 1.0 beta 4 (2026-11-20)

- [x] PR #12 (after-1.0b3) merged into main (237c843); tag v1.0b4; GitHub
      prerelease "DockAwesome 1.0 beta 4" with DockAwesome-1.0b4.lha
      (171,130 bytes, SHA-256 252bfb43...02dbc8bd), packed with LhA 2.15 on
      ApolloOS; lha t passes and all five files match build/release. The
      archive's dates are ApolloOS's clock (18 May 2026).
- [x] Release binary (DockAwesome 1.0 (20.11.2026) beta 4) installed on
      ApolloOS; the old -locked-delete copies there are deleted. After
      the next boot (2026-11-21) the 0.518 -locked-delete is deleted too,
      and 1.0 beta 4 started from WBStartup by itself.
- [x] Coffin (2026-11-21): -locked-delete, -2 and -3 deleted; release
      binary (1.0 beta 4) installed and running; the previous build is now
      Disabled/DockAwesome-locked-delete: delete after the next boot.
- [x] 3.1.4.1 (2026-11-21): the 10 Nov -locked-delete deleted; release
      binary (1.0 beta 4) installed and running; the previous build is now
      Disabled/DockAwesome-locked-delete: delete after the next boot.
- [x] AmiKit XE (2026-11-21): release binary (1.0 beta 4) installed over
      the unlocked previous build (no -locked-delete needed) and running,
      started with Run.
- [x] AmigaOS 3.9 (plain, AOS39 boot, 2026-11-21): four old -locked-delete
      copies deleted; release binary installed over build 509 (now
      Disabled/DockAwesome-locked-delete: delete after the next boot) and
      running. Agent: the disk icon dropped on the plate pinned at slot 7
      and dragged out again; prefs exactly as before. The coloured dots at
      y 649-652 under the rainbow bar stay after Workbench redraws there:
      they are in the wallpaper. Release 1.0 beta 4 now runs on all six
      boots (ApolloOS, 3.2.3, Coffin, 3.1.4.1, AmiKit XE, 3.9).
- [x] Personal:1234-5678 (partial copy from the drop bug; user: "radera
      den"): already gone on 2026-11-21. No 1234-5678#? anywhere on
      Personal (searched ALL), nor in the roots of Archives, AOS32, BOOT.
      3.2.3 had no -locked-delete files.
- [x] AmigaOS 3.2.3: release binary (1.0 beta 4, from the published LHA)
      installed and running; build 518 is now
      Disabled/DockAwesome-locked-delete: delete after the next 3.2.3 boot.
- [x] NIL: handles did nothing: dostags.h says NP_Input/NP_Output already
      default to Open("NIL:") with NP_Close* TRUE. The reporter still saw
      8700000E with the test build. Change reverted. Wrong because I did not
      read dostags.h defaults before changing; lesson: check the documented
      default before adding a tag.
- [ ] Not reproducible here: AmiFire and AmiPixelShop are not on this
      AmiKit XE 11.5, and the programs the user tried gave no alert. Need the
      programs, or a hint what they have in common (overlays? packer? language).
- [x] AN_FileReclosed = 0x0700000E (exec/alerts.h): a file handle closed
      twice. Our test machine is 3.2.0 (KS 47.96), the reporter's 3.2.3
      (KS 47.115): newer dos detects it, so it is probably latent here.
      Hypothesis 2: dos opens NIL: defaults and closes them at exit, and the
      program's startup code closes them too. Launch now passes
      NP_Input/NP_Output 0 (as Workbench). Built; DockAwesome-test2.lha in
      ~/Downloads for the reporter. Not verifiable on 3.2.0.
- [ ] Reporter result for test2. If still alert: next suspects are
      UnLoadSeg on the WBStartup reply, and what DOpus passes.

## AmiKit user patch (2026-11-24)
- [x] Reviewed the handoff (Downloads/DockAwesome-AmiKit-Developer-Handoff.zip):
      launch part equals 10ab3e6 (reporter: 8700000E gone); rest applied
      as 8f55e11. Cross-build clean (-Werror), host tests pass.
- [x] AmiKit XE (2026-11-24): 8f55e11 (build 0.527) installed in
      SYS:WBStartup (old binary deleted). --caps: dopus yes, workbench_port
      no, so the Magellan motion path is active. Idle 28 s, matched
      baseline (10ab3e6 built the same way): wakeups 266 vs 264, polls
      262 vs 260, both clean on Ctrl-C. Agent input does not reach the
      dock here (Input: downs=0), so drops and hover need the user's mouse.
- [ ] Pre-existing, not from the patch: on AmiKit about one main-loop
      pass a second takes 20-65 ms in the "polls" phase, equally in the
      baseline. Find which poll (DOpus/program status?) costs it.
- [x] User on AmiKit (physical mouse, 77 s): "Tycker det såg bra ut".
      Profile: downs/ups 6/6, Workbench drops messages=1 added=1 (pin
      dragged out again, prefs unchanged), receiver presses_armed=2,
      motion_polls=921 at 26.5 ms avg (the new 10/33 ms mix). Dock
      restarted with Run (no profile); RAM: test files deleted.
- [x] ApolloOS (2026-11-24, 16-bit screen, so the 24/32-bit capture
      change is not exercised): 8f55e11 installed (release 1.0b4 renamed
      to Disabled/DockAwesome-locked-delete, WBRun binary in use). Agent
      test with --profile: hover 0 stalled frames, desktop drop pinned
      RAM Disk at slot 7 (drops added=1) and dragging it out removed it;
      prefs as before; clean Ctrl-C. Restarted with WBRun; RAM: clean.
      The idle 20-60 ms polls passes show here too (pre-existing).
- [x] Delete SYS:WBStartup/Disabled/DockAwesome-locked-delete after the
      next ApolloOS boot (done 2026-11-25, see "ApolloOS rerun with build
      550").
- [x] Coffin (2026-11-24, 24-bit Workbench, so the new capture path runs
      without the Magellan motion path): the old -locked-delete deleted;
      8f55e11 installed (release 1.0b4 is the new
      Disabled/DockAwesome-locked-delete). Agent test with --profile:
      hover 1 stall in 38 frames, desktop drop pinned Ram Disk (added=1),
      dragging it out removed it, prefs as before, clean Ctrl-C.
      Restarted with WBRun; RAM: clean.
- [ ] Delete SYS:WBStartup/Disabled/DockAwesome-locked-delete after the
      next Coffin boot.
- [ ] Open (not compared with baseline under hover): during hover
      animation about every third frame spends ~23 ms in the polls phase
      (surface->poll, live capture), 109 of 743 frames stalled. The
      animation rate is 10 ms before and after the patch, so probably
      pre-existing; an A/B hover run with the user's mouse would settle it.
- [x] AmigaOS 3.2.3 (2026-11-24, 24-bit Workbench): the old -locked-delete
      deleted; 8f55e11 installed (release 1.0b4 is the new
      Disabled/DockAwesome-locked-delete: delete after the next 3.2.3 boot).
      Agent test with --profile: hover 0 stalled frames, desktop drop pinned
      RAM Disk at slot 8 (added=1), clean Ctrl-C. Dragging it out failed at
      y 700 with the patch AND with release 1.0b4 (downs=0: the press was
      below the icons at size 36); at y 680 it worked (downs=1), prefs as
      before. Restarted with WBRun; RAM: clean.
- [x] Patch verified on all four boots it touches (AmiKit, ApolloOS,
      Coffin, 3.2.3). Ready to push with the branch.

## Release 1.0 beta 5 (2026-11-24)
- [x] PR #13 (after-1.0b4) merged into main (989cfa7); tag v1.0b5; GitHub
      prerelease "DockAwesome 1.0 beta 5" with DockAwesome-1.0b5.lha
      (171,903 bytes, SHA-256 79f5d996...63088c0b9), packed with LhA 2.15
      on AmigaOS 3.2.3; lha t passes and all five files match build/release.
- [x] AmigaOS 3.2.3: release binary (1.0 beta 5) installed and started with
      WBRun; both old binaries (1.0b4, 0.527) deleted; RAM: clean.
- [ ] Install the release binary on AmiKit XE, ApolloOS and Coffin (they run
      0.527, the same code) at their next boot, and delete the
      Disabled/DockAwesome-locked-delete left on ApolloOS and Coffin.

## Folder Stacks for every pinned drawer, no divider (2026-11-24)
Plan: tasks/plan-folder-stacks.md (D1-D17). Branch folder-stacks, one PR.
- [x] §4.2 goldens filtered from the 384-entry table (keep i with bit 2
      clear, new index ((i>>3)<<2)|(i&3)) before the renderer changed.
- [x] D1 divider out of the core (end_gap, extra_gap, draw_divider,
      add_divider, STACK_DIVIDER_GAP).
- [x] D2 dock_prefs_migrate; "folder" read, never written; startup saves
      once after migrating (not while a pin is missing).
- [x] D3-D9 every pinned directory (not a volume root) is a stack; append
      at the end; stacks move and are removed like pins; open-stack
      identity by pin id; a click on another stack closes the box and
      opens the other; no running state for stacks.
- [x] D10 stack menu: Show Contents, Open in Workbench (DOCK_UI_OPEN_DRAWER).
- [x] D11 Settings without the Folder Stack section, ASL and blocking hook.
- [x] D12-D15 close button on the first stack, round-robin prefetch with
      the real page size, 512-entry hashed cache without prefetch
      eviction, profile line.
- [x] D16 host tests; D17 documents.
- [x] Self-review against plan §3 (one fix: an open box is dismissed when
      its stack is removed).
- [x] make host-test, make host-sanitize (after replacing a sprintf the
      sanitizer build rejected), make amiga: green, no warnings.
- [x] impeccable polish (in-thread; its detector is web-only): Settings is
      Size, Scale, Position, Cancel/Save with equal 12 px margins above and
      below; the stack menu's two rows are sized by the longer label. No
      change needed; both checked in screenshots on ApolloOS.
- [x] Agent smoke run, ApolloOS R9.5 (build 530, --config RAM:mig.json, a
      copy of the user's 1.0b5 file with "folder": "Personal:Downloads"):
      --caps identical to the baseline; after start the file had no
      "folder" and Personal:Downloads last in "paths"; a second start left
      the file untouched. The stack opened (first paint 104.8 ms) and closed
      by its pin. A dropped volume (Archives:) opened a window, its menu
      showed Close. A dropped drawer (Archives:Games) became a stack; with
      it open, one click on Downloads closed it and opened Downloads. Menu
      rows "Show Contents" and "Open in Workbench" and the box's tile all
      worked; with the Downloads window open the menu showed no Close.
      Reordered a stack to slot 0 (saved first), dragged a stack out
      (removed from the file). Settings had no Folder Stack section; Save
      wrote the file. Profile: prefetch stacks=1 pending=0 reads=63
      cache=63/512 evicted=0; Input downs=7 ups=7. --pip-dump: the first and
      last of 13 icons 492 px apart = 12 x 41 (5 px gaps, no divider).
- [x] Opus round 1: no P0/P1; one P2 (a legacy "folder" that no longer
      loads blocks saving like any missing pin) is plan assumption A5, kept
      as designed and listed for the owner. No code change, so no round 2.
- [x] Codex round 1: P2, a finished prefetch slot kept its old first page
      after Scale moved the anchor; fixed (slot starts over when the
      anchor moves, 3f1fe03). Codex round 2: no findings.
- [x] Hardware matrix §4.4, ApolloOS R9.5 (Kickstart 51.51, core 12856),
      builds 530 (steps 2-8, smoke run above) and 533 (reviewed):
      1 --caps of build 533 identical to the 1.0b5 baseline (diff).
      2 migration passed (smoke run; again in each build-533 run below).
      3 no divider: 13 icons at a 41 px pitch (--pip-dump, build 530).
      4-8 passed with build 530 (smoke run). Build 533 differs only in the
      prefetch slot reset; with it three stacks opened and closed, and the
      user's real file migrated.
      9 build 533 installed in place (SYS:WBStartup/DockAwesome), started
      with WBRun, its Downloads stack opened from the dock; RAM: clean.
      ENVARC:DockAwesome.json migrated: no "folder", Personal:Downloads
      last. The old 0.527 binary is SYS:WBStartup/Disabled/
      DockAwesome-locked-delete (in use until reboot).
- [x] Performance, ApolloOS (extra; not one of the required boots), build
      533 against main 989cfa7 built in a worktree, same config (13 pins
      including Archives:Games and ApolloOS:Prefs, "folder" Downloads),
      45 s idle with the pointer off the dock before the first click:

      | run | stack | first paint ms | entrance ms |
      | --- | --- | --- | --- |
      | old 1 | Downloads | 110.5 | 148.3 |
      | new | Downloads | 106.3 | 132.2 |
      | new | Games (30 entries, same box) | 109.9 | 149.9 |
      | new | Prefs (smaller page) | 70.3 | 153.4 |
      | old 2 | Downloads | 110.2 | 174.7 |

      New: prefetch stacks=3 pending=0 reads=81 cache=81/512 evicted=0.
      Pass: every new value is under the old maximum (110.5 / 174.7).
      Hover, --bench-sweep 8 (synthetic sweep), two old/new/old sets:
      render_us_avg 6800/6802/6793 and 7091/6967/7106; stalled 0 in all
      six runs. New lies within the spread of the old runs (6793-7106).
- [ ] NOT RUN: matrix on Coffin, AmigaOS 3.2.3, 3.9, 3.1.4.1 and AmiKit XE;
      first-open comparison on AmiKit; hover comparison on Coffin and
      AmiKit. Only ApolloOS was booted and no user was available for
      reboots or the AmiKit gestures.
- [x] Delete SYS:WBStartup/Disabled/DockAwesome-locked-delete after the
      next ApolloOS boot (done 2026-11-25, build 550 rerun).
- [x] Fable final pass: no code blocker; merge blocked only by the hardware
      not run (above) and the owner's decision on A5.
- [x] Handover (tasks/handover-folder-stacks-pr1.md), release-notes draft.
- [x] One push; PR opened as a draft, left a draft (blocked on hardware).

### Owner decision A5: missing migrated Folder Stack is dropped (ApolloOS, build 0.534)
- [x] Code: App.migrated_pin; a pin appended by the migration whose drawer cannot be locked is skipped at start; the first save removes it.
- [x] make host-test green, make amiga builds without warnings.
- [x] ApolloOS, --config RAM:ta.json with "folder": "RAM:DoesNotExist": no message, dock runs, file after start has paths ["Personal:Downloads"] and no "folder".
- [x] Control, "folder": "SYS:Prefs" not in paths: appended last as ApolloOS:Prefs, no "folder" key.
- [ ] Not run: the other boots, a host test for the missing-drawer case (the skip lives in amiga_main.c).

### Folder Stack Sort by per stack (2026-11-25, owner request)
Scope: the stack's right-click menu gets Sort by Name / Date Modified /
Kind (check mark on the active one) above Show Contents and Open in
Workbench, saved per stack; the read-ahead uses the stack's order.
Out of scope: Date Added/Created (not in AmigaDOS), Display as, View
content as, Options.
- [x] dock_sort.c/h (host): Stricmp-style fold (ASCII + Latin-1 to
      capitals, so "_tmp" stays after "zebra" as before), newest first by
      the entry's own DateStamp (not its .info), drawers then extension
      (none first) then name; ties end in byte order (total order).
- [x] amiga_folder_read(path, sort, ...): reads the whole drawer as before,
      sorts by name to pair .info files, then in the stack's order. No
      extra directory reads.
- [x] Prefs: "sorts": {path: key}, only non-name stacks written; read by
      path (case-insensitive), before or after "paths"; unknown keys,
      non-string values, other shapes read as name. Item.sort follows the
      Item through reorder/removal; save_config writes it; migration
      appends the old Folder Stack as name.
- [x] UI: drawer_items with a muted heading, three sort rows, a 7 px
      separator row; variable row tops (popup_top), check column,
      heading/separator inert (a click keeps the menu). Other menus keep
      their geometry (top 3+i*row, width label+24).
- [x] Prefetch: slot keyed by sort too; a new choice saves at once (reverts
      if the save fails) and re-arms that stack.
- [x] Tests: tests/test_sort.c (fold, date, kind, permutation stability,
      keys); test_prefs test_sorts (write format, round trip, order of
      keys, case, unknown/odd values, duplicate keys, reorder, removal,
      hand-edited order, migration) and bad files.
- [x] 1.0 beta 5 compatibility: v1.0b5's dock_prefs.c (git archive) built
      on the host read a file written by this build with "sorts" (paths
      intact).
- [x] make host-test, make host-sanitize green; make amiga clean rebuild,
      no warnings (build 537).
- [x] impeccable polish (in-thread; detector is web-only): separator inset
      10 px both sides like the labels; checked on ApolloOS screenshots.
- [x] ApolloOS R9.5, build 537 from RAM:, --config RAM:t1.json (copy of the
      user's file, Downloads stack, 35 entries on one page):
      Name order as before; menu showed the check on Name; Date Modified
      saved "sorts": {"Personal:Downloads": "date"} and the box showed
      newest first (SAGADriver.readme/.lha first: their DateStamp is day
      46751 = 2106, List shows "01-Jan-06"); Kind saved "kind": drawers,
      files without extension, .adf, .jic, .key, .lha, .readme, .zip.
      A click on the heading kept the menu. After a restart the box kept
      Kind and the menu showed the check on Kind; Date the same.
- [x] ApolloOS, RAM:t2.json (CLI, Games date, Downloads kind, Prefs):
      Downloads dragged to slot 0 -> file order Downloads, CLI, Games,
      Prefs with Downloads kind / Games date; Games dragged out -> gone
      with its sort, Downloads kind kept; menus: Downloads Kind, Prefs Name;
      Show Contents still opens the box.
- [x] ApolloOS, RAM:t3.json with "folder": "Archives:Games" and "sorts"
      before "paths": after start Games last, "folder" gone, Downloads
      "date" kept.
- [x] First open after 45 s idle, fresh dock per run, Downloads (Input
      downs/ups matched, prefetch pending=0 evicted=0 each run):

      | sort | first paint ms | entrance ms |
      | --- | --- | --- |
      | Name | 106.9 | 132.6 |
      | Name | 107.7 | 132.8 |
      | Date Modified | 107.7 | 137.6 |
      | Date Modified | 108.1 | 131.6 |
      | Kind | 117.2 | 144.2 |
      | Kind | 109.1 | 132.8 |

      Reference build 533 Name 106.3 / 132.2; 1.0 beta 5 max 110.5 / 174.7.
      One Kind run (117.2) is above that maximum; the repeat was 109.1, so
      it reads as run noise (one run is noise, lessons). Every other run is
      under 110.5.
- [x] Large drawer, ApolloOS:C (253 entries, 3 pages of 119), timing probe
      at ChangeTaskPri -30, 10 reads each, whole amiga_folder_read and
      the sort alone:

      | sort | read+sort ms | sort alone ms |
      | --- | --- | --- |
      | Name | 102.1 | 4.3 |
      | Date Modified | 108.4 | 6.6 |
      | Kind | 116.4 | 14.6 |

      Downloads (30): 16.8 / 17.2 / 17.9 ms. The directory read dominates;
      sorting by pointers instead of moving the 128-byte entries measured
      no gain (14.0 ms Kind) and was reverted. In the dock the box of
      ApolloOS:C opened in every order (first paint 335-384 ms, set by how
      many of the 119 icons were cached), dock frames during its motion
      us_avg=4049, prefetch evicted=0.
- [x] Opus review: one P2 (a press on the heading/separator did not
      disarm an earlier press, so a later drag onto that row fired it);
      fixed in ad454f3; on ApolloOS (build 538) press Show Contents ->
      release on the separator, press heading -> release on Show Contents:
      the menu closed, no box opened.
- [x] Codex review (base main): no findings; tree unchanged after it.
- [x] Build 538 installed in place (SYS:WBStartup/DockAwesome, WBRun,
      running); the user's ENVARC:DockAwesome.json has no "sorts" (all
      Name) and its Downloads menu shows Name checked. RAM: clean.
- [x] Delete SYS:WBStartup/Disabled/DockAwesome-locked-delete,
      -locked-delete2 and -locked-delete3 after the next ApolloOS boot
      (done 2026-11-25: not locked after the reboot, deleted).
- [ ] NOT RUN: Coffin, AmigaOS 3.2.3, 3.9, 3.1.4.1, AmiKit XE (not booted).

### Coffin hardware matrix and performance (2026-11-25)
Coffin r65 on the Vampire (Kickstart 45.57, Workbench 45.5, core 12856,
24-bit 1280x720), agent input. Builds: 539 (HEAD 57d6ef8) for steps 1-2,
540 (= 539 + the release-hit fix below) for the rest; baseline tag v1.0b5
(989cfa7) built in a scratch worktree outside the repo. All destructive
runs used --config RAM:<file>.json; the user's ENVARC:DockAwesome.json
(1.0b5 format, "folder": "Personal:Downloads") was only copied.
- [x] 1 --caps: installed 0.527, new 539 and v1.0b5 identical (Diff).
- [x] 2 Migration: a copy of the user's file -> after start no "folder",
      Personal:Downloads last in "paths"; a second start left it
      byte-identical (Diff).
- [x] 3 No divider (--pip-dump, 18 icons): icon lefts at 135 + 37k up to
      the Downloads stack at 764, so the last two icons are 5 px apart.
      v1.0b5 with the same file: 17 icons at 37 px, then 12 px more before
      its Folder Stack.
- [x] 4 Drop a drawer (Archives:Games) -> stack appended last; click
      opens, a click on its pin (close button) closes; with two stacks a
      click on the other closes one and opens the other, both ways (after
      the fix below; before it, only the box closed).
- [x] 5 Drop a volume (Ram Disk:) -> click opened its window; its menu
      showed Close, which closed the window.
- [x] 6 Stack menu: Sort by heading, Name/Date Modified/Kind with the
      check, separator, Show Contents, Open in Workbench; no Close, also
      with the drawer's window open. The box's Open in Workbench tile and
      the menu row each opened a Workbench window.
- [x] 7 Downloads dragged to slot 0 (saved first); Games dragged out
      (gone from "paths").
- [x] 8 Settings: Size, Scale, Position, Cancel/Save, no Folder Stack
      section; Save rewrote the file and kept "sorts".
- [x] Sort by: Date Modified saved "sorts": {"Personal:Downloads":
      "date"}, box newest first (SAGA readme/lha first, as on ApolloOS);
      Kind saved "kind"; after a restart the box kept Kind (drawers, no
      extension, .adf, .jic, .key, .lha, .readme, .zip) and the menu showed
      Kind checked; a click on the heading kept the menu; Name removed the
      "sorts" key. The other stack's menu kept Name.
- [x] A5: "folder": "RAM:DoesNotExist" -> no message, dock running, file
      saved with paths [EditPad, Personal:Downloads] and no "folder".
- [x] Defect found (also in v1.0b5): with a box open, a quick click on
      another pin only closed the box; the other stack did not open and a
      program pin (EditPad) did not start; a 1 s hold worked. Diagnostic
      build: the press reached apply_gesture with the right pin, the
      release came with hit_id=0, because the box's closing motion
      (started by the press activating another window) made
      input_publish withdraw the handler's snapshot. Fix: the handler marks
      such a release INPUT_HIT_UNRESOLVED and the main task hit-tests the
      row it drew (still while the UI holds the dock). Build 540: switch
      Games->Downloads and Downloads->Games with quick clicks, EditPad
      started and the box closed, the open stack's own pin still only
      closes it.
- [x] make host-test, make host-sanitize green; make amiga no warnings
      (build 540).
- [x] Opus review of the fix: no P0/P1. P2 fixed: message() now cancels a
      press held when its requester opens (else that press's release came
      unresolved and launched after OK); comment says "the row as last
      drawn". P2 not fixed (older, same root, at the press): a press during
      the ~130 ms box motion is not taken by the handler and goes to what
      lies under the dock; left for the owner (changing what "the UI holds
      the dock" means for input touches every caps branch).
- [x] Build 541 (540 + those P2 fixes): host-test, host-sanitize green,
      make amiga no warnings. Coffin, RAM:tq.json: Downloads->Games switch
      with quick clicks, EditPad started from an open box; a double-click
      on a pin whose file was deleted showed one requester and nothing
      after OK (Input downs=4 ups=4).
- [x] First open after 45 s idle, pointer off the dock, fresh dock per
      run, config = the user's file + Archives:Games + System:Prefs
      (old: 19 pins + Folder Stack Downloads; new: migrated, 3 stacks):

      | run | stack | first paint ms | entrance ms |
      | --- | --- | --- | --- |
      | old 1 (v1.0b5) | Downloads | 101.4 | 135.9 |
      | new (540) | Downloads | 85.1 | 134.5 |
      | new (540) | Games (30 entries, same box) | 86.4 | 133.2 |
      | new (540) | Prefs (58 entries, larger box) | 141.1 | 154.0 |
      | old 2 (v1.0b5) | Downloads | 89.6 | 130.3 |

      New: prefetch stacks=3 pending=0 reads=120 cache=120/512 evicted=0.
      Pass: Downloads and Games under the old maximum (101.4 / 135.9);
      Prefs has a larger first page (exempt by §4.5).
- [x] Hover, agent sweep (relative moves to x 350, 500, 650, 800, 950,
      then off, one per message, after 45 s idle), old/new/old:
      render_us_avg 10400 / 9410 / 10525, stalled 3 / 3 / 4, redraws
      101 / 100 / 100. Each old run holds one ~104 ms frame (max 103745 /
      104206; without it ~9466 / ~9579); new max 18760. New stalled within
      the old spread; render average below it.
- [x] Hover, --bench-sweep 8, old/new/old/new/old: render_us_avg 7898 /
      5546 / 7435 / 5900 / 5805, stalled 50 / 49 / 49 / 51 / 49 of
      195-199 frames (the synthetic sweep stalls a quarter of its frames
      on this 24-bit screen with either build).
- [x] 9 Build 541 installed in place: SYS:WBStartup/DockAwesome.new
      copied, the boot's 0.527 (in use) renamed to
      SYS:WBStartup/Disabled/DockAwesome-locked-delete (the older one there
      from Saturday deleted), .new renamed into place, started with WBRun;
      its Downloads stack opened and closed from the dock; Status shows no
      other dock. ENVARC:DockAwesome.json migrated by it (no "folder",
      Personal:Downloads last); the tests only copied it. RAM: clean; the
      v1.0b5 binary deleted from RAM:, its worktree removed on the Mac.
- [x] Delete SYS:WBStartup/Disabled/DockAwesome-locked-delete (0.527) on
      Coffin after its next boot (done 2026-11-25, not locked).
- [ ] NOT RUN on Coffin: nothing of §4.4/§4.5 is left; AmigaOS 3.2.3,
      3.9 (plain), 3.1.4.1 and AmiKit XE not booted; ApolloOS not rerun
      with build 540.

### AmigaOS 3.2.3 hardware matrix and performance (2026-11-25)
AmigaOS 3.2.3 on the Vampire (Kickstart 47.115, Workbench 47.5, core
12856, 24-bit 1280x720), agent input (Input downs=11 ups=11 in the main
run). Build 541 (HEAD e6f433b; `make amiga` had nothing to rebuild);
baseline tag v1.0b5 (989cfa7) built in a scratch worktree outside the
repo. All destructive runs used --config RAM:<file>.json; the user's
ENVARC:DockAwesome.json (1.0b5 format, "folder": "Personal:Downloads") was
only copied (its date stayed 24-Nov 23:30:55 until the installed build
started).
- [x] 1 --caps: installed 1.0 beta 5 and 541 identical (diff on the Mac).
- [x] 2 Migration: a copy of the user's file -> after start no "folder",
      Personal:Downloads last in "paths"; a second start did not rewrite
      it (cmp against a copy: identical).
- [x] 3 No divider (--pip-dump, 13 icons): icon centres 41 px apart
      (36 + 5) up to the Downloads stack, last two 620 / 661.5.
- [x] 4 Drop a drawer (Archives:Games) -> stack appended last; a click
      opens it, a click on its pin closes it. Switch Games -> Downloads and
      Downloads -> Games with quick clicks and with 1 s holds, both ways.
      With the Games box open, a quick click on the TextEdit pin closed the
      box and started TextEdit (fix 32e645f).
- [x] 5 Drop a volume (RAM Disk:) -> a click opened its window; its menu
      showed Close, which closed the window.
- [x] 6 Stack menu: Sort by heading, Name (checked), Date Modified, Kind,
      separator, Show Contents, Open in Workbench; no Close, also with the
      drawer's window open. The box's Open in Workbench tile and the menu
      row each opened a Workbench window; Show Contents opened the box.
- [x] Sort by: a click on the heading kept the menu; Date Modified saved
      "sorts": {"Personal:Downloads": "date"}, box newest first (SAGA
      readme/lha first, as on the other boots); Kind saved "kind" (drawers,
      no extension, .adf, .jic, .key, .lha, .readme, .zip); the Games menu
      kept Name; after a restart the box kept Kind and the menu showed Kind
      checked; Name removed the "sorts" key.
- [x] 7 Downloads dragged to slot 0 (first in "paths", "kind" kept);
      Games dragged out (gone from "paths").
- [x] 8 Settings: Size, Scale, Position, Cancel/Save, no Folder Stack
      section; Save rewrote the file and kept "sorts".
- [x] A5: "folder": "RAM:DoesNotExist" -> no requester, dock running, file
      saved with paths [TextEdit, Personal:Downloads] and no "folder".
- [x] First open after 45 s idle, pointer off the dock, fresh dock per
      run, config = the user's file + Archives:Games + SYS:Prefs (old: 14
      pins + Folder Stack Downloads; new: migrated, 3 stacks):

      | run | stack | first paint ms | entrance ms |
      | --- | --- | --- | --- |
      | old 1 (v1.0b5) | Downloads | 98.7 | 134.3 |
      | new 1 (541) | Downloads | 93.3 | 145.4 |
      | new 1 | Games (same box) | 92.7 | 146.3 |
      | new 1 | Prefs (smaller box) | 72.8 | 142.2 |
      | old 2 | Downloads | 99.0 | 137.6 |
      | new 2 | Downloads | 94.3 | 134.7 |
      | new 2 | Games | 92.9 | 134.0 |
      | old 3 | Downloads | 110.2 | 146.8 |

      New: prefetch stacks=3 pending=0 reads=86 cache=86/512 evicted=0.
      Pass: every new value is under the old maximum (110.2 / 146.8).
      new 2's Games entrance drew one frame (134 ms, fields=0); not seen in
      the ~20 other entrances this session. A discarded new run had the
      pointer left on the dock from the previous click: reads=0 pending=3
      (prefetch waits while the pointer is on the dock), 86.4 / 133.6.
- [x] Hover, --bench-sweep 8 at ChangeTaskPri -30, old/new/old/new/old:
      render_us_avg 6799 / 6602 / 6804 / 7707 / 7889, stalled 41 / 41 /
      42 / 41 / 44 of ~166 frames. Pass.
- [x] Hover, agent sweep (relative moves to x 350, 500, 650, 800, 950,
      then off, one per message, after 45 s idle): FAILED with build 541;
      cause found and fixed in build 542, see "Hover pacing on AmigaOS
      3.2.3" below (not a Folder Stack effect: v1.0b5 shows it with the
      same row).
      At ChangeTaskPri -30, old/new/old/new/old: render_us_avg 10076 /
      12049 / 9569 / 14718 / 9557, stalled 1 / 6 / 0 / 14 / 0. At the
      shell's priority (1), old/new: 9549 / 12087, stalled 0 / 7.
      Isolated: the slow frames are the label frames while the pointer
      rests at x 800 (the SD-Card pin, beside the Games and Prefs stacks):
      "convert" 9-30 ms instead of ~2 ms. Control, same build with Games
      and Prefs replaced by two program pins: normal (10788 / 1 stall and
      10171 / 0); v1.0b5 at the same rest: normal. A task sampler (pri 100,
      2 ms) showed no other task taking more CPU than in the control. A
      diagnostic build (scratch worktree, not committed) counted 23 draws,
      all on a field pass, vblank rate ok, and 9 back-buffer retirement
      waits: amiga_pip_back_buffer WaitTOFs because GfxBase->VBCounter
      still equals the last present's field, so the time is a wait for a
      field, not CPU. That wait is unchanged since 1.0 beta 5. Why the
      stacks beside the hovered pin trigger it was not found; not fixed.
- [x] 9 Build 541 installed in place: SYS:WBStartup/DockAwesome.new
      copied, the 1.0 beta 5 binary deleted (not locked), .new renamed
      into place, started with WBRun; its Downloads stack opened and closed
      from the dock; no CLI dock in Status. ENVARC:DockAwesome.json
      migrated by it (no "folder", Personal:Downloads last). RAM: clean;
      the v1.0b5 binary and all tools deleted; both scratch worktrees
      removed on the Mac. SYS:WBStartup/Disabled holds only the user's
      WBDock.
- [ ] NOT RUN on 3.2.3: nothing of §4.4/§4.5 is left except the failed
      agent hover comparison above. Still not booted: plain 3.9, 3.1.4.1,
      AmiKit XE.

### Hover pacing on AmigaOS 3.2.3 (2026-11-25)
Build 542 (541 + this fix), AmigaOS 3.2.3 on the Vampire, agent input,
ChangeTaskPri -30, all runs with --config RAM: copies (ENVARC untouched).
Configs: pn = the user's 12 pins + Archives:Games + AOS32:Prefs +
Personal:Downloads (3 stacks, Games and Prefs beside SD-Card); pl = the
same 15 paths for v1.0b5 (no "folder": the same evenly spaced row, canvas
896, Games/Prefs/Downloads plain drawer pins); po = v1.0b5 with Downloads
as its Folder Stack (divider, canvas 928), as in the earlier runs.
- [x] Root cause (pre-existing since 1.0 beta 5, in the frame pacing, not
      in the stacks): when one frame's present lands after the next VBlank
      (wake 3.5-4 ms + compose 10-12.5 ms at the large zoom levels + convert
      > 20 ms), its flip waits for the following field. The pass for the
      field already crossed is pending, so the next frame starts at once,
      composes, and blocks in `amiga_pip_back_buffer`'s WaitTOF until the
      next field; its present again lands just after a VBlank, and so on:
      every later frame of the motion waits 9-30 ms (every fourth or so
      misses a field when the check races the VBlank), until the row
      rests. Trace (build 541 and v1.0b5 pl): one frame with done_us
      17-20.7 ms, then convert_us 9-30 ms on every frame of the long tick-8
      tail at the x 800 / x 950 rests.
- [x] Evidence it is not the stacks: v1.0b5 with the same 15-pin row (pl,
      no stack code involved) fails the same way, render_us_avg 13508 /
      13561 / 10679, stalled 10 / 10 / 4. The divider of the old layout put
      other icons under x 800, which is why v1.0b5 (po) looked normal; po
      also shows the cascade sometimes (this session 10931, stalled 3, with
      35 ms frames). The earlier control (Time/Locale in the stacks'
      places) changed the artwork at that rest, so compose stayed short.
      Ruled out earlier and still: prefetch (pending=0, reads=86 before the
      sweep; it only runs while the pointer is off the dock), other tasks
      (sampler), CPU in the main task.
- [x] Fix: `amiga_pip_back_busy()`; a field pass that finds the last
      present in the current field moves its redraw to the next field
      (once per redraw; the WaitTOF stays as the safeguard). Profile line
      `Frame pacing:` gains `retire_waits` (presents that waited) and
      `retire_defers`.
- [x] Hover agent sweep, pn/pl, order L1 (moves sent together) / F1 / L2 /
      F2 / L3 / N1 / F3, plus po:

      | run | build | config | render_us_avg | stalled | max us |
      | --- | --- | --- | --- | --- | --- |
      | L1 | v1.0b5 | pl | 13508 | 10 | 37116 |
      | F1 | 542 | pn | 9695 | 2 | 18533 |
      | L2 | v1.0b5 | pl | 13561 | 10 | 36706 |
      | F2 | 542 | pn | 9697 | 3 | 18165 |
      | L3 | v1.0b5 | pl | 10679 | 4 | 35921 |
      | N1 | 541 | pn | 14096 | 9 | 112418 |
      | F3 | 542 | pn | 9711 | 2 | 18172 |
      | P1 | v1.0b5 | po | 10931 | 3 | 36300 |

      542: retire_waits=0, retire_defers=2 in each run; the x 800 / 950
      tails convert in ~1.8 ms. Pass: 542 below every old run.
- [x] First open (45 s idle, pointer off the dock, Downloads): 542 first
      paint 95.8 / 95.5 ms, entrance 139.5 / 138.6 ms; 541 this session
      95.4 / 144.5; v1.0b5 (po) 98.1 / 138.8. prefetch pending=0 reads=86
      evicted=0. Pass (under the old maximum of the earlier 3.2.3 table).
- [x] --bench-sweep 8 (ChangeTaskPri -30), v1.0b5 po / 542 pn / v1.0b5 po:
      render_us_avg 6836 / 6582 / 6826, stalled 41 / 41 / 42 (542
      retire_waits=0). Pass.
- [x] make host-test, make host-sanitize green; make amiga no warnings
      (build 542); Opus review of the delta: no P0/P1/P2.
- [x] Build 542 installed in place on 3.2.3: copied to
      SYS:WBStartup/DockAwesome.new, 541 deleted (not locked), renamed,
      started with WBRun; FindTask finds DockAwesome. RAM: clean (only
      amiagent's RAM:T files); ENVARC:DockAwesome.json not written by the
      tests (date 10:45:25, from 541's migration); scratch worktree removed.
- [ ] Not run: Coffin and ApolloOS with 542 (the fix is in the shared PiP
      path; Coffin's earlier sweep had stalled 3 / 3 / 4 with either
      build); AmiKit XE, plain 3.9, 3.1.4.1 not booted.


### AmiKit XE: checks without gestures (2026-11-25)
AmiKit XE 11.5 on the Vampire (Kickstart 47.96, Workbench 47.2, core
12856, 24-bit 1280x720, Directory Opus desktop; the Amiga clock says
02-Aug-25). Build 542 (HEAD 98bbf1a, `make amiga` had nothing to rebuild);
baseline tag v1.0b5 (989cfa7) built in a scratch worktree outside the
repo. All runs used --config RAM: copies; ENVARC:DockAwesome.json (1.0b5
format, "folder": "Personal:Downloads") was only copied.
- [x] Agent input: an amiga_click on a pin gave `Input: downs=0` (as
      before on AmiKit). Agent pointer moves DO reach the dock: hover is
      polled from the screen's pointer (redraws=20 after two moves), so
      the hover sweep below was run by the agent.
- [x] 1 --caps: installed 0.527, 542 and v1.0b5 identical (diff on the
      Mac): dopus yes, workbench_port no, shaped_windows no, depth 24.
- [x] 2 Migration: a copy of the user's file -> after start no "folder",
      Personal:Downloads last in "paths"; a second start did not rewrite it
      (date unchanged). Every pinned drawer is now a stack: 5 stacks
      (AmiKit:Utilities/RiVA, AmiKit:Utilities/AmigaAMP, Personal:Pictures,
      Personal:Demos, Personal:Downloads); RiVA and AmigaAMP opened a
      window in 1.0 beta 5 (D3 as planned).
- [x] A5 + Sort by persistence: "folder": "RAM:DoesNotExist", paths
      [SYS:System/Shell, Personal:Pictures], "sorts": {"Personal:Pictures":
      "kind"} -> no requester (screenshot), dock running, file saved with
      the two paths, no "folder", "sorts" kept.
- [x] 3 No divider (--pip-dump, 10 icons, canvas 704x114): icon lefts
      151 + 41k (36 + 5), the last two (Demos, Downloads) 41 px apart;
      v1.0b5 with the same file: 41 px up to Demos, then 53 px (12 px
      divider) to its Folder Stack.
- [x] --bench-sweep 8 at ChangeTaskPri -30 (render_us_avg / compose_us_avg
      / stalled):

      | config | v1.0b5 | 542 |
      | --- | --- | --- |
      | user's file (old: divider + Folder Stack) | 7112/5248/42, 7061/5224/39, 7052/5222/37 | 7359/5716/38, 7382/5737/37 |
      | same 10-pin row (old: Downloads a plain pin) | 6524/4826/40, 6521/4832/36, 7348*/4832/36 | 7391/5737/38, 7374/5732/41 |
      | 5 pins, no drawers (no stacks) | 5567/4164/20, 5580/4176/20 | 6248/4860/20, 6237/4840/20 |

      (*one 100 ms frame.) Stalled frames equal. FINDING: compose is
      ~0.7-0.9 ms (+16-19 %) slower per frame in 542 with the same row,
      also with no stack at all; the installed 0.527 matches v1.0b5
      (4177 / 4837). Same 5-pin row per commit: v1.0b5 4164, 3f1fe03 4269,
      70cc053 4444, 10843c9 4767, 32e645f 4779, 98bbf1a 4867. The compose
      code itself changed only by the divider's removal, and 70cc053 (+175
      us) touches only amiga_main.c/amiga_app.h, which link before
      dock_compose.o: points to code or data placement, not to stack
      logic. Cause not found; open for the owner.
- [x] Hover, agent sweep (relative moves to x 350, 500, 650, 800, 950 at
      y 690, then off, one per message, after 45 s idle; x 350 and 950 lie
      beside this 10-pin plate), old (same row) / new / old / new / old:
      render_us_avg 7034 / 8497 / 10629* / 10700* / 6914, stalled 1 / 0 /
      2 / 1 / 1, redraws 41 / 41 / 41 / 41 / 39 (*one ~100 ms frame each;
      without it 6849 / 8301 / 8421 / 8452 / 6715). 542 retire_waits=0,
      prefetch stacks=5 pending=0 reads=153 cache=153/512 evicted=0.
      Stalled within the old spread; the first new run's average lies
      inside the old spread, the second 71 us above it (both runs with the
      100 ms frame); without that frame new is at the top of the old
      spread, consistent with the compose finding above.
- [x] Build 542 installed in place: SYS:WBStartup/DockAwesome.new copied,
      0.527 deleted (not locked), renamed; Version says 0.542 build 542.
      v1.0b5 and bisect binaries deleted from RAM:, worktrees removed.
- [x] Gesture batch: the user did part of it by hand (Personal:Audio
      finding below); the rest ran agent-driven with build 544 (next
      section "AmiKit XE: agent gestures").
- [x] After the batch: test dock stopped, installed dock started, RAM:
      cleaned (next section).

### AmiKit XE: "dos device not mounted" for Personal:Audio (2026-11-25)
User, mid gesture batch: dropping Personal:Audio on the dock and clicking it
gave a window saying "dos device not mounted"; Downloads and Demos work as
stacks.
- [x] Personal:Audio (shell only): a plain directory on Personal: (DH12,
      mounted, 61G), not an assign, link or volume; Personal:Audio.info is
      a PNG icon whose icOn type is 2 (WBDRAWER). Contents Apollo.mp3, MOD,
      MP3, Streams (all ordinary). RAM:ak.json stores "Personal:Audio"
      (canonical). The dock's --profile showed stacks=6 (Audio among them)
      and a 185x156 box (Audio's 4 entries + Open cell), so it opened as a
      stack.
- [x] Window: a Directory Opus 5 lister titled `DOS error: device (or
      volume) is not mounted`, path `"Personal:Audio"/`. Reproduced through
      ARexx without the dock: `lister new "Personal:Audio"` gives exactly
      that lister; `lister new Personal:Audio` opens the drawer. DOpus 5
      takes the rest of the line as the path, quotes included; unquoted,
      `RAM:T/a b` (space) and `RAM:T/q"x` (quote) opened correctly too
      (`lister query <h> path` = `RAM:T/a b/`, `RAM:T/q"x/`).
- [x] Cause: the dock's own command, `lister new "<path>"` in
      launch_object (src/amiga_launch.c, since 050f9c8 / 1.0 beta 2;
      identical in v1.0b5). Every drawer the dock opens without Workbench
      (Open in Workbench in a stack's box or menu, a drawer or disk pin)
      opened such an error lister on AmiKit. Not a DOS requester: the
      folder reads already run with pr_WindowPtr -1, and the prefetch did
      not raise anything. Todo item "3. Drawers and disks without
      Workbench" was never run on DOpus 5 before.
- [x] Fix (build 544, Opus review: no P0/P1/P2): `dock_dopus_lister_new` (dock_text.c, host-tested)
      builds `lister new <path>` unquoted; refuses an empty path, control
      codes and no room; the quote refusal is gone (DOpus takes quotes).
      make host-test, host-sanitize green; make amiga no warnings.
- [x] Installed: build 544 in place of 542 (SYS:WBStartup/DockAwesome, 542
      deleted, Version 0.544). The test dock was stopped (stop.s) and
      restarted with the same arguments (run.s, --config RAM:ak.json
      --profile >RAM:ak.prof); the batch profile before the restart is
      saved as RAM:T/ak-batch1.prof and on the Mac.
- [x] End to end through the dock (agent, build 544, next section): Open
      in Workbench on Personal:Audio opens a lister on `Personal:Audio/`,
      no DOS error; the Archives: pin opens a lister on `Archives:`.

### AmiKit XE: agent gestures, first open, compose (2026-11-25)
Build 544 (HEAD 4ed4e32), installed in SYS:WBStartup; every test dock used
--config RAM: copies; v1.0b5 built from the tag in a scratch worktree.
- [x] Agent input works on AmiKit. The warp (IECLASS_POINTERPOS) lands at
      screen = 2*w-4 on both axes, so earlier clicks pressed beside the
      dock (`downs=0`). An input.device spy (pri 127 / 59 around the
      dock's 60) showed the agent's button events reach the dock and are
      captured; no handler above the dock. Working path: amiga_click /
      amiga_drag / amiga_move_mouse at ((X+4)/2, (Y+4)/2), checked with
      amiga_pointer; relative moves are accelerated (~1.1x), ui_click
      misses. tasks/lessons.md has the rule.
- [x] §4.4 on AmiKit, agent, profile `Input: downs=18 ups=18 right_seen=6`:

      | check | result |
      | --- | --- |
      | Clock pin | starts Clock |
      | Personal:Audio menu -> Open in Workbench | DOpus lister `Personal:Audio/`, no DOS error (fix 4ed4e32) |
      | Archives: pin (volume) | DOpus lister `Archives:` |
      | stack -> other stack, quick click | switches (Downloads->Demos 3x, Pictures->RiVA) |
      | program pin with a box open | Shell starts: quick click 3 of 4 (plus 1 of 1 with the diagnostic build), 1 s hold 1 of 1 |
      | Sort by rows | heading, Name/Date Modified/Kind, check mark moves Name->Kind->Name; file keeps only Demos "date" |
      | reorder (Audio to slot 3) | saved in RAM:ak.json |
      | drag out (Audio) | removed from RAM:ak.json |
      | Settings | no Folder Stack section; Save writes the file |

      The one failed quick click (Demos box open, receiver not armed) is
      the lost-first-press finding below. A .png in Personal:Pictures
      opened from the box gave "Could not open ... object not found": its
      icon's default tool is DH0:Utilities/MysticView/MysticView and DH0:
      does not exist on AmiKit (correct). Not run: the box's Open in
      Workbench tile and the "no dot" check (both only in PiP).
- [x] §4.5 first open after 47 s idle, pointer off the dock, Pictures
      (584x527, 190 entries) as v1.0b5's Folder Stack (RAM:o.json = the
      user's file with "folder": "Personal:Pictures") and as a stack in
      544 (RAM:n.json = migrated user's file + Demos "date"):

      | run | build | first paint ms | entrance ms | frames (fields) |
      | --- | --- | --- | --- | --- |
      | o1 | v1.0b5 | 363.5 | 159.4 | 4 (0223) |
      | n2 | 544 | 238.7 | 164.7 | 4 (0124) |
      | o2 | v1.0b5 | 282.8 | 134.3 | 3 (023) |
      | n3 | 544 | 285.8 | 154.2 | 4 (0132) |
      | o3 | v1.0b5 | 251.8 | 136.1 | 4 (0123) |
      | n4 | 544 | 285.4 | 174.2 | 4 (0124) |
      | o4 | v1.0b5 | 239.2 | 138.5 | 3 (023) |
      | n5 | 544 | 254.5 | 132.5 | 3 (023) |

      First paint: every new run under the old maximum (363.5), mean 266
      vs 284: pass. Entrance: FAIL by the §4.5 rule: n2 and n4 lie above
      the old maximum (159.4), mean 156 vs 142. Both are 4-frame entrances
      whose last frame came at 61-65 ms (o1's at 59 ms); box compose per
      frame is alike (new 13.9-23.3 ms, old 15.0-20.6). Not fixed: no
      cause in the stack code found; AmiKit's 20-65 ms main-loop "polls"
      passes (pre-existing, above) are the candidate. Open for the owner.
      Other stacks, 544 (n3; n1 in brackets), first paint / entrance ms:
      RiVA 242x209 69.8 / 148.3 (66.7 / 132.7), AmigaAMP 299x262 92.7 /
      147.2 (101.1 / 138.5), Demos 185x209 54.8 / 145.0 (40.4 / 139.4),
      Downloads 356x368 109.9 / 134.6 (127.5 / 131.7); prefetch stacks=5
      pending=0 reads=153 cache=153/512 evicted=0 in every run.
- [ ] FINDING (pre-existing, also v1.0b5): the first click after the dock
      has idled can be lost: no box, the press goes to the receiver or the
      desktop (`downs=0`, `presses_armed=1`). Seen on n2, n5, o3 and a
      diagnostic run (4 of 12 fresh-start first clicks after 47 s; the
      second click always worked). A diagnostic build (counters only, not
      committed) showed `nosnap=1`: the handler's hit snapshot was
      withdrawn at the press, after 38 withdrawals in that run (a normal
      run has 1). Which input_publish condition withdrew it is not split
      yet. A physical mouse moves onto the dock first (the zoom republishes),
      so users see it less. Not this PR; open for the owner.
- [x] Compose finding (compose_us_avg +16-19 % in --bench-sweep 8): not a
      code regression. 5-pin row (Shell, Clock, AmiKit:, Archives:,
      MultiView), ChangeTaskPri -30:

      | binary | compose_us_avg per run |
      | --- | --- |
      | v1.0b5 | 3778, 3783, 3793 |
      | 544 installed | 3820, 3837 |
      | HEAD + 0..28 byte code pad (8 binaries, 2 batches) | 3767-3848, outliers 5378 (pad 4), 5323 (pad 24), 5391 (pad 0) |
      | HEAD with address log, 12 runs | 3799-3830, one 5329 |

      The 12 logged runs had the same code (dock_render_oriented_clip at
      07a88d28) and canvas address; the outliers come and go per run of
      the same binary (pad 4 and 24 slow in batch 1, normal in batch 2),
      so they are run-level interference by other tasks above pri -30,
      not code or data placement. Clean runs: 544 is within 1 % of v1.0b5.
- [x] End state: test docks stopped (Ctrl-C; stop.s confirmed none left);
      the installed 544 started with Run from a Stack 400000 script (WBRun
      fails under DOpus, see above) and running, the only dock (Status);
      ENVARC:DockAwesome.json migrated by it as designed (no "folder",
      Personal:Downloads last), tests only used RAM: copies; RAM: and
      RAM:T clean (configs, profiles, scripts, FindTask, InputSpy,
      v1.0b5 and diagnostic binaries, ENV:dpid); scratch worktrees removed.

### ApolloOS rerun with build 550 (2026-11-25)
ApolloOS R9.5 on the Vampire (Kickstart 51.3, core 12856, 16-bit
1280x720), freshly rebooted, agent input. Build 550 = HEAD 2f42fad: the
repo's build/ held build 549 from the AmiKit pad/address-log runs, so
obj080 and the binary were rebuilt clean (`make amiga`, no warnings;
`make host-test` green). No code change in this follow-up. Baseline tag
v1.0b5 built in a scratch worktree outside the repo. Every test dock used
--config RAM: copies; ENVARC:DockAwesome.json (migrated by 533, 12 paths,
Downloads last) kept its date (18:06:15 on the Amiga clock) throughout.
- [x] Old leftovers: SYS:WBStartup/Disabled/DockAwesome-locked-delete,
      -2 and -3 were not locked after the reboot and are deleted.
- [x] Install in place: 550 copied to SYS:WBStartup/DockAwesome.new; the
      running 538 stopped (BreakTask), but its file stayed "in use"
      (started from WBStartup at boot), so it is now
      SYS:WBStartup/Disabled/DockAwesome-locked-delete (delete after the
      next ApolloOS boot); .new renamed into place, Version 0.550.
- [x] §4.4 checks the fixes touch (configs: the user's 11 pins + "folder"
      Downloads, plus Archives:Games, ApolloOS:Prefs and Archives: where
      needed; three stacks after migration):

      | check | result |
      | --- | --- |
      | --caps | 550, v1.0b5 and 538 identical (diff on the Mac) |
      | migration | no "folder", Downloads last; a second start left the file identical (cmp) |
      | no divider | --pip-dump, 12 icons at lefts 142 + 41k, last two 552 / 593 (36 + 5) |
      | stack switch, quick click | Downloads -> Games and Games -> Downloads |
      | stack switch, 1 s hold | Downloads -> Games and Games -> Downloads |
      | program pin with a box open, quick click | Editor started, box closed (from Downloads and from Games, 2 of 2) |
      | volume pin (Archives:) | Workbench window; its menu shows Close, which closes it |
      | stack menu | Sort by heading, Name/Date Modified/Kind, separator, Show Contents, Open in Workbench; no Close with the drawer's window open |
      | Open in Workbench | menu row (Games) and box tile (Downloads) each opened a Workbench window |
      | Sort by | Date saved "sorts" {"Personal:Downloads": "date"}, box newest first (SAGA readme first); heading click keeps the menu; Kind saved "kind"; Games kept Name; after a restart Kind checked and the box in Kind order; Name removed "sorts" |
      | A5 | "folder": "RAM:DoesNotExist" -> no requester, dock running, file saved with paths [Editor] only |

      Profile of the gesture run: Input downs=9 ups=9; prefetch stacks=3
      pending=0 evicted=0.
- [x] §4.5 first open after 45 s idle, pointer off the dock, fresh dock
      per run (config = the user's 11 pins + Archives:Games +
      ApolloOS:Prefs + "folder" Downloads; new: migrated, 3 stacks):

      | run | stack | first paint ms | entrance ms | frames (fields) |
      | --- | --- | --- | --- | --- |
      | o1 v1.0b5 | Downloads | 107.6 | 130.9 | 4 (0112) |
      | n1 550 | Downloads | 104.6 | 133.9 | 5 (01112) |
      | n1 | Games (same box) | 107.7 | 146.1 | 5 (01121) |
      | n1 | Prefs (smaller box) | 70.3 | 149.4 | 6 (011111) |
      | o2 v1.0b5 | Downloads | 107.8 | 131.7 | 4 (0112) |
      | n2 550 | Downloads | 105.1 | 141.2 | 5 (01121) |
      | n2 | Games | 107.4 | 145.7 | 5 (01121) |
      | o3 v1.0b5 | Downloads | 108.1 | 149.8 | 5 (01121) |

      New prefetch: stacks=3 pending=0 reads=81 cache=81/512 evicted=0.
      Pass: every new value is under the old maximum (108.1 / 149.8).
      The entrance is a 130 ms timed motion; whether a fifth frame fits
      before 130 ms decides 131 vs 141-150 ms in both builds (o3 shows it
      in v1.0b5; the motion code is unchanged since v1.0b5).
- [x] §4.5 hover, agent sweep (relative moves to x 350, 500, 650, 800,
      950 at y 688, then off, one per message, after 45 s idle; same 15-pin
      row for both builds: the 14 paths above + Downloads as a plain path,
      canvas 896), ChangeTaskPri -30:

      | run | build | render_us_avg | stalled | redraws |
      | --- | --- | --- | --- | --- |
      | ho1 | v1.0b5 | 10211 | 0 | 93 |
      | hn1 | 550 | 9174 | 0 | 88 |
      | ho2 | v1.0b5 | 9185 | 0 | 88 |
      | hn2 | 550 | 9236 | 1 | 87 |
      | ho3 | v1.0b5 | 9118 | 0 | 88 |
      | hn3 | 550 | 9185 | 0 | 88 |
      | ho4 | v1.0b5 | 9077 | 0 | 88 |

      render_us_avg: every new run inside the old spread (9077-10211).
      Stalled: hn1 and hn3 0 as old; hn2 1 (outside the old 0). That
      stall is the 542 pacing defer (retire_defers=1, retire_waits=0): a
      rest frame composed 14 ms after a late wake and presented after
      the VBlank, so the next frame was drawn at the following field. The
      old path would wait in WaitTOF in that frame instead and show it at
      the same field; the old runs did not hit a late present at all
      (max done_us 17.4 ms). Not a code defect; noted for the owner (the
      stall counter counts a defer, not a WaitTOF).
- [x] Lesson: BreakTask finds a dock by task name only when Workbench
      started it; a CLI-started test dock is named after its CLI, so the
      second start ran beside the first ("PiP2 is already in use"; OK
      clicked, rerun cleanly). Stop CLI docks with Status COM= + Break.
- [x] End state: 550 started with WBRun from SYS:WBStartup (the user's
      ENVARC config), FindTask finds it, Status shows no CLI dock; its
      Downloads stack opened and closed. RAM: and RAM:T clean (tools,
      configs, profiles, dumps, ENV:dpid deleted); scratch worktree
      removed on the Mac. C:ApolloExplorerSrv (started this session) runs
      until the next reboot.
- [ ] Delete SYS:WBStartup/Disabled/DockAwesome-locked-delete (538, in
      use) after the next ApolloOS boot.
- [ ] NOT RUN on ApolloOS this time: drawer/volume drops (pins came from
      the config), reorder and drag-out, Settings, --bench-sweep.

### Folder Stack "Click to": Show Stack / Open Folder per stack (2026-11-25, owner request)
Scope: the stack menu's action rows Show Contents / Open in Workbench become
options under a muted "Click to" heading: Show Stack (default, the box) or
Open Folder (a left click opens the drawer as a Workbench window, a DOpus
lister when DOpus is the desktop, through launch_pin as the old menu row
and the 1.0b5 drawer pins). Saved per stack as "clicks": {path: "folder"}
like "sorts". Open Folder stacks are not read ahead, have no running dot
(plan decision 3 / D9: stacks go to the status worker as kind 0) and no
Close row. The box's Open in Workbench tile stays.
- [x] dock_prefs: DockClick; "sorts" and "clicks" share one path-map
      reader (read_map + parse callback) and writer (write_map); "clicks"
      lists only "folder" stacks, after "sorts"; unknown values, other
      shapes, paths for no pin read as Show Stack; migration appends Show
      Stack.
- [x] amiga_main: Item.click (loaded with the path, saved by save_config,
      moved by dock_reorder, gone with remove_pin); stack_box(); a left
      click on an Open Folder stack closes an open box and calls
      launch_pin(path,1) + status_changed, no record_launch (no app may
      claim it); stack_next_pin and the read-ahead only for Show Stack
      stacks (`stacks=` in the profile counts those); click_menu_item saves
      at once, reverts on a failed save, re-arms the read-ahead on Show
      Stack.
- [x] amiga_ui: drawer_items = Sort by group, separator, "Click to" heading,
      Show Stack, Open Folder; popup_checked is a row mask; the
      DOCK_UI_SHOW_CONTENTS / DOCK_UI_OPEN_DRAWER actions are gone
      (DOCK_UI_CLICK_STACK / _FOLDER).
- [x] Tests: test_prefs test_clicks (no key when all Show Stack, exact
      format alone and after "sorts", round trip, before "paths", case,
      unknown/odd values, duplicate keys, empty and array shapes, reorder,
      removal, hand-edited order, migration, out-of-range not written) and
      three bad files.
- [x] 1.0 beta 5 compatibility: v1.0b5's dock_prefs.c (git archive, host)
      read a file written by this build with "sorts" and "clicks" (paths
      intact); build 550's reader too.
- [x] make host-test, make host-sanitize green; make amiga no warnings
      (build 551).
- [x] impeccable polish (in-thread; the detector is web-only): the menu on
      ApolloOS shows Sort by / Name / Date Modified / Kind, the hairline,
      Click to / Show Stack / Open Folder; headings muted, checks in one
      column, labels aligned past it, 115x146 px. No change needed: the
      group order matches the macOS Dock (Sort by, then Display as) and the
      heading reads as a sentence with each option.
- [x] ApolloOS R9.5, build 551 from RAM:, --config RAM:t1.json (the user's
      14 pins + Archives:Games + ApolloOS:Prefs, Downloads last; four
      stacks: Audio, Games, Prefs, Downloads):

      | check | result |
      | --- | --- |
      | menu | Sort by group, separator, Click to, check on Show Stack |
      | Downloads -> Open Folder | file gains "clicks": {"Personal:Downloads": "folder"}; menu check on Open Folder |
      | left click, Open Folder | Workbench window "Downloads", no box window |
      | Show Stack again | "clicks" removed; left click opens the box (356x368) |
      | Games, Audio -> Open Folder | both in "clicks" |
      | restart | Games menu checked Open Folder; left click opens window "Games" |
      | box open + quick click on Audio | box closed, window "Audio" opened |
      | reorder Games to slot 0 | paths start with Archives:Games, "clicks" kept for Games and Audio; click at slot 0 opens window "Games"; Prefs (Show Stack) still opens its box |
      | drag Audio out | Audio gone from paths and "clicks"; Games kept |
      | migration (RAM:t3.json: "clicks" and "sorts" before "paths", "folder" Downloads) | Downloads appended, no "folder", Games "date" and "folder" kept, Downloads not in "clicks" |

      Profile after the first session: stacks=2 (Prefs, Downloads; Audio and
      Games were Open Folder at exit).
- [x] First open after 45 s idle, pointer off the dock, fresh dock per run,
      config o.json (the build-550 runs' config: user's 11 pins of then +
      Games + Prefs + "folder" Downloads); of.json = the same with Games as
      Open Folder:

      | run | config | first paint ms | entrance ms | frames | prefetch |
      | --- | --- | --- | --- | --- | --- |
      | n1 551 | o.json | 107.8 | 142.0 | 5 | stacks=3 reads=81 |
      | n2 551 | of.json | 114.7 | 150.3 | 4 | stacks=2 reads=48 |
      | n3 551 | o.json | 107.7 | 138.4 | 5 | stacks=3 reads=81 |
      | n4 551 | of.json | 107.4 | 137.6 | 4 | stacks=2 reads=48 |
      | n5 551 | of.json | 107.2 | 138.2 | 5 | stacks=2 reads=48 |

      Build 550 (same config): 104.6 / 105.1 ms, 133.9 / 141.2 ms; v1.0b5
      max 108.1 / 149.8. Pending=0 evicted=0 in every run. n2 is one run
      above the old maximum: one prepare stage took 15.7 ms (7.4-7.7 in the
      other runs); n4/n5 with the same config are at 107 ms, so it reads as
      run noise. The Open Folder stack is skipped by the read-ahead (reads
      48 instead of 81).
- [x] Hover agent sweep (t.json, 15 pins, canvas 896, ChangeTaskPri -30,
      x 350/500/650/800/950 at y 688, relative, one per message):
      551 render_us_avg 9733 / 9677, stalled 2 / 0; 550 (installed
      binary, same run) 9492, stalled 1. Old spread from the 550 runs
      9077-10211. The two 551 stalls are the first two frames after idle.
      No hover code changed.
- [x] Opus review (fresh, diff only): no P0/P1/P2. Its note: switching a
      stack to Open Folder leaves its read-ahead page in memory until the
      slot is reused (no behaviour change).
- [x] Build 551 installed in place: copied to SYS:WBStartup/DockAwesome.new;
      the 550 file was "in use" (WBRun), so it is now
      SYS:WBStartup/Disabled/DockAwesome-locked-delete2; .new renamed into
      place, Version 0.551; started with WBRun, FindTask finds it, the
      only dock (Status lists no CLI dock); with the user's ENVARC config
      (14 pins) the Downloads menu shows Name and Show Stack checked.
      ENVARC:DockAwesome.json kept its date (17:40:52 on the Amiga clock):
      not written by the tests. RAM: clean (tools, configs, profiles,
      ENV:dpid deleted); scratch files removed on the Mac.
- [ ] Delete SYS:WBStartup/Disabled/DockAwesome-locked-delete (538) and
      -locked-delete2 (550) after the next ApolloOS boot (both in use).
- [ ] NOT RUN: Coffin, AmigaOS 3.2.3, 3.9, 3.1.4.1, AmiKit XE (Open Folder
      as a DOpus lister is untested end to end on AmiKit; the launch path
      is the one 4ed4e32 verified there for the old menu row), no-dot
      check of an Open Folder stack (PiP only, ask the user), the box's
      Open in Workbench tile (unchanged).

### Coffin rerun with build 551 (2026-11-25)
Coffin r65 (Kickstart 45.57, Workbench 45.5, core 12856, 24-bit 1280x720),
agent input. Build 551 = HEAD 5cc7184 (no source newer than
build/DockAwesome; not rebuilt, no code change in this follow-up). Baseline
tag v1.0b5 built in a scratch worktree outside the repo; build 542
(98bbf1a) built the same way as a control. Every test dock used
--config RAM: copies; ENVARC:DockAwesome.json (migrated by 541, 18 paths,
Downloads last) kept its date (10:03:46 on the Amiga clock) throughout.
- [x] Old leftover SYS:WBStartup/Disabled/DockAwesome-locked-delete
      (0.527) was not locked and is deleted.
- [x] §4.4 checks for what changed since 541 (configs: the user's 17
      paths + "folder" Downloads; t.json adds Archives:Games, System:Prefs
      and Archives:, so three stacks and a volume after migration):

      | check | result |
      | --- | --- |
      | --caps | 551, installed 541 and v1.0b5 identical (Diff) |
      | migration | no "folder", Downloads last; a second start left the file unchanged (Diff, same date) |
      | A5 | "folder": "RAM:DoesNotExist" -> no requester, dock running, file saved with paths [EditPad] only |
      | no divider | --pip-dump, 18 icons at lefts 135 + 37k, the last (Downloads) at 764: last two 5 px apart |
      | stack switch, quick click | Downloads -> Games and Games -> Downloads |
      | stack switch, 1 s hold | Downloads -> Games and Games -> Downloads |
      | program pin with a box open, quick click | EditPad started, box closed |
      | volume pin (Archives:) | Workbench window; its menu shows Close, which closed it |
      | Sort by | Date Modified saved "sorts": {"Personal:Downloads": "date"}; menu check moved; Games kept Name; after a restart Downloads still Date Modified |
      | Click to, Open Folder | Downloads -> "clicks": {"Personal:Downloads": "folder"}; left click opened Workbench window "Downloads", no box window; menu with the window open has no Close row |
      | box open + quick click on an Open Folder stack | Prefs box closed, window "Games" opened |
      | no running dot | --pip-dump at exit with the Games and Downloads windows open (both stacks) and the Archives: window open: a dot under Archives: only |
      | restart | Games menu checked Open Folder, Downloads Date Modified + Open Folder |
      | Show Stack again | Downloads left "clicks"; left click opened the box (354x337) |
      | reorder | Games dragged to slot 0: paths start with Archives:Games, "clicks" kept for Games, "sorts" kept for Downloads; a click at slot 0 opened window "Games"; Prefs (Show Stack) still opened its box |
      | drag out | Downloads dragged out: gone from paths with its "sorts"; Games kept "folder" |
      | Settings | Size, Scale, Position, Cancel/Save, no Folder Stack section; Save rewrote the file and kept "clicks" |

      Test note: amiga_drag with relative:true and from == to is not a
      hold (it homes the pointer, so the dock saw a drag of the pin; the
      file was unchanged and nothing opened); the 1 s holds were sent as
      absolute amiga_drag with settle 50 after a relative click had placed
      the pointer. Coffin's Prefs icon is a flat bar: hit at y 680, not 692.
- [x] §4.5 first open after 45 s idle, pointer off the dock, fresh dock
      per run, ChangeTaskPri -30, config = the user's 17 paths + Archives:Games
      + System:Prefs + "folder" Downloads (old: 19 pins + Folder Stack;
      new: migrated, 3 stacks). n1, n2, n3 and o1, o2 sent the next agent
      command at once after each click; n4, o3, o4 had seconds between
      clicks:

      | run | build | Downloads first paint / entrance ms | Games ms | frames (fields) |
      | --- | --- | --- | --- | --- |
      | o1 | v1.0b5 | 89.4 / 130.7 | - | 4 (0112) |
      | n1 | 551 | 85.1 / 149.7 | 86.1 / 153.8 | 4 (0122) / 5 (01122) |
      | o2 | v1.0b5 | 89.8 / 150.3 | - | 5 (01121) |
      | n2 | 551 | 86.1 / 168.6 | 86.5 / 153.4 | 4 (0114) / 5 (01122) |
      | c1 | 541 (control) | 87.7 / 132.3 | 88.4 / 131.3 | 4 (0112) |
      | c2 | 542 (control) | 102.4 / 137.5 | 88.5 / 130.5 | 5 (01112) / 4 |
      | n3 | 551 | 90.2 / 144.4 | 89.0 / 131.3 | 5 (01121) / 4 |
      | o3 | v1.0b5 | 90.0 / 149.8 | - | 5 (01121) |
      | n4 | 551 | 90.8 / 143.8 | 89.3 / 130.3 | 5 (01121) / 4 |
      | o4 | v1.0b5 | 89.8 / 150.0 | - | 5 (01121) |

      n1 Prefs (larger box, 468x433, exempt): 140.1 / 199.8. New prefetch
      in every run: stacks=3 pending=0 reads=120 cache=120/512 evicted=0.
      Old maximum 90.0 / 150.3. Against it:
      - Entrance: n3 and n4 (Downloads 144.4 / 143.8, Games 131.3 / 130.3)
        pass. n2 Downloads 168.6 FAILS: its last frame took 60.3 ms (25 ms
        in every old run); n1 and n2 Games 153.8 / 153.4 are 3 ms over.
      - First paint: n1 and n2 pass (85-86 ms); n3 and n4 are 0.2 and 0.8
        ms over (90.2 / 90.8); the step is the "layer" prepare stage
        (19.3 ms in n3/n4, 15.1 in n1/n2, 15.2-15.6 in the old runs).
      Not fixed, open for the owner: no slow final frame or 3 ms Games
      excess came back in n3/n4 or in the 541/542 controls, and the open
      and entrance code is the same in 542 and 551 (84def22 changed the
      menu rows, the check mask and the click routing only). The dock
      runs at -30 here, below every task an agent command starts.
- [x] §4.5 hover, agent sweep (relative moves to x 350, 500, 650, 800,
      950 at y 692, then off, one per message, after 45 s idle), the same
      20-pin row for both builds (pl.json: the user's 17 paths +
      Archives:Games, System:Prefs, Personal:Downloads, no "folder";
      canvas 992), ChangeTaskPri -30:

      | run | build | render_us_avg | max us | stalled | redraws |
      | --- | --- | --- | --- | --- | --- |
      | ho1 | v1.0b5 | 13966 | 101512 | 11 | 102 |
      | hn1 | 551 | 9284 | 18788 | 4 | 100 |
      | ho2 | v1.0b5 | 10526 | 35374 | 6 | 102 |

      Pass: new render_us_avg and stalled below both old runs. The old
      runs show the pacing cascade at the x 950 rest (stalls of 35 ms with
      a 29-32 ms present, the 3.2.3 finding); 551 has retire_waits=0
      retire_defers=2 and no such stall (its 4 stalls are 12-13 ms frames
      at the start of a motion). First check of the 542 pacing fix on
      Coffin.
- [x] No defect found that needs a code change; no commit to src/.
- [x] End state: 551 copied to SYS:WBStartup/DockAwesome.new; the 541
      file was "in use" (WBRun), so it is now
      SYS:WBStartup/Disabled/DockAwesome-locked-delete; .new renamed into
      place, Version 0.551; started with WBRun, FindTask finds it, Status
      lists no CLI dock; its Downloads stack opened and closed with the
      user's ENVARC config. RAM: and RAM:T clean (tools, configs,
      profiles, dumps, binaries), ENV:dpid deleted; scratch worktrees
      (v1.0b5, 542) removed on the Mac. ApolloExplorerSrv was already
      running at boot.
- [x] Delete SYS:WBStartup/Disabled/DockAwesome-locked-delete (541, in
      use) on Coffin after its next boot (done in the 569 session; it was
      no longer locked).
- [ ] NOT RUN on Coffin this time: drawer/volume drops (pins came from the
      config), the box's Open in Workbench tile (unchanged), --bench-sweep;
      Open Folder as a DOpus lister (Workbench is Coffin's desktop).

### AmigaOS 3.9 matrix and performance with build 551 (2026-11-25)
Plain AmigaOS 3.9 on the Vampire (Kickstart 40.68 ROM, Workbench 45.5,
exec 40.10, graphics 40.24, intuition 40.85, icon 51.4, workbench 45.132:
the dock's V39/V44 requirements hold), core 12856, 24-bit 1280x720
(cybergraphics 41.4), agent input (the warp lands where aimed;
amiga_pointer 640,300 after a move to 640,300). amiagent did not start at
boot (port 7846 refused for ~20 min); the user started it by hand from
Personal:Applications/AmiMCP. ApolloExplorerSrv was not running; started
with Run >NIL: C:ApolloExplorerSrv (stays until reboot). Installed before:
release 1.0 beta 4 ("DockAwesome 1.0", 21-Nov) in SYS:WBStartup, running.
Build 551 = HEAD 1fb8556 (`make amiga` had nothing to rebuild); baseline
tag v1.0b5 built in a scratch worktree outside the repo. Every test dock
used --config RAM: copies; ENVARC:DockAwesome.json (1.0b5 format, 12 paths,
"folder": "Personal:Downloads") kept its date (Saturday 01:25:41) until
the installed 551 started. --caps: shaped_windows no, rgb_rastport no,
alpha_text no, aa_text no, pip yes, pixels ammx, ui_glass yes (the box and
menus are real windows, so screenshots show them).
- [x] Old leftover SYS:WBStartup/Disabled/DockAwesome-locked-delete
      (build 509, 20-Nov) was not locked and is deleted.
- [x] §4.4 (configs: mig = the user's file; t = its 12 paths +
      Archives:Games, AOS39:Prefs, Archives: + "folder" Downloads, so
      three stacks and a volume after migration):

      | check | result |
      | --- | --- |
      | --caps | 551, v1.0b5 and the installed 1.0 beta 4 identical (diff on the Mac) |
      | migration | no "folder", Personal:Downloads last; a second start left the file unchanged (same date) |
      | A5 | "folder": "RAM:DoesNotExist" -> no requester, dock running, file saved with paths [AOS39:Tools/Editor] only |
      | no divider | --pip-dump, 13 icons: centres 169.5 + 41k, last two (SD-Card, Downloads) 620 / 661.5: 5 px apart (36 px icons) |
      | stack switch, quick click | Downloads -> Games and Games -> Downloads |
      | stack switch, 1 s hold | Downloads -> Games and Games -> Downloads |
      | program pin with a box open, quick click | Editor started, box closed |
      | volume pin (Archives:) | Workbench window; its menu shows Close, which closed it |
      | drawer drop | Personal:Audio dropped from the Personal window -> pinned at the drop slot as a stack; click opened its box, a click on its pin closed it |
      | volume drop | Ram Disk: dropped -> appended last; click opened its window |
      | Sort by | heading click kept the menu; Date Modified saved "sorts": {"Personal:Downloads": "date"}, check moved, box newest first (SAGA readme/lha first); Games kept Name |
      | box tile | Open in Workbench in the box opened Workbench window "Downloads" |
      | Click to, Open Folder | Downloads -> "clicks": {"Personal:Downloads": "folder"}; left click opened Workbench window "Downloads", no box; menu with the window open has no Close row |
      | box open + quick click on an Open Folder stack | Prefs box closed, window "Games" opened |
      | no running dot | --pip-dump at exit with the Games, Downloads (both stacks) and Archives: windows open: one dot, under Archives: only |
      | restart | Downloads menu Date Modified + Open Folder, Games Name + Open Folder |
      | Show Stack again | Downloads left "clicks"; left click opened the box (356x368) |
      | reorder | Games dragged to slot 0: paths start with Archives:Games, "clicks" kept for Games, "sorts" kept for Downloads; a click at slot 0 opened window "Games"; Prefs (Show Stack) still opened its box |
      | drag out | Downloads dragged out: gone from paths with its "sorts"; Games kept "folder" |
      | Settings | Size, Scale, Position, Cancel/Save, no Folder Stack section; Save rewrote the file (new date) and kept "clicks" |

- [x] §4.5 first open after 45 s idle, pointer off the dock, fresh dock per
      run, ChangeTaskPri -30, config o = the user's 12 paths +
      Archives:Games + AOS39:Prefs + "folder" Downloads (old: 14 pins +
      Folder Stack; new: migrated, 3 stacks), clicks seconds apart:

      | run | build | Downloads first paint / entrance ms | Games ms | Prefs ms (smaller box 356x315) |
      | --- | --- | --- | --- | --- |
      | o1 | v1.0b5 | 114.4 / 145.6 | - | - |
      | n1 | 551 | 97.4 / 137.6 | 96.4 / 138.7 | 87.7 / 152.1 (1 frame, fields=0) |
      | o2 | v1.0b5 | 110.4 / 144.7 | - | - |
      | n2 | 551 | 97.8 / 137.2 | 96.9 / 138.1 | 88.2 / 131.5 |
      | o3 | v1.0b5 | 100.6 / 134.5 | - | - |
      | n3 | 551 | 97.7 / 137.3 | 96.4 / 138.8 | 87.6 / 130.9 |

      New prefetch in every run: stacks=3 pending=0 reads=92 cache=92/512
      evicted=0. Old maximum 114.4 / 145.6. Every new first paint and
      every Downloads and Games entrance is under it. One exception, open
      for the owner: n1's Prefs entrance drew a single frame (fields=0)
      and ended at 152.1 ms, 6.5 ms over; n2 and n3 (131.5 / 130.9, 4
      frames) did not repeat it. A one-frame entrance was seen once on
      3.2.3 as well (Games, 134 ms, under the maximum there).
- [x] §4.5 hover, agent sweep (relative moves to x 350, 500, 650, 800,
      950 at y 692, then off, one per message, after 45 s idle), the same
      15-pin row for both builds (pl: the user's 12 paths +
      Archives:Games, AOS39:Prefs, Personal:Downloads, no "folder"; canvas
      896), ChangeTaskPri -30:

      | run | build | render_us_avg | max us | stalled | redraws |
      | --- | --- | --- | --- | --- | --- |
      | ho1 | v1.0b5 | 9460 | 18290 | 0 | 98 |
      | hn1 | 551 | 9540 | 18218 | 0 | 99 |
      | ho2 | v1.0b5 | 9561 | 18282 | 1 | 97 |

      Pass: new render_us_avg and stalled within the old spread; 551
      retire_waits=0 retire_defers=1.
- [x] No defect found that needs a code change; no commit to src/.
- [x] End state: 551 copied to SYS:WBStartup/DockAwesome.new; the 1.0
      beta 4 file was "in use" (WBStartup), so it is now
      SYS:WBStartup/Disabled/DockAwesome-locked-delete; .new renamed into
      place, Version 0.551; started with WBRun, FindTask finds it, no CLI
      dock in Status; it migrated ENVARC:DockAwesome.json (no "folder",
      Personal:Downloads last) and its Downloads stack opened and closed.
      RAM: clean (tools, scripts, configs, profiles, dumps, binaries,
      Editor's RAM:T/JanoRecentFiles), ENV:dpid deleted; scratch worktree
      removed on the Mac.
- [x] Delete SYS:WBStartup/Disabled/DockAwesome-locked-delete (1.0 beta 4,
      in use) on 3.9 after its next boot (done in the 569 session; it was
      no longer locked).
- [x] amiagent does not start at boot on this 3.9 install (the user
      started it by hand). Solved outside the dock by the user's two
      S:User-Startup lines; seen working at the 569 session's boot.
- [ ] NOT RUN on 3.9: Open Folder as a DOpus lister (Workbench is the
      desktop here). --bench-sweep ran in the 569 session.

### AmigaOS 3.1.4.1 with builds 551 and 553 (2026-11-25)
AmigaOS 3.1.4.1 on the Vampire (Kickstart 46.143, exec 46.45, Workbench
45.194, graphics 45.27, intuition 40.87, layers 45.30, icon 51.4,
workbench 45.194, cybergraphics 41.4, input.device 45.2: the dock's
V39/V44 requirements hold, so it does not refuse), core 12856, 24-bit
1280x720. Installed before: release 1.0 beta 4 ("DockAwesome 1.0",
21-Nov), running from WBStartup. amiagent was up at boot;
ApolloExplorerSrv was not (started with Run >NIL: C:ApolloExplorerSrv).
Build 551 = HEAD 9bc7080 (not stale); baseline tag v1.0b5 built in a
scratch worktree outside the repo, no warnings.
- [x] Old leftover SYS:WBStartup/Disabled/DockAwesome-locked-delete
      (21-Nov) was not locked and is deleted.
- [x] --caps: 551, v1.0b5 and the installed 1.0 beta 4 identical (diff on
      the Mac): shaped_windows no, pip yes, pixels ammx, ui_glass yes,
      aa_text no, error none.
- [x] DEFECT (also in v1.0b5): a Break during startup killed the dock
      without cleanup and froze Intuition input. Migration test: the
      installed dock stopped (BreakTask), 551 started on a copy of the
      user's file (--config RAM:mig.json --profile), stop.s sent Break
      within about a second. Its output file held only "***Break";
      RAM:mig.json.new (582 bytes, migrated) was left and mig.json not
      renamed, so it died inside save_config, after install_input. A
      second start showed "PiP2 is already in use." and the agent's
      pointer moves and clicks no longer moved the pointer. A window
      walk (tool Wins) found a leaked 1x1 dock receiver window whose
      owner task was gone; the "DockAwesome.PiP2" lease semaphore was
      still listed with no process; an input.device spy saw only its own
      two handlers in the chain and no events.
      Cause: libnix's read() and write() call __chkabort, which raises
      SIGINT when Ctrl-C is pending: it prints "***Break" and exits
      without main's cleanup (input handler, PiP lease, windows,
      AppWindow). The event loop clears Ctrl-C in its
      Wait, so only a Break that lands before the loop (startup) or
      while a file or stdout write runs hits it; --bench-sweep polls
      Ctrl-C without clearing it, so a Break there also aborted at the
      profile report.
      Reproduced on v1.0b5 and 551 without a leak that matters: a tool
      (CtrlCRun) runs the dock in its own process with Ctrl-C pending;
      `--caps` printed only "***Break", rc 20, on both.
      Fix: `void __chkabort(void) {}` in amiga_main.c, so libnix never
      aborts and Ctrl-C is the event loop's alone (nm: the binary's
      ___chkabort is an rts, _raise is no longer linked). Same CtrlCRun
      test with 552: the full --caps output, rc 0.
      Opus review of the delta: no blocking finding. Its non-blocking
      point is done in build 553: the stock __chkabort also cleared
      Ctrl-C, so --bench-sweep now reads and clears it
      (SetSignal(0,SIGBREAKF_CTRL_C)) and main clears a Break the loop
      never took before it returns, so it cannot stop the next command
      of a script. CtrlCRun with 553: full --caps output, rc 0,
      Ctrl-C not left pending. make host-test, make host-sanitize green;
      make amiga (552 and 553) without warnings.
- [x] Installed: 552 copied to SYS:WBStartup/DockAwesome.new; the 1.0
      beta 4 file was "in use", so it is now
      SYS:WBStartup/Disabled/DockAwesome-locked-delete; .new renamed
      into place; then 553 replaced 552 the same way (552 was never
      started, so it was deleted), Version 0.553. Not started: the leaked PiP2 lease and
      the frozen input need a reboot first. ENVARC:DockAwesome.json
      (1.0b5 format, "folder": "Personal:Downloads", 12 paths) kept its
      date (Saturday 00:40:38); 553 migrates it on its first start.
      The second test dock's "PiP2 is already in use." requester (the
      one the user saw) could not be clicked with frozen input; a tool
      (ReqOK) sent its window the OK gadget's GADGETUP, the dock exited
      and Status lists no dock. RAM: clean (only amiagent's own file);
      ENV:dpid deleted; scratch worktree removed on the Mac. The leaked
      1x1 window, the PiP2 lease and the frozen input stay until the
      reboot.
- [x] Rebooted by the user. 553 started from WBStartup and migrated
      ENVARC:DockAwesome.json at boot (no "folder", Personal:Downloads
      last, 582 bytes; stamped 02-Aug-25 15:35:16 because the clock was
      not yet set); the tests never wrote it. The 1.0 beta 4 file in
      Disabled was no longer locked and is deleted. Agent input reaches
      the dock here: warps land 1:1 (move to 640,300 -> pointer 640,300).
      Before every start: Status (CLI docks), FindTask, tool Sems (PiP
      leases); each test dock got 5 s or more before stop.s.
- [x] Break checks with 553 (installed dock stopped first; Sems empty):

      | test | result |
      | --- | --- |
      | Ctrl-C pending through a whole start (CtrlCRun, migration config) | migrated and saved (582 bytes), one frame drawn, normal exit with the full profile, rc 0, no Ctrl-C left, no lease, no leaked window |
      | Break sent at once after a start (stop.s right after Run) | migrated file saved, full profile, no lease, pointer moves |
      | Break 3 s after starting --bench-sweep 8 | ended at the sweep's start (redraws 0), full profile, no lease |
      | Break 5 s after starting --bench-sweep 8 | ended mid-sweep (redraws 50 of 165), full profile, no lease, pointer moves |

- [x] §4.4 with 553 (configs: m = the user's 1.0b5 file; t = its 12
      paths + Archives:Games, AOS3141:Prefs, Archives: + "folder"
      Downloads, so three stacks and a volume after migration):

      | check | result |
      | --- | --- |
      | --caps | 551, v1.0b5 and 1.0 beta 4 identical (before the fix; 553 prints the same) |
      | migration | no "folder", Personal:Downloads last; a second start left the file unchanged (same date) |
      | A5 | "folder": "RAM:DoesNotExist" -> no requester, dock running, file saved with paths [AOS3141:Tools/Editor] only |
      | no divider | --pip-dump, 16 icons, slots 140 + 41k on the 928 canvas, last two (Archives:, Downloads) at 714 and 755: 5 px apart |
      | stack switch, quick click | Downloads -> Games and Games -> Downloads |
      | stack switch, 1 s hold | Downloads -> Games and Games -> Downloads |
      | program pin with a box open, quick click | Editor started, box closed |
      | volume pin (Archives:) | Workbench window; its menu shows Close, which closed it |
      | Sort by | heading click kept the menu; Date Modified saved "sorts": {"Personal:Downloads": "date"}, check moved, box newest first (SAGA readme/lha first); Games kept Name |
      | Click to, Open Folder | "clicks" saved; left click opened Workbench window "Downloads", no box; menu with the window open has no Close row |
      | box open + quick click on an Open Folder stack | Prefs box closed, window "Games" opened |
      | no running dot | --pip-dump at exit, pointer off the dock, with the Games, Downloads (both stacks) and Archives: windows open: one dot, under Archives: only |
      | restart | Downloads Date Modified + Open Folder, Games Name + Open Folder |
      | Show Stack again | Downloads left "clicks"; left click opened the box (356x368) |
      | box tile | Open in Workbench in the Downloads box opened its Workbench window |
      | reorder | Games dragged to slot 0: paths start with Archives:Games, "clicks" kept; a click at slot 0 opened window "Games"; Prefs (Show Stack) still opened its box |
      | drag out | Downloads dragged out: gone from paths with its "sorts" |
      | drawer drop | Personal:Audio dropped from the Personal window -> pinned at the drop slot as a stack; its box opened and closed |
      | volume drop | Ram Disk: dropped -> pinned at the drop slot; click opened its window |
      | Settings | Size, Scale, Position, Cancel/Save, no Folder Stack section; Save rewrote the file (new time) and kept "clicks" |

- [x] §4.5 first open after 45 s idle, pointer off the dock, fresh dock per
      run, ChangeTaskPri -30, config o = the user's 12 paths +
      Archives:Games + AOS3141:Prefs + "folder" Downloads (old: 14 pins +
      Folder Stack; new: migrated, 3 stacks), clicks seconds apart:

      | run | build | Downloads first paint / entrance ms | Games ms | Prefs ms (smaller box 299x315) |
      | --- | --- | --- | --- | --- |
      | o1 | v1.0b5 | 101.4 / 135.6 | - | - |
      | n1 | 553 | 99.2 / 136.1 (1 frame) | 95.7 / 143.3 | 72.3 / 142.5 |
      | o2 | v1.0b5 | 101.3 / 135.9 | - | - |
      | n2 | 553 | 98.9 / 136.5 | 96.4 / 133.9 | 72.6 / 142.7 |
      | o3 | v1.0b5 | 101.2 / 135.6 | - | - |
      | n3 | 553 | 99.5 / 135.6 | 96.5 / 153.5 (fields 01113) | 72.5 / 146.4 |

      New prefetch in every run: stacks=3 pending=0 reads=84 cache=84/512
      evicted=0. Old maximum 101.4 / 135.9. Every new first paint is
      under it. Entrance: NOT a clean pass, open for the owner like the
      Coffin and 3.9 outliers: Downloads 135.6-136.5 (up to 0.6 ms over),
      Games 133.9 once but 143.3 and 153.5 (a slow last frame), Prefs
      142.5-146.4 (6.6-10.5 ms over) although its box is smaller. The
      old runs are very steady (135.6-135.9, fields 01112).
- [x] §4.5 hover, agent sweep (relative moves to x 350, 500, 650, 800,
      950 at y 692, then off, one per message, after 45 s idle), the same
      15-pin row for both builds (pl: the 12 paths + Archives:Games,
      AOS3141:Prefs, Personal:Downloads, no "folder"; canvas 896),
      ChangeTaskPri -30:

      | run | build | render_us_avg | max us | stalled | redraws |
      | --- | --- | --- | --- | --- | --- |
      | ho1 | v1.0b5 | 9529 | 18392 | 1 | 98 |
      | hn1 | 553 | 9610 | 18504 | 1 | 98 |
      | ho2 | v1.0b5 | 10719 | 100610 | 0 | 98 |

      Pass: render_us_avg and stalled within the old spread; 553
      retire_waits=0 retire_defers=1.
- [x] §4.5 --bench-sweep 8 (same row, ChangeTaskPri -30), v1.0b5 / 553 /
      v1.0b5: render_us_avg 6753 / 6440 / 6736, stalled 41 / 41 / 43 of
      165 redraws (553 retire_waits=0). Pass.
- [x] No defect beyond the startup Break; no further code change.
- [x] End state: 553 in SYS:WBStartup (Version 0.553), started with WBRun
      after the tests, FindTask finds it, Status lists no CLI dock, one
      "DockAwesome.PiP2" lease (its own); its Downloads stack opened and
      closed with the user's ENVARC config. SYS:WBStartup/Disabled holds
      only WBDock. RAM: clean (tools, scripts, configs, profiles, dumps,
      binaries, Editor's RAM:T/JanoRecentFiles), ENV:dpid deleted;
      scratch worktree removed on the Mac. ApolloExplorerSrv runs until
      reboot.
- [ ] NOT RUN on 3.1.4.1: Open Folder as a DOpus lister (Workbench is the
      desktop here); Break during startup with the old build beyond the
      in-process --caps proof.

### AmigaOS 3.2.3 rerun with build 553 (2026-11-25)
AmigaOS 3.2.3 on the Vampire (Kickstart 47.115, Workbench 47.5; exec 47.13,
intuition 47.53, graphics 47.10), core 12856, 24-bit 1280x720, agent input
(warps land 1:1). Installed before: build 542 (running from WBStartup).
Build 553 = HEAD 24573c0, not stale (`make amiga`: nothing to rebuild; built
without warnings); baseline tag v1.0b5 built in a scratch worktree outside
the repo. No code change in this rerun. All destructive runs used
--config RAM:<file>.json; ENVARC:DockAwesome.json kept its date (10:45:25,
from 541's migration) through every test and the install. SYS:WBStartup/
Disabled held only WBDock: no leftover to delete.
- [x] Aiming on this boot: at rest the icon centres are 640 + 41*(k-(n-1)/2).
      The last icon moves about 8 px right when the row zooms, so a press at
      its rest centre lands in the gap (`downs=0`, `presses_unarmed=1`, an
      input spy showed the press passed through): Downloads at x 955, not 931,
      with 16 icons. Not a defect; the same in every build.
- [x] Break checks with 553 (installed dock stopped first; Sems empty, no CLI
      dock before each start):

      | test | result |
      | --- | --- |
      | Ctrl-C pending through a whole start (CtrlCRun, migration config) | migrated and saved (568 bytes), one frame, full profile, rc 0, ctrl_c_left 0, no lease, no leaked window (Wins), pointer moves |
      | Break sent at once after a start | migrated file saved, full profile, no lease |
      | Break 3 s after starting --bench-sweep 8 | ended at the sweep's start (redraws 0), full profile, no lease |
      | Break 6 s after starting --bench-sweep 8 | ended mid-sweep (redraws 91 of 165), full profile, no lease, pointer moves |

- [x] §4.4 with 553 (configs: m = the user's paths without Downloads +
      "folder" Downloads, 1.0b5 format; t = m + Archives:Games, AOS32:Prefs,
      Archives:, so three stacks and a volume after migration):

      | check | result |
      | --- | --- |
      | --caps | 553, v1.0b5 and the installed 542 identical (diff on the Mac); dopus no |
      | migration | no "folder", Personal:Downloads last; a second start left the file unchanged (same date) |
      | A5 | "folder": "RAM:DoesNotExist" -> no requester, dock running, file saved with paths [AOS32:Tools/TextEdit] only |
      | no divider | --pip-dump, 13 icons on the 832 canvas: centres 169.5 + 41k, last two (SD-Card, Downloads) 620 / 661.5 |
      | stack switch, quick click | Downloads -> Games and Games -> Downloads |
      | stack switch, 1 s hold | Downloads -> Games and Games -> Downloads |
      | program pin with a box open, quick click | TextEdit started, box closed |
      | volume pin (Archives:) | Workbench window; its menu shows Close, which closed it |
      | Sort by | heading click kept the menu; Date Modified saved "sorts": {"Personal:Downloads": "date"}, check moved, kept after a restart; Games kept Name (box order not looked at: PiP only) |
      | Click to, Open Folder | "clicks" saved; left click opened Workbench window "Downloads", no box; the menu with the window open has the same 8 rows, no Close |
      | box open + quick click on an Open Folder stack | Prefs box closed, window "Games" opened |
      | no running dot | --pip-dump at exit, pointer off the dock, with the Prefs (stack), Downloads (Open Folder) and Archives: windows open: one dot, under Archives: only |
      | restart | Downloads kept Date Modified + Open Folder; Show Stack removed "clicks" and brought the box back |
      | box tile | Prefs box (299x315, 23 entries): Open in Workbench (last tile) opened Workbench window "Prefs" |
      | reorder | Downloads dragged to slot 0: first in "paths", "sorts" kept; a click at slot 0 opened its box |
      | drag out | Games (Open Folder) dragged past the row's left end: gone from "paths" with its "clicks" |
      | drawer drop | Personal:Audio dropped from the Personal window -> pinned at the drop slot (7) as a stack; its box (185x156) opened and closed |
      | volume drop | RAM Disk icon -> "RAM Disk:" pinned at the drop slot; a click opened its window |
      | Settings | Size, Scale, Position, Cancel/Save, no Folder Stack section; Save rewrote the file (new time) and kept "sorts" |

- [x] §4.5 first open after 45 s idle, pointer off the dock, fresh dock per
      run, ChangeTaskPri -30, config o = the user's paths + Archives:Games +
      AOS32:Prefs + "folder" Downloads (old: 14 pins + Folder Stack; new:
      migrated, 3 stacks), Downloads opened and closed, v1.0b5 / 553
      alternating, 8 runs each (fields = fields between frames; last = the
      last motion frame's time):

      | run | build | first paint ms | entrance ms | frames | fields | compose / present us avg | frame_us |
      | --- | --- | --- | --- | --- | --- | --- | --- |
      | o1 | v1.0b5 | 109.7 | 130.3 | 4 | 0112 | 13193 / 3281 | 7978 12857 18235 26748 |
      | n1 | 553 | 93.1 | 146.8 | 4 | 0113 | 18003 / 2791 | 7795 13813 18396 43092 |
      | o2 | v1.0b5 | 96.4 | 201.1 | 4 | 0111 | 9630 / 2685 | 4916 10586 14960 18719 |
      | n2 | 553 | 93.0 | 146.8 | 4 | 0113 | 18064 / 2835 | 7968 13814 18561 43174 |
      | o3 | v1.0b5 | 110.0 | 149.8 | 5 | 01121 | 11228 / 3602 | 7881 12792 18289 26479 8613 |
      | n3 | 553 | 92.8 | 147.0 | 4 | 0113 | 18060 / 2888 | 8064 13807 18533 43312 |
      | o4 | v1.0b5 | 112.3 | 147.4 | 5 | 01121 | 11854 / 3488 | 8049 14192 17346 28433 8589 |
      | n4 | 553 | 95.4 | 144.4 | 5 | 01121 | 11867 / 2995 | 6949 12729 17375 28555 8596 |
      | o5 | v1.0b5 | 109.8 | 130.0 | 4 | 0112 | 13081 / 3339 | 7877 12754 18430 26542 |
      | n5 | 553 | 116.2 | 143.6 | 5 | 01121 | 11746 / 3019 | 6590 12915 17072 28568 8584 |
      | o6 | v1.0b5 | 96.4 | 142.9 | 5 | 01121 | 10366 / 3397 | 6240 11653 16462 25766 8596 |
      | n6 | 553 | 95.9 | 143.9 | 5 | 01121 | 11689 / 3129 | 7083 12781 16888 28649 8594 |
      | o7 | v1.0b5 | 110.1 | 149.7 | 5 | 01121 | 11200 / 3612 | 7858 12533 18414 26552 8604 |
      | n7 | 553 | 95.6 | 144.3 | 5 | 01121 | 11844 / 3038 | 7048 12730 17368 28565 8601 |
      | o8 | v1.0b5 | 109.8 | 149.9 | 5 | 01121 | 11215 / 3636 | 7858 12764 18414 26497 8624 |
      | n8 | 553 | 95.7 | 144.2 | 5 | 01121 | 11815 / 3066 | 7041 12581 17403 28698 8586 |

      Entrance: v1.0b5 130.0-201.1 ms (median 148.6), 553 143.6-147.0 ms
      (median 144.4). First paint: v1.0b5 96.4-112.3 (median 109.8), 553
      92.8-95.9 except n5 116.2. New prefetch in every run: stacks=3
      pending=0 reads=86 cache=86/512 evicted=0. In n1-n3 also Games (first
      paint 93.0 / 92.9 / 102.9, entrance 142.2 / 142.2 / 136.7) and Prefs
      (72.9 / 72.9 / 85.3, entrance 130.9 / 146.9 / 134.7).
- [x] The entrance finding (a few ms slower than v1.0b5 on Coffin, 3.9,
      3.1.4.1): NOISE, not a difference between the builds. Numbers:
      - The entrance length is set by two things both builds share: whether
        the box needs 4 or 5 motion frames (130 ms against 143-150 ms; both
        builds do both) and how long the frame at scale ~235 with a 3/4 fade
        takes (it mixes the box with the backdrop twice). Frames 1-3 are the
        same in both builds.
      - n1-n3 (the session's first new runs) had that frame at 43.1-46.8 ms
        (Downloads and Games, identical to 0.2 ms) against 25.8-28.7 ms in
        every other run of either build. The same 553 binary was at 19.7 ms
        (entrance 139.1) with a 32 KB block allocated before the start
        (tool Hold), and at 28.6 ms in n4-n8 later in the session with no
        shift: the slow frame follows where memory lands, not the code.
      - The entrance code (advance_folder_motion, compose_folder_scale) is
        byte-identical in v1.0b5 and 553 (diff of both functions). What
        changed around it (prefetch, 512-entry cache with an FNV-1a key,
        per-slot prefetch, stack morphs) runs before the motion clock starts
        or not at all during it (prefetch waits while a box is open).
      - Diagnostic builds (scratch worktree, not committed) logged per frame
        the elapsed time, scale, fade quarter, compose time and task
        dispatches: 553 diag 25.3-27.5 ms, v1.0b5 diag 26.2 ms for that
        frame (compose 21-23.5 ms, 2-6 dispatches, idle 0). A sweep of the
        output buffer's address (16 openings, offset 0-30 KB) gave 25.2-28.3
        ms with no slow case, so it is not aliasing of that buffer. Why the
        frame took 43 ms in that memory state was not found.
      - First paint: the "capture" stage (screen backdrop read) is bimodal in
        both builds (v1.0b5 10.2 or 23.9-24.1 ms; 553 10.2 ms, once 30.7 ms
        in n5), which explains n5's 116.2 ms; the other 553 runs are 13-17
        ms faster than v1.0b5's slow-capture runs.
      No code change.
- [x] §4.5 hover, agent sweep (relative moves to x 350, 500, 650, 800, 950
      at y 692, then off, one per message, after 45 s idle), the same 15-pin
      row for both builds (pl: the user's paths + Archives:Games, AOS32:Prefs,
      Personal:Downloads, no "folder"; canvas 896), ChangeTaskPri -30:
      v1.0b5 / 553 / v1.0b5 render_us_avg 13374 / 10378 / 13868, max 37253 /
      18847 / 127367, stalled 9 / 4 / 8 (553 retire_waits=0
      retire_defers=5). Pass (the pacing cascade shows in v1.0b5 only).
- [x] §4.5 --bench-sweep 8 (pl, ChangeTaskPri -30), 165 redraws each:

      | build | render_us_avg | compose us | stalled |
      | --- | --- | --- | --- |
      | v1.0b5 | 7035 / 6495 / 6481 | 4753 / 4766 / 4746 | 42 / 41 / 41 |
      | 553 | 8018 / 7998 / 7432 | 6251 / 5683 / 5664 | 41 / 40 / 41 |

      Stalled equal; render_us_avg NOT within the old spread: compose is
      ~0.9 ms (about 20%) higher in 553. Same damage (kpx) and zoom levels
      frame for frame; 553 composes each frame 0.06-2.8 ms slower. The rest
      frames are pixel-identical (--pip-dump). Not memory placement: with
      Hold 32 KB / 300000 B, 553 7469 / 7478 (compose 5684 / 5710), v1.0b5
      6526 / 6612 (4784 / 4791). Code placement: the compose code is
      unchanged since v1.0b5 except the deleted divider, yet unrelated
      edits move it: v1.0b5 + a 30-line diagnostic in amiga_ui.c 5154 us;
      553 + diagnostic 6597; 553 + diagnostic + buffer pad 5104; 553 built
      with -falign-functions=32 -falign-loops=32 -falign-jumps=16 4943 /
      4947, but the padded variant with the same flags 5854 / 5868. So the
      same compose code costs 4.75-6.6 ms depending on the binary's layout,
      and alignment flags do not pin it. On 3.1.4.1 the same 553 binary was
      faster than v1.0b5 (6440 against 6753 / 6736). Not fixed (no code
      cause found); open for the owner: find the layout-sensitive part, for
      example with the 68080's data-cache hit/miss counters (DCH/DCM, movec
      $80E/$80F) around the compose.
- [x] End state: 553 in SYS:WBStartup/DockAwesome (Version 0.553; 542 was
      not locked and is deleted, copied as .new and renamed), started with
      WBRun, FindTask finds it, Status lists no CLI dock, one
      "DockAwesome.PiP2" lease (its own); its Downloads stack opened and
      closed with the user's ENVARC config. ENVARC:DockAwesome.json not
      written (10:45:25). SYS:WBStartup/Disabled holds only WBDock. RAM:
      clean (Clipboards, ENV, empty T), ENV:dpid deleted; Hold, diagnostic
      and aligned builds deleted; the three scratch worktrees removed on the
      Mac. AmigaAMP, started once by a missed click on the Music pin, was
      closed.
- [ ] NOT RUN on 3.2.3: Open Folder as a DOpus lister (Workbench is the
      desktop, --caps dopus no); the box order after Date Modified (PiP
      only); Sort by Kind and Name removing "sorts" on this boot (run with
      541).

### AmiKit XE rerun with build 553 (2026-11-25)
AmiKit XE 11.5 on the Vampire (KS 47.96 in the title bar, exec 47.7, graphics 47.4,
intuition 47.51, workbench 47.33, dopus5.library 68.16, core 12856, 24-bit
1280x720, Directory Opus 5 desktop; the clock says 02-Aug-25). Installed
before: build 544, started from WBStartup (FindTask found it).
ApolloExplorerSrv was not running and was started with Run. Build 553 = HEAD
f47f5c5, not stale (`make -n amiga` compiles nothing; the binary is
byte-identical to the 3.2.3 run's DockNew), built without warnings; baseline
tag v1.0b5 built in a scratch worktree outside the repo (no warnings,
identical to the earlier DockOld). Every destructive run used --config
RAM:<file>.json; ENVARC:DockAwesome.json (424 bytes, 17:49:26, already
migrated by 544) kept its size and date through every test. Agent input:
warps land at screen = 2*w-4 (move to 322,152 -> pointer 640,300), so every
click and drag was sent at ((X+4)/2, (Y+4)/2).
- [x] Installed in place: 544 was not locked; 553 copied as .new, 544
      deleted, renamed; Version 0.553 build 553. SYS:WBStartup/Disabled
      holds no DockAwesome leftover.
- [x] --caps: 553, v1.0b5 and the installed 544 identical (dopus yes,
      workbench_port no, shaped_windows no, depth 24, error none).
- [x] Break checks with 553 (installed dock stopped with BreakTask first;
      FindTask, Status and Sems empty before every start):

      | test | result |
      | --- | --- |
      | Ctrl-C pending through a whole start (CtrlCRun, migration config) | migrated and saved (424 bytes, no "folder", Downloads last), one frame, full profile, rc 0, ctrl_c_left 0, no lease, no leaked dock window (Wins) |
      | Break sent at once after a start | migrated file saved, full profile, no lease |
      | Break 3 s after starting --bench-sweep 8 | ended early in the sweep (redraws 10 of 113), full profile, no lease |
      | Break 6 s after starting --bench-sweep 8 | ended near the sweep's end (redraws 111 of 113), full profile, no lease |
      | pointer after the four tests | moves (322,152 -> 640,300) |

- [x] §4.4 with 553 (configs: mig = the user's 9 pins + "folder"
      Downloads, 1.0b5 format; t = the user's migrated 10 pins: 5 stacks
      RiVA, AmigaAMP, Pictures, Demos, Downloads and the volumes AmiKit:,
      Archives:):

      | check | result |
      | --- | --- |
      | migration | no "folder", Personal:Downloads last; a second start left the file unchanged (same date) |
      | A5 | "folder": "RAM:DoesNotExist" -> no requester (screenshot), dock running, file saved with [SYS:System/Shell, Personal:Pictures] and "sorts" kept |
      | stack switch, quick click | Pictures -> Demos and Demos -> Pictures |
      | stack switch, 1 s hold | Demos -> Pictures and Pictures -> Demos |
      | program pin with a box open, quick click after the pointer rests on it | Shell started from an open Pictures box, Clock from an open Demos box (2 of 2), box closed |
      | volume pin (Archives:) | DOpus lister "Archives" |
      | Sort by | heading click keeps the menu; Date Modified saved "sorts" {Pictures: date} with the check moved, kept after a restart; Kind saved "kind"; Name removed "sorts" |
      | Click to, Open Folder | "clicks" saved; a left click opens a DOpus lister "Personal:Downloads", no box, no DOS error; the menu with the lister open has the same 8 rows, no Close |
      | Open Folder on Personal:Audio (the path of the old "dos device not mounted" report) | DOpus lister "Personal:Audio" with its 3 drawers and 4 files, no DOS error |
      | box open + quick click on an Open Folder stack | Pictures box closed, Downloads lister opened |
      | restart | Downloads still opens a lister; Pictures still Date Modified |
      | Show Stack again | "clicks" entry removed; a left click opened the box (356x368) |
      | box tile | Open in Workbench in the Demos box (slot 6 of 3x3) opened a DOpus lister "Personal:Demos" |
      | no running dot | --pip-dump at exit, pointer off the dock, listers open for Downloads (Open Folder), Demos (box tile) and Archives: (volume): one dot, under Archives: only |
      | drawer drop | Personal:Audio dragged from the Personal lister -> pinned at the drop slot as a stack, its box (185x156) opened |
      | volume drop | Ram Disk dragged from the desktop -> "Ram Disk:" pinned, a click opened a DOpus lister "Ram Disk" |
      | reorder | Audio (Open Folder) dragged to slot 0: first in "paths", "clicks" kept; a click there opened its lister |
      | drag out | Audio dragged up off the dock: gone from "paths" with its "clicks" |
      | Settings | Size, Scale, Position, Cancel/Save, no Folder Stack section; Save rewrote the file (new time) and kept "sorts" |

- [x] Agent-input artifact, not a dock defect: a click warped straight from
      a pin under a zoomed row to a pin far away (Pictures -> Shell, -> Clock)
      pressed where the drawn row had no icon (`downs` not counted,
      `presses_unarmed` +1): the press reaches the dock before the row has
      zoomed around the new pointer position, and the far end icons are
      shifted outward. The same clicks after a move to the pin (as a mouse
      does) worked every time, and clicks from off the dock (row at rest)
      always hit. Same geometry as the 3.2.3 lesson on the end icon. Not
      rerun on v1.0b5.
- [x] §4.5 first open after 45 s idle, pointer off the dock, fresh dock per
      run, ChangeTaskPri -30, alternating oP / oD / n x 8. oP = the user's
      paths without Pictures + "folder" Pictures; oD = the user's 1.0b5 file
      ("folder" Downloads); n = the user's migrated file (5 stacks): Pictures
      opened after 45 s, closed, then Downloads after another 45 s. First
      paint / entrance ms:

      | run | v1.0b5 Pictures (oP) | 553 Pictures (n) | v1.0b5 Downloads (oD) | 553 Downloads (n) |
      | --- | --- | --- | --- | --- |
      | 1 | 264.0 / 134.4 | 284.6 / 134.2 | 262.1 / 156.7 | 132.8 / 145.8 |
      | 2 | 284.6 / 134.3 | 245.3 / 141.1 | 131.0 / 146.1 | 131.9 / 147.0 |
      | 3 | 306.9 / 131.7 | 284.1 / 134.3 | 143.9 / 155.0 | 134.3 / 145.3 |
      | 4 | 290.3 / 168.4 | 264.7 / 134.2 | 137.2 / 161.3 | 132.5 / 146.4 |
      | 5 | 264.0 / 174.3 | 284.2 / 134.3 | 140.9 / 157.9 | 132.3 / 246.5 |
      | 6 | 273.3 / 172.8 | 383.2 / 135.3 | 134.0 / 145.0 | 126.8 / 152.1 |
      | 7 | 356.1 / 170.3 | 285.0 / 133.8 | 148.8 / 137.3 | 121.5 / 137.4 |
      | 8 | 284.5 / 174.3 | 278.0 / 140.7 | 137.8 / 161.2 | 126.7 / 152.1 |

      Medians: Pictures first paint old 284.6 / new 284.2, entrance old
      169.4 / new 134.3 (old 4 of 8 runs have a 63-65 ms last frame, new
      none: last frame 43 ms in every run); Downloads first paint old 139.4
      / new 132.1, entrance old 155.9 / new 146.7. Box 584x527 and 356x368
      in both builds. New prefetch in every run: stacks=5 pending=0
      reads=153 cache=153/512 evicted=0. Every first open after idle
      opened at the first click (24 of 24; the earlier 544 session lost 4
      of 12).
      Two single-run values above the old maximum, judged noise:
      - n6 Pictures first paint 383.2 (old max 356.1): one stage, the glass
        capture of the screen (prepare stage 3, 159 ms against 22-52 in
        every other run of either build). The old build has single-stage
        spikes of the same size elsewhere: oD1 OpenWindow 145 ms (first
        paint 262 against 131-149), oP7 last stage 161 ms (356.1).
      - n5 Downloads entrance 246.5 (old max 161.3): one present waited 7
        fields (fields 0137, present_us_avg 33 ms); the motion code is
        byte-identical to v1.0b5 (3.2.3 analysis). Not repeated in the
        other 7 runs (137.4-152.1).
      The earlier AmiKit entrance failure (544, Pictures 164.7 / 174.2
      against an old maximum of 159.4) does not repeat: 553 133.8-141.1
      against v1.0b5 131.7-174.3. No code change.
- [x] §4.5 hover, agent sweep (relative moves to x 350, 500, 650, 800, 950
      at y 690, then off, one per message, after 45 s idle), the same 10-pin
      row for both builds (the user's migrated paths, no "folder"; v1.0b5
      shows Downloads as a plain pin), ChangeTaskPri -30:

      | run | build | render_us_avg | max us | stalled | redraws |
      | --- | --- | --- | --- | --- | --- |
      | ho1 | v1.0b5 | 12506 | 100510 | 2 | 41 |
      | hn1 | 553 | 6960 | 14894 | 4 | 40 |
      | ho2 | v1.0b5 | 10502 | 37030 | 3 | 40 |
      | hn2 | 553 | 7005 | 14719 | 1 | 40 |
      | ho3 | v1.0b5 | 7578 | 15114 | 3 | 41 |

      render_us_avg: new below the old minimum. Stalled: new 4 and 1 against
      old 2-3 (means 2.5 / 2.7, about 40 frames a run); within the noise.
      553 retire_waits 0, retire_defers 0 / 1. Pass. (A first ho1 with the
      five moves sent in one message, 9942 us, stalled 0, was discarded.)
- [x] §4.5 --bench-sweep 8 (same row, ChangeTaskPri -30, one script,
      113 redraws each): render_us_avg v1.0b5 / 553 / v1.0b5 / 553 /
      v1.0b5 6221 / 5865 / 6214 / 5873 / 7039 (one 99 ms frame), compose
      4576 / 4268 / 4559 / 4276 / 5393, stalled 40 / 40 / 40 / 38 / 40.
      Pass. Compose is 0.3 ms faster in 553 here, slower on 3.2.3 and faster
      on 3.1.4.1: it follows the binary's layout, as on 3.2.3.
- [x] No defect found; no code change.
- [x] Seen during the session, cause not found: the three Directory Opus
      listers that were open at the start (Personal, Personal:Applications,
      Personal:Applications/AmiMCP) were closed when the tests ended. They
      were still open after the drag tests; no agent click was aimed at
      their close gadgets. Reopened by ARexx at the same places and sizes
      in icon mode.
- [x] End state: 553 in SYS:WBStartup/DockAwesome (Version 0.553), started
      with Run from a Stack 400000 script (WBRun does not work under DOpus),
      the only dock (Status lists it once, no test dock); its Downloads stack
      opened (356x368) and closed with the user's ENVARC config, which the
      tests never wrote (424 bytes, 17:49:26). RAM: holds only Clipboards,
      ENV, Logs and T (amiagent's own files); ENV:dpid deleted; the scratch
      worktree is removed on the Mac. ApolloExplorerSrv runs until reboot.
- [ ] NOT RUN on AmiKit: a v1.0b5 control for the warp-click artifact; the
      box order after Date Modified (PiP only).

### Open Folder tile in the Folder Stack box (AmiKit XE, 2026-11-25)
Owner's request: the box's last tile looked too large next to the real
icons. Implemented exactly as decided: a smaller ring, one line "Open
Folder", the muted heading colour. The action is unchanged.
- [x] `src/amiga_ui.c`: `folder_open_radius` (icon*36/100, rounded: 13 px
      at 36, 17 at 48, 26 at 72, about 72%) at the same centre;
      `folder_open_weight` keeps the ring weight of the old radius
      (icon/2-2), in the AA path and the native fallback; the arrow scales
      with the ring (a filled shape). Label `FOLDER_OPEN_LABEL` "Open
      Folder" (shared with the menu's Click to row), in `FOLDER_MUTED`
      like the ring; `centered_folder_text` takes the colour. `action_wrap`
      and its extra row removed; `folder_grid` makes cells wide enough for
      "Workbench 0" and "Open Folder" (the wider + 6), so the label always
      fits `cell_width-6`. No DOpus-specific label.
- [x] `--pip-dump` writes `prefix-folder.ppm` when a box page settles with
      its icons (`dock_ui_folder_artwork`, keyed by a page counter so
      drawers opened inside and page flips dump again; Opus review P2).
- [x] Builds: before = HEAD + dump (554), after (555), final with the
      review fix 556; `make amiga` without warnings, `make host-test` and
      `make host-sanitize` pass (the layout is in amiga_ui.c and not
      host-tested; no test asserted the wrap).
- [x] Review (Opus, fresh): no P0/P1; P2 dump not repeated for a page
      shown in place -> fixed with the page counter, checked on the
      Amiga (Pictures box dumped, then its Wallpapers drawer dumped again
      as a 185x200 box).
- [x] Before/after on AmiKit (Demos, 6 entries; Pictures, 89 entries) at
      the user's settings with only the size changed, test configs in RAM:

      | case | before | after |
      | --- | --- | --- |
      | Demos, size 36 | 185x209, ring fills the cell, "Open in" / "Workbench" white | 185x200, smaller muted ring, "Open Folder" muted on one line |
      | Demos, size 48 | 224x255 | 224x246 |
      | Demos, size 72 | 334x342 (one line already) | 334x342 |
      | Pictures, size 36 | 584x527 | 584x518 |

      Box widths and cells unchanged; at 36 and 48 the box is one text line
      (9 px) shorter. "Open Folder" draws whole (no "...") in every case.
- [x] Tile click: build 555 test dock and the installed 556 each opened a
      DOpus lister "Personal:Demos" from the Demos box tile, box closed.
- [x] First open after 45 s idle, Pictures, pointer off the dock, fresh
      dock per run, ChangeTaskPri -30, alternating (first paint / entrance
      ms; prefetch pending=0 reads=153 in every run):

      | run | 553 | 556 |
      | --- | --- | --- |
      | 1 | 282.4 / 134.3 | 273.0 / 165.7 |
      | 2 | 360.0 / 178.9 | 306.0 / 132.9 |
      | 3 | 264.3 / 134.5 | 284.4 / 134.5 |

      Medians 282.4 / 134.5 (553) and 284.4 / 134.5 (556); the earlier
      AmiKit figures for 553 were 284.2 / 134.3. Each build has one slow
      entrance (553 178.9, 556 165.7, a frame waiting 4 fields): within
      the same-binary spread. Pass.
- [x] Hover sweep (5 moves at y 690, one per call, 45 s idle, same row):
      553 render_us_avg 7359, max 14546, stalled 0; 556 7283, 14535,
      stalled 2 (553 had 4 and 1 in the earlier runs). Pass. A first 556
      sweep sent the moves in one message and was discarded.
- [x] End state: 556 in SYS:WBStartup/DockAwesome (copied as .new, 553
      deleted, renamed; Version 0.556), started with Run from a Stack
      400000 script, the only dock (Status lists it once); Disabled holds no
      DockAwesome; ENVARC:DockAwesome.json untouched (424 bytes, 17:49:26);
      RAM: holds only Clipboards, ENV, Logs and T; ENV:dpid deleted.
      Images on the Mac in the session scratchpad `design/`.
- [x] Seen: the box's "Demos" entry in Personal:Demos is an iGame project;
      a test click on it opened iGame's "can't open ... library"
      requester (iGame's own, closed with OK). Not a dock issue.
- [ ] NOT RUN: the tile on ApolloOS, Coffin, 3.2.3, 3.9, 3.1.4.1 (same
      code path on every system; not booted); the native (no AA) fallback
      of the ring, which no booted system uses; a paged box (no drawer here
      has more than one page at 36 px). docs/images/folder-stack.png still
      shows the old tile (and the old divider).

### Open Folder tile on AmigaOS 3.2.3 with build 556 (2026-11-25)
AmigaOS 3.2.3 on the Vampire (Workbench desktop), agent input (warps 1:1).
Build 556 = HEAD 3ea4b09, not stale (`make amiga`: nothing to rebuild; no
warnings). Installed before: 553 from WBStartup, stopped with BreakTask
(FindTask and the PiP2 lease gone after 5 s). Test docks from RAM: with
`--config RAM:` copies (pl = the user's 12 pins + Archives:Games +
AOS32:Prefs + Personal:Downloads, three stacks); exactly one dock at a time.
- [x] `--pip-dump` of the Downloads box (356x359, 30 entries, size 36): the
      tile shows the smaller muted ring and "Open Folder" on one line, like
      AmiKit. Images: scratchpad `design/323-556-downloads.png` (and -2x,
      `323-556-cell-4x.png`).
- [x] Tile click: Workbench window "Downloads" opened, box closed.
- [x] Quick switch Games -> Downloads and Downloads -> Games (quick clicks,
      a move to the pin first); with the Games box open a quick click on
      TextEdit started TextEdit and closed the box.
- [x] First open after 45 s idle, pointer off the dock, fresh dock per run,
      ChangeTaskPri -30, pl copies, Downloads opened and closed:

      | run | first paint ms | entrance ms | frames | note |
      | --- | --- | --- | --- | --- |
      | 1 | 90.2 | 149.6 | 5 | |
      | 2 | 103.5 | 136.0 | 5 | capture stage 23.4 ms (bimodal, known) |
      | 3 | 90.1 | 145.0 | 5 | |
      | 4 | 181.9 | 138.2 | 5 | backdrop-copy stage 94.5 ms (3.0 elsewhere) |
      | 5 | 103.6 | 136.2 | 5 | capture stage 23.6 ms |

      553 (same boot, earlier): first paint 92.8-95.9 (once 116.2), entrance
      143.6-147.0. Entrance within the spread; the fast-capture first paints
      (90.1/90.2) are under 553's. Run 4 is a single-stage spike in a stage
      the tile change does not touch (the text stage is 6.0-6.4 ms in every
      run); judged noise. Prefetch pending=0 reads=86 every run.
- [x] Hover sweep (relative moves to x 350, 500, 650, 800, 950 at y 692,
      one per call, 45 s idle, pl, -30): render_us_avg 9626, max 18342,
      stalled 2 (553: 10378 / 18847 / 4). Pass.
- [x] Owner's report: the Downloads window (1034,599 169x96) opened "behind
      the dock". Findings:

      | question | answer |
      | --- | --- |
      | where the position comes from | the drawer icon's snapshot: Personal:Downloads.info is a PNG icon; its icOn chunk holds drawer x 1034, y 599, w 169, h 96 (file dated 18-May-26). The dock calls `OpenWorkbenchObjectA(path,NULL)` and sets no position |
      | v1.0b5 | same call; its plain drawer pin for Personal:Downloads opened the window at 1034,599 169x96 |
      | dock pixels over the window? | `--pip-dump` (composed vs source), 15 pins: at rest x 327-952, y 665-716; zoomed on Downloads x 270-965, y 610-716 (with the label). Window x 1034-1202: 0 covered pixels, 69 px clear. The user's 13-pin row ends further left |
      | what can cover it | the open Downloads box (746-1101 x 279-637 with 15 pins, 706-1061 with 13) covers the window's top-left corner while the box is open or closing (popover over everything, by design) |

      Not a regression and no overlap with the row measured; no code change.
      Options for the owner if wanted: none for the row; for the box, open
      the window only after the dismissal motion ends (it already closes
      first). Moving a window away from its snapshot would override the
      user's own placement, so not proposed.
- [x] Seen once: with the 556 test dock running, a click on the window's
      close gadget right after it opened did nothing; repeated 3 times
      later (twice with the installed 556 right after the tile click, once
      with no dock) it closed every time. Not reproduced.
- [x] End state: 556 in SYS:WBStartup/DockAwesome (copied as .new, 553
      deleted (not locked), renamed; Version 0.556), started with WBRun;
      FindTask finds it, one DockAwesome.PiP2 lease, no CLI dock in Status;
      its Downloads box opened and closed, its tile opened the window.
      ENVARC:DockAwesome.json untouched (568 bytes, 17:36:48). Disabled
      holds only WBDock. RAM: holds only Clipboards, ENV, T (empty); ENV:dpid
      deleted. Scratch upload/download folders removed on the Mac.
- [ ] NOT RUN on 3.2.3: DOpus lister (Workbench is the desktop); the tile on
      a paged box.

### Folder Stack box buttons: centred glyphs, smaller disc, parent-drawer arrow (AmigaOS 3.2.3, 2026-11-26)
Owner's report: the back button's "<" was not centred in its circle; the
page buttons "<" ">" share the code. Follow-up requests in the same task:
draw the buttons smaller; make the parent-drawer button look different
from the page-back button (an up chevron, then a "Back"/"BACK" pill, both
rejected; the owner now chooses from candidates A-E, the build carries A).
- [x] Measured before the fix (build 556, `--pip-dump` box images of a
      RAM: test drawer with a Sub drawer of 360 files, page 2 of 4-8, icon
      sizes 36, 41, 48, 72; the control is 19 px at every size, Helvetica 9
      + 10). The chevron's pixel bounding box sat 0.50 px toward its open
      side in every button (back and page-back right, page-forward left),
      the ink centroid 0.70-0.73 px; vertical 0.00 (footer readings
      within +-0.1 of the glass gradient). Host model: the end points were
      shifted radius/16 (0.56 px at 19 px, 0.50 at 18 px).
- [x] Fix (`dock_button_glyph`/`dock_button_coverage`/`dock_button_pixels`
      in dock_geometry.c, host-tested): the glyph is up to three
      round-capped segments whose ink box is centred on the disc; disc and
      glyph drawn at 5/6 of the control (12 px floor), stroke half width
      11/16 px (was 13/16, scaled with the glyph); hit rectangles, layout
      and actions unchanged. The header button shows a stemmed up arrow
      (`DOCK_BUTTON_PARENT`, candidate A). Native fallback: same disc
      inset, chevrons two pixels wide, the arrow's stem one pixel; centred
      exactly along the pointing direction, half a pixel across at even
      controls.
- [x] After (build 563 dumps, sizes 36/48/72; 41 with 559, same page-button
      code): page buttons bbox offset 0.00 px both axes (footer y +0.00 to
      +0.08 = glass-gradient noise of the measurement), ink centroid
      0.16-0.17 px toward the open side; disc 15.9 px (was 19.0). Parent
      arrow bbox 0.00/0.00, ink centroid 0.54 px up (the head carries more
      ink; the bounding box is centred). The 72 px page-forward button lies
      over the bright backdrop logo in the glass and does not measure
      (before and after alike). Host: exact for every control 8-24 px,
      both pixel phases; within half a pixel above 24 px.
- [x] Clicks with 559/560/563 test docks: the header button opens the
      parent drawer, ">" and "<" turn pages.
- [x] Host tests: `test_button_glyph` (centring, mirror symmetry, size
      5/6, native polylines) in test_geometry; `make host-test`,
      `make host-sanitize` pass; `make amiga` without warnings.
- [x] Reviews (Opus, fresh): first diff no P0/P1, four P2 fixed (native
      arm clamp before the parity step, native size guard, one direction
      rule, test comment); second review (arrow diff): no P0/P1, page
      chevrons byte-identical to the first fix for sizes 6-60; three P2
      fixed (native arrow direction asserted, head comment, control capped
      at 96 px for the 32-bit distance math).
- [x] First open (3 runs, 45 s idle, pl copies, -30, build 559, the paint
      code of the page buttons is the same): first paint 90.3 / 103.8 /
      90.1 ms, entrance 149.4 / 136.1 / 149.8 ms (556: 90.1-103.6 /
      136.0-149.6). Hover sweep: render_us_avg 9857, max 18667, stalled 1
      (556: 9626 / 18342 / 2). Pass.
- [x] Images: scratchpad `design/btn-compare-{36,48,72}.png` (10x, before
      556 / after 563, cross-hair on the control centre),
      `design/btn-{before,after}-36-nested-p2-2x.png`, candidates
      `design/parent-candidates-sheet.png` (A-E at 4x and real size beside
      the page buttons, sizes 36/48/72) and `design/parent-*-1x.png`.
- [x] Installed: build 564 (HEAD = the arrow commit) in
      SYS:WBStartup/DockAwesome (copied as .new, 560 deleted, renamed),
      started with WBRun; FindTask finds it, one DockAwesome.PiP2 lease, no
      CLI dock. The same file run from the CLI with a RAM: test config and
      --pip-dump gave a box image byte-identical to 563's (arrow bbox
      0.00/0.00, page buttons 0.00); its arrow opened the parent drawer.
      ENVARC:DockAwesome.json untouched (568 bytes, 17:36:48). RAM: holds
      only Clipboards, ENV, T; ENV:dpid deleted.
- [x] Owner chose C (return arrow, up then left) from the contact sheet.
      Build 566/567: 8 round-capped segments on a grid of one pixel at the
      19 px control (stem x 3 from y 5 to 1, quarter turn of radius 2, bar
      at y -1 to x -3, head arms from (-1,-3) and (-1,1)), stroke half
      9/16 px (the sheet's 11 with a smaller head was muddy at 1x; one
      pixel lines keep the head crisp). Box-centred it read high (ink
      centroid 0.85 px up), so it sits one pixel low: box offset +1.00 px
      down, ink centroid 0.15 px down and 0.29 px right (options in
      `design/parent-C-centring-options.png`). Dumps at 36/48/72: exactly
      that in all three; page buttons unchanged (0.00). The arrow opens the
      parent drawer. Coverage now skips the segment tests outside the
      glyph's ink box (reviewer: byte-identical without the early-out).
      Native: three one pixel polylines, half a pixel off both ways at even
      controls.
- [x] Review (Opus, fresh, delta): no P0/P1; seven P2 (comments, an ink
      assertion, an unused mirror glyph in the test) fixed.
- [x] Installed: 567 in SYS:WBStartup/DockAwesome (in place, WBRun), the
      only dock; the installed file run from the CLI with --pip-dump gave
      a box image byte-identical to 566's. ENVARC untouched (568 bytes,
      17:36:48); RAM: holds only Clipboards, ENV, T; ENV:dpid deleted.
      Images also in ~/Downloads/DockAwesome-design/.
- [ ] NOT RUN: the native (no AA) path on hardware (no booted system uses
      it; host-tested only); an 18 px control (topaz 8 fallback; host
      only); other OSes.
- [x] Owner's correction 2026-11-26: "Alltid exakt!" and one shared,
      slightly more faded tone for the controls. Build 569:
      - Return arrow centred by its box again (0.00 / 0.00 in the dumps at
        36/48/72; ink centroid 0.24 px right, 0.71 px up, not compensated).
      - Open Folder ring: was centred on a pixel centre, half a pixel right
        and down of an even icon's box (ring box centre 662.50, 522.50 for
        the icon box 662, 522 at size 36); now 662.00, 522.00. Coverage in
        `dock_open_glyph`/`dock_open_glyph_row` (host-tested); the arrow
        outlines are centred on the ring by their box (tables shifted
        (-1, +3)/64) and their vertices lie on multiples of 4/16 px so no
        sample falls on one; ring and arrow alone are pixel-exact for every
        icon size 16-192 (host test). Native ring still half a pixel off
        for even icons (DrawEllipse; documented).
      - Tones measured in the 556/566 dumps: page/parent strokes, title and
        page count #f2f4f8; Open Folder ring and label #aeb8c7; file names
        #f2f4f8. Now all controls #dee2e9 (`FOLDER_GLYPH`, the menu text
        30% toward the muted tone); file names #f2f4f8; disc tint
        unchanged. WCAG contrast of #dee2e9: 13.26 on the darkest glass
        (black backdrop, #181b22), 8.76 on the lightest (white backdrop,
        #373a41, measured over a Workbench window), 7.52 / 5.08 on the disc
        over those; #f2f4f8 had 15.65 / 10.34 / 8.87 / 6.00 (-15%), #aeb8c7
        8.60 / 5.68 / 4.87 / 3.30.
      - Review (Opus, fresh): no P0/P1; four P2 fixed (arrow-only pixel
        test and the vertex rule, a pen-collision guard for FOLDER_GLYPH,
        the native ring comment, a stale test comment).
      - Installed 569 in place (WBRun), the only dock; the installed file
        run from the CLI gave a dump byte-identical to the test run.
        ENVARC:DockAwesome.json was changed by the owner at 01:03 (added
        Personal:Audio, 590 bytes), not by the tests (they use RAM:
        copies). A Workbench window "Personal" was open during the runs
        (not opened by the tests' clicks as far as seen; left open).
      - Images: scratchpad `design/btn-compare-{36,48,72}.png`,
        `design/tone-compare-36.png`, `design/btn-{before,after}-36-nested-p2-2x.png`,
        also in ~/Downloads/DockAwesome-design/.

### AmigaOS 3.1.4.1 with build 569 (2026-11-26)
AmigaOS 3.1.4.1 on the Vampire (Kickstart 46.143, Workbench 45.194),
24-bit 1280x720, agent warps 1:1. Build 569 = HEAD d175ad7 (the last
commit touched only docs; `make amiga`: nothing to rebuild, no warnings).
Installed before: 553 from WBStartup. ApolloExplorerSrv started with
Run >NIL:. Test docks from RAM: with `--config RAM:` copies, exactly one
dock at a time (Status, FindTask and Sems checked before each start, 5 s
or more before each stop).
- [x] Installed 569 in place: 553 stopped (BreakTask, gone after 5 s),
      569 copied to SYS:WBStartup/DockAwesome.new, the old file was "in
      use" so it became SYS:WBStartup/Disabled/DockAwesome-locked-delete,
      .new renamed into place (Version 0.569); WBRun started it, FindTask
      finds it, one DockAwesome.PiP2 lease.
- [x] Box dumps (`--pip-dump`, RAM:BtnTest = Sub (360 files) + t10-t15;
      pins Shell, RAM:BtnTest, Editor), nested Sub page 2, sizes 36/48/72:
      the same box sizes as on 3.2.3 (698x598, 836x541, 1252x465).

      | control | 3.1.4.1 bbox offset (x / y) | 3.2.3 (569) |
      | --- | --- | --- |
      | parent arrow, 36 / 48 / 72 | 0.00 / 0.00 each | 0.00 / 0.00 |
      | page back "<", 36 / 48 / 72 | 0.00 / -0.01, 0.00 / +0.02, 0.00 / 0.00 | 0.00 / +0.06, 0.00 / +0.02, 0.00 / 0.00 |
      | page forward ">", 36 / 72 | 0.00 / -0.03, -0.01 / 0.00 | 0.00 / +0.03, - (over the backdrop logo) |
      | page forward ">", 48 | over the backdrop's bright "OS3" logo: does not measure; its full-tone pixel at the same place as on 3.2.3 | 0.00 / +0.08 |
      | Open Folder ring box, 36 | 648..676 x 508..536, centre 662.00 / 522.00 = icon box centre | the same |

      Footer y readings within +-0.03 are the glass gradient. Ink centroids
      equal 3.2.3's (arrow 0.24 right / 0.71 up; chevrons 0.12-0.13 toward
      the open side). The tile's full-tone pixels (ring, arrow, label) lie
      at exactly the 3.2.3 positions at all three sizes (198 / 354 / 767
      pixels). The fixed `ring.py` threshold read the 3.1.4.1 backdrop
      texture under the glass; `ring2.py` (mean > 70) measures both dumps
      the same.
- [x] Tone (36, brightest pixel of each element): title, parent arrow,
      "<", ">", page count, Open Folder ring and label all #dee2e9; file
      names #f2f4f8; contrast on the glass 12.67-13.26 (3.2.3: 13.00-13.26).
- [x] Downloads box at the dock's own size (36, a RAM: copy of the user's
      ENVARC file): 356x359, 30 entries, the tile last with the small ring
      and "Open Folder" on one line, both #dee2e9; title #dee2e9, names
      #f2f4f8.
- [x] Behaviour (569 test docks):

      | check | result |
      | --- | --- |
      | Open Folder tile (BtnTest; Downloads) | Workbench window "BtnTest" / "Downloads" (1034,599 169x96) opened, box closed |
      | parent arrow in Sub | back to the BtnTest box (185x200) |
      | page buttons | ">" to 2/4 (36), 2/5 (48), 2/8 (72); "<" back to page 1 (no "<" shown there) |
      | quick switch (pl copy, 15 pins) | Downloads -> Games (box anchor 744 -> 667) -> Downloads (746) |
      | program pin with a box open | Editor started, box closed |
      | Sort by | Date Modified saved "sorts": {"Personal:Downloads": "date"}, check moved |
      | Click to | Open Folder saved "clicks", left click opened the window, no box; Show Stack removed "clicks", left click opened the box |

      A dump copied right after a page click still showed the previous
      page: the dump is written when the new page has settled with its
      icons. Wait 2 s before copying.
- [x] First open after 50 s idle, pointer off the dock, fresh dock per
      run, ChangeTaskPri -30, o.json (1.0b5 format, migrated each run):

      | run | first paint ms | entrance ms | fields |
      | --- | --- | --- | --- |
      | 1 | 94.1 | 140.7 | 01112 |
      | 2 | 94.3 | 139.5 | 01112 |
      | 3 | 94.0 | 140.8 | 01112 |
      | 4 | 94.2 | 139.1 | 01112 |

      553 on 3.1.4.1: 98.9-99.5 / 135.6-136.5. First paint 5 ms faster;
      entrance 3-4 ms longer with the same five frames and field pattern
      (no extra field; a phase within the last field, the known
      memory-placement spread; 3.2.3 showed 136-150 for one binary).
      Prefetch pending=0 reads=84 every run. Pass.
- [x] Hover sweep (relative moves to x 350, 500, 650, 800, 950 at y 692,
      one per call, 50 s idle, pl copy, -30): render_us_avg 10680, max
      101715 (the first frame after idle), stalled 2. 553: 9610 / 18504 /
      1; v1.0b5: 9529 and 10719 (max 100610) / stalled 1 and 0. Within the
      old builds' spread. Pass.
- [x] Seen: RAM:BtnTest stayed "in use" after its contents were deleted
      (empty drawer, held until reboot). Not reproduced in five control
      runs on RAM:LockA (box, Sub, page 2, parent, tile window, close;
      quit with the box open on page 2; Break right after a page click;
      the same with --pip-dump) nor with acp downloads of an assign or of
      RAM: (killed mid-way and complete): every drawer deleted. The holder
      is unknown; no dock defect shown, no code change.
- [x] My diagnostic tool CDirs (NameFromLock on every process's current
      and home directory) blocked inside a DOS call, twice, and ignores
      Break; two "RAM:CDirs" processes and the empty RAM:cd.txt stay until
      reboot. Do not call NameFromLock on other processes' locks.
- [x] End state: 569 in SYS:WBStartup/DockAwesome, started with WBRun,
      FindTask finds it, one DockAwesome.PiP2 lease, no CLI dock; its
      Downloads box opened and closed. ENVARC:DockAwesome.json untouched
      (582 bytes, 02-Aug-25 15:35:16, the migration date at the reboot
      after 553). RAM: holds Clipboards, ENV, T and, until reboot, the
      empty BtnTest drawer and cd.txt; ENV:dpid deleted; DL: assign removed.
      SYS:WBStartup/Disabled holds WBDock and, until reboot (locked by
      WBRun), DockAwesome-locked-delete (553). Images: scratchpad
      `design/3141-569-*.png`, also in ~/Downloads/DockAwesome-design/.
- [ ] NOT RUN on 3.1.4.1: the native (no AA) path (not used here); DOpus
      lister (Workbench is the desktop).

### AmiKit XE with build 569 (2026-11-26)
AmiKit XE 11.5 (KS 47.96, Directory Opus 5 desktop), 24-bit 1280x720,
agent input at ((X+4)/2, (Y+4)/2). Build 569 = 08de37b (HEAD 5fead28; the
commits since touched only docs; `make amiga`: nothing to rebuild, no
warnings; the binary is byte-identical to the 3.1.4.1 upload). Installed
before: 556 from WBStartup (FindTask found it, one PiP2 lease), stopped
with BreakTask. ApolloExplorerSrv started with Run. Test docks from RAM:
with `--config RAM:` copies, exactly one dock at a time (Status, FindTask
and Sems before each start).
- [x] Installed 569 in place: copied as DockAwesome.new, 556 deleted (not
      locked), renamed; Version 0.569 build 569. Disabled holds no
      DockAwesome.
- [x] Box dumps (`--pip-dump`, RAM:BtnTest = Sub (360 files) + t10-t15;
      pins Shell, RAM:BtnTest, MultiView), nested Sub page 2 (2/4, 2/5,
      2/8), sizes 36/48/72: 698x598, 836x541, 1252x465 as on 3.2.3. A CON
      window with no text (`Run >NIL: Wait 1500 >CON:0/23/1280/640/Plain`)
      lay behind the box so the glass was even: on the bare desktop the BOOT
      icon lay under the 36 px parent arrow and read +1.65 px.

      | control | AmiKit bbox offset (x / y), 36 / 48 / 72 | 3.2.3 (569) |
      | --- | --- | --- |
      | parent arrow | 0.00 / -0.05, 0.00 / 0.00, 0.00 / 0.00 | 0.00 / 0.00 each |
      | page back "<" | 0.00 / 0.00 each | 0.00 / +0.06, 0.00 / +0.02, 0.00 / 0.00 |
      | page forward ">" | 0.00 / 0.00 each | 0.00 / +0.03, -0.01 / +0.08, - (backdrop logo) |
      | Open Folder ring (full-tone box, 36) | 649..675 x 509..535, centre 662.00 / 522.00 = icon box centre | the same |

      The -0.05 is the CON window's title bar under the glass (gradient
      noise, as the +-0.08 on 3.2.3). Ink centroids equal 3.2.3's (arrow
      0.24 right / 0.63-0.71 up; chevrons 0.13 toward the open side). Every
      full-tone #dee2e9 pixel (buttons, ring, arrow, label, title, page
      count) lies at exactly the 3.2.3 positions: 274 / 431 / 845 pixels at
      36 / 48 / 72, 0 differ. Tone (36, brightest pixel per element): title,
      parent arrow, "<", ">", page count, ring and label #dee2e9; file names
      #f2f4f8.
- [x] Images: scratchpad `design/ak-569-btn-compare-{36,48,72}.png` (10x,
      3.2.3 above AmiKit, cross-hair on the control centre),
      `design/ak-569-nested-p2-36-2x.png`, `-48.png`, `-72.png`,
      `design/ak-569-open-folder-36-6x.png`; also in
      ~/Downloads/DockAwesome-design/.
- [x] Behaviour (569 test docks, agent clicks):

      | check | result |
      | --- | --- |
      | Open Folder cell, BtnTest box | DOpus lister "Ram Disk:BtnTest" (Sub, t10-t15), no DOS error, box closed |
      | Open Folder cell, Personal:Audio box (185x147) | DOpus lister "Personal:Audio" (MOD, MP3, Streams, Apollo.mp3), no DOS error |
      | parent arrow in Sub (page 1) | back to the BtnTest box (185x200) |
      | page buttons | ">" to 2/4 (36), 2/5 (48), 2/8 (72); "<" back to 1/4 (dump) |
      | quick switch | Downloads (356x359) -> Demos (185x200) -> Downloads |
      | program pin with a box open | Downloads box open, pointer moved to Clock, click: Clock started, box closed |
      | Click to | Open Folder saved "clicks" {Downloads: folder}; left click opened a DOpus lister "Personal:Downloads", no box; Show Stack removed "clicks"; left click opened the box |

      The three test listers and Clock were closed again.
- [x] First open after 45 s idle, pointer off the dock (parked at 640,300
      on the DOpus desktop), fresh dock per run, ChangeTaskPri -30, a copy
      of the user's ENVARC file per run; Pictures opened and closed, then
      Downloads after another 45 s; 556 control runs alternated (first
      paint / entrance ms):

      | run | Pictures | Downloads |
      | --- | --- | --- |
      | 569 f1 | 284.5 / 214.4 (fields 0118) | 128.2 / 130.9 |
      | 556 g1 | 283.7 / 134.6 | 121.4 / 137.8 |
      | 569 f2 | 271.9 / 165.6 | 123.8 / 153.8 |
      | 556 g2 | 284.6 / 174.4 | 121.3 / 137.6 |
      | 569 f3 | 287.7 / 170.8 | 124.8 / 154.0 |
      | 569 f4 | 284.3 / 135.1 | 129.5 / 188.2 (fields 0113) |

      Medians 569: Pictures 284.4 / 168.2, Downloads 126.5 / 153.9. Earlier
      AmiKit figures: 556 Pictures 273.0-306.0 / 132.9-165.7 (medians 284.4
      / 134.5); 553 Downloads 121.5-134.3 / 137.4-246.5 (medians 132.1 /
      146.7). Pictures first paint equal; Downloads first paint inside 553's
      range, 4-8 ms over the two 556 runs. Entrances: same frame counts;
      the spread is in the fields a present waits (f1 one 8-field wait,
      f2/f3 Downloads 01213 against 556's 01122), the pattern of the known
      single-run outliers (553 246.5, v1.0b5 174.3). Per stage the 569
      "layer" step (finish_folder_layer) took 47.8-48.7 ms (Pictures) and
      30.7-31.3 ms (Downloads) against 556's 40.5 and 21.5, and the "read
      back" step was shorter; finish_folder_layer and its callees are
      unchanged since 556, so that shift follows the binary's layout or
      memory, not code (3.2.3 lesson). Not shown to be a regression; no
      rebuild with an unrelated change was run. Prefetch pending=0
      reads=153 in every run.
- [x] FINDING, pre-existing (544 finding; also 556 here): the first agent
      click after idle was lost in 5 of 8 first opens with 569 (Pictures 4
      of 4, Downloads 1 of 4) and 3 of 4 with 556 (Pictures 2 of 2,
      Downloads 1 of 2); a second click opened the box each time and the
      figures above are from that click. Profile: `PiP receiver: arms=1
      disarms=0 presses_armed=1..2`: with the pointer parked on the DOpus
      desktop the receiver rests on the plate (dopus_desktop, by design for
      DOpus drops), and the warp press lands on it before any hover. The
      553 session opened 24 of 24 at the first click; why is not known (its
      parking spot was not recorded). Same in both builds, so not a 569
      defect; physical-mouse effect not measured; left open for the owner
      as before.
- [x] Hover sweep (relative moves to x 350, 500, 650, 800, 950 at y 690,
      one per message, 45 s idle, user's file copy, -30): render_us_avg
      6957, max 14374, stalled 2 (556: 7283 / 14535 / 2; 553: 7359 / 14546
      / 0). Pass. A first sweep sent the five moves in one message and was
      discarded.
- [x] Seen: the Amiga clock jumped from 15:47 to 17:29 mid-session
      (AmiKit's time sync); the measuring CON window's `Wait 1200` ended at
      that moment and the open box closed with it. The box then reopened
      normally.
- [x] End state: 569 in SYS:WBStartup/DockAwesome, started from a script
      `Run >NIL: Stack 400000 +` / `SYS:WBStartup/DockAwesome` (the script
      ends at once and was deleted; a script that runs the dock itself
      stays "in use"); Status shows it once with stk 400000, one
      DockAwesome.PiP2 lease, no test dock; its Downloads box opened
      (356x359) and closed with the user's config. ENVARC:DockAwesome.json
      untouched (424 bytes, 17:49:26, as since the 553 session). RAM: holds
      only Clipboards, ENV, Logs and T (amiagent's files); ENV:dpid
      deleted; DL: assign removed. No DOpus listers were open at the start
      (the machine had been rebooted; `lister list` empty, desktop
      screenshot), so none were left open. ApolloExplorerSrv runs until
      reboot.
- [ ] NOT RUN on AmiKit: a rebuild with an unrelated change to settle the
      layer-stage shift; the native (no AA) path (not used here).

### ApolloOS with build 569 (2026-11-26)
ApolloOS R9.5 on the Vampire (Kickstart 51.3, 16-bit 1280x720), agent
input with relative moves. Build 569 = 08de37b (HEAD 866c55d changed only
docs; `make amiga`: nothing to rebuild, no warnings). Installed before:
551. Test docks from RAM: with `--config RAM:` copies, exactly one dock at
a time (Tasks/FindTask/Sems before each start, 5 s or more before each
stop).
- [x] "Not in Status": the dock was running. Status lists only CLI
      processes; the exec task list showed "DockAwesome" (pri 1) and its
      "DockAwesome Workbench status" task, and the DockAwesome.PiP2
      semaphore existed. It had started from SYS:WBStartup at boot (551).
      No WBRun needed.
- [x] Installed 569 in place: 551 copied to RAM:DockOld for controls,
      stopped (BreakTask, gone after 5 s); the old leftovers
      Disabled/DockAwesome-locked-delete (538) and -locked-delete2 (550)
      were not locked after the reboot and are deleted; 569 copied as
      DockAwesome.new, the 551 file was "in use" (started from WBStartup),
      so it is now SYS:WBStartup/Disabled/DockAwesome-locked-delete
      (delete after the next ApolloOS boot); .new renamed into place,
      Version 0.569 build 569.
- [x] Box dumps (`--pip-dump`, RAM:BtnTest = Sub (360 files) + t10-t15;
      pins CLI, RAM:BtnTest, Editor), nested Sub page 2 (2/4), sizes
      36/48/72: 698x598, 836x541, 1252x465 as on 3.2.3.

      | control | ApolloOS bbox offset (x / y), 36 / 48 / 72 | 3.2.3 (569) |
      | --- | --- | --- |
      | parent arrow | 0.00 / 0.00, +0.01 / -0.01, 0.00 / 0.00 (*) | 0.00 / 0.00 each |
      | page back "<" | -0.01 / -0.01, -0.01 / 0.00, 0.00 / 0.00 | 0.00 / +0.06, 0.00 / +0.02, 0.00 / 0.00 |
      | page forward ">" | 0.00 / 0.00, 0.00 / +0.01, -0.01 / +0.01 | 0.00 / +0.03, -0.01 / +0.08, - |
      | Open Folder ring (36) | 648..676 x 508..536, centre 662.00 / 522.00 = icon box centre | the same |

      (*) At 72 the coverage measure read -0.04 / -0.87: the backdrop's
      red nebula lies under the glass there. The neutral stroke pixels
      (min channel > 110 and > 140) give the box 20..27 x 12..21, 0.00 /
      0.00, the same as 3.2.3's dump. Every full-tone #dee2e9 pixel lies
      at exactly the 3.2.3 positions: 274 / 431 / 845 pixels at 36 / 48 /
      72, 0 differ. Ink centroids equal 3.2.3's (arrow 0.24 right / 0.70
      up; chevrons 0.11-0.15 toward the open side). Tone (36, brightest
      pixel per element): title, parent arrow, "<", ">", page count, ring
      and label #dee2e9; file names #f2f4f8; contrast 11.97-13.13 on the
      glass.
- [x] Behaviour (569 test docks):

      | check | result |
      | --- | --- |
      | Open Folder cell, BtnTest box | Workbench window "BtnTest" opened, box closed |
      | parent arrow in Sub | back to the BtnTest box (185x200 / 224x246 / 334x342 at 36 / 48 / 72) |
      | page buttons | ">" to 2/4 at every size; "<" back to page 1 (36) |
      | quick switch (o.json, 14 pins) | Downloads (anchor 906) -> Games (824) -> Downloads (906) |
      | program pin with a box open | Downloads box open, pointer moved to Editor, click: Editor started, box closed |
      | Click to | Open Folder saved "clicks" {Personal:Downloads: folder}; left click opened Workbench window "Downloads", no box; Show Stack removed "clicks"; left click opened the box (356x359) |

      Profile: Input downs=6 ups=6, prefetch stacks=3 pending=0.
- [x] First open of Downloads after 50 s idle, pointer off the dock,
      fresh dock per run, ChangeTaskPri -30, o.json (the 550/551 runs'
      config, migrated each run), 551 controls (the installed binary from
      RAM:) alternated:

      | run | build | first paint ms | entrance ms | fields |
      | --- | --- | --- | --- | --- |
      | n1 | 569 | 100.7 | 138.0 | 01112 |
      | o1 | 551 | 102.6 | 136.0 | 0121 |
      | n2 | 569 | 100.2 | 139.5 | 01112 |
      | o2 | 551 | 102.6 | 136.9 | 01121 |
      | n3 | 569 | 100.7 | 137.7 | 01112 |
      | n4 | 569 | 100.9 | 139.0 | 01112 |

      551 recorded earlier: 107.2-114.7 / 137.6-150.3. 569 first paint
      2 ms under today's 551 runs; entrance 1-3 ms over, the same five
      frames (the box is 9 px lower with the one-line tile: 356x359
      against 356x368). Prefetch pending=0 reads=81 every run. Pass.
- [x] Hover sweep (t.json, 15 pins, relative moves to x 350, 500, 650,
      800, 950 at y 688, one per call, 45 s idle, -30): render_us_avg
      9516, max 18313, stalled 0 (551 earlier 9733 / 9677, stalled 2 / 0;
      old spread 9077-10211). Pass.
- [x] Seen again (as on 3.1.4.1): RAM:BtnTest stayed "in use" after its
      contents were deleted (empty drawer, held until reboot). Two control
      runs on RAM:LockA (box, Sub, ">", "<", parent, Open Folder window,
      close it, quit) and RAM:LockB (box, Sub, ">", parent, quit with the
      box open) deleted cleanly, and a drawer opened and closed by WBRun
      did too. Holder unknown; no dock defect shown, no code change.
- [x] End state: 569 in SYS:WBStartup/DockAwesome, started with WBRun,
      FindTask finds it, one DockAwesome.PiP2 semaphore, Status lists no
      CLI dock; its Downloads box opened (356x359) and closed with the
      user's config. ENVARC:DockAwesome.json untouched by the tests (588
      bytes, 18:10:59 on the Amiga clock, already so at the start; the 551
      session recorded 17:40:52, so it changed between the sessions, not
      by the tests; 14 pins with Personal:Audio and Personal:). RAM: holds Clipboards, ENV,
      T and, until reboot, the empty BtnTest drawer; ENV:dpid deleted;
      DL: assign removed. SYS:WBStartup/Disabled holds WBDock and, until
      reboot, DockAwesome-locked-delete (551). C:ApolloExplorerSrv
      (started this session) runs until reboot. Images: scratchpad
      `design/ao-569-btn-compare-{36,48,72}.png` (10x, 3.2.3 above
      ApolloOS), `design/ao-569-nested-p2-36-2x.png`, `-48.png`, `-72.png`;
      also in ~/Downloads/DockAwesome-design/.
- [x] Delete SYS:WBStartup/Disabled/DockAwesome-locked-delete (551) after
      the next ApolloOS boot (done in the menu glass session).
- [ ] NOT RUN on ApolloOS: the native (no AA) path (not used here); Sort
      by on this build (unchanged since 551, run there).

### Coffin with build 569 (2026-11-26)
Coffin r65 on the Vampire (Kickstart 45.20, AmigaOS 3.9 Workbench is the
desktop; Directory Opus 4 is only a pinned program), 24-bit 1280x720,
agent input with relative moves (absolute warps miss the dock here).
Build 569 = 08de37b (HEAD c3d1cb2 changed only docs; `make amiga`:
nothing to rebuild, no warnings; byte-identical to the ApolloOS upload).
Installed before: 551, started from SYS:WBStartup at boot. Test docks
from RAM: with `--config RAM:` copies, exactly one dock at a time
(Status COM=, FindTask and Sems before each start, 5 s or more before
each stop).
- [x] At this boot amiagent and ApolloExplorerSrv started by themselves
      from the last two lines of S:User-Startup (`run > NIL:
      Personal:Applications/AmiMCP/amiagent TOKEN=amiga` and `run > NIL:
      Personal:Applications/ApolloExplorer-Amiga/ApolloExplorerSrv`), not
      from WBStartup; Status showed both before any command of mine.
- [x] Installed 569 in place: 551 copied to RAM:DockOld for controls;
      the old Disabled/DockAwesome-locked-delete (541) was no longer
      locked and is deleted; 551 stopped (BreakTask, gone after 5 s, no
      PiP2 semaphore), 569 copied as DockAwesome.new, the 551 file was "in
      use" (WBStartup), so it is now
      SYS:WBStartup/Disabled/DockAwesome-locked-delete (delete after the
      next Coffin boot); .new renamed into place, Version 0.569 build 569.
- [x] Box dumps (`--pip-dump`, RAM:BtnTest = Sub (360 files) + t10-t15;
      pins Shell, RAM:BtnTest, EditPad), nested Sub page 2 (2/4, 2/5,
      2/8), sizes 36/48/72: 698x598, 836x541, 1252x465 as on 3.2.3. A CON
      window with no text (`Run >NIL: Wait 1800 >CON:0/20/1280/650/Plain`,
      black on Coffin) lay behind the box, so the glass was even.

      | control | Coffin bbox offset (x / y), 36 / 48 / 72 | 3.2.3 (569) |
      | --- | --- | --- |
      | parent arrow | 0.00 / 0.00 each | 0.00 / 0.00 each |
      | page back "<" | 0.00 / 0.00 each | 0.00 / +0.06, 0.00 / +0.02, 0.00 / 0.00 |
      | page forward ">" | 0.00 / 0.00 each | 0.00 / +0.03, -0.01 / +0.08, - (backdrop logo) |
      | Open Folder ring (36) | 648..676 x 508..536, centre 662.00 / 522.00 = icon box centre | the same |

      Ink centroids equal 3.2.3's (arrow 0.24 right / 0.71 up; chevrons
      0.13 toward the open side). Every full-tone #dee2e9 pixel (buttons,
      ring, arrow, label, title, page count) lies at exactly the 3.2.3
      positions: 274 / 431 / 845 pixels at 36 / 48 / 72, 0 differ. The 72 px
      ">" measures here (on 3.2.3 it lies over the backdrop logo). Tone
      (36, brightest pixel per element): title, parent arrow, "<", ">",
      page count, ring and label #dee2e9 (contrast 13.26 on the glass);
      file names #f2f4f8 (15.65).
- [x] Images: scratchpad `design/co-569-btn-compare-{36,48,72}.png` (10x,
      3.2.3 above Coffin, cross-hair on the control centre),
      `design/co-569-nested-p2-36-2x.png`, `-48.png`, `-72.png`,
      `design/co-569-open-folder-36-6x.png`,
      `design/co-569-btntest-top-36-2x.png`, `design/co-569-downloads-2x.png`;
      also in ~/Downloads/DockAwesome-design/.
- [x] Behaviour (569 test docks, agent clicks):

      | check | result |
      | --- | --- |
      | Open Folder cell, BtnTest box (185x200) | Workbench window "BtnTest" opened, box closed |
      | Open Folder cell, Downloads box (354x328, size 32) | Workbench window "Downloads" (1034,599 169x96) opened, box closed |
      | parent arrow in Sub | back to the BtnTest box (185x200 at 36 from page 1, 224x246 at 48 from page 2) |
      | page buttons | ">" to 2/4 (36), 2/5 (48), 2/8 (72); "<" back to page 1 (36) |
      | quick switch (o.json copy, migrated, 3 stacks) | Downloads (anchor 988) -> Games (916; 354x328, its own entries) -> Downloads (988) |
      | program pin with a box open | Downloads box open, click on EditPad: EditPad started, box closed |
      | Sort by | Date Modified saved "sorts": {"Personal:Downloads": "date"}; the check moved in the reopened menu |
      | Click to | Open Folder saved "clicks" {Personal:Downloads: folder}; left click opened Workbench window "Downloads", no box; the menu showed the check on Open Folder; Show Stack removed "clicks"; left click opened the box |

      Profile: Input downs=6 ups=6 right_seen=3. The Downloads box is 354x328
      (551: 354x337; the one-line tile, as on the other systems).
      Drawer icons on Coffin are a flat bar: aim at the bar from a box
      dump (36: dock pin at y 682; 48: 672; 72: 650).
- [x] First open of Downloads after 50 s idle, pointer off the dock
      (640,300), fresh dock per run, ChangeTaskPri -30, o.json (the 551
      runs' config, migrated each run), 551 controls (the installed binary
      from RAM:) alternated:

      | run | build | first paint ms | entrance ms | fields |
      | --- | --- | --- | --- | --- |
      | n1 | 569 | 88.3 | 146.1 | 01121 |
      | o1 | 551 | 90.2 | 144.6 | 01121 |
      | n2 | 569 | 88.1 | 131.9 | 0112 |
      | o2 | 551 | 89.9 | 144.9 | 01121 |
      | n3 | 569 | 88.6 | 146.3 | 01121 |
      | n4 | 569 | 88.2 | 146.4 | 01121 |

      551 recorded earlier on Coffin: 85.1-90.8 / 143.8-168.6 (v1.0b5
      89.4-90.0 / 130.7-150.3). 569 first paint 1.6-2.1 ms under today's
      551 runs; entrance 1.2-1.8 ms over them with the same five frames
      and field pattern (n2 one frame fewer). Prefetch stacks=3 pending=0
      reads=120 every run. Pass.
- [x] Hover sweep (pl.json copy, 20 pins, relative moves to x 350, 500,
      650, 800, 950 at y 692, one per call, 50 s idle, -30): render_us_avg
      9128, max 18538, stalled 3, redraws 100, retire_waits=0
      retire_defers=3 (551 earlier 9284 / 18788 / 4; v1.0b5 10526-13966).
      Pass.
- [x] RAM:BtnTest stayed "in use" after its contents were deleted (as on
      3.1.4.1 and ApolloOS); stopping the installed dock did not free it.
      Three control drawers (L48: box, Sub, ">", parent, quit with the box
      open; L72: box, Sub, ">", quit on page 2; L36: box, Open Folder
      window, ARexx close, quit) deleted cleanly. The holder is
      Workbench: after ARexx `WINDOW "RAM:BtnTest" OPEN` and `CLOSE` the
      empty drawer deleted at once. Not a dock defect; no code change.
- [x] No defect found; no change to src/.
- [x] End state: 569 in SYS:WBStartup/DockAwesome, started with WBRun,
      FindTask finds it, one DockAwesome.PiP2 semaphore, Status lists no
      CLI dock; its Downloads box opened (354x328) and closed with the
      user's config. ENVARC:DockAwesome.json untouched (751 bytes,
      Yesterday 10:03:46 on the Amiga clock, as since the 551 session).
      RAM: holds only Clipboards and T; ENV:dpid deleted; DL: assign
      removed; the measuring CON window closed (Break). SYS:WBStartup/
      Disabled holds the user's WBDock, Matrix and others and, until
      reboot (locked by WBStartup), DockAwesome-locked-delete (551).
      amiagent and ApolloExplorerSrv keep running.
- [x] Delete SYS:WBStartup/Disabled/DockAwesome-locked-delete (551) after
      the next Coffin boot (done in the beta 7 session, 2026-11-27).
- [ ] NOT RUN on Coffin: the native (no AA) path (not used here); a DOpus
      lister for Open Folder (Workbench is the desktop).

### AmigaOS 3.9 with build 569 (2026-11-26)
Plain AmigaOS 3.9 on the Vampire (the agent reports kickstart 40.10,
graphics 40; volume AOS39), 24-bit 1280x720, agent input with relative moves. Build
569 = 08de37b (HEAD 2b57cc9 changed only docs; `make amiga`: nothing to
rebuild, no warnings; byte-identical to the Coffin upload). Installed
before: 551, started from SYS:WBStartup at boot. Test docks from RAM:
with `--config RAM:` copies, exactly one dock at a time (Status COM=,
FindTask and Sems before each start, 5 s or more before each stop).
- [x] At this boot amiagent and ApolloExplorerSrv started by themselves
      from the two lines the user added at the end of AOS39:S/User-Startup
      (`run > NIL: Personal:Applications/AmiMCP/amiagent TOKEN=amiga` and
      `run > NIL: Personal:Applications/ApolloExplorer-Amiga/ApolloExplorerSrv`);
      Status showed both before any command of mine. On the 551 session's
      boot amiagent had not started. The user added the same lines on
      3.1.4.1 (as reported to me); 3.1.4.1 has not been booted with them
      yet, so that is not verified.
- [x] Installed 569 in place: 551 copied to RAM:DockOld for controls,
      stopped (BreakTask, gone after 5 s, no PiP2 semaphore); the old
      Disabled/DockAwesome-locked-delete (1.0 beta 4) was no longer locked
      and is deleted; 569 copied as DockAwesome.new, the 551 file was "in
      use" (WBStartup), so it is now
      SYS:WBStartup/Disabled/DockAwesome-locked-delete (delete after the
      next 3.9 boot); .new renamed into place, Version 0.569 build 569.
- [x] Box dumps (`--pip-dump`, RAM:BtnTest = Sub (360 files) + t10-t15;
      pins Shell, RAM:BtnTest, Editor), nested Sub page 2 (2/4, 2/5,
      2/8), sizes 36/48/72: 698x598, 836x541, 1252x465 as on 3.2.3. An
      empty CON window (`Run >NIL: Wait 1800 >CON:0/20/1280/650/Plain`,
      grey on 3.9) lay behind the box, so the glass was even.

      | control | 3.9 bbox offset (x / y), 36 / 48 / 72 | 3.2.3 (569) |
      | --- | --- | --- |
      | parent arrow | 0.00 / 0.00 each | 0.00 / 0.00 each |
      | page back "<" | 0.00 / 0.00 each | 0.00 / +0.06, 0.00 / +0.02, 0.00 / 0.00 |
      | page forward ">" | 0.00 / 0.00 each | 0.00 / +0.03, -0.01 / +0.08, - (backdrop logo) |
      | Open Folder ring (36) | 648..676 x 508..536, centre 662.00 / 522.00 = icon box centre | the same |

      Ink centroids equal 3.2.3's (arrow 0.24 right / 0.69 up; chevrons
      0.13 toward the open side). Every full-tone #dee2e9 pixel (buttons,
      ring, arrow, label, title, page count) lies at exactly the 3.2.3
      positions: 274 / 431 / 845 pixels at 36 / 48 / 72, 0 differ. Tone
      (36, brightest pixel per element): title, parent arrow, "<", ">",
      page count, ring and label #dee2e9 (contrast 11.51 on this grey
      glass, #24272e); file names #f2f4f8 (13.58).
- [x] Images: scratchpad `design/39-569-btn-compare-{36,48,72}.png` (10x,
      3.2.3 above 3.9, cross-hair on the control centre),
      `design/39-569-nested-p2-36-2x.png`, `-48.png`, `-72.png`,
      `design/39-569-open-folder-36-6x.png`,
      `design/39-569-btntest-top-36-2x.png`; also in
      ~/Downloads/DockAwesome-design/.
- [x] Behaviour (569 test docks, agent clicks):

      | check | result |
      | --- | --- |
      | Open Folder cell, BtnTest box (185x200) | Workbench window "BtnTest" opened, box closed |
      | Open Folder cell, Downloads box (356x359) | Workbench window "Downloads" (1034,599 169x96) opened, box closed |
      | parent arrow in Sub | back to the BtnTest box (185x200 at 36 from page 1, 224x246 at 48 from page 2) |
      | page buttons | ">" to 2/4 (36), 2/5 (48), 2/8 (72); "<" back to page 1 (36) |
      | quick switch (t.json copy, migrated, 16 pins) | Downloads (box x 766) -> Games (645; its own entries) -> Downloads (766) |
      | program pin with a box open | Downloads box open, pointer moved to Editor, click: Editor started, box closed |
      | volume pin (Archives:) | Workbench window "Archives" opened |
      | Sort by | Date Modified saved "sorts": {"Personal:Downloads": "date"}; the check moved in the reopened menu; box newest first (SAGA readme first) |
      | Click to | Open Folder saved "clicks" {Personal:Downloads: folder}; left click opened Workbench window "Downloads", no box; the menu showed the check on Open Folder; Show Stack removed "clicks"; left click opened the box |

      Profile: Input downs=8 ups=8 right_seen=3; prefetch stacks=3
      pending=0. The Downloads box is 356x359 (551: 356x368; the one-line
      tile, as on the other systems).
- [x] First open of Downloads after 45 s idle, pointer off the dock
      (640,300), fresh dock per run, ChangeTaskPri -30, o.json (the 551
      runs' config, migrated each run), 551 controls (the installed binary
      from RAM:) alternated:

      | run | build | first paint ms | entrance ms | frame ms | fields |
      | --- | --- | --- | --- | --- | --- |
      | n1 | 569 | 92.7 | 142.2 | 6.0 11.7 16.0 23.5 9.0 | 01121 |
      | o1 | 551 | 95.5 | 139.7 | 5.7 11.8 16.3 20.4 9.1 | 01121 |
      | n2 | 569 | 92.6 | 146.9 | 7.5 12.8 17.1 26.7 8.9 | 01121 |
      | o2 | 551 | 95.4 | 139.5 | 5.9 11.3 16.5 20.2 9.2 | 01121 |
      | n3 | 569 | 93.0 | 141.7 | 6.1 11.3 15.8 21.5 9.0 | 01121 |
      | n4 | 569 | 92.2 | 147.0 | 7.5 13.0 17.5 26.5 8.9 | 01121 |

      551 recorded earlier on 3.9: 97.4-97.8 / 137.2-137.6 (v1.0b5
      100.6-114.4 / 134.5-145.6). 569 first paint 2.4-3.2 ms under
      today's 551 runs. Entrance 2.0-7.5 ms over them with the same five
      frames and field pattern: no extra field. The fourth frame (the one
      that spans two fields) takes 21.5-26.7 ms in 569 against 20.2-20.4
      in 551, and n2/n4 are slower in every frame (memory placement, as
      in the earlier per-binary lesson); the other systems showed the same
      direction (Coffin +1.2-1.8, ApolloOS +1-3, 3.1.4.1 +3-4). n2 and n4
      end 1.3-1.4 ms over the old v1.0b5 maximum 145.6. Open for the
      owner as a cost note, not a defect: nothing is drawn later (same
      fields). Prefetch stacks=3 pending=0 reads=92 every run.
- [x] Hover sweep (pl.json copy, 15 pins, relative moves to x 350, 500,
      650, 800, 950 at y 692, then off, one per call, 45 s idle, -30):

      | run | build | render_us_avg | max us | stalled | redraws |
      | --- | --- | --- | --- | --- | --- |
      | hn1 | 569 | 9835 | 18297 | 1 | 98 |
      | ho1 | 551 | 9950 | 18015 | 2 | 99 |

      551 earlier 9540 / stalled 0, v1.0b5 9460 / 9561. 569 under today's
      551 control. Pass; retire_waits=0 retire_defers=3.
- [x] --bench-sweep 8 (pl.json copy, one script, ChangeTaskPri -30,
      v1.0b5 from tag v1.0b5 built in a scratch worktree outside the repo),
      165 redraws each:

      | build | render_us_avg | max us | compose us | stalled |
      | --- | --- | --- | --- | --- |
      | v1.0b5 | 7169 | 97527 (first frame) | 4796 | 40 |
      | 569 | 6804 | 11715 | 5020 | 43 |
      | v1.0b5 | 6622 | 10949 | 4800 | 40 |

      Same damage (compose 24%, conversion 29% of the pixels). Pass:
      render_us_avg within the old spread; compose 0.22 ms (4.6%) over
      v1.0b5 (3.2.3 had shown 0.9 ms with 553), stalled 3 more of 165.
- [x] RAM:BtnTest stayed "in use" after the tests (as on 3.1.4.1,
      ApolloOS and Coffin; the tests had opened and closed a Workbench
      window on it). With RexxMast started for it, ARexx `WINDOW
      "RAM:BtnTest" OPEN` and `CLOSE` released it and it deleted at once;
      RexxMast stopped again with RXC. Workbench was the holder, as on
      Coffin. Not a dock defect.
- [x] No defect found; no change to src/.
- [x] End state: 569 in SYS:WBStartup/DockAwesome, started with WBRun,
      FindTask finds it, one DockAwesome.PiP2 semaphore, Status lists no
      CLI dock; its Downloads box opened (356x359) and closed with the
      user's config. ENVARC:DockAwesome.json untouched (566 bytes,
      Yesterday 16:37:26 on the Amiga clock: the migration by the
      installed 551 in the 551 session; 13 pins, Downloads last). RAM:
      holds only Clipboards, ENV, T and amiagent's own file; ENV:dpid,
      Editor's RAM:T/JanoRecentFiles and an Execute temp file deleted; DL:
      assign removed; the measuring CON window closed (Break). SYS:WBStartup/
      Disabled holds the user's WBDock and, until reboot (locked by
      WBStartup), DockAwesome-locked-delete (551). Scratch worktree
      removed on the Mac. amiagent and ApolloExplorerSrv keep running.
- [ ] Delete SYS:WBStartup/Disabled/DockAwesome-locked-delete (551) after
      the next 3.9 boot.
- [ ] Verify at the next 3.1.4.1 boot that the same two User-Startup lines
      start amiagent and ApolloExplorerSrv there.
- [ ] NOT RUN on 3.9: the native (no AA) path (not used here); a DOpus
      lister for Open Folder (Workbench is the desktop).

### Menu glass over the dock's icons (ApolloOS, 2026-11-26)
User report (photo, build 569 / 1.0 beta 6): a stack's right-click menu is
glass against the desktop but covers the dock's zoomed icons as if opaque.
The brief said AmigaOS 3.9 was booted; the machine was ApolloOS R9.5
(Kickstart 51.3, volumes ApolloOS/Personal/Archives, no AOS39), 16-bit
1280x720, so everything below ran on ApolloOS. Installed before: 569 from
SYS:WBStartup (one task, one PiP2 semaphore). ApolloExplorerSrv was not
running; started with Run >NIL: C:ApolloExplorerSrv.
- [x] Root cause (code + dump): the menu's glass is `capture_glass` of the
      screen bitmap, tinted (`tint_popup_glass`, shift 2: a quarter of the
      backdrop); the dock is in PiP, never in the screen bitmap, and
      `pip_mask` cuts the menu's span out of PiP. So over the row the glass
      held only the desktop. 1.0 beta 5 has the same capture and mask
      (`git show v1.0b5`): same defect by code, not run. The Folder Stack
      box ends 3 px above the row's full zoom height (FOLDER_GAP 10 -
      FOLDER_TIP 7 above `hover_label_anchor`), so it was not affected.
      Settings/About are opaque GadTools windows (PiP is keyed out there):
      no glass, not affected.
- [x] Dump first (commit b7d94dc, build 571 = the "before" binary):
      `--pip-dump` writes each menu's window read back from the screen
      (`-popup.ppm`), its tinted glass (`-popupglass`), the capture
      (`-popupdesk`) and the canvas keyed magenta where PiP shows the
      screen (`-popupdock`), and prints both positions.
- [x] Fix (build 572): `dock_overlay_onto_capture` (dock_compose.c, pure)
      copies the canvas's covered pixels in its visible columns into the
      capture before the tint, through a DockUI hook (`menu_under`) set
      on every DockUI (start and Workbench reopen), menus only. The
      canvas is opaque and already blended over the backdrop, so the copy
      is "dock over desktop". `open_menu` draws once before opening when
      the hover label or a press must go: in 571's dump the canvas at
      opening still held the "Audio" label, which the next frame removed.
      Without RGB glass (no `popup_rgb`) `capture_glass` returns nothing
      and the pens are unchanged.
- [x] Row under an open menu: `update_target` returns while the UI holds
      the dock and `open_menu` copies placement to target, so the row
      does not ease back. Measured (canvas at opening vs at exit with the
      menu still open, 4-5 s later): only 986-1099 pixels changed, all
      inside the menu's rectangle (the plate re-mixing the menu window
      from the live capture, hidden by PiP's cut-out). Decision: snapshot
      at opening, no refresh (a dot that changes under an open menu shows
      the old dot until it closes); no repaint, no flicker.
- [x] Host: `test_overlay_onto_capture` (4000 random sizes/offsets incl.
      negative, visible < width, NULL coverage, against a per-pixel
      reference, plus the screen case); `make host-test`, `make
      host-sanitize` pass; `make amiga` no warnings.
- [x] Agent right-click on the Audio stack (zoomed, label shown), user's
      config copy (14 pins, size 36), `--pip-dump`; measured inner glass
      pixels covered by the dock (rim and corners excluded; tint
      [20,23,29] fitted on the desktop-only pixels, 100% share):

      | edge | build | menu / dock | dock px in capture | glass vs tint(desktop+dock) | glass vs tint(desktop) |
      | --- | --- | --- | --- | --- | --- |
      | bottom | 571 | 520,540 115x146 / 208,612 864x108 | - | max 54, 1.5% within 1 | max 0 |
      | bottom | 572 | same | 3682 | max 0, 100% within 1 (2894 px) | max 54 |
      | left | 572 | 30,286 / 3,13 192x707 (visible 184) | 5104 | max 0 (4214 px) | max 61 |
      | right | 572 | 1135,286 / 1093,13 192x707 (visible 187) | 5182 | max 0 (4288 px) | max 55 |

      The window read back from the screen equals the glass at RGB565
      on 89-97% of those pixels; the rest are the rows' text and checks.
      Hover on a row over the icons repainted the band with the icons
      around it (screenshot). There is no top edge (DockEdge: bottom,
      left, right).
- [x] Images (4x): scratchpad `design/menu-glass-bottom-before-after-4x.png`,
      `-bottom-before-4x`, `-bottom-after-4x`, `-left-after-4x`,
      `-right-after-4x`, `-dock-at-open-before-4x` (label still in the
      canvas) and `-dock-at-open-after-4x`; also in
      ~/Downloads/DockAwesome-design/.
- [x] First open of Downloads after 55 s idle, pointer off the dock, fresh
      dock per run, ChangeTaskPri -30, o.json (migrated each run), 569
      (the installed binary) alternated:

      | run | build | first paint ms | entrance ms | fields |
      | --- | --- | --- | --- | --- |
      | n1 | 572 | 100.4 | 137.9 | 01112 |
      | o1 | 569 | 101.0 | 137.6 | 01112 |
      | n2 | 572 | 100.4 | 138.0 | 01112 |
      | o2 | 569 | 100.7 | 138.0 | 01112 |
      | n3 | 572 | 100.8 | 137.7 | 01112 |
      | n4 | 572 | 107.8 | 130.6 (4 frames) | 0112 |
      | o3 | 569 | 101.7 | 137.8 | 01112 |
      | n5 | 572 | 100.2 | 137.8 | 01112 |

      569 earlier on ApolloOS: 100.2-100.9 / 137.7-139.5. n4's first paint
      had one prepare stage at 29.3 ms (21.3-22.2 in every other run);
      that path does not run the changed code (menus only) and n5
      repeated normally. Prefetch stacks=3 pending=0 reads=81 every run.
      Pass.
- [x] Hover sweep (t.json, relative moves to x 350, 500, 650, 800, 950 at
      y 688, one per call, 50 s idle, -30):

      | run | build | render_us_avg | max us | stalled | redraws |
      | --- | --- | --- | --- | --- | --- |
      | hn1 | 572 | 9529 | 17539 | 0 | 93 |
      | ho1 | 569 | 9614 | 17709 | 0 | 93 |
      | hn2 | 572 | 9605 | 18217 | 1 | 93 |
      | ho2 | 569 | 9614 | 17690 | 0 | 94 |
      | hn3 | 572 | 9651 | 18214 | 0 | 92 |
      | hn4 | 572 | 9524 | 17633 | 0 | 93 |

      569 earlier 9516 / stalled 0. Pass (the hover path is unchanged).
- [x] Old SYS:WBStartup/Disabled/DockAwesome-locked-delete (551) was no
      longer locked after the reboot and is deleted.
- [x] Independent review (Opus) of the diff: no P0/P1; four P2 fixed in
      build 573: `open_menu` also draws a frame the loop pass already
      had pending (`redraw`, `redraw_due`: a new running dot or a
      field-deferred zoom step would otherwise miss the glass); the
      random host test filled every pixel with 0 (cast before shift),
      so it now checks real pixel content; an early return when nothing
      overlaps (no out-of-range pointer arithmetic); a comment that the
      one extra draw is unpaced and may wait up to a field.
- [x] Build 573 rerun: bottom dump identical to 572 (3682 dock pixels,
      glass vs tint(desktop+dock) max 0 over 2894 px); first open 100.5 /
      137.8 ms, 01112; hover sweep 10102 (95 redraws) / 9419 / 9605,
      stalled 0, against 569 ho3 9640 in the same session (569 runs
      9614 / 9614 / 9640).
- [x] End state: 573 in SYS:WBStartup/DockAwesome (Version 0.573), started
      with WBRun; the task list shows one DockAwesome and its status
      task, one DockAwesome.PiP2 semaphore, Status lists no CLI dock;
      its Audio menu opened over the zoomed row and closed (Esc). The 569
      file was "in use" (WBStartup), so it is now
      SYS:WBStartup/Disabled/DockAwesome-locked-delete. ENVARC:DockAwesome.json
      untouched (588 bytes, 18:10:59 as at the start). RAM: holds only
      Clipboards, ENV, T; DL: assign removed, ENV:dpid deleted.
      amiagent and C:ApolloExplorerSrv (started this session) keep running.
- [x] Delete SYS:WBStartup/Disabled/DockAwesome-locked-delete (569) after
      the next ApolloOS boot (done in the disk stack session).
- [ ] NOT RUN: AmigaOS 3.9, Coffin, AmigaOS 3.2.3, AmigaOS 3.1.4.1, AmiKit
      XE (same renderer, menu code and PiP cut-out; 24-bit screens there);
      1.0 beta 5 on hardware (same code, by reading); a right-click on the
      plate while the row is still easing (the review's case, covered by
      the pending-frame draw, not reproduced).

### Disks as Folder Stacks, opt-in per pin (ApolloOS, 2026-11-26, owner decision)
Scope: a disk pin (a drawer pin whose path ends in ':') gets the stack menu
(Sort by, Click to). Default Open Folder = 1.0 beta 6's disk pin (window,
running dot, Close while open, no read-ahead); Show Stack makes it a stack
(box, no dot, no Close, read ahead). Close stays offered while the window
is open, last, after a second hairline. Persisted in "clicks" by path, only
non-default values ("stack" for a disk, "folder" for a drawer).
- [x] dock_prefs: `dock_click_default(path)`; the reader gives each path its
      own default, the writer omits it; "stack" and "folder" accepted for
      any path; unknown values read as the path's default. Migration keeps
      the 1.0 beta 5 Folder Stack showing its box (a disk there: "stack").
- [x] amiga_main: `disk_pin` (derived from kind + path), `stack_menu`,
      `set_click` (sets `Item.stack` for disks, clears running/owner/lister
      when the flag changes, arms the read-ahead for a box); `pin_path`
      and startup go through it; `click_menu_item` builds the close button
      and calls `status_changed` on a flag change; `read_folder_icon` runs
      without requesters; icon reload keeps the disk loader for disks.
- [x] amiga_status: a reply for a pin that is a stack now is ignored (a
      poll in flight when a disk became a stack).
- [x] amiga_ui: `drawer_items` + separator + Close (`DRAWER_ROWS` without
      them, all rows with `DOCK_UI_CLOSE_ITEM`); a root's box title drops
      the colon ("Personal", as the hover label).
- [x] Host: `test_disk_clicks` (defaults per path, exact "clicks" format
      with "stack"/"folder", round trip, explicit defaults not written
      back, unknown/odd values, key for no pin, reorder, removal, hand
      edit, migration of a disk Folder Stack); `make host-test`, `make
      host-sanitize` pass; `make amiga` no warnings (build 576).
- [x] Compatibility (host, `git archive` of each tag's dock_prefs.c, a file
      with "Personal:": "stack", "Personal:Downloads": "folder" and sorts):
      v1.0b5 reads all 5 paths, ignores "sorts" and "clicks" and writes
      neither; v1.0b6 reads all 5 paths, Personal: as Show Stack (its disks
      are never stacks: a window opens), keeps "folder" and the sorts and
      drops "stack" at its next save (the disk reads as Open Folder again
      in this build). No path lost.
- [x] impeccable polish (in-thread): group order Sort by, Click to as for
      drawers; Close is an action, not an option, so it goes last below
      its own hairline (the Mac puts Quit last), label in the same column;
      wording kept ("Open Folder" as the box tile). Box title without the
      colon. No other change.
- [x] ApolloOS R9.5, test docks from RAM: (`--config RAM:d1.json`: the
      user's 14 pins + Archives:), one dock at a time (FindTask, Sems,
      Status before each start; 5 s+ before each stop):

      | check | result |
      | --- | --- |
      | right-click Archives: (no window) | Sort by (Name checked), hairline, Click to, Open Folder checked; no Close (115x146) |
      | right-click Personal: (window open) | the same + hairline + Close (115x172) |
      | Archives: -> Show Stack | file gains "clicks": {"Archives:": "stack"}; click opens the box "Archives" (185x200, 8 entries + Open Folder) |
      | Date Modified | "sorts": {"Archives:": "date"}; box order Games, Prefs, C, Demos, iGame.guide, S, Devs, ABDisk (List SORT=DATE agrees) |
      | drawer inside, parent arrow | Archives:Games (356x359, parent arrow) and back to the root box |
      | Personal: (open) -> Show Stack | menu without Close; canvas dump: dot under Personal gone; click opens its box (299x253, 16 entries) |
      | Personal: -> Open Folder | "Personal:" leaves "clicks"; Close back after the status poll; Close closed the window; click opened it again |
      | reorder Archives: to slot 0 | first in "paths", "sorts"/"clicks" kept; slot 0 click opens its box |
      | restart | Archives: menu: Date Modified + Show Stack checked; click opens the box |
      | drag Archives: out | gone from "paths", no "sorts"/"clicks" left |

      These ran on build 575; build 576 (the flag derived from the path,
      see Performance) repeated: idle Archives: menu (Open Folder checked,
      no Close), Show Stack saved as "stack" and the click opened its box
      (185x200), Personal: (window open) menu 115x172 with Close.

- [x] Many entries: BOOT: (about 180 root entries) as Show Stack: box
      698x598, page 1/2; first open after the read-ahead: first paint
      332 ms, entrance 140 ms, 3 frames (the largest box; the read-ahead
      read 149 icons, pending=0).
- [x] Absent volume: an ADF made on the Mac (FFS, "DockGone", 3 entries)
      in IDF0: with MountDiskImage; pins CLI, DockGone: (stack), BOOT:,
      Editor. Box 185x147 while inserted; after EJECT (Info: no volume
      DockGone) a click shows the box with "Cannot read drawer" (185x109),
      no requester, first paint 22 ms; Kind chosen for it re-armed its
      read-ahead, which read once and finished (reads 123 = 3 + 119 + 1,
      pending=0), no requester, no stall. Control: `Dir DockGone:` from the
      Shell opened "Please insert volume "DockGone" in any drive" (cancelled).
      A pin on an empty drive (DF0:, ICD0:) cannot load at startup, as in
      1.0 beta 6 ("Cannot read this icon"), so it never becomes a pin.
- [x] Dumps (`--pip-dump`, pins CLI, Personal: (window open), Archives:
      (stack), Editor; sizes 36/48/72): menu 115x172 at every size; boxes
      185x200, 224x246, 334x342. Images (4x menus, 3x/2x boxes):
      scratchpad `design/disk-stack-menu-{36,48,72}-4x.png`,
      `-menu-idle-36-4x.png`, `-box-36-3x.png`, `-box-48-3x.png`,
      `-box-72-2x.png`, `-absent-36-4x.png`, `-boot-p1-36-2x.png`,
      `-dot-folder-vs-stack-4x.png`; also in ~/Downloads/DockAwesome-design/.
- [x] Performance (ChangeTaskPri -30, fresh dock per run, 573 = the
      installed binary from RAM:). First open of Downloads after 50 s idle,
      o.json:

      | run | build | first paint ms | entrance ms | fields |
      | --- | --- | --- | --- | --- |
      | n1-n4 | 575 | 102.1 / 103.5 / 103.7 / 103.0 | 137.5 / 148.7 / 136.0 / 145.7 | 0122 / 01121 / 01112 / 01121 |
      | o1-o4 | 573 | 102.8 / 103.4 / 103.0 / 104.4 | 136.9 / 131.3 / 145.1 / 135.7 | 01112 / 0112 / 01121 / 0112 |
      | n5-n8 | 576 | 110.2 / 111.9 / 104.7 / 104.3 | 132.5 / 136.1 / 147.3 / 147.8 | 0112 / 01112 / 01121 / 01121 |

      n5/n6: one prepare stage each at 29.4 / 34.0 ms (21-26 in every
      other run; the box's painting, not changed code), as 572's n4. The
      01121 pattern (one field late, ~146 ms) shows on 573 too. Prefetch
      stacks=3 pending=0 reads=81 every run. Pass.
      Hover sweep (t.json, 15 pins, relative moves x 350..950 at y 688,
      one per call, 45 s idle, then off the dock):

      | run | build | render_us_avg | compose_us_avg | stalled | redraws |
      | --- | --- | --- | --- | --- | --- |
      | hn1, hn2 | 575 | 11338 / 11327 | 8755 / 8769 | 5 / 4 | 111 / 96 |
      | ho1, ho2 | 573 | 10065 / 10182 | 7343 / 7501 | 0 / 2 | 97 / 93 |
      | hn3, hn4 | 576 | 10212 / 10227 | 7648 / 7637 | 0 / 1 | 95 / 94 |

      575 had `int volume` in `Item` (App grew 128 bytes): compose 1.4 ms
      slower with unchanged compose code (lessons.md). 576 derives it from
      the path: within 0.5% of 573. Pass.
- [x] Independent review (Opus, the diff only): no P0/P1; two P2 fixed in
      build 577: the 1.0 beta 5 Folder Stack that was also a pin kept its
      default click on migration (a disk then lost its box): now Show
      Stack either way (host test); at startup a click equal to the file
      path's default is not applied over the pin's own default (a
      hand-edited assign such as "C:" pins a drawer and must show its
      box). 577 on ApolloOS: "C:" + "Archives:" ("stack") config: C: opened
      its box (698x598), Archives: its box (185x200); hover sweep 10421 /
      compose 7639 (576: 7648 / 7637), stalled 1.
- [x] End state: 577 in SYS:WBStartup/DockAwesome (Version 0.577), started
      with WBRun; FindTask finds one DockAwesome, one DockAwesome.PiP2
      semaphore, Status lists no CLI dock; with the user's config the
      Personal: menu shows Open Folder checked and Close (its window is
      open). The 573 file was "in use" (WBStartup), so it is now
      SYS:WBStartup/Disabled/DockAwesome-locked-delete; the 569 one from
      the last session was deleted. ENVARC:DockAwesome.json untouched (588
      bytes, 17:16:08, as at the start). RAM: holds only Clipboards, ENV, T;
      DL: assign removed, ENV:dpid deleted, IDF0: empty (the test ADF
      ejected and deleted). amiagent and ApolloExplorerSrv keep running.
- [ ] Delete SYS:WBStartup/Disabled/DockAwesome-locked-delete (573) after
      the next ApolloOS boot.
- [ ] NOT RUN: Coffin, AmigaOS 3.2.3, AmigaOS 3.9, AmigaOS 3.1.4.1, AmiKit
      XE (a disk's Open Folder there opens a Directory Opus lister, as
      before; Show Stack on a disk untested there); the running dot on the
      monitor (PiP; seen only in the canvas dump); a real CD or floppy
      taken out (an ejected ADF image stood in).

### Dock hides on a right-click in the lower half of the screen (ApolloOS, 2026-11-27)
User report and video IMG_2662 (ApolloOS only): the dock row vanishes and
fades back for each right-click in the lower half of the Workbench screen,
not in the upper half; no menu near the dock. Build 577 installed; test
docks used RAM:da.json / RAM:dl.json copies; ENVARC untouched.
- [x] Observe PiP objectively. PiP2 registers are write-only, so the tool
      PipWatch (scratch, read-only) read the dock's own `AmigaPip`
      (`configured` = DMA on, front, physical x/y) through the
      "DockAwesome.PiP2" lease semaphore, every 2 ms (timer.device, 20-50 s
      bounded), with an input.device handler at pri 127 logging button
      events, the pointer, PeekQualifier and the screen's top layers on one
      EClock.
- [x] Agent clicks do not reproduce it: amiga_click/amiga_button events
      carry no button qualifier, PeekQualifier stayed "free", so the dock's
      `native_menu_held()` never fired (PiP stayed ON in every agent
      right-click, also a 6 s hold). A tool sending IND_ADDEVENT with
      IEQUALIFIER_RBUTTON (as the gameport path does) reproduced it.
- [x] Timeline, build 577 (installed), dock box 208,612-1071,719:

      | right button | pointer (y) | PiP2 |
      | --- | --- | --- |
      | click | 400,200 / 300,350 | stays ON |
      | click | 400,500 / 300,362 / 300,600 | OFF 2.3 ms after down, ON 41 ms after up |
      | down lower, up upper | 450,650 -> 450,200 | OFF at down, ON 36 ms after up |
      | down upper, up lower | 450,200 -> 450,650 | stays ON |
      | left click (control) | 400,560 | stays ON |
      | Personal window active | 300,600 | OFF / ON as above |
      | on the dock | 640,705 | dock's own menu, stays ON |
      | no dock running | 400,500 | no lease (nothing turns PiP on or off) |

      Intuition's menu window is always [0,0-1279,11] (the title bar);
      the Workbench menu drops to [1,12-164,92]. IControl prefs: ic_Flags
      $1000C01E, ICF_POPUPMENUS off (pull-down menus).
- [x] Root cause: the dock. `menu_near_dock()` (commit 7171165, made for
      MagicMenu's pop-up menus on Coffin) hid PiP for any native menu
      opened within 256 px of the dock: dock top 612 - 256 = y 357, the
      "lower half". ApolloOS menus drop from the title bar. 1.0 beta 5
      (built from the tag) does the same (native_menu_hides=2, OFF at
      400,500 and 300,650); 577 matched run native_menu_hides=3.
- [x] Fix (build 578, review fixes in 579): Intuition V50 (ApolloOS) draws menus in windows
      (`caps.menu_windows`), so while a native menu is held the dock reads
      the windows in front of its own (AttemptSemaphore only) and steps
      aside once one overlaps its PiP box, polling at the fast rate until
      then. Other systems keep the pointer rule, once per menu.
      `dock_menu_covers()` holds the decision; host test in
      test_geometry.
- [x] 578, bottom dock (pw5): right-clicks at y 200 / 500 / 362, press
      lower release upper, and the Workbench menu dropped and released off
      it: PiP2 ON throughout, native_menu_hides=0.
- [x] 578, left dock (dl.json, box 3,13-194,719): right-click at 600,400
      stays ON (the old rule always hid a dock whose top is < 256); the
      Window menu dropping over the dock ([73,12-218,154]) turned PiP OFF in
      the same 2 ms poll, ON again after release; native_menu_hides=2.
- [x] Hover sweep, 8 relative moves along y 690, one run each: 578
      render_us_avg 9151, compose 6699 (130 redraws); 577 9993 / 7459
      (132). No regression (single runs; not claimed as a gain).
- [x] Independent review (Opus, the diff): one P1 fixed in build 579:
      any foreign window in front of the dock counted as a menu, so a
      program's window over the dock would still hide it. Menu windows on
      R9.5 are borderless with no UserPort (PipWatch: Flags $00040800,
      Workbench windows $2A24107F with a port); only those count now. P2
      fixed: a new right press is a new menu even when the loop missed the
      release (`right_seen`); no layer walk while another screen is in
      front or a reset runs. P2 left: a layer lock busy for a whole menu
      leaves the dock up (none seen: layer_busy 0-1 per run); the tested
      box includes the keyed zoom headroom (as before the change).
- [x] 579 bottom (pw9): clicks at 400,500 / 300,650 / 400,200 and the
      Window and Workbench menus dropped: PiP2 ON throughout,
      native_menu_hides=0; hover sweep render 9500 / compose 7052 (133
      redraws; 577 9993 / 7459). 579 left (pw10): click at 600,400 stays
      ON; the Window menu over the dock: OFF 5 ms after its window
      appeared, ON 82 ms after release; native_menu_hides=1.
- [x] make host-test, make host-sanitize, make amiga (no warnings).
- [ ] NOT RUN: Coffin, AmigaOS 3.2.3, 3.9, 3.1.4.1, AmiKit XE (unchanged
      pointer rule there; plain Intuition pull-down menus on AmigaOS 3.x
      would hide a bottom dock in the lower half the same way: to check
      with a qualifier-correct right-click); ApolloOS pop-up menus
      (ICF_POPUPMENUS); the monitor itself (PiP is not in screenshots: the
      dock's register writes were observed, not the picture).
- [x] End state: 579 in SYS:WBStartup/DockAwesome (Version 0.579), started
      with WBRun; one PiP2 lease, Status lists no CLI dock; re-measured
      after install: right-clicks at 400,500 / 300,650 / 400,200 keep PiP2
      ON. 577 was "in use", so it is SYS:WBStartup/Disabled/
      DockAwesome-locked-delete-577 (573's locked copy is still there
      too). ENVARC:DockAwesome.json untouched (588 bytes, 17:16:08). RAM:
      holds only Clipboards, ENV, T (amiagent files); the probe tools,
      configs, logs and test builds are deleted; the 1.0 beta 5 scratch
      worktree is removed.
- [ ] Delete SYS:WBStartup/Disabled/DockAwesome-locked-delete and
      DockAwesome-locked-delete-577 after the next ApolloOS boot.

### Coffin, beta 7 items, build 579 (2026-11-27)
Coffin r65 on the Vampire (Kickstart 45.20, AmigaOS 3.9 Workbench, MagicMenu
pop-up menus), 24-bit 1280x720. Build 579 = 95699ba (HEAD 21056eb changed
only docs; `make amiga`: nothing to rebuild, no warnings; byte-identical
to the ApolloOS upload). Installed before: 569 from SYS:WBStartup. Test
docks from RAM: with `--config RAM:` copies, one dock at a time (FindTask,
Sems, Status COM= before each start; 5 s or more before each stop);
agent input with relative moves.
- [x] Install: the old Disabled/DockAwesome-locked-delete (551) was no
      longer locked and is deleted; 569 stopped (BreakTask, gone after
      5 s, no PiP2 semaphore), 579 copied as DockAwesome.new, the 569 file
      was "in use" (WBStartup), so it is now
      SYS:WBStartup/Disabled/DockAwesome-locked-delete; .new renamed into
      place (Version 0.579 build 579).
- [x] (a) Menu glass over the row: user's config copy (18 pins, size 32),
      right-click on Downloads (zoomed, last pin), `--pip-dump`: menu
      902,536 105x160, dock 176,613 928x107, 3362 dock pixels in the
      capture. Inner glass pixels covered by the dock (rim and corners
      excluded; tint [20,23,29] fitted on the desktop-only pixels, 100%
      share): 2803 px, glass vs tint(desktop+dock) max 0 (100% within 1);
      vs tint(desktop only) max 59, mean 29.2 (the old look). The window
      read back from the screen equals the glass exactly (24-bit) on 95%
      of the menu; the rest are text and checks. Canvas at opening vs at
      exit with the menu still open: 1177 covered pixels changed, as on
      ApolloOS (the plate re-mixing the menu window). Same numbers as
      ApolloOS 572/573. Pass.
- [x] (b) Disk stacks, test config CLI (DH0:System/Shell), Personal:,
      Archives:, BOOT:, DockGone: (an FFS ADF in IDF0: with
      MountDiskImage: Docs, Tools, readme.json), Downloads, EditPad:

      | check | result |
      | --- | --- |
      | right-click Archives: (no window) | Sort by (Name), hairline, Click to, Open Folder checked; no Close (105x160) |
      | Archives: -> Show Stack, Date Modified | "clicks": {"Archives:": "stack"}, "sorts": {"Archives:": "date"}; box "Archives" 183x184, order Games, Prefs, C, Demos, iGame.guide, S, Devs, ABDisk (List SORT=DATE agrees; C/S/Demos share a timestamp) |
      | restart | Archives: menu: Date Modified + Show Stack checked |
      | Personal: click (Open Folder) | Workbench window "Personal" opened; its menu then 105x188 with Close below a hairline; Close closed the window |
      | Personal: -> Show Stack | "Personal:": "stack"; box 297x232 (16 entries); -> Open Folder: "Personal:" leaves "clicks" |
      | BOOT: (about 180 root entries) -> Show Stack | box 696x547 (page 1); first open after 30 s idle: first paint 267 ms, entrance 131 ms (2 frames, 03) |
      | DockGone: Show Stack, inserted | box 183x136, first paint 22 ms |
      | DockGone: ejected (MountDiskImage EJECT, Info: no disk) | click: box 183x103 "Cannot read drawer", no requester, first paint 18 ms; Kind chosen (re-arms the read-ahead), 25 s idle: no requester (screenshot), next click the same box (first paint 109 ms: one OpenWindow stage 92 ms, not the read) |
      | reorder Archives: to slot 0 (drag) | first in "paths", "sorts"/"clicks" kept; slot 0 click opens its box |
      | restart with DockGone: absent | "Cannot read this icon: DockGone:" at startup, as in 1.0 beta 6; the path and its "clicks"/"sorts" stay in the file |
      | read-ahead | Open Folder disks are not read: 3 stacks (Archives:, BOOT:, Downloads) with Personal: at Open Folder, pending=0 reads=157 |

      Profile: right_seen=7 right_icons=7, downs=8 ups=8. Pass.
- [x] (c) Right-click rule (unchanged on Coffin: pointer within 256 px of
      the dock while the right button is held). Agent clicks carry no
      button qualifier, and input.device here has no IND_ADDEVENT (DoIO
      -3, IOERR_NOCMD); IND_WRITEEVENT passes the event but leaves
      PeekQualifier() free (PiP stayed ON). So the test tool RmbHold
      (scratch) wrote the press and release with IND_WRITEEVENT and
      patched PeekQualifier (SetFunction, restored at once, checked) to
      report the button held between them, as the gameport path does for
      the physical mouse. A CON window without menus was active so that
      no MagicMenu menu opened. PipWatch (read-only, 2 ms polls) logged on
      one EClock, dock box 176,613-1104,720 (user's config):

      | right button held 0.5 s at | PiP2 |
      | --- | --- |
      | 400,500 | OFF 0.4 ms after down, ON 47 ms after up |
      | 300,650 | OFF at down, ON 28 ms after up |
      | 1270,450 (166 px right of the dock) | OFF at down, ON 28 ms after up |
      | 640,358 (dy 255) | OFF at down, ON 27 ms after up |
      | 640,356 (dy 257) | stays ON |
      | 400,200 / 640,300 | stays ON |
      | on the Downloads pin (954,700) | dock's own menu opens, stays ON |

      As designed. native_menu_hides=8 for the 4 hides: with the menuless
      CON window active every written press reached the handlers twice
      (logged twice at the same EClock), so the decision ran twice per
      press with the same result; on the pin it arrived once. Not a dock
      defect. NOT RUN: the hold with Workbench active (a MagicMenu pop-up
      would open under the pointer; see lessons.md) and the physical mouse.
- [x] (4) First open of Downloads after 45 s idle (o.json, migrated each
      run, pointer off the dock, fresh dock per run, ChangeTaskPri -30),
      569 (the installed binary, from RAM:) alternated:

      | run | build | first paint ms | entrance ms | fields |
      | --- | --- | --- | --- | --- |
      | n1 | 579 | 84.0 | 135.9 | 01112 |
      | o1 | 569 | 83.6 | 136.1 | 01112 |
      | n2 | 579 | 83.5 | 131.1 | 0112 |
      | o2 | 569 | 84.1 | 130.5 | 0112 |
      | n3 | 579 | 83.5 | 136.0 | 01112 |
      | n4 | 579 | 84.4 | 135.3 | 01112 |

      569 on Coffin yesterday: 88.1-88.6 / 131.9-146.4. Prefetch stacks=3
      pending=0 reads=120 every run. Pass.
      Hover sweep (pl.json, 20 pins, relative moves x 350..950 at y 692,
      one per call, 50 s idle, -30):

      | run | build | render_us_avg | compose_us_avg | max us | stalled | redraws |
      | --- | --- | --- | --- | --- | --- | --- |
      | hn1 | 579 | 9292 | 6505 | 18196 | 8 | 113 |
      | ho1 | 569 | 9213 | 6578 | 18517 | 4 | 100 |
      | hn2 | 579 | 9193 | 6495 | 18208 | 5 | 98 |

      569 yesterday 9128 / 18538 / 3. Compose equal; the stalls are two-field
      frames in both builds (hn1 had more redraws). Pass.
- [x] Images: scratchpad `design/co7-menu-glass-bottom-4x.png`,
      `co7-menu-glass-bottom-before-after-4x.png` (left: the same dump with
      the glass recomputed from the desktop alone, as 1.0 beta 6 drew it),
      `co7-disk-menu-{archives-idle,archives-after-restart,personal-open-close}-3x.png`,
      `co7-disk-stack-{archives-date,personal,dockgone-inserted,dockgone-absent}-3x.png`,
      `co7-disk-stack-boot-p1-2x.png`; also in ~/Downloads/DockAwesome-design/.
- [x] No defect found; no change to src/.
- [x] End state: 579 in SYS:WBStartup/DockAwesome, started with WBRun,
      FindTask finds it, one DockAwesome.PiP2 semaphore, Status lists no
      CLI dock; its Downloads menu opened (105x160) and closed.
      ENVARC:DockAwesome.json untouched (751 bytes, Wednesday 10:03:46).
      RAM: holds only Clipboards and T; IDF0: empty (the test ADF ejected
      and deleted); DL: assign removed, ENV:dpid deleted; the CON window
      closed; probe tools deleted. amiagent and ApolloExplorerSrv keep
      running.
- [ ] Delete SYS:WBStartup/Disabled/DockAwesome-locked-delete (569) after
      the next Coffin boot.
- [ ] NOT RUN on Coffin: the menu glass with the dock on the left/right;
      a real disk taken out (an ejected ADF stood in); the PiP picture on
      the monitor (dumps and register state only).

### AmigaOS 3.2.3, beta 7 items, builds 579 and 582 (2026-11-27)
AmigaOS 3.2.3 on the Vampire (Kickstart 47.13, volume AOS32), 24-bit
1280x720, stock Intuition pull-down menus (no icontrol.prefs: defaults;
the 3.2 NDK's IControl has no pop-up menu option). Installed before: 569
from SYS:WBStartup (one task, one PiP2 lease). Build 579 = HEAD 9e4ccf2
(`make amiga`: nothing to rebuild, byte-identical to the ApolloOS/Coffin
upload). Test docks from RAM: with `--config RAM:` copies, one at a time
(FindTask, Sems, Status COM= before each start; 3 s or more before each
stop); 569 copied to RAM: for matched runs. Agent warps land 1:1.
- [x] (a) Menu glass over the row: user's config copy (14 pins, size 36),
      right-click on Audio (zoomed), `--pip-dump`: menu 768,542 115x146,
      dock 208,610 864x110, 3770 dock pixels in the capture. Inner glass
      pixels covered by the dock (rim and corners excluded; tint [20,23,29]
      fitted on the desktop-only pixels, 100% share): 3018 px, glass vs
      tint(desktop+dock) max 0 (100% within 1); vs tint(desktop only) max
      63, mean 27.4 (the old look). The window read back from the screen
      equals the glass exactly on 95.4% of the menu (92.6% of the covered
      part at RGB565); the rest are text and checks. Canvas at opening vs
      at exit with the menu still open: 1237 covered pixels changed (the
      plate re-mixing the menu window), as on ApolloOS and Coffin. Pass.
- [x] (b) Disk stacks, test config Shell, Personal:, Archives:, BOOT:,
      DockGone: (the FFS ADF in IDF0: with `MountDiskImage DRIVE IDF0:
      INSERT`; Docs, Tools, readme.json made on it), Personal:Downloads,
      TextEdit (build 579):

      | check | result |
      | --- | --- |
      | right-click Archives: (no window) | Sort by (Name), hairline, Click to, Open Folder checked; no Close (115x146) |
      | Archives: -> Show Stack, Date Modified | "clicks": {"Archives:": "stack"}, "sorts": {"Archives:": "date"}; box "Archives" 185x200, order Games, Prefs, C, Demos, iGame.guide, S, Devs, ABDisk (List SORT=DATE agrees; C/S/Demos share a timestamp) |
      | Personal: click (Open Folder) | Workbench window "Personal" opened; its menu then 115x172 with Close below a hairline; Close closed the window |
      | Personal: -> Show Stack | "Personal:": "stack"; box 299x253 (16 entries, first paint 52 ms); -> Open Folder: "Personal:" leaves "clicks" |
      | BOOT: (about 180 root entries) -> Show Stack | box 698x598, page 1/2; first open after 30 s idle: first paint 292 ms, entrance 134 ms (3 frames, 023) |
      | DockGone: Show Stack, inserted | box 185x147, first paint 23 ms |
      | DockGone: ejected (MountDiskImage EJECT, Info: no disk) | click: box 185x109 "Cannot read drawer", no requester, first paint 19 ms; Kind chosen (re-arms the read-ahead), 30 s idle: no requester (screenshot) |
      | reorder Archives: to slot 0 (relative drag) | first in "paths", "sorts"/"clicks" kept; slot 0 click opens its box (185x200) |
      | read-ahead | Personal: at Open Folder is not read: stacks=4 (Archives:, BOOT:, DockGone:, Downloads) pending=0 reads=158 |
      | restart with DockGone: absent | "Cannot read this icon: DockGone:" at startup, as in 1.0 beta 6; Archives: menu shows Date Modified + Show Stack checked; the path and its "clicks"/"sorts" stay in the file |

      Profile: downs=9 ups=9 right_seen=8 right_icons=8. Pass.
- [x] (c) Right-click hiding, reproduced on 569 first. Agent clicks carry
      no button qualifier; input.device on 3.2.3 has IND_ADDEVENT (DoIO 0),
      so the scratch tool Rmb sent the press and release with
      IEQUALIFIER_RBUTTON (PeekQualifier "held" between them). PipWatch
      (read-only, 2 ms polls) logged on one EClock; dock box 208,610-
      1072,720 (user's config). With Workbench active each press showed
      Intuition's menu bar at the top (screenshot), no menu at the pointer.
      PipWatch's layer_busy matched the menu time exactly (e.g. 3097 polls
      = 6.9 s for 6.9 s of held menus): Intuition holds the screen's
      LayerInfo lock from press to release; the dock's own right-click
      (consumed by its handler) takes none.

      | right button held 0.5 s at | 569 (installed) | 579 | 582 |
      | --- | --- | --- | --- |
      | 400,500 | OFF 1.5 ms after down, ON 26 ms after up | OFF 0.6 ms after down, ON 27 ms after up | stays ON |
      | 300,650 | OFF 1.5 ms after down, ON 42 ms after up | OFF 1.4 ms after down, ON 27 ms after up | stays ON |
      | 1270,450 (198 px right of the dock) | - | OFF 0.6 ms after down, ON 27 ms after up | stays ON |
      | 640,355 (dy 255) | - | OFF 0.7 ms after down, ON 27 ms after up | stays ON |
      | 640,353 (dy 257) | - | stays ON | - |
      | 400,200 | stays ON | stays ON | stays ON |
      | Workbench menu dropped (press at 30,5, released at 400,400 off its items) | - | stays ON | stays ON |
      | on the Audio pin (825,692) | - | dock's own menu, stays ON | dock's own menu, stays ON |

      native_menu_hides: 579 4, 582 0. Confirmed: the same bug as on
      ApolloOS (IMG_2662), here for every right-click within 256 px of the
      dock; the menu itself always drops from the title bar.
- [x] Fix (build 581, owner check in 582): on Intuition before V50, when
      the screen's LayerInfo lock is held by input.device's task (where
      Intuition runs its menus) as the menu is decided, the menu is
      Intuition's pull-down; the dock reads the active window's MenuStrip
      (items, subitems relative to their item, bounded loops, under
      Forbid; ClearMenuStrip waits for a menu in use) and steps aside only
      if the area from the top to the bar + the lowest item + 16 rows
      (`dock_pulldown_reach`, host-tested) overlaps the dock. Any other
      case (lock free, as MagicMenu's pop-ups left it on Coffin, or held by
      another task) keeps the 256 px pointer rule. ApolloOS's menu-window
      path is unchanged.
- [x] Further checks: the table above (582); left dock (581, 5 pins, l.json): a right-click
      at 600,400 and the Workbench menu dropped over the dock both turn PiP
      OFF at the press and ON 33-36 ms after the release (native_menu_hides=2,
      as with the old rule, which always hid a dock whose top is < 256); 582
      with a menuless CON window active: right-clicks at 400,500 and 400,200 stay
      ON (Intuition still enters menu mode with an empty bar and holds the
      lock; the empty strip reaches only the bar). Installed 582: right-
      clicks at 400,500 / 300,650 / 400,200 stay ON.
- [x] Independent review (Opus, the diff): no P0. P1 fixed in 582: a busy
      lock alone is not proof of Intuition's menu (another task can hold it
      as the menu is decided): only input.device's task counts now. P1
      answered: AmigaOS 3.2 has no pop-up menus of its own (NDK 3.2
      prefs/icontrol.h has no such flag; intuition and IControl release
      notes name none). P2 left: no profile counter for which branch ran
      (a field in App moved hover by 12% once, lessons.md; PipWatch shows
      the outcome); the strip walk is not host-tested (only the reach
      arithmetic and the overlap are).
- [x] make host-test, make host-sanitize, make amiga (no warnings).
- [x] (4) First open of Downloads (356x359 box) after 45 s idle, pl.json
      (the user's 14 paths + Archives:Games + AOS32:Prefs, 16 pins), pointer
      off the dock, fresh dock per run, ChangeTaskPri -30, 569 alternated:

      | run | build | first paint ms | entrance ms | fields |
      | --- | --- | --- | --- | --- |
      | o1 | 569 | 100.0 | 137.8 | 01112 |
      | n1 | 582 | 90.4 | 147.8 | 01121 |
      | o2 | 569 | 90.9 | 130.4 | 0112 |
      | n2 | 582 | 90.5 | 149.2 | 01121 |
      | o3 | 569 | 90.5 | 147.1 | 01121 |
      | n3 | 582 | 90.3 | 145.8 | 01121 |
      | n4 | 582 | 91.3 | 138.8 | 02112 |

      (581 before the owner check: 91.2 / 147.1.) 569 earlier on 3.2.3
      (559 runs): 90.1-103.8 / 136.1-149.8. Prefetch stacks=4 pending=0
      reads=90 every run. Pass.
      Hover sweep (pl.json, relative moves x 350..950 at y 692, one per
      call, 45 s idle, -30):

      | run | build | render_us_avg | compose_us_avg | max us | stalled | redraws |
      | --- | --- | --- | --- | --- | --- | --- |
      | hn1 | 582 | 10474 | 7758 | 19722 | 6 | 113 |
      | ho1 | 569 | 9838 | 7091 | 19350 | 4 | 96 |
      | hn2 | 582 | 10496 | 7770 | 19629 | 6 | 97 |
      | ho2 | 569 | 9715 | 7018 | 19170 | 4 | 99 |
      | hp1 | 579 | 9893 | 7224 | 19294 | - | 99 |
      | hc1 | control | 10915 | 8220 | 101466 | - | 96 |

      582 composed 0.5-0.7 ms slower than 569/579 with no change to the
      compose code. Control (lessons.md: "a per-binary timing gap is not a
      code regression until a rebuild agrees"): 582's sources with the new
      decision not called (`pulldown_reach` kept, linked, never used; the
      579 behaviour, 582's layout) composed 8220 us, slower still. So the
      gap follows the binary's layout (the new code shifts everything
      linked after amiga_surface_pip.o), not the menu code, which runs
      only while the right button is held. Recorded, not fixed (the open
      layout-sensitivity item of the 553 rerun).
- [x] Images: scratchpad `design/32-7-menu-glass-bottom-4x.png`,
      `32-7-menu-glass-bottom-before-after-4x.png` (left: the same dump with
      the covered glass recomputed from the desktop alone, as 1.0 beta 6
      drew it), `32-7-disk-menu-{archives-idle,archives-stack,archives-after-restart,personal-open-close}-3x.png`,
      `32-7-disk-stack-{archives-date,personal,dockgone-inserted,dockgone-absent}-3x.png`,
      `32-7-disk-stack-boot-p1-2x.png`; also in ~/Downloads/DockAwesome-design/.
- [x] End state: 582 in SYS:WBStartup/DockAwesome (Version 0.582 build 582;
      569 was not locked and is deleted; copied as .new and renamed),
      started with WBRun; FindTask finds it, one DockAwesome.PiP2
      semaphore, Status lists no CLI dock; its Downloads menu opened over
      the row and closed (Esc). ENVARC:DockAwesome.json untouched (590
      bytes, 01:03:31 yesterday, as at the start). SYS:WBStartup/Disabled
      holds only WBDock. RAM: holds only Clipboards, ENV, T; IDF0: empty
      (the test ADF ejected and deleted, ENV:DiskImage/unit_0.xml it made
      deleted); DL: assign removed, ENV:dpid deleted; tools, configs, logs,
      dumps and test builds deleted; the control worktree removed.
      amiagent and ApolloExplorerSrv keep running.
- [ ] NOT RUN on 3.2.3: the physical mouse and the monitor (PiP is not in
      screenshots; register state and dumps only); a menu strip long enough
      to reach a bottom dock (host test only); the menu glass on a side
      dock; a real disk taken out (an ejected ADF stood in).
- [ ] NOT RUN with 582: Coffin (MagicMenu: by code unchanged, its menus
      leave the layer lock free), AmigaOS 3.9 and 3.1.4.1 (stock Intuition
      V45/V40: same pull-down path expected, not run), AmiKit XE, ApolloOS
      (menu-window path unchanged; same code before the new branch).

### AmigaOS 3.9, beta 7 items, build 583 (2026-11-27)
Plain AmigaOS 3.9 on the Vampire (Kickstart 40.68 mapped, agent reports
40.10; intuition.library 40.85, input.device 40.1; volume AOS39), 24-bit
1280x720. Stock Intuition pull-down menus: no MagicMenu or other menu
patch in WBStartup, S:Startup-Sequence or S:User-Startup, no
ENVARC:sys/icontrol.prefs. Build 583 = HEAD 2a322f5 (`make amiga`, no
warnings: the 3.2.3 control build had left objects stale, so it rebuilt;
583 differs from 3.2.3's 582 only in the three version digits).
Installed before: 569 from SYS:WBStartup. Test docks from RAM: with
`--config RAM:` copies, one at a time (FindTask, Sems, Status COM= before
each start; 3 s or more before each stop); 569 copied to RAM: for the
controls. Agent input with relative moves.
- [x] Install: the old Disabled/DockAwesome-locked-delete (551) was no
      longer locked and is deleted; 569 stopped (BreakTask, gone after
      5 s, no PiP2 semaphore), 583 copied as DockAwesome.new, the 569
      file was "in use" (WBStartup), so it is now
      SYS:WBStartup/Disabled/DockAwesome-locked-delete; .new renamed into
      place (Version 0.583 build 583). WBRun: FindTask finds it, one
      DockAwesome.PiP2 semaphore.
- [x] (a) Menu glass over the row: user's config copy (13 pins, size 36),
      right-click on Downloads (zoomed, last pin), `--pip-dump`: menu
      831,542 115x146, dock 224,610 832x110, 3201 dock pixels in the
      capture. Inner glass pixels covered by the dock (rim and corners
      excluded; tint [20,23,29] fitted on the desktop-only pixels, 100%
      share): 2649 px, glass vs tint(desktop+dock) max 0 (100% within 1);
      vs tint(desktop only) max 63, mean 30.95 (the old look). The window
      read back from the screen equals the glass exactly on 95.4% of the
      menu (91.5% of the covered part at RGB565); the rest are text and
      checks. Canvas at opening vs at exit with the menu still open: 1081
      covered pixels changed (the plate re-mixing the menu window), as on
      the other systems. The bright pixels at the menu's top corners are
      the desktop (the backdrop's green and yellow V stripes lie there;
      the desktop capture has them too). Pass.
- [x] (b) Disk stacks, test config Shell, Personal:, Archives:, BOOT:,
      DockGone: (the FFS ADF in IDF0: with `MountDiskImage DRIVE IDF0:
      INSERT`; Docs, Tools, readme.json made on it), Personal:Downloads,
      Editor (7 pins):

      | check | result |
      | --- | --- |
      | right-click Archives: (no window) | Sort by (Name), hairline, Click to, Open Folder checked; no Close (115x146) |
      | Archives: -> Show Stack, Date Modified | "clicks": {"Archives:": "stack"}, "sorts": {"Archives:": "date"}; box "Archives" 185x200, order Games, Prefs, C, Demos, iGame.guide, S, Devs, ABDisk (List SORT=DATE agrees; C/S/Demos share a timestamp) |
      | Personal: click (Open Folder) | Workbench window "Personal" opened; its menu then 115x172 with Close below a hairline; Close closed the window |
      | Personal: -> Show Stack | "Personal:": "stack"; box 299x253 (first paint 57 ms); -> Open Folder: "Personal:" leaves "clicks" |
      | BOOT: (about 180 root entries) -> Show Stack | box 698x598; first open after 30 s idle: first paint 296 ms, entrance 141 ms (3 frames, 023) |
      | DockGone: Show Stack, inserted | box 185x147, first paint 23 ms |
      | DockGone: ejected (MountDiskImage EJECT, Info: no disk) | click: box 185x109 "Cannot read drawer", no requester, first paint 18 ms; Kind chosen (re-arms the read-ahead), 30 s idle: no requester (screenshot) |
      | reorder Archives: to slot 0 (relative drag) | first in "paths", "sorts"/"clicks" kept; slot 0 click opens its box (185x200) |
      | read-ahead | Personal: at Open Folder is not read: stacks=4 (Archives:, BOOT:, DockGone:, Downloads) pending=0 reads=178 |
      | restart with DockGone: absent | "Cannot read this icon: DockGone:" at startup, as in 1.0 beta 6; Archives: menu shows Date Modified + Show Stack checked; the path and its "clicks"/"sorts" stay in the file |

      Profile: downs=8 ups=8 right_seen=8 right_icons=8. Pass. The first
      box of that session (Archives:) entered in 204 ms with a 34 ms
      fourth frame (fields 0115); its next open took 145 ms in 6 frames
      (011111), so it was not repeated.
- [x] (c) Right-click hiding. input.device 40.1 has no IND_ADDEVENT
      (Rmb: DoIO -3, IND_WRITEEVENT left PeekQualifier free), so RmbHold
      (IND_WRITEEVENT + PeekQualifier patched for the hold, restored and
      checked each time) gave the held button, as on Coffin. With
      Workbench active each press showed Intuition's menu bar at the top
      (screenshot), and the pull-down from 30,5 dropped below the bar; no
      menu at the pointer. PipWatch (read-only, 2 ms polls) on one EClock;
      dock box 224,610-1056,720 (user's config); layer_busy matched the
      held menus (4601 polls of 2.2 ms = 10.2 s for 10.0 s of menus):
      Intuition 40 holds the screen's LayerInfo lock for its menus, as on
      3.2.3.

      | right button held 0.5 s at | 569 (installed before) | 583 (installed) |
      | --- | --- | --- |
      | 400,500 | OFF 1.4 ms after down, ON 25 ms after up | stays ON |
      | 300,650 | OFF 0.7 ms after down, ON 25 ms after up | stays ON |
      | 1270,450 (214 px right of the dock) | OFF 1.2 ms after down, ON 25 ms after up | stays ON |
      | 640,355 (dy 255) | OFF 1.2 ms after down, ON 25 ms after up | stays ON |
      | 640,350 (dy 260) | stays ON | - |
      | 400,200 | stays ON | stays ON |
      | Workbench menu dropped (press at 30,5) | stays ON (1 s) | stays ON (1 s; 3 s, released at 400,500) |
      | held 3 s at 400,500 | - | stays ON |
      | on the Downloads pin (888,692) | - | dock's own menu, stays ON (native_menu_hides=0) |

      native_menu_hides: 569 4 (its profile; the installed 583 has no
      profile, PipWatch is the record). Left dock (583, the user's 13
      pins, l.json, dock from y 12): a right-click at 640,400 and the
      Workbench menu dropped from 30,5 both turn PiP OFF 0.5-1.0 ms after
      the press and ON 54-55 ms after the release (native_menu_hides=2):
      a menu that can reach the dock still hides it. Confirmed on 3.9:
      the bug (569) and the fix (583), the same as on 3.2.3.
- [x] (4) First open of Downloads (356x359 box) after 45 s idle, o.json
      (migrated each run, 15 pins), pointer off the dock, fresh dock per
      run, ChangeTaskPri -30, 569 alternated:

      | run | build | first paint ms | entrance ms | fields |
      | --- | --- | --- | --- | --- |
      | n1 | 583 | 92.1 | 142.6 | 01121 |
      | o1 | 569 | 92.1 | 147.8 | 01121 |
      | n2 | 583 | 91.4 | 148.4 | 01121 |
      | o2 | 569 | 92.0 | 148.2 | 01121 |
      | n3 | 583 | 92.1 | 148.0 | 01121 |
      | n4 | 583 | 183.3 | 131.8 | 0112 |
      | o3 | 569 | 92.0 | 147.9 | 01121 |
      | n5 | 583 | 92.1 | 142.7 | 01121 |

      569 on 3.9 yesterday: 92.2-93.0 / 141.7-147.0. n4: the last
      prepare stage (window open) took 117 ms against 25.5-26.0 ms in
      every other run; n5 repeated cleanly (as Coffin's one 92 ms
      OpenWindow stage, not the read). Prefetch stacks=3 pending=0
      reads=92 every run. Pass.
      Hover sweep (pl.json, 15 pins, relative moves x 350..950 at y 692,
      one per call, 45 s idle, -30):

      | run | build | render_us_avg | compose_us_avg | max us | stalled | redraws |
      | --- | --- | --- | --- | --- | --- | --- |
      | hn1 | 583 | 9444 | 6871 | 18042 | 1 | 114 |
      | ho1 | 569 | 10505 | 7957 | 100201 | 0 | 99 |
      | hn2 | 583 | 9430 | 6851 | 18393 | 0 | 98 |
      | ho2 | 569 | 9411 | 6874 | 18005 | 0 | 100 |

      569 yesterday 9835 / 18297 / 1. ho1 had one 100 ms frame. Equal.
      Pass (the 3.2.3 gap of 0.5-0.7 ms compose does not show here).
- [x] Images: scratchpad `design/39-7-menu-glass-bottom-4x.png`,
      `39-7-menu-glass-bottom-before-after-4x.png` (left: the same dump
      with the covered glass recomputed from the desktop alone, as 1.0
      beta 6 drew it),
      `39-7-disk-menu-{archives-idle,archives-stack,archives-after-restart,personal-open-close}-3x.png`,
      `39-7-disk-stack-archives-date-3x.png`,
      `39-7-disk-stack-dockgone-absent-2x.png` (on 3.9 the box is a
      real window, so a screenshot shows it); also in
      ~/Downloads/DockAwesome-design/. The Personal:, BOOT: and DockGone:
      inserted box dumps were deleted with the DL: drawer before they
      were fetched; their sizes and timings are above.
- [x] No defect found; no change to src/.
- [x] End state: 583 in SYS:WBStartup/DockAwesome, started with WBRun,
      FindTask finds it, one DockAwesome.PiP2 semaphore, Status lists no
      CLI dock; its Downloads menu opened (115x146) and closed (Esc).
      ENVARC:DockAwesome.json untouched (566 bytes, Yesterday 18:19:58, as
      at the start). RAM: holds only Clipboards, ENV, T and amiagent's own
      file; IDF0: empty (the test ADF ejected and deleted,
      ENV:DiskImage/unit_0.xml it made deleted); DL: assign removed,
      ENV:dpid and an Execute temp file deleted; tools, configs, logs and
      dumps deleted. amiagent and ApolloExplorerSrv keep running.
- [ ] Delete SYS:WBStartup/Disabled/DockAwesome-locked-delete (569) after
      the next 3.9 boot (still "in use" at the end).
- [ ] NOT RUN on 3.9: the physical mouse and the monitor (PiP is not in
      screenshots; register state and dumps only); the menu glass on a
      side dock; a real disk taken out (an ejected ADF stood in).

### AmiKit XE, beta 7 items, builds 583 and 589 (2026-11-27)
AmiKit XE 11.5 on the Vampire (Kickstart 47.7, agent reports KS 47.96 in
the title; intuition.library 47.51, input.device 47.1), Directory Opus 5
desktop, 24-bit 1280x720, agent warps at ((X+4)/2, (Y+4)/2). Menus:
MagicMenu 2 (task running; ENV:MagicMenu.prefs MenuType=0x02 = pull-down
when pressed in the screen's title bar, pop-up elsewhere; NonBlocking=Yes
= every box a borderless window without a port, no layer lock;
PUMode=0x02 = "select": a pop-up stays open after a quick release until
the next click) plus Directory Opus's own pop-ups (desktop, lister files,
lister title: windows with a port); VisualPrefs, Birdie and TitleShadow
run. Installed before: 569 in SYS:WBStartup (FindTask found it, one PiP2
semaphore). Build 583 = HEAD 0555984 (`make amiga`: nothing to rebuild,
byte-identical to the 3.9 upload). ApolloExplorerSrv was not running;
started with `Run >NIL: AmiKit:C/ApolloExplorerSrv`. Test docks from
RAM: with copies of the user's file (u.json = ENVARC:DockAwesome.json,
10 pins, size 36, dock box 288,606-992,720), one at a time (Sems and
Status COM= before each start, 3 s or more before each stop).
- [x] Install: 569 stopped (BreakTask, gone after 5 s, no PiP2
      semaphore), 583 copied as DockAwesome.new, the 569 file was not
      locked and is deleted, .new renamed (Version 0.583 build 583); 589
      replaced it the same way at the end (below).
- [x] (a) Menu glass over the row (586; a CON window with no text behind,
      so no desktop icons): right-click on Downloads (zoomed, last pin),
      `--pip-dump`: menu 769,514 119x174, dock 288,606 704x114, 3167 dock
      pixels in the capture. Inner glass pixels covered by the dock (rim
      and corners excluded; tint [20,23,29] fitted on the desktop-only
      pixels, 100% share): 2629 px, glass vs tint(desktop+dock) max 0
      (100% within 1); vs tint(desktop only) max 36, mean 19.3 (the old
      look; the grey CON window is darker than a backdrop, so the gap is
      smaller than on 3.9). The window read back from the screen equals
      the glass at RGB565 on 91.2% of the covered part (the rest text and
      checks). Canvas at opening vs at exit with the menu still open: 2271
      covered pixels changed (the plate re-mixing the menu window), as on
      the other systems. Pass.
- [x] (b) Disk stacks (586; the same stack code as 587), test config
      Shell, Personal:, Archives:, BOOT:, DockGone: (the Coffin/3.9 FFS
      ADF in IDF0: with `MountDiskImage DRIVE IDF0: INSERT`; Docs, Tools,
      readme.json made on it), Personal:Downloads, MultiView:

      | check | result |
      | --- | --- |
      | right-click Archives: (no window) | Sort by (Name), hairline, Click to, Open Folder checked; no Close (119x174) |
      | Archives: click (Open Folder) | Directory Opus lister "Archives" (326M free), no DOS error; its menu then 119x204 with Close below a hairline; Close closed the lister |
      | Personal: click (Open Folder) | Directory Opus lister "Personal" (3.7G free), no DOS error; Close (menu 119x204) closed it |
      | Archives: -> Show Stack, Date Modified | "clicks": {"Archives:": "stack"}, "sorts": {"Archives:": "date"}; box 185x200, order Games, Prefs, C, Demos, iGame.guide, S, Devs, ABDisk (List SORT=DATE agrees; C/S/Demos/iGame.guide share 12:14:47) |
      | Personal: -> Show Stack | box 299x253 (16 icons read on open: it was not read ahead at Open Folder; first paint 72 ms); -> Open Folder: "Personal:" leaves "clicks" |
      | BOOT: (about 180 root entries) -> Show Stack | box 698x598; first open after 30 s idle: first paint 317 ms, entrance 202 ms (3 frames, 026; 3.9: 296 / 141) |
      | DockGone: Show Stack, inserted | empty disk 185x109; with Docs, Tools, readme.json 185x147, first paint 30 ms |
      | DockGone: ejected (MountDiskImage EJECT, Info: no disk) | click: box 185x109 "Cannot read drawer" (dump), no requester, first paint 40 ms (58 ms in the dump run); Kind chosen, 30 s idle: no requester (screenshot) |
      | reorder Archives: to slot 0 (drag at warp coordinates) | first in "paths", "sorts"/"clicks" kept; slot 0 click opens its box (185x200) |
      | read-ahead | Personal: at Open Folder is not read: stacks=4 (Archives:, BOOT:, DockGone:, Downloads) pending=0 reads=158 |
      | restart (dump run, DockGone: inserted) | Archives: menu shows Date Modified + Show Stack checked |
      | restart with DockGone: absent | the dock's "Cannot read this icon" requester at startup (three lines, as in 1.0 beta 6; ReqAttack's pattern made the screenshot unreadable, the layout matches the message), OK pressed; the path and its "clicks"/"sorts" stay in the file |

      Directory Opus opened its own lister for DockGone at each insert; it
      was closed again.
- [x] (c) Right-click hiding. input.device 47.1 has IND_ADDEVENT (Rmb:
      DoIO 0, PeekQualifier 2000 while held), so the scratch tool Rmb gave
      the held button (`Rmb 25`, or `Rmb down` / `Rmb up` to look at a
      menu and release off its boxes; a select-mode pop-up was closed with
      a second right press). PipWatch (read-only, 2 ms polls, its layer
      list showed every menu box) on one EClock; dock box
      288,606-992,720. With MagicMenu each press reached the handlers twice
      when MagicMenu passed it on (menuless window), once when it showed a
      menu.

      | right button at (what opened) | 569 | 583 | 587 (584/586 where marked or for the DOpus rows) |
      | --- | --- | --- | --- |
      | 400,500, menuless CON active (Intuition's empty bar, layer lock busy) | - | OFF 0.4 ms after down, ON 44 ms after up | stays ON (584) |
      | 300,650, menuless CON | - | OFF at down, ON 48 ms after up | stays ON (584) |
      | 640,356 / 640,350, menuless CON (dy 250 / 256) | - | OFF, ON 26 ms after up / stays ON | - |
      | 1270,450 / 400,200, menuless CON | - | stays ON | stays ON (584) |
      | 640,450 desktop: MagicMenu pop-up 596,440-686,583 (does not reach the dock) | OFF 0.5 ms after down, ON 58 ms after up | OFF at down, ON 42 ms after up | first menu after start: OFF at down, ON 76 ms after up; every later one: stays ON (box opens 68-93 ms after the press) |
      | 640,600 desktop: MagicMenu pop-up 596,578-686,719 over the plate, stays open after the release (select mode) | OFF at down, ON 58 ms after up: PiP over the open menu's lower titles for 3.3 s | the same (ON 40 ms after up, menu open 3.9 s more) | OFF 6.6 ms after the box opened, stays OFF after the release until the menu is cancelled, ON 45 ms after that |
      | 30,6 bar: MagicMenu pull-down (bar window 0,0-1279,24, Opus box 2,21-176,140) | - | stays ON | stays ON |
      | 400,200 desktop: MagicMenu pop-up 356,190-446,333 | - | - | stays ON |
      | 500,530 in a DOpus lister: DOpus file pop-up 492,472-645,719 (a window with a port, opens 234-244 ms after the press) | - | - | OFF 156 ms after the press, before it opened (584 with a 250 ms wait: OFF 13 ms after it opened) |
      | 550,312 lister title: DOpus pop-up 542,308-739,458 | - | - | stays ON |
      | 640,450 desktop with a lister open: DOpus desktop pop-up 632,446-759,571 (port) | - | - | OFF at down (pointer rule; no MagicMenu box seen yet in that dock), ON after up |
      | left dock (586, u.json edge left): 30,6 pull-down / 640,400 pop-up | - | - | OFF 8 ms after down, ON 190 ms after up / stays ON |

      Which rule 583 applied on AmiKit: the 256 px pointer rule for every
      menu (MagicMenu takes no layer lock; for a menuless window the dock
      decided at the press, before MagicMenu passed the event on to
      Intuition). Found: (1) DEFECT (569 and 583): a MagicMenu pop-up kept
      open after the release (select mode) lies under PiP when it reaches
      the dock; (2) every right-click within 256 px hid the dock while
      held, also when the menu opened away from it.
- [x] Fix (builds 584-589; 589 installed): on AmigaOS 3.x a menu is "up"
      while the right button is held or the active window on the dock's
      screen is marked MENUSTATE (Intuition and MagicMenu set it until the
      menu closes; `native_menu_up`). Per menu, `dock_menu_step`
      (host-tested): MagicMenu's boxes (borderless windows without a port
      in front of the dock; windows already there at the press are
      ignored) decide once they show (step aside while one overlaps the
      dock, for the rest of the menu), also undoing a pointer-rule guess;
      Intuition's pull-down decides by its reach (unchanged); otherwise the
      256 px pointer rule. After a menu with boxes, later menus wait up to
      150 ms for their boxes before the pointer rule (no blink). A menu
      kept open after the release that hid the dock keeps the loop in the
      menu branch (no status polls or captures); one that leaves the dock
      shown runs the normal pass as before (which draws PiP again). App
      keeps its size and layout (menu state packed into bytes, the EClock's
      low word for the wait; checked with the cross compiler: sizeof 215690
      and the same offsets). ApolloOS's path is unchanged.
- [x] 589 on AmiKit (PipWatch): 640,450 pop-up: first menu after start
      OFF 5 ms after down, ON 24 ms after up with the menu still open;
      later ones stay ON. 640,600 pop-up over the plate: OFF when its box
      opened (87 ms after the press), OFF after the release until the menu
      was cancelled, ON 26 ms after that. Title-bar pull-down, 400,200
      pop-up, DOpus lister title pop-up: stay ON. DOpus file pop-up in a
      lister (492,472-694,719): OFF 156 ms after the press, 97 ms before
      it opened. Menuless CON at 400,500 and 300,650: 6 of 7 presses stay
      ON; the first press after the CON window opened turned PiP off
      158 ms after the press (the wait ran out before Intuition's lock was
      seen; 583 turned it off at every press). native_menu_hides=4.
- [x] Independent review (Opus, the 587 diff): no P0. P1 "a menu still
      open after the release runs the full pass (status polls, capture)":
      fixed in 588/589 as above for a menu that hides the dock (a blocking
      MagicMenu holding locks then never meets the polls); a menu that
      leaves the dock shown keeps the normal pass, as 569-583 did and as
      AmiKit ran it for seconds without a stall. P1 "an undone pointer
      guess does not redraw while the button is held": 588 routed every
      open menu into the menu branch and so kept PiP off until the menu
      closed; 589 redraws at the release instead (24 ms after up); while
      the button is still held the first menu after start stays hidden as
      in 583 (a redraw there would need a present without capture; left).
      P2 "any borderless window without a port counts as a menu box":
      windows already in front of the dock at the press are now ignored
      for that menu (MENU_OLD_WINDOWS).
- [x] make host-test, make host-sanitize, make amiga (no warnings).
- [x] (4) First open after 45 s idle, pointer parked at 640,300 on the
      DOpus desktop, fresh dock per run, ChangeTaskPri -30, a copy of the
      user's ENVARC file per run; Pictures opened and closed, then
      Downloads after another 45 s (build 586; 587-589 changed only the
      menu code and its packing):

      | run | build | Pictures first paint / entrance ms (fields) | Downloads first paint / entrance ms (fields) |
      | --- | --- | --- | --- |
      | f1 | 586 | 232.8 / 188.6 (0125) | 113.5 / 164.7 (01222) |
      | f2 | 586 | 238.9 / 162.5 (0124) | 110.5 / 174.2 (024) |
      | f3 | 586 | 306.5 / 154.6 (0132) | 112.5 / 142.2 (013) |
      | g1 | 569 | 259.0 / 163.2 (0223) | 112.4 / 135.5 (013) |
      | f4 | 586 | 250.8 / 164.1 (04) | 134.2 / 164.5 (0222) |

      Medians 586: Pictures 244.9 / 163.3, Downloads 113.0 / 164.6;
      569 yesterday: 284.4 / 168.2 and 126.5 / 153.9. First paints are
      faster; the entrances spread in the fields a present waits, as
      before. The first agent click after idle was lost once (f4
      Pictures: `presses_armed=0 presses_unarmed=0`, the click never
      reached the dock) in 8 opens; in f1 the box only opened slowly (a
      second click closed it). Prefetch stacks=5 pending=0 reads=153 in
      every run. Pass.
      Hover sweep (relative moves x 350..950 at y 690, one per call, 45 s
      idle, user's file copy, -30):

      | run | build | render_us_avg | compose_us_avg | max us | stalled | redraws |
      | --- | --- | --- | --- | --- | --- | --- |
      | hn1 | 586 | 10375 | 8478 | 101223 | 0 | 39 |
      | ho1 | 569 | 7397 | 5626 | 14832 | 2 | 40 |
      | hn2 | 586 | 7998 | 6104 | 14811 | 0 | 41 |
      | hp1 | 583 | 7057 | 5134 | 14223 | 1 | 41 |
      | hq1 | 587 | 8159 | 6262 | 15117 | 0 | 40 |
      | hp2 | 583 | 6962 | 5081 | 14893 | 2 | 40 |
      | hs1 | 589 | 8011 | 6104 | 15371 | 0 | 41 |

      569 yesterday 6957 / 14374 / 2. 586-589 compose about 1 ms slower
      than 583, with no stall. Checked: packing the menu state so App
      keeps its size (587) did not change it; the compose, renderer, core,
      AMMX and display objects of 583 and 587 are identical instruction
      for instruction (only local static symbol numbers differ), and 808
      bytes of new code (amiga_main.o +16, amiga_surface_pip.o +792) now
      lie before them in the link. So the gap follows the binary's layout
      (the open layout item of the 553 and 3.2.3 runs), not code that runs
      while hovering. Recorded, not fixed.
- [x] Images: scratchpad `design/ak7-menu-glass-bottom-3x.png`,
      `ak7-menu-glass-bottom-before-after-4x.png` (left: the covered glass
      recomputed from the desktop alone, as 1.0 beta 6 drew it),
      `ak7-disk-menu-{archives-idle,archives-open-close,archives-after-restart}-3x.png`,
      `ak7-disk-stack-{archives-date,dockgone-absent}-3x.png` (box dumps),
      `ak7-disk-archives-open-folder-lister-1x.png`; also in
      ~/Downloads/DockAwesome-design/.
- [x] End state: 589 in SYS:WBStartup/DockAwesome (Version 0.589 build
      589; 583 was not locked and is deleted; copied as .new and renamed),
      started with a script `Run >NIL: Stack 400000 +` /
      `SYS:WBStartup/DockAwesome` (deleted); Status shows it once with stk
      400000, one DockAwesome.PiP2 semaphore, no test dock; its Downloads
      box opened (356x359) and closed with the user's config.
      ENVARC:DockAwesome.json untouched (424 bytes, 17:49:26, as at the
      start). RAM: holds only Clipboards, T, ENV and Logs; ENV:DiskImage
      empty (the test ADF ejected and deleted with its unit_0.xml); DL:
      assign removed, ENV:dpid deleted; tools, configs, logs and dumps
      deleted; no Directory Opus lister open (none was at the start); the
      scratch worktree removed. amiagent and ApolloExplorerSrv keep
      running.
- [ ] NOT RUN on AmiKit: the physical mouse and the monitor (PiP is not in
      screenshots; register state and dumps only); the menu glass on a
      side dock; a real disk taken out (an ejected ADF stood in); first
      open on 589 itself (586 measured).
- [ ] NOT RUN with 589: Coffin (its MagicMenu mode is not recorded: if it
      draws boxes as windows, the new watching applies there too; if not,
      unchanged), ApolloOS (path unchanged), AmigaOS 3.2.3/3.9/3.1.4.1
      (Intuition's lock path unchanged; MENUSTATE only adds menus that stay
      open after the release).
- [ ] Open: the first menu after the dock starts still hides it while the
      button is held near the dock (until MagicMenu's first box is seen);
      Directory Opus's own pop-ups (windows with a port) keep the pointer
      rule.
