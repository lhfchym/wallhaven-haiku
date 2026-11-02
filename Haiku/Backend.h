#ifndef WALLHAVEN_BACKEND_H
#define WALLHAVEN_BACKEND_H

#include <stdint.h>
#include <string>
#include <vector>

namespace wallhaven {

struct Preferences {
    std::string query, apiKey, downloadDirectory;
    std::string categories = "100", purity = "100", sorting = "random";
    std::string language = "en";
    bool strictAspectRatio = false;
};

struct Wallpaper {
    std::string id, url, path, thumbnailURL, resolution, category, purity, fileType;
    std::string localPath;
    int width = 0, height = 0;
    int64_t fileSize = 0;
    double fitScore = 0;
    int fitTier = 3;
};

struct SearchResult {
    std::vector<Wallpaper> wallpapers;
    int page = 1, lastPage = 1, total = 0;
    std::string seed;
};

// Synchronous operations. Run network methods on a worker thread.
// Persistence is serialized internally; each network request owns its CURL handle.
class Backend {
public:
    Backend();
    // Alternate storage root for isolated tools/tests; no user settings touched.
    explicit Backend(const std::string& storageRoot);
    Preferences LoadPreferences();
    bool SavePreferences(const Preferences&, std::string& error);
    bool Search(const Preferences&, int displayWidth, int displayHeight,
        int page, const std::string& seed, SearchResult&, std::string& error);
    bool EnsureThumbnail(const Wallpaper&, std::string& localPath, std::string& error);
    bool Download(const Wallpaper&, const Preferences&, std::string& localPath,
        std::string& error);
    std::string DownloadDirectory(const Preferences&) const;
    std::vector<Wallpaper> LoadDownloaded(const Preferences&);
    bool RecordDisplayed(const std::vector<Wallpaper>&, std::string& error);
    bool RecordSet(const Wallpaper&, std::string& error);

    // Pure helpers shared with the regression checks.
    static void EvaluateFit(Wallpaper&, int displayWidth, int displayHeight);
    static bool ParseSearchResponse(const std::string&, int displayWidth,
        int displayHeight, SearchResult&, std::string& error);
private:
    std::string fSettingsDirectory, fCacheDirectory, fDefaultDownloadDirectory;
};

} // namespace wallhaven
#endif
