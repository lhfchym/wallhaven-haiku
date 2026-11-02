#include "DesktopIntegration.h"

#include <Bitmap.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <InterfaceDefs.h>
#include <Message.h>
#include <Messenger.h>
#include <Node.h>
#include <Path.h>
#include <TranslationUtils.h>
#include <Window.h>
#include <be_apps/Tracker/Background.h>
#include <fs_attr.h>

#include <cerrno>
#include <cstring>
#include <memory>
#include <vector>

namespace {

bool Fail(std::string& error, const char* context, status_t status)
{
    error = std::string(context) + ": " + strerror(status);
    return false;
}

} // namespace

bool SetDesktopWallpaper(const std::string& path, std::string& error)
{
    error.clear();
    BEntry imageEntry(path.c_str(), true);
    BPath absolutePath;
    status_t status = imageEntry.GetPath(&absolutePath);
    if (status != B_OK || !imageEntry.IsFile())
        return Fail(error, "Wallpaper file is unavailable", status == B_OK ? B_ENTRY_NOT_FOUND : status);

    // Tracker also uses Translation Kit. Reject HTML/error downloads and
    // unsupported image formats before replacing a working desktop setting.
    std::unique_ptr<BBitmap> bitmap(BTranslationUtils::GetBitmap(absolutePath.Path()));
    if (!bitmap || bitmap->InitCheck() != B_OK) {
        error = "Haiku cannot decode this wallpaper image";
        return false;
    }
    bitmap.reset();

    BMessenger tracker("application/x-vnd.Be-TRAK", -1, &status);
    if (status != B_OK || !tracker.IsValid())
        return Fail(error, "Tracker is unavailable", status == B_OK ? B_BAD_PORT_ID : status);

    BPath desktopPath;
    status = find_directory(B_DESKTOP_DIRECTORY, &desktopPath);
    if (status != B_OK)
        return Fail(error, "Cannot locate the desktop", status);
    BNode desktop(desktopPath.Path());
    if (desktop.InitCheck() != B_OK)
        return Fail(error, "Cannot open desktop settings", desktop.InitCheck());

    attr_info previousInfo;
    status = desktop.GetAttrInfo(B_BACKGROUND_INFO, &previousInfo);
    const bool hadPrevious = status == B_OK;
    std::vector<char> previous;
    if (hadPrevious) {
        if (previousInfo.size < 0 || previousInfo.size > 1024 * 1024) {
            error = "Existing desktop background settings are unexpectedly large";
            return false;
        }
        previous.resize(static_cast<size_t>(previousInfo.size));
        ssize_t read = desktop.ReadAttr(B_BACKGROUND_INFO, previousInfo.type, 0,
            previous.data(), previous.size());
        if (read != previousInfo.size)
            return Fail(error, "Cannot preserve desktop settings", read < 0 ? read : B_IO_ERROR);
    } else if (status != B_ENTRY_NOT_FOUND && status != B_FROM_POSIX_ERROR(ENOATTR)) {
        return Fail(error, "Cannot read desktop settings", status);
    }

    // These are the public Tracker background fields used by Backgrounds.
    // A single all-workspaces entry replaces any old per-workspace overrides.
    BMessage background;
    status = background.AddString(B_BACKGROUND_IMAGE, absolutePath.Path());
    if (status == B_OK) status = background.AddInt32(B_BACKGROUND_MODE, B_BACKGROUND_MODE_SCALED);
    if (status == B_OK) status = background.AddPoint(B_BACKGROUND_ORIGIN, BPoint(0, 0));
    if (status == B_OK) status = background.AddBool(B_BACKGROUND_ERASE_TEXT, true);
    if (status == B_OK) status = background.AddInt32(B_BACKGROUND_WORKSPACES, B_ALL_WORKSPACES);
    if (status != B_OK)
        return Fail(error, "Cannot prepare desktop settings", status);
    std::vector<char> flattened(static_cast<size_t>(background.FlattenedSize()));
    status = background.Flatten(flattened.data(), flattened.size());
    if (status != B_OK)
        return Fail(error, "Cannot encode desktop settings", status);

    // Removing first prevents an older, longer attribute leaving trailing data.
    // Keep the exact original bytes/type so a failed write can be rolled back.
    if (hadPrevious) {
        status = desktop.RemoveAttr(B_BACKGROUND_INFO);
        if (status != B_OK)
            return Fail(error, "Cannot replace desktop settings", status);
    }
    ssize_t written = desktop.WriteAttr(B_BACKGROUND_INFO, B_MESSAGE_TYPE, 0,
        flattened.data(), flattened.size());
    status = written == static_cast<ssize_t>(flattened.size())
        ? B_OK : (written < 0 ? written : B_IO_ERROR);
    if (status == B_OK) {
        BMessage refresh(B_RESTORE_BACKGROUND_IMAGE);
        status = tracker.SendMessage(&refresh, static_cast<BHandler*>(NULL), 3000000);
    }
    if (status == B_OK)
        return true;

    Fail(error, "Cannot apply desktop wallpaper", status);
    desktop.RemoveAttr(B_BACKGROUND_INFO);
    if (hadPrevious) {
        ssize_t restored = desktop.WriteAttr(B_BACKGROUND_INFO, previousInfo.type, 0,
            previous.data(), previous.size());
        if (restored != static_cast<ssize_t>(previous.size()))
            error += "; restoring the previous desktop setting also failed";
    }
    BMessage refresh(B_RESTORE_BACKGROUND_IMAGE);
    tracker.SendMessage(&refresh, static_cast<BHandler*>(NULL), 3000000);
    return false;
}
