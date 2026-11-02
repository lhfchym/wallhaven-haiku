# Screenshot provenance

These are unretouched captures of the running application on Haiku R1 beta6
hrev59866+79, x86_64, at a 1280×800 desktop resolution. They show the English UI
of application version 1.0.0, package revision 5, captured on 2026-11-02.
The pending revision 6 adds attribution/documentation; its application UI code
is unchanged. These screenshots do not claim a final public package validation.

| Image | What it shows | Artwork |
| --- | --- | --- |
| screenshots/gallery.png | Offline Downloaded gallery with six saved images | demo01–demo06 |
| screenshots/preview.png | Original image preview | demo01, Amber horizon |
| screenshots/desktop.png | The applied background after closing Wallhaven | demo01, Amber horizon |

The gallery is **a local demo collection, not live Wallhaven search results**.
The synthetic demo IDs do not identify real Wallhaven uploads. No website
wallpaper, API key, private path or user configuration is visible. The screen
clock is incidental VM state, not evidence of the capture date.

The six images were authored as SVG for this project using paths, circles and
gradients, with coding-assistant help. No outside image was used as input.
Their editable SVG sources, generator and MIT license are in [demo-art](demo-art/).
The project-created artwork and screenshot contributions are offered under MIT;
Haiku's UI, logos and icons remain subject to their respective rights. These
captures do not imply endorsement by Haiku or Wallhaven.

Capture procedure: use a temporary English profile with an empty API key and a
separate demo download folder; load the six rendered PNG files via Downloaded;
select demo01; capture the gallery and Preview; click Set wallpaper; quit the app
and capture the desktop. Restore the prior profile and background afterwards.
The screenshots have not been composited, retouched or populated with fake UI.
