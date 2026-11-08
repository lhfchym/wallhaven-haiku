// Opt-in integration test. Temporarily changes the test desktop, then restores
// the exact original attribute even on assertion failure. Do not run in chroot.
#include "DesktopIntegration.h"
#include <Application.h>
#include <DataIO.h>
#include <FindDirectory.h>
#include <Message.h>
#include <Messenger.h>
#include <Node.h>
#include <Path.h>
#include <Window.h>
#include <be_apps/Tracker/Background.h>
#include <fs_attr.h>
#include <cstdio>
#include <cstring>
#include <vector>

#define CHECK(test) do { if(!(test)) { fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#test); return 1; } } while(0)
class RestoreBackground {
public:
    BNode node;
    attr_info info{};
    std::vector<char> bytes;
    bool ready=false;
    bool Prepare() {
        BPath path;
        if(find_directory(B_DESKTOP_DIRECTORY,&path)!=B_OK || node.SetTo(path.Path())!=B_OK) return false;
        if(node.GetAttrInfo(B_BACKGROUND_INFO,&info)!=B_OK) return false;
        bytes.resize(info.size);
        ready=node.ReadAttr(B_BACKGROUND_INFO,info.type,0,bytes.data(),bytes.size())==info.size;
        return ready;
    }
    ~RestoreBackground() {
        if(!ready) return;
        node.RemoveAttr(B_BACKGROUND_INFO);
        if(node.WriteAttr(B_BACKGROUND_INFO,info.type,0,bytes.data(),bytes.size())!=info.size)
            fprintf(stderr,"ERROR: background restore failed\n");
        BMessage refresh(B_RESTORE_BACKGROUND_IMAGE);
        BMessenger("application/x-vnd.Be-TRAK").SendMessage(&refresh);
    }
    BMessage Read() {
        attr_info current; BMessage result;
        if(node.GetAttrInfo(B_BACKGROUND_INFO,&current)!=B_OK) return result;
        std::vector<char> data(current.size);
        node.ReadAttr(B_BACKGROUND_INFO,current.type,0,data.data(),data.size());
        BMemoryIO input(data.data(),data.size()); result.Unflatten(&input); return result;
    }
};
static const char* ImageAt(const BMessage& message,int workspace)
{
    for(int i=0;;i++) {
        const char* path;
        if(message.FindString(B_BACKGROUND_IMAGE,i,&path)!=B_OK) return nullptr;
        int32 mask=0;
        if(message.FindInt32(B_BACKGROUND_WORKSPACES,i,&mask)!=B_OK) mask=B_ALL_WORKSPACES;
        if(uint32(mask)&(uint32(1)<<workspace)) return path;
    }
}
int main(int argc,char** argv)
{
    if(argc!=3) { fprintf(stderr,"Usage: DesktopTests IMAGE1 IMAGE2 (test VM only)\n"); return 2; }
    BApplication app("application/x-vnd.lhfchym-Wallhaven-DesktopTests");
    RestoreBackground restore;
    CHECK(restore.Prepare());
    std::string error;
    CHECK(SetDesktopWallpaper(argv[1],B_ALL_WORKSPACES,error));
    CHECK(SetDesktopWallpaper(argv[2],2,error));
    BMessage result=restore.Read();
    CHECK(strcmp(ImageAt(result,0),argv[1])==0);
    CHECK(strcmp(ImageAt(result,1),argv[2])==0);
    CHECK(strcmp(ImageAt(result,31),argv[1])==0);
    CHECK(!SetDesktopWallpaper("/nonexistent/wallhaven-test.png",4,error));
    BMessage unchanged=restore.Read();
    CHECK(strcmp(ImageAt(unchanged,0),argv[1])==0);
    CHECK(strcmp(ImageAt(unchanged,1),argv[2])==0);
    CHECK(SetDesktopWallpaper(argv[2],B_ALL_WORKSPACES,error));
    result=restore.Read();
    CHECK(strcmp(ImageAt(result,0),argv[2])==0 && strcmp(ImageAt(result,31),argv[2])==0);
    puts("DesktopTests: all, selected, untouched workspaces and invalid image passed; restoring original desktop");
}
