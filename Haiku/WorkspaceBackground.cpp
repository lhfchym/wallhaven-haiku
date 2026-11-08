#include "WorkspaceBackground.h"

#include <InterfaceDefs.h>
#include <Window.h>
#include <be_apps/Tracker/Background.h>

namespace {
status_t AddBackground(BMessage& message, const char* path, uint32 mask,
    int32 mode, BPoint origin, bool eraseText)
{
    status_t status=message.AddString(B_BACKGROUND_IMAGE,path);
    if(status==B_OK) status=message.AddInt32(B_BACKGROUND_WORKSPACES,mask);
    if(status==B_OK) status=message.AddInt32(B_BACKGROUND_MODE,mode);
    if(status==B_OK) status=message.AddPoint(B_BACKGROUND_ORIGIN,origin);
    if(status==B_OK) status=message.AddBool(B_BACKGROUND_ERASE_TEXT,eraseText);
    return status;
}
}

status_t MergeWorkspaceBackground(const BMessage& previous, const char* path,
    uint32 workspaces, BMessage& result)
{
    if(!path || !path[0] || workspaces==0) return B_BAD_VALUE;
    // Keep unrelated metadata; only rebuild Tracker's parallel record arrays.
    BMessage merged(previous);
    const char* fields[]={B_BACKGROUND_IMAGE,B_BACKGROUND_WORKSPACES,
        B_BACKGROUND_MODE,B_BACKGROUND_ORIGIN,B_BACKGROUND_ERASE_TEXT};
    const type_code types[]={B_STRING_TYPE,B_INT32_TYPE,B_INT32_TYPE,B_POINT_TYPE,B_BOOL_TYPE};
    for(int i=0;i<5;i++) {
        type_code type;
        status_t status=previous.GetInfo(fields[i],&type);
        if(status!=B_NAME_NOT_FOUND && (status!=B_OK || type!=types[i])) return B_BAD_DATA;
        merged.RemoveName(fields[i]);
    }
    for(int32 i=0;;i++) {
        const char* oldPath;
        if(previous.FindString(B_BACKGROUND_IMAGE,i,&oldPath)!=B_OK) break;
        int32 mask=B_ALL_WORKSPACES, mode=B_BACKGROUND_MODE_TILED;
        BPoint origin(0,0);
        bool eraseText=false;
        if(previous.FindInt32(B_BACKGROUND_WORKSPACES,i,&mask)!=B_OK) mask=B_ALL_WORKSPACES;
        if(previous.FindInt32(B_BACKGROUND_MODE,i,&mode)!=B_OK) mode=B_BACKGROUND_MODE_TILED;
        previous.FindPoint(B_BACKGROUND_ORIGIN,i,&origin);
        previous.FindBool(B_BACKGROUND_ERASE_TEXT,i,&eraseText);
        uint32 remaining=static_cast<uint32>(mask)&~workspaces;
        if(remaining==0) continue;
        status_t status=AddBackground(merged,oldPath,remaining,mode,origin,eraseText);
        if(status!=B_OK) return status;
    }
    status_t status=AddBackground(merged,path,workspaces,B_BACKGROUND_MODE_SCALED,BPoint(0,0),true);
    if(status!=B_OK) return status;
    result=merged;
    return B_OK;
}
