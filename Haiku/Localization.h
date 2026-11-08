#ifndef WALLHAVEN_LOCALIZATION_H
#define WALLHAVEN_LOCALIZATION_H

#include <Catalog.h>
#include <string>

// One immutable catalog shared by the UI and workers. Set up after BApplication,
// before starting threads; language changes take effect on the next launch.
BCatalog* WallhavenCatalog();
void InitializeLocalization(const std::string& language);

#undef B_CATALOG
#define B_CATALOG WallhavenCatalog()

#endif
