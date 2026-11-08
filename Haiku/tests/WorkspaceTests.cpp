#include "WorkspaceBackground.h"

#include <Window.h>
#include <be_apps/Tracker/Background.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>

#define CHECK(test) do { if(!(test)) { fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#test); exit(1); } } while(0)

static void Add(BMessage& m,const char* path,uint32 mask,int32 mode,BPoint origin,bool erase)
{
    m.AddString(B_BACKGROUND_IMAGE,path); m.AddInt32(B_BACKGROUND_WORKSPACES,mask);
    m.AddInt32(B_BACKGROUND_MODE,mode); m.AddPoint(B_BACKGROUND_ORIGIN,origin);
    m.AddBool(B_BACKGROUND_ERASE_TEXT,erase);
}
static int Count(const BMessage& m)
{
    int32 count=0; type_code type; m.GetInfo(B_BACKGROUND_IMAGE,&type,&count); return count;
}
static void Check(const BMessage& m,int i,const char* path,uint32 mask,int32 mode,BPoint origin,bool erase)
{
    const char* found="(missing)"; int32 bits,placement; BPoint point; bool outline;
    m.FindString(B_BACKGROUND_IMAGE,i,&found);
    if(strcmp(found,path)!=0) { fprintf(stderr,"record %d: expected %s, got %s\n",i,path,found); m.PrintToStream(); }
    CHECK(m.FindString(B_BACKGROUND_IMAGE,i,&found)==B_OK && strcmp(found,path)==0);
    CHECK(m.FindInt32(B_BACKGROUND_WORKSPACES,i,&bits)==B_OK && uint32(bits)==mask);
    CHECK(m.FindInt32(B_BACKGROUND_MODE,i,&placement)==B_OK && placement==mode);
    CHECK(m.FindPoint(B_BACKGROUND_ORIGIN,i,&point)==B_OK && point==origin);
    CHECK(m.FindBool(B_BACKGROUND_ERASE_TEXT,i,&outline)==B_OK && outline==erase);
}
int main()
{
    BMessage original, result;
    Add(original,"old",B_ALL_WORKSPACES,B_BACKGROUND_MODE_CENTERED,BPoint(5,9),false);
    original.AddString("custom metadata","keep");
    CHECK(MergeWorkspaceBackground(original,"new",2,result)==B_OK);
    CHECK(Count(result)==2);
    Check(result,0,"old",uint32(B_ALL_WORKSPACES)&~2U,B_BACKGROUND_MODE_CENTERED,BPoint(5,9),false);
    Check(result,1,"new",2,B_BACKGROUND_MODE_SCALED,BPoint(0,0),true);
    CHECK(strcmp(result.GetString("custom metadata",""),"keep")==0);
    Check(original,0,"old",B_ALL_WORKSPACES,B_BACKGROUND_MODE_CENTERED,BPoint(5,9),false);
    CHECK(MergeWorkspaceBackground(result,"third",4,result)==B_OK);
    CHECK(Count(result)==3);
    Check(result,1,"new",2,B_BACKGROUND_MODE_SCALED,BPoint(0,0),true);
    Check(result,2,"third",4,B_BACKGROUND_MODE_SCALED,BPoint(0,0),true);
    for(int i=0;i<100;i++) CHECK(MergeWorkspaceBackground(result,"replacement",2,result)==B_OK);
    CHECK(Count(result)==3); // Repeated changes do not accumulate stale records.
    CHECK(MergeWorkspaceBackground(result,"all",B_ALL_WORKSPACES,result)==B_OK);
    CHECK(Count(result)==1);
    Check(result,0,"all",B_ALL_WORKSPACES,B_BACKGROUND_MODE_SCALED,BPoint(0,0),true);
    CHECK(MergeWorkspaceBackground(result,"last",uint32(1)<<31,result)==B_OK);
    Check(result,1,"last",uint32(1)<<31,B_BACKGROUND_MODE_SCALED,BPoint(0,0),true);
    BMessage defaults; defaults.AddString(B_BACKGROUND_IMAGE,"");
    CHECK(MergeWorkspaceBackground(defaults,"one",1,result)==B_OK);
    Check(result,0,"",uint32(B_ALL_WORKSPACES)&~1U,B_BACKGROUND_MODE_TILED,BPoint(0,0),false);
    CHECK(MergeWorkspaceBackground(BMessage(),"fresh",8,result)==B_OK && Count(result)==1);
    BMessage bad; bad.AddString(B_BACKGROUND_WORKSPACES,"wrong type");
    CHECK(MergeWorkspaceBackground(bad,"bad",1,result)==B_BAD_DATA);
    CHECK(MergeWorkspaceBackground(original,"bad",0,result)==B_BAD_VALUE);
    CHECK(MergeWorkspaceBackground(original,"",1,result)==B_BAD_VALUE);
    CHECK(MergeWorkspaceBackground(original,nullptr,1,result)==B_BAD_VALUE);
    Check(result,0,"fresh",8,B_BACKGROUND_MODE_SCALED,BPoint(0,0),true);
    puts("WorkspaceTests: passed (split, preserve, repeat, all, bit 31, defaults, invalid input)");
}
