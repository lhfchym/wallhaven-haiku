#ifndef WALLHAVEN_WORKSPACE_BACKGROUND_H
#define WALLHAVEN_WORKSPACE_BACKGROUND_H

#include <Message.h>

// Pure merge: preserve settings outside the target mask, with Tracker's defaults
// for optional fields. On failure, result and the source are left untouched.
status_t MergeWorkspaceBackground(const BMessage& previous, const char* path,
    uint32 workspaces, BMessage& result);

#endif
