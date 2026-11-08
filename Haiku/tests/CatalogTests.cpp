#include <Catalog.h>
#include <Entry.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>

#define CHECK(test) do { if(!(test)) { fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#test); return 1; } } while(0)
int main()
{
    entry_ref ref;
    CHECK(get_ref_for_path("Wallhaven",&ref)==B_OK);
    BCatalog chinese(ref,"zh_Hans");
    CHECK(chinese.InitCheck()==B_OK && chinese.CountItems()>=90);
    CHECK(strcmp(chinese.GetString("Search","Wallhaven"),"搜索")==0);
    CHECK(strcmp(chinese.GetString("Current workspace","Wallhaven"),"当前工作区")==0);
    CHECK(strcmp(chinese.GetString("Invalid search settings.","Backend"),"搜索设置无效。")==0);
    CHECK(strcmp(chinese.GetString("Cannot read desktop settings","Desktop"),"无法读取桌面设置")==0);
    CHECK(strcmp(chinese.GetString("Network error: %detail%","Backend"),"网络错误：%detail%")==0);
    BCatalog english(ref,"en");
    CHECK(strcmp(english.GetString("Search","Wallhaven"),"Search")==0);
    BCatalog missing(ref,"xx_XX");
    CHECK(strcmp(missing.GetString("Search","Wallhaven"),"Search")==0);
    puts("CatalogTests: embedded Chinese, English and missing-catalog fallback passed");
}
