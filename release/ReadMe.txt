DockAwesome - a magnifying dock for the Vampire V4
==================================================

Requirements
  Vampire V4 (68080) with ApolloOS R9.5, AmigaOS 3.2.3, AmigaOS 3.9,
  AmigaOS 3.1.4.1 or AmiKit XE. The dock draws through SAGA PiP and uses
  AMMX, on a 16, 24 or 32-bit Workbench screen. When PiP cannot run, the
  dock says why and quits.

Install
  1. Copy DockAwesome and DockAwesome.info to SYS:WBStartup to start it
     with Workbench, or anywhere else to start it by hand.
  2. If you use WBDock, move WBDock and WBDock.info from SYS:WBStartup to
     SYS:WBStartup/Disabled so only one dock starts.
  3. Reboot, or double-click DockAwesome.

  The icon carries the DONOTWAIT tooltype, so Workbench does not wait for
  the dock during startup. Keep it if you make your own icon.

Use
  Drag icons from Workbench onto the dock to pin them. Drag a pin off the
  dock to remove it. Right-click the dock for Settings, About and Quit.
  A pinned drawer is a Folder Stack: click it to browse its contents above
  the dock. Right-click a stack to sort it (Name, Date Modified, Kind) or
  to make a click open the drawer as a window (Open Folder). Disks stay
  ordinary pins that open a window.
  Programs with a window on screen get a dot under their pin, or a
  temporary icon while they run.

  Settings only offer sizes that fit your screen. Saving new sizes resizes
  every icon, which takes a few seconds; the Settings title shows progress.
  When you change the screen resolution, the dock closes and reopens on the
  new Workbench screen (uses screennotify.library, included in ApolloOS).

  Preferences are saved in ENVARC:DockAwesome.json.

Uninstall
  Quit the dock from its menu, delete DockAwesome and DockAwesome.info,
  and move WBDock back if you used it.

What is new is in RELEASE_NOTES.md. New releases:
  https://github.com/delwall/DockAwesome/releases

Report problems to ZyBeR on Discord. Include what you did, what you saw,
the version from About and the output of DockAwesome --caps.
