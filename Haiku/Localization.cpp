#include "Localization.h"

#include <Application.h>
#include <Roster.h>

BCatalog* WallhavenCatalog()
{
    static BCatalog catalog;
    return &catalog;
}

void InitializeLocalization(const std::string& language)
{
    app_info info;
    if (!be_app || be_app->GetAppInfo(&info) != B_OK) return;
    const char* requested = language == "system" ? nullptr
        : language == "zh-Hans" ? "zh_Hans" : language.c_str();
    // An absent catalog naturally falls back to the English source strings.
    WallhavenCatalog()->SetTo(info.ref, requested);
}
