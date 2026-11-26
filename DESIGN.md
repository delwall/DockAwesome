# DockAwesome visual design

Use the recorded [macOS Dock](docs/macos-reference.md) as the motion reference.
Keep the original Workbench artwork, aspect ratio, and alpha.

## Layout and motion

- Float the plate 3 px from any screen edge. Use four rounded corners, a thin
  rim, and 8 px of end padding.
- Use 32 px resting icon boxes and up to 64 px at the default zoom. Center the
  resting boxes on the plate. Zoom toward the desktop from a fixed baseline.
- Keep the plate thickness fixed during zoom. Follow the icon row at both ends.
- Use a 5 px resting gap. Narrow it by up to 2 px as neighbors grow.
- Track the pointer across icon centers and gaps. Round each box separately so
  distant icons stay steady.
- Use a 160 ms entry and exit at 50 Hz. Moving the pointer should not restart
  the transition. Stop the animation timer when the layout settles.

## Interaction

- Darken a pressed icon. Open it on a valid release.
- Show an insertion gap for reordering and Workbench drops.
- Show **Remove** beyond the drag boundary. Dropping there removes only the pin.
- Put the hover label on the desktop side of the icon.
- Use a compact dark context menu. Match the dock's glass and thin rim.
  Its glass shows whatever lies behind it, the dock's own icons, running dots
  and plate included.
- A Folder Stack's menu follows the macOS Dock's: a muted **Sort by**
  heading, **Name**, **Date Modified** and **Kind** with a check mark on the
  active one, a hairline separator, then a muted **Click to** heading,
  **Show Stack** and **Open Folder** with a check mark on what a left click
  does. All its labels start past the check column.
- A Folder Stack box ends each page with an **Open Folder** tile below
  the real icons: a ring about 72% of the icon size at the icon's centre,
  ring and one-line label in the muted heading tone.
- A box's page buttons show **<** and **>** in a faint disc 5/6 of the
  button (its click area stays the full button); inside a drawer, the
  header's button to the parent drawer shows a return arrow in the same
  disc (a stem up from the bottom right turning left into an arrowhead,
  one-pixel lines).
- Centre every control glyph exactly by its bounding box: the chevrons and
  the return arrow on their disc, the Open Folder ring and arrow on the
  tile's icon box. Never shift a glyph for optical balance.
- One tone for every control in a box (FOLDER_GLYPH #dee2e9, the menu
  text a little faded): the buttons' glyphs, the Open Folder ring, arrow
  and label, the title, the page count and the empty/error notice. File
  names under the icons keep the menu text colour; the buttons' disc keeps
  its faint tint of the menu text.
- Keep **Size**, **Scale** and position in a compact native
  window. **Save** applies the draft. **Cancel** discards it.
- Keep side icons upright. Use the same zoom and input rules on all edges.
