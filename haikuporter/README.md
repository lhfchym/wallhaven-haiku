# Recipes

`wallhaven-1.1.0.recipe` pins the exact tested source archive attached to the
1.1.0 release, with its SHA-256. BUILD only builds; TEST runs offline backend,
request-header, workspace and embedded-catalog tests. The opt-in DesktopTests
requires a running desktop and is outside the chroot TEST phase.

`wallhaven-1.0.0.recipe` is retained for the older release. No separate debug-info
package or redundant objcopy prerequisite is needed for this application.
HaikuPorts inclusion remains paused; these recipes are distributed with the
project and no upstream submission is part of this release.
