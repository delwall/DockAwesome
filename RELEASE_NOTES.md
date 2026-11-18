# Release notes

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
