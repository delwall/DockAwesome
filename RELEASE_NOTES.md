# Release notes

## 1.0 beta 6

Changes since 1.0 beta 5. Checked on ApolloOS, Coffin, AmigaOS 3.2.3,
AmigaOS 3.9, AmigaOS 3.1.4.1 and AmiKit XE.

**Folder Stacks**
- Every drawer you pin is a Folder Stack: a click opens its contents above
  the dock. Disks stay ordinary pins that open a window.
- Stacks move and are removed like any other pin. A click on another stack
  closes the open one and opens the one you clicked.
- Right-click a stack to choose what a click on it does: **Show Stack**
  (its contents above the dock, the default) or **Open Folder** (its
  drawer as a window, as a pinned drawer did in 1.0 beta 5; a Directory
  Opus lister on AmiKit). The choice is saved per stack and kept when the
  stack is moved (checked on ApolloOS, Coffin, AmigaOS 3.2.3, AmigaOS 3.9,
  AmigaOS 3.1.4.1 and AmiKit XE).
- Each stack can be sorted by **Name**, **Date Modified** (newest first) or
  **Kind** (drawers first, then by file type) from its right-click menu;
  the choice is saved per stack.
- Stacks show no running dot.
- The tile at the end of a stack's box is now **Open Folder** on one
  line, with a smaller ring in the muted grey of the menu headings, so it
  no longer outweighs the real icons; at small icon sizes the box is one
  text line shorter (checked on AmiKit XE at sizes 36, 48 and 72; the tile
  opens a Directory Opus lister there, and a Workbench window on AmigaOS
  3.2.3, AmigaOS 3.1.4.1, ApolloOS, Coffin and AmigaOS 3.9).
- The page buttons in a stack's box draw their **<** and **>** exactly in
  the middle of a slightly smaller disc (before, half a pixel toward the
  open side); the button that goes back to the parent drawer shows a
  return arrow (up, then left), so it no longer looks like the page-back
  button; the box's **Open Folder** tile sits exactly on its icon cell
  (before, half a pixel off). All of the box's controls (buttons, tile,
  title, page count) now share one slightly softer tone; file names keep
  theirs. The click areas are unchanged (checked on AmigaOS 3.2.3,
  AmigaOS 3.1.4.1, AmiKit XE, ApolloOS, Coffin and AmigaOS 3.9 at icon
  sizes 36, 48 and 72).
- The Folder Stack setting is gone from **Settings**. A Folder Stack set in
  1.0 beta 5 becomes the last pin on the first start.
- The divider before the last icon is gone: all icons are evenly spaced.
- While the dock idles, the first page of every stack is read ahead, so the
  first opening is as quick as with the single stack of 1.0 beta 5.

**Fixes**
- With a Folder Stack open, a quick click on another pin closes the stack
  and starts that pin (or opens that stack). Before, only the stack closed
  (seen on Coffin, also in 1.0 beta 5; checked on Coffin, AmigaOS 3.2.3,
  AmigaOS 3.9, AmigaOS 3.1.4.1, ApolloOS and AmiKit XE).
- Zoom: when one frame ran late, every later frame of the same motion
  waited up to 30 ms for the display, and the zoom stuttered until the row
  came to rest (also in 1.0 beta 5). The dock now draws such a frame at the
  next field and keeps pace (checked on AmigaOS 3.2.3 and Coffin).
- AmiKit (Directory Opus 5 as the desktop): disk pins open a Directory
  Opus lister again, and so do Open Folder stacks and the box's Open
  Folder tile. Before, Directory Opus showed an
  empty lister titled "DOS error: device (or volume) is not mounted" (also
  in 1.0 beta 5; checked on AmiKit XE).
- A Ctrl-C (Break) that reaches the dock while it starts no longer makes
  it quit half closed. Before, it could quit without removing its input
  hook and display overlay: the mouse stopped moving and the next start
  said "PiP2 is already in use" until a reboot (seen on AmigaOS 3.1.4.1;
  1.0 beta 5 quits the same way). Checked on AmigaOS 3.1.4.1, AmigaOS
  3.2.3 and AmiKit XE: a Break during a start and during --bench-sweep
  leaves nothing behind.

## 1.0 beta 5

Changes since 1.0 beta 4.

**Fixes**
- A program started from the dock no longer brings up a Recoverable Alert
  8700000E when it quits (AmigaOS 3.2.3, AmiKit XE).
- AmiKit with Directory Opus Magellan as the desktop: the pointer moves
  smoothly again while the dock runs.

Thanks to an AmiKit XE user for tracking these down and sending the
patch.

## 1.0 beta 4

Changes since 1.0 beta 3.

**Dropping icons on the dock**
- An icon dropped on the dock where a drawer lies under it goes to the
  dock, not into the drawer (AmigaOS 3.2.3 started copying a dropped disk
  into the drawer).
- Dropping an icon on the desktop no longer makes icons beside the dock
  and in its glass blink (ApolloOS).
- Clicking the dock between icons leaves your window active.
- AmiKit: icons dropped from the Directory Opus desktop are caught without
  disturbing the desktop, and an AppIcon such as GlobalTrash dropped on
  the dock no longer crashes the machine.
- No stray window border is painted over the dock's icons while you drag
  a desktop icon (AmigaOS 3.2.3), and no border under the dock flips
  colour at the press (ApolloOS).

**Windows under the dock**
- ApolloOS: a window dragged or scrolled under the dock shows through the
  glass as it moves.
- A drawer scrolled with the wheel under the dock shows its new icons
  there at once (AmigaOS 3.2.3).
- No noise in the glass over a window under the dock after it was moved
  (AmigaOS 3.9).

**Rearranging pins**
- The gap follows the pointer also off the row, and opens and closes with
  the same slide as the icons; the end of the plate moves with it instead
  of jumping.
- The gap opens where the dock is drawn, not up to about 77 px past it
  with large magnification.
- The Remove label under a pin dragged off the dock is the same glass
  label as the name labels, on every system.

**Fixes**
- ApolloOS: a dock on the left edge lines up with the screen; windows
  under it looked shifted a few pixels.

## 1.0 beta 3

Changes since 1.0 beta 2.

**More systems**
- Runs on AmiKit XE, AmigaOS 3.9 and AmigaOS 3.1.4.1 on the Vampire.
- Works with Directory Opus Magellan II as the desktop (AmiKit): icons
  from the desktop and from listers can be dropped on the dock.

**Look and feel**
- Name labels appear at once when you point at an icon and follow it as it
  zooms, like the macOS Dock. They are drawn in the dock itself, on every
  system and on every edge.
- Moving along the dock no longer stutters: each label used to cost the
  dock a frame or two.
- The row keeps its pace when the zoom ends and follows the pointer more
  closely.
- The dock keeps zooming while a Folder Stack is open, and the stack's tip
  follows its pin.
- The Folder Stack opens more smoothly, with a frame every field and no
  hitch just before it is fully open.

**PiP only**
- The dock always uses SAGA PiP. When PiP cannot run (screen depth, core,
  PiP already in use), it says why and quits, instead of switching quietly
  to a static background that hid parts of windows behind the dock.
  `--snapshot` and `--opaque` are gone; `--pip` is still accepted.
- On a CPU other than the 68080 the dock says so instead of crashing with
  an illegal instruction.

**Fixes**
- On 24-bit screens the dock spends about half as much time watching the
  background while it rests.
- No old strip left over a window that closes next to the dock (seen on
  AmigaOS 3.9).
- A running program's dot goes away when it quits, also without Workbench.
- A dragged icon no longer blinks over the dock.
- A selection box drawn across the dock stays visible.

## 1.0 beta 2

Changes since 1.0 beta 1.

**More systems**
- Runs on AmigaOS 3.2.3 and AmigaOS 3.9 on the Vampire, with live PiP
  transparency, as well as on ApolloOS.
- When another desktop replaces Workbench, the dock starts pins itself the
  way Workbench does: Start from Shell and ARexx icons, `$STACK` and
  `TOOLPRI`.

**Look and feel**
- A zoom wave like the macOS Dock, paced by the display. Size goes up to
  72 px, Scale up to 2.50× and the zoom up to 128 px.
- Round, antialiased name labels.
- Folder Stack grows out of its pin and fades in, with round glass buttons
  and default icons for files without their own.
- Folder Stack icons load faster: the first opening went from 0.9 s to
  0.13 s, and later openings take about 0.01 s.

**Fixes**
- Dragging a pin out of the dock is smooth on every system, without
  tearing or a box around the pin.
- Right-clicking a pin with nothing to offer shows no menu.
- The dock steps aside only for a menu that could reach it.
- No old dock picture after a reboot or a switch to another OS.
- Less drawing work: only the changed parts of the dock are redrawn and
  uploaded.

**New**
- About links to the releases page. A click opens it in the browser when
  OpenURL is installed.
- `DockAwesome --caps` lists what the dock found on your machine. Add it to
  bug reports.
