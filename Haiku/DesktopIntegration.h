#ifndef WALLHAVEN_DESKTOP_INTEGRATION_H
#define WALLHAVEN_DESKTOP_INTEGRATION_H

#include <string>

// The image must remain at this path. Applies Haiku's native scaled placement
// to every workspace and asks Tracker to redraw immediately.
// Call from a process with a BApplication instance, outside the UI looper.
bool SetDesktopWallpaper(const std::string& path, std::string& error);

#endif
