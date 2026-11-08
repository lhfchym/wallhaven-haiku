#include "Backend.h"
#include "DesktopIntegration.h"
#include "Localization.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Wallhaven"

#include <Application.h>
#include <Alert.h>
#include <Bitmap.h>
#include <Button.h>
#include <CheckBox.h>
#include <Entry.h>
#include <FilePanel.h>
#include <LayoutBuilder.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Messenger.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Roster.h>
#include <Screen.h>
#include <ScrollBar.h>
#include <ScrollView.h>
#include <SeparatorView.h>
#include <StringView.h>
#include <StringFormat.h>
#include <TextControl.h>
#include <TextView.h>
#include <TranslationUtils.h>
#include <View.h>
#include <Window.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace wallhaven;
const char* kSignature = "application/x-vnd.lhfchym-Wallhaven";
enum { kSearch='srch', kNext='next', kLocal='locl', kSelect='selc',
    kDownload='down', kSet='wall', kPreview='prev', kSource='sour', kFolder='fold',
    kSettings='sett', kSave='save', kChoose='chdr', kChosen='drch',
    kResults='rslt', kThumb='thmb', kDone='done', kActionDone='actd',
    kWork='work', kSettingsSaved='stsv', kFilter='filt', kScope='scop', kLanguage='lang' };

static void OpenPath(const std::string& path)
{
    entry_ref ref;
    if (get_ref_for_path(path.c_str(), &ref) == B_OK) be_roster->Launch(&ref);
}
static void FitBitmap(BView* view, BBitmap* bitmap, BRect area)
{
    if (!bitmap) return;
    BRect src = bitmap->Bounds();
    float scale = std::min((area.Width()+1)/(src.Width()+1), (area.Height()+1)/(src.Height()+1));
    float width = (src.Width()+1)*scale, height = (src.Height()+1)*scale;
    BRect dest(area.left+(area.Width()+1-width)/2, area.top+(area.Height()+1-height)/2,
        area.left+(area.Width()+1+width)/2-1, area.top+(area.Height()+1+height)/2-1);
    view->DrawBitmap(bitmap, src, dest, B_FILTER_BITMAP_BILINEAR);
}
static BBitmap* ReadThumbnail(const std::string& path)
{
    std::unique_ptr<BBitmap> original(BTranslationUtils::GetBitmap(path.c_str()));
    if(!original || original->InitCheck()!=B_OK) return nullptr;
    BRect bounds=original->Bounds();
    float scale=std::min(1.f,600.f/std::max(bounds.Width()+1,bounds.Height()+1));
    if(scale==1) return original.release();
    auto reduced=std::make_unique<BBitmap>(BRect(0,0,std::max(1.f,std::floor((bounds.Width()+1)*scale))-1,
        std::max(1.f,std::floor((bounds.Height()+1)*scale))-1),B_BITMAP_ACCEPTS_VIEWS,B_RGB32);
    if(reduced->InitCheck()!=B_OK || !reduced->Lock()) return nullptr;
    auto view=new BView(reduced->Bounds(),"scale",B_FOLLOW_NONE,0);
    reduced->AddChild(view); view->DrawBitmap(original.get(),bounds,reduced->Bounds(),B_FILTER_BITMAP_BILINEAR);
    view->Sync(); reduced->RemoveChild(view); delete view; reduced->Unlock();
    return reduced.release();
}

class ImageView : public BView {
public:
    ImageView(const char* name) : BView(name, B_WILL_DRAW) {
        SetViewColor(28,32,38); SetExplicitMinSize(BSize(170,140));
    }
    void Load(const std::string& path, bool original=false) {
        fBitmap.reset(original ? BTranslationUtils::GetBitmap(path.c_str()) : ReadThumbnail(path)); Invalidate();
    }
    void Clear() { fBitmap.reset(); Invalidate(); }
    void Draw(BRect) override { FitBitmap(this, fBitmap.get(), Bounds().InsetByCopy(4,4)); }
private:
    std::unique_ptr<BBitmap> fBitmap;
};

class Gallery : public BView {
public:
    Gallery() : BView("gallery", B_WILL_DRAW|B_FRAME_EVENTS|B_NAVIGABLE|B_SCROLL_VIEW_AWARE) {
        SetViewColor(238,240,243); SetExplicitMinSize(BSize(350,260));
    }
    void SetItems(const std::vector<Wallpaper>& items) {
        fItems=items; fImages.clear(); fImages.resize(items.size()); fSelected=-1;
        ScrollTo(0,0); UpdateScroll(); Invalidate();
    }
    void Load(int i, BBitmap* bitmap) {
        std::unique_ptr<BBitmap> owned(bitmap);
        if (i<0 || i>=(int)fItems.size()) return;
        fImages[i]=std::move(owned); Invalidate(Cell(i));
    }
    void Select(int i) {
        if(i<0 || i>=(int)fItems.size()) return;
        BRect cell=Cell(i);
        if(cell.top<Bounds().top) ScrollTo(0,cell.top);
        else if(cell.bottom>Bounds().bottom) ScrollTo(0,cell.bottom-Bounds().Height()+8);
        fSelected=i; Invalidate(); BMessage m(kSelect); m.AddInt32("index",i); Window()->PostMessage(&m);
    }
    void MarkDownloaded(const std::string& id, const std::string& path) {
        for(auto& item:fItems) if(item.id==id) item.localPath=path;
        Invalidate();
    }
    void FrameResized(float,float) override { UpdateScroll(); Invalidate(); }
    void MouseDown(BPoint p) override {
        MakeFocus(); int columns=Columns(); float width=(Bounds().Width()-16)/columns;
        int col=int((p.x-8)/width), row=int((p.y-8)/CellHeight());
        if(p.x>=8 && p.y>=8 && col>=0 && col<columns) Select(row*columns+col);
        int32 clicks=1; Window()->CurrentMessage()->FindInt32("clicks",&clicks);
        if(clicks==2 && fSelected>=0) Window()->PostMessage(kPreview);
    }
    void KeyDown(const char* bytes,int32 n) override {
        if(n && bytes[0]==B_RIGHT_ARROW) Select(std::min((int)fItems.size()-1,fSelected+1));
        else if(n && bytes[0]==B_LEFT_ARROW) Select(std::max(0,fSelected-1));
        else if(n && bytes[0]==B_DOWN_ARROW) Select(std::min((int)fItems.size()-1,fSelected+Columns()));
        else if(n && bytes[0]==B_UP_ARROW) Select(std::max(0,fSelected-Columns()));
        else if(n && bytes[0]==B_ENTER) Window()->PostMessage(kPreview);
        else BView::KeyDown(bytes,n);
    }
    void Draw(BRect update) override {
        if(fItems.empty()) {
            SetHighColor(100,108,118);
            DrawString(B_TRANSLATE("No images. Search or refresh."),BPoint(20,45));
            return;
        }
        for(int i=0;i<(int)fItems.size();i++) {
            BRect r=Cell(i); if(!r.Intersects(update)) continue;
            SetHighColor(255,255,255); FillRoundRect(r,5,5);
            SetHighColor(i==fSelected ? rgb_color{42,117,180,255} : rgb_color{210,215,221,255});
            SetPenSize(i==fSelected?3:1); StrokeRoundRect(r,5,5); SetPenSize(1);
            BRect imageRect=r.InsetByCopy(5,5); imageRect.bottom-=29;
            SetHighColor(218,223,228); FillRect(imageRect);
            FitBitmap(this,fImages[i].get(),imageRect);
            SetHighColor(54,62,72); SetFontSize(11);
            std::string label=fItems[i].resolution;
            if(label.empty()) label=fItems[i].id;
            DrawString(label.c_str(),BPoint(r.left+8,r.bottom-10));
            if(!fItems[i].localPath.empty()) {
                const char* saved=B_TRANSLATE("Saved");
                SetHighColor(41,115,73); DrawString(saved,BPoint(r.right-StringWidth(saved)-8,r.bottom-10));
            }
        }
    }
private:
    int Columns() const { return std::max(2,int(Bounds().Width()/190)); }
    float CellHeight() const { return ((Bounds().Width()-16)/Columns()-14)*0.65f+40; }
    BRect Cell(int i) const {
        float width=(Bounds().Width()-16)/Columns();
        float x=8+(i%Columns())*width, y=8+(i/Columns())*CellHeight();
        return BRect(x,y,x+width-9,y+CellHeight()-9);
    }
    void UpdateScroll() {
        float total=16+((fItems.size()+Columns()-1)/Columns())*CellHeight();
        if(BScrollBar* s=ScrollBar(B_VERTICAL)) {
            s->SetRange(0,std::max(0.f,total-Bounds().Height()));
            s->SetProportion(std::min(1.f,Bounds().Height()/std::max(1.f,total)));
            s->SetSteps(CellHeight()/3,Bounds().Height()*0.85f);
        }
    }
    int fSelected=-1;
    std::vector<Wallpaper> fItems;
    std::vector<std::unique_ptr<BBitmap>> fImages;
};

struct Task {
    uint32 action;
    Preferences prefs;
    Wallpaper wallpaper;
    int page=1, width=1024,height=768,generation=0;
    std::string seed;
    int32 workspace=-1; // -1 means all; otherwise captured at click time.
    bool local=false;
};
struct Result { SearchResult search; std::string error; bool local=false; };

class Worker : public BLooper {
public:
    Worker(BMessenger target) : BLooper("Wallhaven network"), fTarget(target) { Run(); }
    void MessageReceived(BMessage* m) override {
        if(m->what!=kWork) { BLooper::MessageReceived(m); return; }
        Task* ptr=nullptr; if(m->FindPointer("task",(void**)&ptr)!=B_OK) return;
        std::unique_ptr<Task> task(ptr); Backend backend;
        if(task->action==kSearch) {
            auto result=std::make_unique<Result>(); result->local=task->local;
            if(task->local) {
                auto downloads=backend.LoadDownloaded(task->prefs);
                result->search.total=downloads.size();
                result->search.lastPage=std::max(1,int((downloads.size()+23)/24));
                result->search.page=std::min(task->page,result->search.lastPage);
                size_t first=(result->search.page-1)*24;
                size_t end=std::min(first+24,downloads.size());
                result->search.wallpapers.assign(downloads.begin()+first,downloads.begin()+end);
            }
            else backend.Search(task->prefs,task->width,task->height,task->page,task->seed,result->search,result->error);
            auto items=result->search.wallpapers;
            BMessage reply(kResults); reply.AddInt32("generation",task->generation);
            reply.AddPointer("result",result.get());
            if(fTarget.SendMessage(&reply)!=B_OK) return;
            result.release();
            if(!task->local && !items.empty()) { std::string ignored; backend.RecordDisplayed(items,ignored); }
            std::atomic<size_t> next{0}, loaded{0};
            auto load=[&]() {
                size_t i;
                while((i=next.fetch_add(1))<items.size()) {
                    std::string path,error;
                    if(!items[i].localPath.empty()) path=items[i].localPath;
                    else if(!backend.EnsureThumbnail(items[i],path,error)) continue;
                    if(path.empty()) continue;
                    auto bitmap=std::unique_ptr<BBitmap>(ReadThumbnail(path));
                    if(!bitmap) continue;
                    BMessage thumb(kThumb); thumb.AddInt32("generation",task->generation);
                    thumb.AddInt32("index",i); thumb.AddString("path",path.c_str());
                    thumb.AddPointer("bitmap",bitmap.get());
                    if(fTarget.SendMessage(&thumb)==B_OK) { bitmap.release(); ++loaded; }
                }
            };
            std::thread a(load),b(load),c(load); load(); a.join(); b.join(); c.join();
            BMessage done(kDone); done.AddInt32("generation",task->generation);
            done.AddInt32("failed",items.size()-loaded.load()); fTarget.SendMessage(&done);
        } else {
            std::string path,error;
            bool ok=backend.Download(task->wallpaper,task->prefs,path,error);
            if(ok && task->action==kSet) {
                ok=SetDesktopWallpaper(path,task->workspace < 0 ? B_ALL_WORKSPACES : (uint32(1) << task->workspace),error);
                if(ok) { std::string ignored; backend.RecordSet(task->wallpaper,ignored); }
            }
            BMessage done(kActionDone); done.AddBool("ok",ok); done.AddInt32("action",task->action);
            done.AddInt32("workspace",task->workspace);
            done.AddString("path",path.c_str()); done.AddString("error",error.c_str());
            done.AddString("id",task->wallpaper.id.c_str()); fTarget.SendMessage(&done);
        }
    }
private:
    BMessenger fTarget;
};

class SettingsWindow : public BWindow {
public:
    SettingsWindow(Preferences prefs,BMessenger target) : BWindow(BRect(160,150,740,490),
        B_TRANSLATE("Wallhaven settings"),B_TITLED_WINDOW,
        B_ASYNCHRONOUS_CONTROLS|B_AUTO_UPDATE_SIZE_LIMITS|B_NOT_ZOOMABLE|B_NOT_RESIZABLE),
        fPrefs(prefs),fTarget(target) {
        fKey=new BTextControl("key",B_TRANSLATE("API key:"),prefs.apiKey.c_str(),nullptr);
        fKey->TextView()->HideTyping(true);
        fDir=new BTextControl("directory",B_TRANSLATE("Download folder:"),Backend().DownloadDirectory(prefs).c_str(),nullptr);
        fDir->SetExplicitMinSize(BSize(380,B_SIZE_UNSET));
        fStrict=new BCheckBox("strict",B_TRANSLATE("Match screen aspect ratio exactly"),nullptr);
        fStrict->SetValue(prefs.strictAspectRatio);
        auto languages=new BPopUpMenu("language");
        languages->SetRadioMode(true); languages->SetLabelFromMarked(true);
        const char* codes[]={"en","zh-Hans","system"};
        const char* labels[]={"English","简体中文",B_TRANSLATE("System language")};
        for(int i=0;i<3;i++) {
            auto msg=new BMessage(kLanguage); msg->AddString("value",codes[i]);
            auto item=new BMenuItem(labels[i],msg); languages->AddItem(item);
            item->SetMarked(prefs.language==codes[i]);
        }
        if(!languages->FindMarked()) languages->ItemAt(0)->SetMarked(true);
        fLanguage=new BMenuField("language",B_TRANSLATE("Language:"),languages);
        BLayoutBuilder::Group<>(this,B_VERTICAL,12).SetInsets(16)
            .Add(fKey)
            .Add(new BStringView("help",B_TRANSLATE("Optional; restricted content needs a key. Stored locally.")))
            .AddGroup(B_HORIZONTAL,8).Add(fDir).Add(new BButton("choose",B_TRANSLATE("Choose…"),new BMessage(kChoose))).End()
            .Add(fStrict).Add(fLanguage)
            .Add(new BStringView("scope",B_TRANSLATE("Restart Wallhaven to apply language changes.")))
            .AddGroup(B_HORIZONTAL,8).AddGlue()
                .Add(new BButton("cancel",B_TRANSLATE("Cancel"),new BMessage(B_QUIT_REQUESTED)))
                .Add(new BButton("save",B_TRANSLATE("Save"),new BMessage(kSave))).End();
        Show();
    }
    void MessageReceived(BMessage* m) override {
        switch(m->what) {
        case kLanguage: break; // BMenu marks the selected language.
        case kChoose:
            if(!fPanel) {
                BMessenger target(this); BMessage chosen(kChosen);
                fPanel.reset(new BFilePanel(B_OPEN_PANEL,&target,nullptr,B_DIRECTORY_NODE,false,&chosen));
            }
            fPanel->Show(); break;
        case kChosen: {
            entry_ref ref; if(m->FindRef("refs",&ref)==B_OK) { BPath p(&ref); fDir->SetText(p.Path()); } break;
        }
        case kSave: {
            fPrefs.apiKey=fKey->Text(); fPrefs.downloadDirectory=fDir->Text();
            fPrefs.strictAspectRatio=fStrict->Value(); fPrefs.language=fLanguage->Menu()->FindMarked()->Message()->GetString("value","en");
            std::string error;
            if(!Backend().SavePreferences(fPrefs,error)) { (new BAlert("error",error.c_str(),B_TRANSLATE("OK")))->Go(); break; }
            fTarget.SendMessage(kSettingsSaved); PostMessage(B_QUIT_REQUESTED); break;
        }
        default: BWindow::MessageReceived(m);
        }
    }
private:
    Preferences fPrefs; BMessenger fTarget;
    BTextControl *fKey,*fDir; BCheckBox* fStrict; BMenuField* fLanguage;
    std::unique_ptr<BFilePanel> fPanel;
};

class MainWindow : public BWindow {
public:
    MainWindow() : BWindow(BRect(24,70,984,710),"Wallhaven · Haiku",B_TITLED_WINDOW,
        B_ASYNCHRONOUS_CONTROLS|B_AUTO_UPDATE_SIZE_LIMITS), fPrefs(Backend().LoadPreferences()) {
        fQuery=new BTextControl("query",nullptr,fPrefs.query.c_str(),new BMessage(kSearch));
        fSearch=Button("search",B_TRANSLATE("Search"),kSearch);
        fRefresh=Button("refresh",B_TRANSLATE("Refresh"),kSearch);
        fNext=Button("next",B_TRANSLATE("Next page"),kNext);
        fDownloads=Button("downloads",B_TRANSLATE("Downloaded"),kLocal);
        fCategories=Menu("categories",{B_TRANSLATE("General"),B_TRANSLATE("Anime"),B_TRANSLATE("People"),B_TRANSLATE("All")},{"100","010","001","111"},fPrefs.categories);
        fPurity=Menu("purity",{B_TRANSLATE("SFW"),B_TRANSLATE("SFW + Sketchy"),B_TRANSLATE("All (API key)")},{"100","110","111"},fPrefs.purity);
        fSorting=Menu("sorting",{B_TRANSLATE("Random"),B_TRANSLATE("Toplist"),B_TRANSLATE("Latest"),B_TRANSLATE("Favorites"),B_TRANSLATE("Views"),B_TRANSLATE("Relevance")},{"random","toplist","date_added","favorites","views","relevance"},fPrefs.sorting);
        auto sidebar=new BView("sidebar",0); sidebar->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
        sidebar->SetExplicitMinSize(BSize(165,0)); sidebar->SetExplicitMaxSize(BSize(180,B_SIZE_UNLIMITED));
        BScreen screen(this); BRect frame=screen.Frame();
        fWidth=frame.IntegerWidth()+1; fHeight=frame.IntegerHeight()+1;
        std::string display=std::to_string(fWidth)+" × "+std::to_string(fHeight);
        auto title=new BStringView("brand","Wallhaven"); BFont font(be_bold_font); font.SetSize(22); title->SetFont(&font);
        BLayoutBuilder::Group<>(sidebar,B_VERTICAL,10).SetInsets(12)
            .Add(title).Add(new BStringView("native",B_TRANSLATE("Native wallpaper picker")))
            .Add(new BSeparatorView(B_HORIZONTAL))
            .Add(new BStringView("category-label",B_TRANSLATE("Category"))).Add(fCategories)
            .Add(new BStringView("purity-label",B_TRANSLATE("Content"))).Add(fPurity)
            .Add(new BStringView("sort-label",B_TRANSLATE("Sort"))).Add(fSorting)
            .Add(fRefresh).Add(fDownloads).AddGlue()
            .Add(new BStringView("display",display.c_str()))
            .Add(Button("settings",B_TRANSLATE("Settings…"),kSettings));
        fGallery=new Gallery();
        auto scroll=new BScrollView("gallery-scroll",fGallery,0,false,true,B_PLAIN_BORDER);
        fStatus=new BStringView("status",B_TRANSLATE("Loading…")); fStatus->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED,25));
        fStatus->SetTruncation(B_TRUNCATE_END);
        auto center=new BView("center",0); center->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
        BLayoutBuilder::Group<>(center,B_VERTICAL,8).SetInsets(8)
            .AddGroup(B_HORIZONTAL,8).Add(fQuery).Add(fSearch).End()
            .Add(scroll)
            .AddGroup(B_HORIZONTAL,8).Add(fStatus).Add(fNext).End();
        auto inspector=new BView("inspector",0); inspector->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
        inspector->SetExplicitMinSize(BSize(190,0)); inspector->SetExplicitMaxSize(BSize(220,B_SIZE_UNLIMITED));
        fPreview=new ImageView("preview"); fPreview->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED,180));
        fInfo=new BStringView("info",B_TRANSLATE("Select an image"));
        fMeta=new BStringView("metadata","");
        fWorkspaces=new BMenuField("workspaces",nullptr,new BPopUpMenu("workspaces"));
        fWorkspaces->Menu()->SetRadioMode(true); fWorkspaces->Menu()->SetLabelFromMarked(true);
        RebuildWorkspaces();
        fSet=Button("set",B_TRANSLATE("Set wallpaper"),kSet);
        fDownload=Button("download",B_TRANSLATE("Download original"),kDownload);
        fFull=Button("preview-button",B_TRANSLATE("Preview"),kPreview);
        fSource=Button("source",B_TRANSLATE("Open source page"),kSource);
        BLayoutBuilder::Group<>(inspector,B_VERTICAL,10).SetInsets(12)
            .Add(new BStringView("preview-title",B_TRANSLATE("Wallpaper preview")))
            .Add(fPreview).Add(fInfo).Add(fMeta).Add(fFull).Add(fDownload)
            .Add(new BStringView("workspace-label",B_TRANSLATE("Apply to:"))).Add(fWorkspaces).Add(fSet)
            .Add(fSource).AddGlue().Add(Button("folder",B_TRANSLATE("Open downloads"),kFolder));
        BLayoutBuilder::Group<>(this,B_HORIZONTAL,0).Add(sidebar)
            .Add(new BSeparatorView(B_VERTICAL)).Add(center)
            .Add(new BSeparatorView(B_VERTICAL)).Add(inspector);
        SetSizeLimits(880,2000,560,1600);
        fWorker=new Worker(BMessenger(this));
        UpdateActions(); Show(); PostMessage(kSearch);
    }
    void WorkspacesChanged(uint32, uint32) override { RebuildWorkspaces(); }
    void WorkspaceActivated(int32, bool active) override { if(active) RebuildWorkspaces(); }
    void MenusBeginning() override { RebuildWorkspaces(); }
    bool QuitRequested() override { be_app->PostMessage(B_QUIT_REQUESTED); return true; }
    void MessageReceived(BMessage* m) override {
        switch(m->what) {
        case kScope: {
            int32 scope=m->GetInt32("workspace",-2);
            if(!fAction && scope>=-2 && scope<(int32)count_workspaces()) {
                fWorkspace=scope; RebuildWorkspaces();
            }
            break;
        }
        case kSearch: if(!fBusy&&!fAction) StartSearch(false,1); break;
        case kFilter: if(!fBusy&&!fAction) StartSearch(false,1); break;
        case kLocal: if(!fBusy&&!fAction) StartSearch(true,1); break;
        case kNext: if(!fBusy&&!fAction&&fPage<fLastPage) StartSearch(fLocal,fPage+1); break;
        case kResults: {
            Result* raw=nullptr; m->FindPointer("result",(void**)&raw); std::unique_ptr<Result> result(raw);
            if(!result || m->GetInt32("generation",-1)!=fGeneration) break;
            fItems=result->search.wallpapers; fPage=result->search.page; fLastPage=result->search.lastPage; fSeed=result->search.seed;
            fSelected=-1; fPaths.assign(fItems.size(),""); fPreview->Clear();
            fGallery->SetItems(fItems); fInfo->SetText(B_TRANSLATE("Select an image")); fMeta->SetText("");
            fError=result->error;
            if(!fError.empty()) Status(fError);
            else Status(B_TRANSLATE("Loading thumbnails…"));
            UpdateActions(); break;
        }
        case kThumb: {
            BBitmap* raw=nullptr; m->FindPointer("bitmap",(void**)&raw); std::unique_ptr<BBitmap> bitmap(raw);
            int i=m->GetInt32("index",-1);
            if(m->GetInt32("generation",-1)!=fGeneration || i<0 || i>=(int)fItems.size()) break;
            fPaths[i]=m->GetString("path",""); fGallery->Load(i,bitmap.release());
            if(i==fSelected) fPreview->Load(fPaths[i]);
            break;
        }
        case kDone:
            if(m->GetInt32("generation",-1)==fGeneration) {
                fBusy=false;
                if(fError.empty()) {
                    BString count;
                    BStringFormat format(B_TRANSLATE("{0, plural, one{# wallpaper} other{# wallpapers}}"));
                    format.Format(count,static_cast<int64>(fItems.size()));
                    BString status(B_TRANSLATE("%count% · Page %page% of %pages%"));
                    if(fLocal) status=B_TRANSLATE("%count% · Downloads");
                    status.ReplaceAll("%count%",count.String());
                    status.ReplaceAll("%page%",std::to_string(fPage).c_str());
                    status.ReplaceAll("%pages%",std::to_string(fLastPage).c_str());
                    Status(status.String());
                    int32 failed=m->GetInt32("failed",0);
                    if(failed>0) {
                        BStringFormat failures(B_TRANSLATE("{0, plural, one{# thumbnail failed; refresh to retry.} other{# thumbnails failed; refresh to retry.}}"));
                        failures.Format(status,failed); Status(status.String());
                    }
                }
                UpdateActions();
            } break;
        case kSelect: {
            int i=m->GetInt32("index",-1); if(i<0 || i>=(int)fItems.size()) break;
            fSelected=i; const auto& w=fItems[i];
            fInfo->SetText((w.id+" · "+w.resolution).c_str());
            fMeta->SetText((w.category+" / "+w.purity).c_str());
            fPreview->Clear(); if(!fPaths[i].empty()) fPreview->Load(fPaths[i]); UpdateActions(); break;
        }
        case kDownload: case kSet: case kPreview:
            if(fSelected>=0&&!fAction) {
                fAction=true; UpdateActions(); Status(B_TRANSLATE("Preparing original image…"));
                auto task=new Task; task->action=m->what; task->prefs=fPrefs; task->wallpaper=fItems[fSelected];
                RebuildWorkspaces();
                task->workspace=fWorkspace==-2 ? current_workspace() : fWorkspace;
                Queue(task);
            } break;
        case kActionDone: {
            fAction=false; UpdateActions();
            if(!m->GetBool("ok",false)) {
                std::string error=m->GetString("error",B_TRANSLATE("Unknown error")); Status(error);
                (new BAlert("Wallhaven",error.c_str(),B_TRANSLATE("OK"),nullptr,nullptr,B_WIDTH_AS_USUAL,B_WARNING_ALERT))->Go(nullptr); break;
            }
            std::string path=m->GetString("path","");
            for(auto& item:fItems) if(item.id==m->GetString("id","")) item.localPath=path;
            fGallery->MarkDownloaded(m->GetString("id",""),path);
            uint32 action=m->GetInt32("action",0);
            if(action==kPreview) {
                auto window=new BWindow(BRect(90,90,910,660),B_TRANSLATE("Wallhaven — Preview"),B_TITLED_WINDOW,B_ASYNCHRONOUS_CONTROLS|B_AUTO_UPDATE_SIZE_LIMITS);
                auto view=new ImageView("original"); view->Load(path,true); BLayoutBuilder::Group<>(window,B_VERTICAL,0).Add(view); window->Show();
            }
            if(action==kSet) {
                int32 workspace=m->GetInt32("workspace",-1);
                if(workspace<0) Status(B_TRANSLATE("Wallpaper updated on all workspaces"));
                else {
                    BString status(B_TRANSLATE("Wallpaper updated on workspace %number%"));
                    status.ReplaceAll("%number%",std::to_string(workspace+1).c_str()); Status(status.String());
                }
            } else Status(B_TRANSLATE("Original saved to downloads"));
            break;
        }
        case kSource:
            if(fSelected>=0&&!fItems[fSelected].url.empty()) {
                const char* url=fItems[fSelected].url.c_str(); be_roster->Launch("text/html",1,const_cast<char**>(&url));
            } break;
        case kFolder: OpenPath(Backend().DownloadDirectory(fPrefs)); break;
        case kSettings:
            if(!fSettings.IsValid()) fSettings=BMessenger(new SettingsWindow(fPrefs,BMessenger(this)));
            else { BMessage activate(B_SET_PROPERTY); activate.AddSpecifier("Active"); activate.AddBool("data",true); fSettings.SendMessage(&activate); }
            break;
        case kSettingsSaved:
            fPrefs=Backend().LoadPreferences(); Status(B_TRANSLATE("Settings saved; restart to change language.")); break;
        default: BWindow::MessageReceived(m);
        }
    }
private:
    void RebuildWorkspaces() {
        if(fWorkspace>=(int32)count_workspaces()) fWorkspace=-2;
        BMenu* menu=fWorkspaces->Menu();
        while(BMenuItem* item=menu->RemoveItem(int32(0))) delete item;
        for(int32 workspace=-2;workspace<(int32)count_workspaces();workspace++) {
            BString label;
            if(workspace==-2) label=B_TRANSLATE("Current workspace");
            else if(workspace==-1) label=B_TRANSLATE("All workspaces");
            else {
                label=B_TRANSLATE("Workspace %number%");
                label.ReplaceAll("%number%",std::to_string(workspace+1).c_str());
            }
            auto msg=new BMessage(kScope); msg->AddInt32("workspace",workspace);
            auto item=new BMenuItem(label.String(),msg); menu->AddItem(item);
            item->SetTarget(this); item->SetMarked(fWorkspace==workspace);
        }
    }
    BButton* Button(const char* name,const char* label,uint32 what) {
        return new BButton(name,label,new BMessage(what));
    }
    BMenuField* Menu(const char* name,std::vector<const char*> labels,
        std::vector<const char*> values,const std::string& selected) {
        auto menu=new BPopUpMenu(name); menu->SetRadioMode(true); menu->SetLabelFromMarked(true);
        for(size_t i=0;i<values.size();i++) {
            auto msg=new BMessage(kFilter); msg->AddString("value",values[i]);
            auto item=new BMenuItem(labels[i],msg); menu->AddItem(item); item->SetMarked(selected==values[i]);
        }
        if(!menu->FindMarked()) menu->ItemAt(0)->SetMarked(true);
        return new BMenuField(name,nullptr,menu);
    }
    std::string Value(BMenuField* field) { return field->Menu()->FindMarked()->Message()->GetString("value",""); }
    void Status(const std::string& text) { fStatus->SetText(text.c_str()); fStatus->SetToolTip(text.c_str()); }
    void Queue(Task* task) {
        BMessage msg(kWork); msg.AddPointer("task",task);
        if(fWorker->PostMessage(&msg)!=B_OK) delete task;
    }
    void StartSearch(bool local,int page) {
        fPrefs.query=fQuery->Text(); fPrefs.categories=Value(fCategories); fPrefs.purity=Value(fPurity); fPrefs.sorting=Value(fSorting);
        std::string error; Backend().SavePreferences(fPrefs,error);
        fBusy=true; fLocal=local; fError.clear(); ++fGeneration; UpdateActions();
        Status(B_TRANSLATE("Fetching wallpapers…"));
        auto task=new Task; task->action=kSearch; task->prefs=fPrefs; task->page=page; task->local=local;
        task->width=fWidth; task->height=fHeight; task->generation=fGeneration; task->seed=page>1?fSeed:""; Queue(task);
    }
    void UpdateActions() {
        bool idle=!fBusy&&!fAction;
        fSearch->SetEnabled(idle); fRefresh->SetEnabled(idle); fDownloads->SetEnabled(idle);
        fCategories->SetEnabled(idle); fPurity->SetEnabled(idle); fSorting->SetEnabled(idle);
        fNext->SetEnabled(idle&&fPage<fLastPage);
        bool selected=fSelected>=0&&!fAction;
        fWorkspaces->SetEnabled(!fAction);
        fSet->SetEnabled(selected); fDownload->SetEnabled(selected); fFull->SetEnabled(selected); fSource->SetEnabled(selected);
    }
    Preferences fPrefs; bool fBusy=false,fAction=false,fLocal=false;
    int32 fWorkspace=-2;
    int fWidth=1024,fHeight=768,fPage=1,fLastPage=1,fGeneration=0,fSelected=-1;
    std::string fSeed,fError;
    Worker* fWorker; BMessenger fSettings;
    BTextControl* fQuery; BMenuField *fCategories,*fPurity,*fSorting,*fWorkspaces;
    BButton *fSearch,*fRefresh,*fNext,*fDownloads,*fSet,*fDownload,*fFull,*fSource;
    BStringView *fStatus,*fInfo,*fMeta; Gallery* fGallery; ImageView* fPreview;
    std::vector<Wallpaper> fItems; std::vector<std::string> fPaths;
};

class WallhavenApp : public BApplication {
public:
    WallhavenApp() : BApplication(kSignature) {}
    void ReadyToRun() override {
        InitializeLocalization(Backend().LoadPreferences().language);
        fWindow=new MainWindow;
    }
    void MessageReceived(BMessage* msg) override {
        // Native BMessage commands also make the UI accessible to Haiku's hey tool.
        switch(msg->what) {
        case kSearch: case kLocal: case kNext: case kDownload: case kSet: case kPreview:
        case kSelect: case kSettings: case kFolder: case kScope:
            if(fWindow) { fWindow->PostMessage(msg); BMessage reply(B_REPLY); reply.AddInt32("error",B_OK); msg->SendReply(&reply); } break;
        default: BApplication::MessageReceived(msg);
        }
    }
private:
    MainWindow* fWindow=nullptr;
};
int main() { WallhavenApp app; app.Run(); return 0; }
