# Project lessons

## Communication

- A local build is "build N", not a version. Versions exist only for
  release packages (for example "1.0 beta 1").
- Reply to the user in Swedish, in plain words. Present comparisons as
  "icon one, two, three steps from the pointer" with sizes in pixels, not
  internal terms such as falloff, tail or neighbour index.

## Build and test

- Build successfully before uploading. An old binary can survive a failed build.
- A different compiler is a different program. After a toolchain change,
  build clean and compare the dock's frame with --pip-dump against the
  installed build before judging any other change. GCC 6.5.0b
  (GCC-6.50-Dev) at -O2 -m68080 got an int64_t expression in the layout
  wrong; objects left from the old compiler hid it until a header change
  rebuilt them.
- Cross-compile the host tests for the Amiga to find such miscompiles,
  and run them from an Execute script that sets Stack and FailAt first:
  extra lines in one amiga_shell call do not run.
- Run CPU-bound work on ApolloOS below priority -21 (ChangeTaskPri -30 in
  the script). The SD card tasks SDIO0/SDIO1 run at -21: at -5 the tests
  starved them, so Workbench, WBRun and the dock hung on disk access and
  the machine looked frozen. It was never a crash.
- Find a compiler bug by halving: optimization flags (-fbbb=-, one bbb
  letter at a time), then a counter on the suspect pass in a self-built
  cc1, run on the Vampire. Fix it in ApolloCrossDev as a patch; build the
  toolchain with its script and a PATH without spaces.
- Wait for the native window or gadget before sending input or reading saved
  preferences. A visible window alone does not prove the dock responds.
- Redirect the profiled command's output from an AmigaDOS script.
- Test each native path on its target OS. Measure full frames and individual
  stages. Elapsed time and competing-task throughput measure different costs.
- Clip image bounds before negating or subtracting coordinates. Test extremes
  under a sanitizer.
- Use the observed video for motion. Use physical PiP output for scanout.
  Screenshots and exported buffers can omit or miss display issues.

- Drive the dock and Workbench windows with relative motion
  (amiga_drag/amiga_click relative:true). Absolute warps did not move a
  window and missed the dock's hover and drag paths.
- Test the popup on the plate strip under the icons. A right-click on a pin
  without menu actions opens nothing by design, and zoom moves the icons, so
  a fixed point can hit a pin in one run and the plate in the next.
- Measure backdrop changes by running the dock from an Execute script with
  --profile, dragging a drawer window across the dock, then Break. Compare
  render_us_avg and the Damage line against the same scenario on the old
  build.
- Parallel worktree agents can start from an older commit: tell each to reset
  to the integration commit. Merge one branch at a time and run
  --damage-check on the Vampire. Bisect a regression by building each branch
  head on its own.

## Motion and appearance

- Zoom inward from a fixed baseline. Keep plate thickness fixed and let its
  ends follow the icon row. Middle hovers can center. End hovers can shift.
- Measure the zoom ratio, wave, and activation area. The reference is about
  2× and grows the icons 1, 2 and 3 pitches away by 0.86, 0.48 and 0.04 of
  the zoom. Stop the timer when the row settles.
- Keep the wave's weights summing to a constant at every pointer phase and
  floor the phase once for all icons. Otherwise the row's total growth
  breathes and icons outside the wave move by a pixel.
- Keep hover continuous across enlarged icons. Use icon alpha for button hits.
- Map the pointer between centers. Separate icon and gap mapping caused a
  7 px reversal in one five-pin test.
- Keep Q8 target centers separate from rounded cache widths. Round each icon
  on its own so distant icons stay steady.
- Compare current and target placements to decide when to stop ticking. Draw
  only on raster changes. Use a one-pixel minimum correction capped by the
  remaining distance to avoid a long fractional tail.
- Keep level 0 when Scale is 1.00×. Identical cache images need no animation.
- Measure gaps at the artwork, including transparent margins. When tighter
  zoom gaps are requested, keep rest spacing and compact as neighbors grow.
- Check every corner, the thin rim, icon centering, and the screen gap.
  Preserve corner samples when adjusting the straight rim.
- Keep the right PiP transport padding separate from the 3 px screen gap.
- Compare original PNG alpha with the decoded icon before changing scaling.
- Use a Vampire screenshot as the baseline for Workbench icon comparisons.
- Keep running dots small and sharp, near the screen-side icon baseline.
- A reference video from the user is the specification. Extract frames,
  crop the detail at full resolution and measure it (for example, where
  the stack tip points in each frame) before planning or building.

## PiP and Intuition

- Keep layout, pixel kernels, and screen output separate. Vampire and SAGA
  are hardware targets. Snapshot and opaque modes support other window stacks.
- Probe native alpha support before relying on it. Simple Refresh does not
  retain hidden pixels. Keep the input window clear of visible PiP artwork.
- Wait for `FirstScreen` after `ScreenToFront`. Check known source colors.
- Screen movement can require a new PiP frame even when source bytes match.
- Coalesce capture deadlines. Measure native refresh and shape updates
  separately from pixel composition.
- Inspect the locked layer chain for depth. `Screen.FirstWindow` is creation
  order on ApolloOS.
- Validate source and physical alignment with window edges crossing the overlay.
  Check calibrated registers if the rim is missing on the display.
- Keep DMA rows aligned to 64 bytes, in 32-pixel width steps, with zero modulo.
  A 336 px test showed stripes. Retest physical output after geometry changes.
- Pass a full `ULONG` count to ApolloOS text calls. A classic `UWORD` inline
  can leave stale upper bits. Check generated register writes.
- If a task stays in a library call after Ctrl-C, use a user-approved restart.
- Do not change a window shape per frame: Workbench redraws the desktop
  behind each change (dock frames 10 -> 24 ms). Shape the whole area a
  part moves in once and repaint it from the saved backdrop.

## Workbench input and drops

- Prepare the temporary receiver before Workbench locks its layers. Capture
  the clean desktop first and backfill newly exposed regions from that image.
  Skip normal layer and bitmap reads during the gesture.
- Check Workbench port ownership and client bounds. Read selection bounds
  between gestures. The selection hook can block on an icon press.
- Test fast empty-desktop clicks and stationary holds. Keep the idle receiver
  shape empty and avoid a visible marker.
- Preserve the sampled release position. USB events can combine motion with
  a press or release, unlike basic automation.
- Keep the insertion gap until the AppMessage arrives. Verify the original
  desktop icon and saved pin order through an add-and-remove test.
- Check for duplicates and enough zoom space before reusing the live surface.
  Keep the surface when the row fits to avoid a visible restart.
- Keep internal reorder zoom active. Incoming drops use the resting sizes.
- Verify the full right-button pair, including a held press on an icon or
  outside an open popup. Preserve independent ownership for the popup.
- Test the click while Workbench has focus and while the dock has focus.

## Settings and menus

- Use the click to place the context menu. The PiP rectangle includes clear
  padding. Check the visible artwork over the menu's actual span.
- Sample the desktop for menu glass and mask the overlay inside its shape.
  Mask owned dialogs as they move and refresh the old area.
- Test Settings on the machine. Keep the window and controls compact.
  Center button labels by glyph bounds and use equal opposite padding.
- For GadTools MX labels, set `PLACETEXT_RIGHT` and set the heading
  separately with `GTMX_TitlePlace`.
- Use real relative motion when automation cannot move a slider knob.
  Verify the value and saved file. Persist the scale factor separately.
- Keep decorative native windows untitled. Patched Workbench can show titles
  for borderless windows.
- Keep the one configured Folder Stack separate from regular drawer pins.
  Check its icon at display size and test navigation and saving.
- Test folder glass over a window edge during motion and after close.
  Sample the backdrop at the destination. Simple Refresh can repaint on close.
- Use JSON for preferences. Preserve the user's file during a format change.

## Open items

- Track the object that stays open. Shell can outlive its Workbench launcher.
  Drawers and volumes use Workbench's open drawer list.
- Match a document separately from its default tool. Check that closing the
  actual item clears the dot. Test unpinned app discovery separately.
- Check Shell close dispatch separately from its indicator. ApolloOS can
  route raw console close through the active window.

## Transfers and benchmarks

- After `gh pr merge`, fetch before fast-forwarding and building main: a
  stale origin/main built the old code once.

- Transfer files with ApolloExplorer `acp`, not base64. Start
  `C:ApolloExplorerSrv` on the Amiga first. If `acp` cannot load Qt, run a
  copy next to a `Frameworks` link to the app bundle and set
  `DYLD_FRAMEWORK_PATH`. Uploads to `host:RAM:` work; downloads keep only the
  text before the second colon, so fetch a volume root.
- Benchmark cache builds with the real decoded pins and live settings. A
  synthetic icon overstated translucent pixels and hid where time went.
- 64-bit math in per-tap code becomes library calls on the 68k.
- When moving code to another file, diff the m68k disassembly per function.
  GCC had inferred that a same-file callee was pure; across files it
  reloaded fields after the call until the declaration said so.
- Diff `gcc -S` output, not objdump: Amiga objects hold several code hunks
  with overlapping addresses, so objdump assigns code to the wrong symbol.
  In the single file GCC put draw, the event loop and live_capture in
  .text.unlikely (optimized for size); moved behind a function table they
  compile for speed. Separate that from real changes with a diagnostic
  build that adds `__attribute__((cold))` and `-fno-inline`.
- Give delegated native fixes an independent review before deploying.
  Fixes for blocking and bounded waits can move the risk to exit time:
  never exit while the system can still signal the task or call our code.

## Icons

- Match the pinned icons: 48x48 PNG icons whose artwork fills about 44x44,
  with a selected image (ApolloOS darkens it and adds an active detail).
- Show distinct design options side by side before installing an icon.
  Use Workbench colors; neon colors read as non-Amiga.
- At 48 px, use one bold symbol with a strong silhouette that fills the
  canvas. Small scenes (plates, rows, shelves) read as clutter.
- After a rejected design, offer several distinct concepts at real size on
  the real backdrop instead of iterating on the same idea.
- Base the program icon on the actual dock. Keep the Amiga reference small.
- A WBStartup program that keeps running needs the DONOTWAIT tooltype,
  or Workbench stalls startup with "has not yet returned" and the later
  WBStartup programs (AmiMCP included) never start. Check it with
  GetDiskObject/FindToolType on the installed icon.
- Time outline (diskfont) fonts cold, right after a reboot: a warm cache
  hid an 11 s open. Prerender static UI text instead of opening outline
  fonts on demand.
- Probes that draw text must use the ULONG Text/TextLength wrappers from
  amiga_text.h; the NDK UWORD inlines hung a probe on ApolloOS.
- The ApolloOS source in ~/GitRepos/ApolloOS is older than R9.5 (Intuition
  LVOs differ by one slot). Verify OS behavior with a probe on the target.
- Workbench reset notifications on ApolloOS come from screennotify.library
  (AddWorkbenchClient, type 3), not Intuition's SNOTIFY_BEFORE_CLOSEWB.
- A framebench win on malloc buffers does not carry over to the PiP buffer:
  measure conversion changes in the real dock with --profile before keeping
  them (a fused kernel 30% faster in the bench was 9% slower live).
- Workbench-started programs do not appear in Status. Before blaming a stale
  PiP2 lease, check for a dock started from WBStartup.

- Installing a dock build on the Amiga: quit the dock, then Rename the WBStartup binary to Disabled/ BEFORE copying (Workbench keeps a WBRun binary locked: Copy fails with "object is in use"). Run install steps one call at a time and check each result before deleting the uploaded copy or starting WBRun; a parallel batch restarted the old build and deleted the new upload (2026-11-09).
- Never run `Avail FLUSH` through the Amiga agent on the Vampire: on
  ApolloOS with 1.5 GB it did not return, Break did not stop it, and
  afterwards neither the agent nor new programs started. Read free memory
  from the Workbench title bar instead (2026-11-09).
- Read ../amiga-refs (RKM Libraries/Devices 3rd ed., HRM, Apollo docs)
  before building on OS behaviour, not only third-party source: its
  Workbench chapter showed that Workbench runs a project with the project
  icon's stack and TOOLPRI, which the first launcher had wrong. The NDK 3.2
  R4 autodocs and release notes are now in ../amiga-refs/amigaos-3.2; the
  wb release notes showed $STACK: and the CLI/REXX tool types, which I had
  wrongly called absent from Workbench 3.x (2026-11-09).
- Screenshots do not show the SAGA PiP layer. The installed dock (PiP, from
  WBStartup) is invisible in them, and its hover labels ("Music",
  "Internet") look like stray text. Put a test dock on another edge, or quit
  the installed one, before clicking: test clicks at the bottom started
  AmigaAMP through the installed dock (2026-11-09).
- Before blaming the dock for a leaked lock (or other resource), repeat the
  launch with a control: the same tool through Workbench, and a tool with no
  C startup. A drawer "leak" here came from the libnix test tool on
  ApolloOS; Workbench left the same lock (2026-11-09).
- RX treats a quoted first argument as ARexx source, not a file name. Run
  a script by path with RX "call '<path>'" (2026-11-09).
- AmigaOS 3.9 Intuition holds the screen's LayerInfo lock through window
  drags. A poll that gives up on AttemptSemaphore stalls for the whole
  drag; skip only the part that needs the lock (Coffin PiP probe,
  2026-11-09).
- PiP and other overlays are invisible to screenshots: ask the user what
  the monitor shows, and test the hardware path alone (a bare register
  probe) before debugging the dock around it. The Coffin PiP "failure"
  was the dock's own visible-width mask, not the hardware (2026-11-09).
- Test Amiga interactions myself (amiga_drag, amiga_click, --profile
  counters) before asking the user; ask only for what the agent cannot see
  (the PiP layer on the monitor, a physical mouse, reboots). The user had
  to say so after a run of "please try" requests (2026-11-09).
- AmigaOS 3.9 Workbench cancels an icon drag if windows are moved or
  restacked as it starts: a receiver that moves onto the dock at the press
  stalled input about a second and got no AppMessage. A drop target there
  must already be in place before the press (2026-11-09).
- After uploading a build, quit every running dock and check Status shows
  exactly one RAM:DockAwesome before asking anyone to look: a second PiP
  dock finds the PiP2 lease taken and quits silently under Run >NIL:, so
  the old build kept running and the user tested it (2026-11-09).

## Prove a visual cause before building its fix (2026-11-09)
- I blamed PiP1 on/off toggling for a doubled dock image seen in the user's video and built "PiP1 stays on". The next video showed the same doubling, so the cause was a guess.
- What was ruled out on Coffin: compose/convert mismatch (--damage-check: 0/0), a dock image in the captured backdrop (--pip-dump source is clean).
- Rule: when only the user can see the symptom, first confirm with them what exactly they mean (send a still from their own video), then change code.
- Go through the user's video frame by frame (ffmpeg crop + tile at full 60 fps) before guessing; the 3 fps sheet hid detail.

## SAGA overlay order: PiP2 lies over PiP1 (2026-11-09)
- On Coffin (core 11900+) PiP2's opaque pixels cover PiP1. I read a blurry video frame the other way and moved the dragged icon to PiP1 only; the user saw it vanish behind the dock's plate.
- Rule: the dock (PiP2) draws whatever lies on its surface; PiP1 only adds what lies beyond it, as the same picture so the two meet without a seam.

## Cancelling a Workbench drag in tests on Coffin (2026-11-09)
- A right-click to cancel the drag opened a MagicMenu menu; Esc then closed the active drawer window. Reopen it with ARexx `WINDOW "<path>" OPEN`.
- Rule: end a test drag by releasing over the icon's own window (or its start point) instead of the right button.

## Workbench draws its drag image after the pointer moves (2026-11-10)
- A per-sample log on Coffin showed the pointer at its new place while the screen still held the drag image at the old one; it caught up a sample later. Shifting the reveal to the pointer therefore made it worse, and holes in PiP2 always lag the image.
- Rule: over the dock, paint the drag image's sampled pixels into the PiP buffer instead of revealing the screen; log per-sample data before choosing between such fixes.
- Check the test itself before reading its numbers: my press missed the moved icon and started a lasso, and I read the lasso's pixel count as a regression. Take a screenshot during each scripted drag.

## Probe a capability by using it (2026-11-10)
- On Coffin, rgb_rastport (graphics LVO table size) and alpha_text (cybergraphics opens as V51) both said yes, but RGB pens drew palette orange and BltTemplateAlpha drew nothing: an orange Folder Stack frame and an invisible menu text.
- Rule: a capability that draws is probed by drawing one pixel into a bitmap like the screen's and reading it back.

## Leave the user's dock running after a test (2026-11-10)
- After a profiled test run I sent Break and asked the user to film the dock; it was not running ("Ser inte dockan").
- Rule: every test that stops the dock ends with Run >NIL: of the normal command and a Status check, before any request to the user.

## Two SAGA overlays must not share rows with a wide one (2026-11-10)
- With the dock at the bottom (PiP2 992 px wide) on Coffin's 24-bit screen, a tooltip in PiP1 scrambled the whole dock; the PiP1 overlay was 96 rows tall and reached over the dock's rows. Beside a 96 px side dock the same overlay showed fine.
- Keeping PiP1 off the dock's rows (even 4 rows clear) did not help: every PiP1 label or drag still scrambled the 992 px dock. Measured fine: 128 + 96 px.
- Rule: use a second overlay only where its combination was seen working (PiP1 beside a narrow dock); test layout changes (edge, size) on the hardware, not only on the edge that was verified; after two failed hardware guesses, fall back to the known-good path instead of a third.

## Probes must tolerate the screen's precision (2026-11-10)
- The RGB pen probe compared the read-back pixel exactly; on ApolloOS's 16-bit screen the colour comes back with 5-6 bits per channel, so it said "no" and the UI lost its RGB colours. Compare at the screen's precision (RGB565 masks).
- Run --caps on every OS after changing a probe, not only on the OS the change was made for.

## A fix for "all OSes" covers every branch of the code (2026-11-10)
- The user said the drag-out edge fix applied to all versions and OSes. I fixed the PiP1 and window paths on Coffin, but the shaped (ApolloOS) branch of update_drag_preview kept its own rule: no window until Remove armed, so the dock's icon was cut at PiP2's invisible edge. My ApolloOS regression said "drag preview works" from screenshots, which do not show PiP.
- Rule: when a fix must hold on every OS, list every branch that selects behaviour by caps (shaped, PiP1, window) and change or rule out each one. A screenshot never verifies what PiP shows; ask the user for that part.

## Without shapes, the armed drop receiver must not cover windows (2026-11-10)
- Filmed on Coffin (IMG_2484): a stale band over a Workbench list window where it reached into the dock's box. The receiver arms with the buttons up (pointer on a Workbench icon, anywhere in a drawer window, and stays armed on the dock), went to the front over the whole box and backfilled from a snapshot; whatever changed under it (wheel scrolling, a move) showed stale through PiP's keyed headroom.
- A screenshot shows this (the receiver is in the screen bitmap): on AmigaOS 3.2.3 a window moved into the box was cut at the box's top edge.
- Rule: a helper window that waits on the dock's box stays behind the other windows (WindowToBack); check windows near the dock with the pointer inside a Workbench window after any change to arming.
- On AmigaOS 3.2.3 a Run process is not found by task name: use Status COMMAND=<path> and Break <n>.

## See the screen mid-drag before handing a drag fix over (2026-11-10)
- Twice the user saw the same drag fault after my fix: the drawn pin was allowed one try per gesture, so leaving the dock a second time in the same drag fell back to the window preview. Counters (opens, places) said "drawn" and I believed them.
- What showed it: holding the button with amiga_button, moving with relative moves (absolute warps did not register as a press on the left-edge dock), and a screenshot at each step; the window preview's grey Remove label gave it away.
- Rule: for any drag change, test out, back in and out again in one gesture with the button held, and look at each step. Count only work that is needed (a still pin was redrawn 1000 times).

## SAGA PiP rows wrap their last columns; composite before claiming a fix (2026-11-10)
- A left PiP dock showed pixels of a dragged pin at the screen's left edge and lost its own last columns: SAGA shows the last columns of a PiP row at the start of the next. The right dock's keyed tail had already been found; the left dock lacked it. Calibration (+17/-1) was right; my guess that it was wrong cost a round.
- What finally showed every fault: the button held, the pin stepped across the edge in small steps, and each screenshot composited with that frame's PiP front buffer (--pip-dump). Do this before handing over any change at the PiP/screen seam, and keep the steps small, as the user asked.
- Programs started with Run >file lose their stdout here: run them from an Execute script, and have probes also write results to a file of their own.

## Native menus: MagicMenu holds IntuitionBase and P96's bitmap lock (2026-11-10)
- On Coffin a desktop right-click (MagicMenu) hid the dock for the whole menu. Painting the menu over the dock instead (screen diff, as for Workbench drag images) blocked the dock: LockBitMap on the 24-bit screen waits until MagicMenu closes, and the status polls' LockIBase did too. MagicMenu's menus are not layers on the screen (the LayerInfo lock stayed free).
- Rule: read nothing from the screen and take no Intuition lock while a native menu is held. Decide once per menu from the pointer whether it can reach the dock (256 px), hide PiP only then, and let the loop wait for the menu.
- Testing menus with the agent: a held right button can leave MagicMenu in sticky mode with the pointer over items like Power > Reboot now. Move away and close with Esc; Ctrl-C to the dock is answered only after the menu closes (cleanup needs Intuition).

## SAGA PiP survives a reboot (2026-11-10)
- A "black box at ApolloOS boot" (twice) and, photographed on AmigaOS 3.2.3, Coffin's last dock frame at boot: PiP stays on through a reboot, showing the previous system's buffer (black once that memory is cleared). My first guess (window backfill) was reverted unmerged.
- Rule: ask what exactly is seen (shape, content, timing) before guessing at a boot-time artefact; a photo of the stale frame identified it at once.
- The dock turns both units off at start (unless leased), on Ctrl-Amiga-Amiga (keyboard reset handler), and --pip-off exists for S:User-Startup.

## Release notes list only what has run on hardware (2026-11-10)
"Works with Directory Opus 5 as the desktop" went into the 1.0b2 notes from a
commit title, while tasks/todo.md said "Not run: DOpus 5 listers; needs AmiKit".
- Before writing a release-notes line, find its verification in tasks/todo.md
  (or the commit body). "Built, not yet run on hardware" stays out.
- Commit titles describe intent; the verification notes describe what is true.

## AmiKit: Directory Opus is the desktop (2026-11-10)
- WRONG until 2026-11-25: "agent input does not reach the dock on AmiKit".
  It does; the agent's warp lands elsewhere. See "AmiKit: agent clicks land
  at twice the coordinates" below.
- `a || b` on bit flags collapses them to 1, which is RUNNING_WORKBENCH: a
  window's dot became Workbench evidence that is kept while the Workbench list
  cannot be read (no Workbench). Combine flags with `|`.
- The drop receiver over the DOpus desktop clipped DOpus's selection box. DOpus
  sets the same window flags for an icon drag and a selection box; tell them
  apart by the drag image around the pointer.
- Measure before the next guess: a per-frame count of painted drag pixels
  (all 1561 or 0, never partial) showed whole reads between Directory Opus's
  erase and redraw, where the "keep half-torn reads" guess had made it worse.
- Read the other program's source before theorising about it (2026-11-19).
  Directory Opus 5 (opus_magellan) takes LockLayers at the first drag move and
  holds it until release, then asks WhichLayer at the release point; a foreign
  window gets the drop only when it is frontmost there and in DOpus's own
  AppWindow list (it patches AddAppWindowA). So the receiver must already lie
  over the dock before the press: arming on the drag image or on the first
  move lands after the release (messages=0), and moving a window during the
  drag ends it (the icon stuck or vanished). Three wrong theories (late arm by
  press point, timing gap, "DOpus keeps its drag when a window moves") each
  cost the user a test before I read the code.
- Every window move over the DOpus desktop makes DOpus redraw under it; a
  copy taken before that redraw shows a stale rectangle through PiP. Keep the
  receiver in place across clicks and drops, and wait before re-arming.
- Shrinking a region changes every consumer of it: the catch box lost the
  label strip, and the drag-image baseline (read from the receiver's copy
  as if it covered the whole canvas) failed silently, so the dragged icon
  vanished under the dock (frames=0). The first fix read the strip from the
  last capture, which can be stale, and painted fragments. Grep every user
  of a buffer before resizing it, and give a baseline only where it is exact.
- A window that rests armed must cover only what the dock itself covers
  (2026-11-19): resting on the whole catch box (651 rows, zoom headroom) it
  took clicks on desktop icons above and beside the dock and froze DOpus's
  redraws there (half an icon left behind, video IMG_2580). Ask what else a
  permanent window hides before making one permanent.
- Cover every path to the same window, not only the one in the report: the
  plate rule was tied to the desktop arm, so a DOpus lister icon still armed
  the whole box, and leaving it made DOpus erase and redraw every desktop
  icon there (IMG_2581, 18.5 s). Step through the video frame by frame
  (ffmpeg fps=15 tiles) to see what moves, not the 1 fps overview.
- Test with windows lying under the dock (IMG_2582): a receiver behind the
  other windows loses the drop to a lister under the dock, and the lister
  redrawn by the press reads as drag image. Where the dock covers, the
  receiver must lie in front of other programs' windows, behind the dock's
  own. Ask the user to include overlapping windows in each drag test.
- A screenshot shows the real screen without PiP: use it to tell a stray
  pixel on the screen from one in the dock's picture (break the dock, look
  again). ChangeWindowBox from 1x1 carries the window's old pixel to its new
  corner; the backfill hook only fills what the move uncovered (2026-11-19).
- A catch area must cover the plate as it grows while making room, not only
  at rest: the drag image is drawn only inside it.
- Clip a drawn copy by what hides the original, not by a nearby rectangle:
  the drag image belongs wherever the dock is opaque (the native one is
  hidden there) and nowhere on clear parts outside the receiver (the native
  one shows). Clipped to the receiver, the plate's edge row crossed the image
  as a line (IMG_2586).
- A drop can carry no lock: Directory Opus sends an AppIcon (GlobalTrash,
  "Stop Apollo Explorer") as its name alone. NameFromLock on a zero lock
  crashed AmiKit twice before I looked; check every WBArg field from another
  program before using it, and read its source for what it sends.
- A Workbench project can be its icon alone (AmigaOS 3.2 Shell: Shell.info,
  tool SYS:System/CLI, no Shell file). A launcher that replaces Workbench
  must pass drawer + name without locking the file. Read the requester text
  with an enlarged screenshot crop when VisualPrefs patterns garble it.
- A diagnostic that needs a semaphore another program holds records nothing:
  AttemptSemaphore failed while DOpus held LayerInfo. Record unconditionally,
  resolve later.

- A dock started from WBStartup has no CLI number for Break. Stop it with a
  small tool that does FindTask("DockAwesome") and Signal(SIGBREAKF_CTRL_C)
  under Forbid; restart it with WBRun SYS:WBStartup/DockAwesome and check
  for its 800x96 window (2026-11-17).
- Matched --bench-sweep runs before believing a review's cycle estimate:
  "45-50 cycles per translucent pixel" for blend_pixel became 5.5 % of
  compose on hardware (2026-11-17).
- Test a review's hardware claim with the probe before building on it:
  "misaligned AMMX stores cost 1.7x" measured as no difference on core
  12856 (decode and half blend, destination 4 mod 8 against 0 mod 8)
  (2026-11-17).
- Wait a few seconds after starting a test dock before the first click:
  a click during its icon loading is not a press on a pin (2026-11-17).
- Hover labels on ApolloOS are in PiP1: no screenshot shows them. Find an
  icon by the row arithmetic (size, gap 5, divider 12) instead (2026-11-17).
- Temporary timing code must not use TimerBase outside the dock's own
  profile window: profile_close clears it before the surface closes, and a
  capture during cleanup then jumped through NULL (Software Failure, PiP2
  lease kept, reboot). Guard diagnostics, and run a start-and-quit of any
  diagnostic build before a measured run (2026-11-17).
- After BreakTask on the installed dock, wait a few seconds before starting
  a test dock: the old one still holds the PiP2 lease while it quits.
- Time each OS call before blaming our own loops: the "3 ms compare" on a
  24-bit screen was LockBitMap (2.6 ms a call under P96 on Coffin).
- A fix that depends on a time window (first band fix: "exposures within
  half a second of arming are the initial fill") failed on the first test.
  Compare state instead (which layers lie on the box).
- One run of a profile average is noise here (unchanged polls 1.8-4.5 ms
  across runs with motion). Measure old/new/old in one script with the
  scenario reduced to the one thing compared (2026-11-17).
- A screenshot taken after the test dock has quit proves nothing about a
  screen artefact the dock causes: quitting clears it. Capture first,
  then Break (2026-11-17).
- The independent review of a day's fixes found seven defects in them,
  two in code that had passed on hardware. Run it before installing, not
  after (2026-11-17).
- "It stutters" with on-time frames: look at motion, not timing. A frame
  trace showed every zoom frame inside its field and --damage-check found
  no wrong pixel; the hitch was a velocity change where the zoom envelope
  handed over to pointer following (2026-11-18). Simulate a moving pointer
  on the host before tuning curves the user cannot tell apart.
- Several amiga_move_mouse calls in one message run back to back: each
  entry was cut off by the next exit after 40 ms. One move per message,
  with the test dock started and settled first (Wait 6).
- A copy over SYS:WBStartup/DockAwesome can fail "object is in use" while
  no dock runs; copy to .new and Rename instead. The 64-entry TaskList hid
  the dock (now 256 entries).
- A film of the screen is the measurement for "it stutters": count camera
  frames that do not change during motion (30 fps over 50 Hz). Then
  reproduce with a smooth input.device sweep, not with pointer warps,
  and time each section of the loop pass (2026-11-18).
- Work moved to a lower-priority task still stalls the main task when the
  main task takes a lock that work holds (LockBitMap, LockIBase, layers):
  skip or defer those calls while the task works.
- Files started with WBRun stay "in use" after the program quits on
  ApolloOS; replace them by Rename, delete old ones after a reboot.
- A pointer warp can land in the 5 px gap between icons: "no label" then
  means "no hover", not a bug. Aim at an icon's centre from a pip-dump,
  and log the gating state while the pointer is in, not at exit (the
  exit path has already hidden PiP).
- A later change can make an earlier one dead weight: after moving the
  label into the canvas, the label task only served the no-AA fallback.
  Review the whole day's diff, not each step, before installing.
- "Draw unless this field already has a present" lets a redraw start
  late in a field and run into the next one; drawing at the field's start
  measured better (2 vs 4 stalls per sweep).
- No rollback copies or test variants left behind (user, 2026-11-18:
  "Sluta spara allt gamalt skit"): install by replacing the build in
  place, delete variants once chosen and RAM: test files after each batch.
- One path for every system (user, 2026-11-18: "Jag gillar inte att du har
  massa special för olika versioner"): before adding a per-system branch,
  check what the feature really needs (the AA label font needed no
  library at all) and make it work everywhere instead.
- On AmigaOS 3.9 (Coffin) a Run'd command's task is not named after the
  command: stop it with Break <process number> from Status.
- LhA 2.15 keeps a folder in the archive only with a base directory
  argument: `LhA -r -x a RAM:X.lha RAM:pkg/ DockAwesome/#?` stores
  DockAwesome/...; `CD RAM:pkg` + `DockAwesome/` drops the folder.
- Check the generated release body before publishing (a sed error left
  only the install line; fixed with gh release edit).
- An alignment probe at one position does not prove alignment at a screen
  edge: test where the overlay actually lies (a left dock at x 0 was off
  by a few pixels while the centre was exact), and check that the probe's
  arguments take effect (X Y were ignored when W H were given).
- When the user names a spot that flickers ("fönstrets underkant bakom
  dockan"), measure exactly that spot frame by frame before theorising;
  then check what the OS itself changes there (a window border is drawn
  darker when active) and when the dock's copy of it is taken.
- A drop target that waits behind other windows hands drops to them: a
  disk dropped on the plate over a drawer started copying into it (3.2.3).
  Weigh such a trade-off with the user up front; then cover its side
  effect where it shows (the wheel scrolling a window hidden under the
  receiver).
- Check what the machine is (Version, volumes) before naming the system in
  reports, and take the user's word for which boot it is.
- A shape that bounds the plate must move the way the row does. A value set
  at once (the gap's reach) jumped where only animated icons bounded the
  plate before; animate it with the row's own animator and test the
  plate's end per tick on both ends and every edge, so symmetry is proven
  on the host instead of by the user.
- Hit tests (near/outside) must use the geometry on screen, not a copy
  kept from the press: the frozen zoomed row made "near" reach ~77 px past
  the plate the user saw.

## A receiver fix for one OS family reaches the other (2026-11-20)
- The no-shapes receiver was narrowed to the plate (Coffin, AmiKit, 3.2.3) because covering the box made Workbench redraw windows in the headroom. The shaped (ApolloOS) receiver kept covering the whole box, and the same redraw showed there: a drawer's icons above the plate blinked out at every desktop drop (IMG_2599).
- Rule: when a cause is found on one caps branch (shaped / no shapes), check whether the other branch does the same thing, and fix both in one path.

## Giving up a cover makes simple windows redraw (2026-11-20)
- The shaped receiver took the plate at each press and gave it up at each release. Layers always damages what a shrinking layer uncovers on a simple-refresh window, so Workbench redrew there after every drop, even one far from the dock (IMG_2600). Narrowing the cover only moved the blink.
- Rule: a receiver that must be in place before a gesture should rest there and step back only when what it covers changes. Do not toggle a cover per gesture.

## Reproduce with real input before guessing (2026-11-20)
- IMG_2601 took several wrong fixes, each argued from code. What found it: a tool that injects input events (pointer, press, relative moves) so PowerWindowsNG drags a window like a user, records the screen per step, and stops the dock with Ctrl-C (--pip-dump) at chosen moments; the user only confirmed which runs glitched. The screen capture then showed the noise directly, and one counter in the backfill hook named the cause.
- Rule: when the user says a fix changed nothing, stop theorising; build a repro that drives the real input path, capture frames, and instrument the suspect before the next change.

## Layer hooks get offscreen bitmaps (2026-11-20)
- A backfill hook that paints only when the RastPort's bitmap is the screen's leaves the parts Layers fills offscreen (a layer growing or moving behind another, AmigaOS 3.x) as uninitialised memory, shown once the layer comes in front.
- Rule: a backfill hook must paint whatever bitmap it is given; place the source by the message's layer-relative offset there, not by the bounds.

## Read the documented default before adding a tag (2026-11-21)
- I added NP_Input/NP_Output NIL: to CreateNewProcTags as the "fix" for
  8700000E; dostags.h already says both default to Open("NIL:"). The test
  build changed nothing and the reporter saw the same alert.
- Rule: before changing a call's arguments, read the header's default for
  each argument. And a "no alert" on the dev machine is not evidence without
  a baseline where the alert shows.

## A failed agent gesture: aim before blaming the build (2026-11-24)
- On 3.2.3 a drag-out at y 700 did nothing; the same drag on release
  1.0b4 failed the same way, and at y 680 it worked. The dock's icons sit
  higher at size 36 than at Coffin's 32, and a press off the icons is not
  counted (downs=0, no ambiguous/overflow).
- Rule: when an agent gesture fails, check downs/ambiguous in the profile
  and rerun it on the previous build before treating it as a regression.
  Also: the dock writes only ENVARC:DockAwesome.json; ENV: is the boot copy.

## Bound every Amiga listing (2026-11-25)
- `List Archives: ALL PAT=...` over a large volume ran past the agent's
  timeout; the first Break went to two processes and could have stopped the
  test dock. List one drawer level with a pattern instead, and check Status
  after any Break the agent sends.
- A Codex review run in the repository follows AGENTS.md and writes its
  own section into tasks/todo.md: check `git status` after every Codex run.

## A quick click is not a long hold (Coffin, 2026-11-25)
- With a Folder Stack open, a quick click on another stack only closed the
  box, while my press-wait-release test switched stacks. The release lost
  its pin: the box's closing motion, started by the press, withdrew the
  input handler's hit snapshot; a 1 s hold released after the motion.
  The same loss kept a program pin from starting, also in 1.0 beta 5.
- Rule: test clicks at normal speed (amiga_click) as well as held presses,
  and when a release does nothing, log what the release carried (pin,
  hit, flags) before theorising.
- A pin's hit test follows its artwork's alpha: Coffin's Prefs drawer icon
  is a flat bar, so a click at the row's middle height missed it
  (downs=0). Aim at the artwork from a --pip-dump.

## First-open runs need the pointer off the dock (3.2.3, 2026-11-25)
- A first-open run started right after a click on the dock kept the pointer
  over it: the read-ahead waits while the pointer is on the dock, so the
  profile said reads=0 pending=3 after 45 s idle.
- Rule: move the pointer off the dock before every idle wait, and accept a
  first-open run only when its prefetch line reads pending=0.
- The trace's convert_us includes the PiP back-buffer retirement wait
  (WaitTOF in amiga_pip_back_buffer): a slow "convert" with few pixels is
  a wait for a field, not CPU. Count the waits before blaming other tasks.

## Compare the same row before blaming a feature (3.2.3, 2026-11-25)
- The 3.2.3 hover failure was put on the stacks beside the hovered pin.
  v1.0b5 had looked normal only because its divider put other icons under
  the same pointer x; with the same 15 evenly spaced pins (no "folder")
  v1.0b5 failed the same way. The cause was an old pacing cascade (a late
  present makes every later frame wait in WaitTOF), triggered by heavier
  icons at that rest.
- Rule: for a hover/zoom regression, first run the old build with the same
  pixel layout (same pin count and spacing, canvas width in the profile
  header) and the same gestures; a control that swaps icons also swaps the
  compose cost. Read the frame trace for the first late frame (done_us near
  20 ms) before looking for other tasks or features.


## Quote rules are per receiver: DOpus 5 keeps quotes in `lister new` (2026-11-25)
- `lister new "<path>"` was written from Shell/ARexx habit and shipped from
  1.0 beta 2 to 542 unrun on DOpus 5; DOpus 5 takes the rest of the line as
  the path, quotes included, and showed "device (or volume) is not mounted".
- Before sending a command string to another program, run the exact string
  against that program once (ARexx from amiga_arexx is enough) with a plain
  path, a path with a space and one with a quote.
- A "not mounted" error for a mounted path: look at the exact path the
  error names (here `"Personal:Audio"/`) before suspecting DOS or devices.

## AmiKit: agent clicks land at twice the coordinates (2026-11-25)
- amiga_move_mouse/amiga_click/amiga_drag warp with IECLASS_POINTERPOS. On
  AmiKit XE (1280x720, 24-bit) the pointer lands at screen = 2*w-4 on both
  axes: a click "at 476,690" pressed at 948,719, beside the dock, so the
  profile said `Input: downs=0`. One such click was taken as "input does not
  reach the dock" and every AmiKit gesture went to the user for weeks.
- The button events themselves are fine: an input.device spy (handlers at
  pri 127 and 59 around the dock's 60) showed RAWMOUSE LBUTTON, qualifier
  0x8000, ie_X/Y 0, and the dock nulled both (captured). No other handler
  sits above the dock (commodities 53, intuition 50, console 0).
- Works on AmiKit: pass w = ((X+4)/2, (Y+4)/2) to amiga_click, amiga_drag and
  amiga_move_mouse; check with amiga_pointer first. relative:true is not
  exact (pointer acceleration: 32 ticks gave ~35 px). amiga_ui_click uses the
  unscaled warp and misses. Screenshots and amiga_ui_tree report real pixels.
- The dock and the Folder Stack box are drawn in PiP (not in screenshots);
  the box's window position and size are in amiga_ui_tree, and the stack and
  dock menus are real windows that screenshots show.
- Rule: when an agent gesture "does nothing", read amiga_pointer after the
  move before concluding anything about the target program.

## Stop a CLI test dock by its CLI, not by task name (ApolloOS, 2026-11-25)
- BreakTask/FindTask "DockAwesome" find the dock only when Workbench
  started it (WBStartup, WBRun). A dock run from an Execute script is a
  CLI process named after its CLI, so BreakTask said "not found", the
  first test dock kept running and the next start showed "PiP2 is already
  in use".
- Rule: stop CLI-started docks with `Status COM=<binary>` + `Break <n>`
  (stop.s) and check that Status lists none before the next start; use
  BreakTask only for the Workbench-started dock.

## A relative amiga_drag is not a hold; first-open runs need quiet (Coffin, 2026-11-25)
- amiga_drag with relative:true and from == to homes the pointer with the
  button down, so the dock saw a drag of the pin, not a 1 s hold: the box
  closed and nothing opened. Send holds as an absolute amiga_drag (settle
  50) once a relative click has put the pointer on the dock.
- At ChangeTaskPri -30 every task an agent command starts runs above the
  dock. The only slow last entrance frames (one 60 ms, two 3 ms over)
  came in runs that sent "Wait 1" right after the click, though one such
  run was clean; the cause is not proven. Time first-open clicks alone
  (the next agent call seconds later) and repeat a run before calling it
  a regression.

## libnix aborts on a pending Ctrl-C inside any write (3.1.4.1, 2026-11-25)
- A test dock got its Break within a second of starting. libnix's write()
  called __chkabort, which printed "***Break" and exited past main's
  cleanup: the input handler, the PiP lease and the receiver window
  stayed behind and Intuition took no more input. The output file held
  only "***Break"; that line is the tell.
- Rule: an Amiga program that owns system resources defines an empty
  `__chkabort` and handles Ctrl-C itself. Test "Break at startup" by
  running the program in-process with Ctrl-C already pending (RunCommand
  after SetSignal), not by racing a Break against the start.
- My procedure caused the "PiP2 is already in use." the user saw: I sent
  stop.s within a second of starting a test dock, it died with the lease
  held, and my next test dock hit the lease. Give a just-started test
  dock a few seconds before stop.s; before every start check Status (CLI
  docks), FindTask (the Workbench dock) and the PiP2 lease
  (FindSemaphore): exactly one owner or none.
- A requester that frozen input cannot reach can still be answered: send
  its window the IDCMP_GADGETUP its OK gadget would send (PutMsg to the
  window's UserPort with your own reply port).

## The end icon moves when the row zooms (3.2.3, 2026-11-25)
- Five agent clicks at the last stack's rest centre did nothing
  (`downs=0`, `presses_unarmed=1`; an input spy showed the press passing
  through): the zoomed row shifts the end icon about 8 px outward, so the
  rest centre lies in the gap. At rest the centres are 640 + 41*(k-(n-1)/2)
  on a 1280 px screen at size 36.
- Rule: aim at an end icon from a --pip-dump taken with the pointer on it,
  or a few px outward; a right-click there that opens the plate menu
  (Settings/About/Quit) says "gap".

## A per-binary timing gap is not a code regression until a rebuild agrees (3.2.3, 2026-11-25)
- 553 composed --bench-sweep frames 0.9 ms (20%) slower than v1.0b5 on the
  same damage, frame for frame, while the compose code was unchanged; a
  30-line diagnostic in another file moved the same code by -1.5 to +0.9 ms,
  and -falign-* flags did not pin it. The 43 ms entrance frame followed the
  memory state (gone with a 32 KB shift and later in the session).
- Rule: before calling a performance difference between two builds a
  regression, rebuild each with an unrelated change (and run one with a
  memory shift, tool Hold) and see whether the gap survives; diff the
  functions that run in the measured window.

## A warp click from a zoomed row misses far pins (AmiKit, 2026-11-25)
- With a box open and the row zoomed around a stack, amiga_click warped
  straight to Shell or Clock: the press arrived before the row zoomed
  around the new pointer position, where the drawn row had no icon
  (`downs` not counted, `presses_unarmed` +1). It looked like the old
  "program pin does not start from an open box" defect.
- Rule: before a click on a pin away from where the pointer rests on the
  dock, move the pointer onto it in its own call (as a mouse does), or
  click from off the dock (row at rest). Check `Input: downs` before
  calling a missed click a dock defect.

## See the PiP-only Folder Stack box with --pip-dump (AmiKit, 2026-11-25)
- Screenshots do not show the box. `--pip-dump prefix` now writes
  `prefix-folder.ppm` once a box page has settled with its icons; build
  the "before" binary with the dump alone to compare a visual change.
- At size 72 a test dock took over 10 s before its first press counted
  (`Input: downs=0`, profile `elapsed_ms` 4 s of a 14 s run). Wait 20 s
  after starting a large-size test dock, and check `downs` before calling
  a missed click a defect.
- Repeated: several amiga_move_mouse calls in one message are not a
  sweep; the run was discarded and redone one move per call.

## AmigaDOS script loops ran away (3.2.3, 2026-11-26)
- A test-file loop `Set i 100 / Lab / Echo >f$i / Set i \`Eval $i + 1\` /
  If $i LT 400 / Skip BACK` never stopped: If compared the numbers as
  strings ("1000" < "400"), and `If VAL` did not stop the next version
  either. It wrote about 17000 files to RAM:, amiga_break did not reach
  it, and two unbounded `List` calls of the result flooded the session.
- What stopped it: `Status` to find the script's CLI (a "No command
  loaded" process with the agent's 262144 stack), then `Break <n> D`
  (Ctrl-D ends a script; Ctrl-C did not).
- Rule: generate test files with a straight-line script written on the
  Mac (one Echo per file), never an AmigaDOS counting loop; bound every
  listing of a test drawer with a narrow PAT and NOHEAD.

## Centre by the bounding box, never by an optical offset (2026-11-26)
- I set the parent-drawer return arrow one pixel below its disc's centre
  because, box-centred, its ink looked high. The owner: "Alltid exakt!"
  Every control glyph is centred exactly by its bounding box.
- The Open Folder ring had sat half a pixel off its icon cell since it was
  added (its centre was a pixel centre, the icon box's an edge for even
  sizes); a host test on the rasterised box would have caught it.
- Rule: centre glyphs by their box and assert it on the rasterised pixels
  in a host test for every size; also check the reference box (cell or
  control) is the one the user sees, not a pixel coordinate near it.
- Rule: before changing a colour for a group of controls, measure every
  element's colour in a dump: the box mixed #f2f4f8 and #aeb8c7 for
  controls without anyone deciding it.

## Diagnostics must not block in DOS calls (3.1.4.1, 2026-11-26)
- A tool that called NameFromLock on every process's pr_CurrentDir and
  pr_HomeDir (to find who held an empty RAM: drawer) blocked inside DOS
  twice; Break did not reach it, amiga_abandon left it running and its
  output file locked until reboot.
- Rule: never send packets on other processes' locks from a probe; and
  before suspecting a leak, reproduce it with the dock against a control
  drawer (five runs here deleted cleanly, so no dock defect was shown).
- Fetch dumps with an assign (`Assign DL: RAM:dl`, acp `host:DL:`): only
  that drawer comes down; `host:RAM:` also tried to read a file the test
  dock still had open and hung until the watchdog. Copy a box dump only
  about 2 s after a page click: it is written once the page has settled.
