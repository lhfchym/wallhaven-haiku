#include "Backend.h"

#include <Message.h>
#include <File.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>

using namespace wallhaven;

// Throw rather than assert/abort: failures must not open Haiku Debugger dialogs.
#define CHECK(condition) do { if (!(condition)) \
    throw std::runtime_error(std::string("line ") + std::to_string(__LINE__) \
        + ": " #condition); } while (0)

static void RemoveTree(const std::string& path)
{
    DIR* dir = opendir(path.c_str());
    if (dir) {
        struct dirent* item;
        while ((item = readdir(dir)) != NULL) {
            if (strcmp(item->d_name, ".") == 0 || strcmp(item->d_name, "..") == 0) continue;
            std::string child = path + "/" + item->d_name;
            struct stat info;
            if (lstat(child.c_str(), &info) == 0 && S_ISDIR(info.st_mode)) RemoveTree(child);
            else unlink(child.c_str());
        }
        closedir(dir);
    }
    rmdir(path.c_str());
}

struct TemporaryStorage {
    std::string path;
    TemporaryStorage() {
        char name[] = "/tmp/wallhaven-tests-XXXXXX";
        char* directory = mkdtemp(name);
        if (!directory) throw std::runtime_error("Cannot create isolated test directory");
        path = directory;
    }
    ~TemporaryStorage() { RemoveTree(path); }
};

static Wallpaper CheckParser()
{
    std::string error;
    SearchResult result;
    const char* fixture = R"({"data":[null,{"id":"bad","path":"file:///boot/home/private","dimension_x":1920,"dimension_y":1080},{"id":"abc123","dimension_x":3840,"dimension_y":2160,"path":"https://w.wallhaven.cc/full/ab/wallhaven-abc123.jpg","thumbs":{"large":"https://th.wallhaven.cc/lg/ab/abc123.jpg"},"file_type":"image/jpeg"}],"meta":{"current_page":1,"last_page":2,"total":25,"seed":"abcdEF"}})";
    CHECK(Backend::ParseSearchResponse(fixture, 1920, 1080, result, error));
    CHECK(result.wallpapers.size() == 1);
    CHECK(result.seed == "abcdEF" && result.page == 1 && result.lastPage == 2 && result.total == 25);
    Wallpaper wallpaper = result.wallpapers[0];
    CHECK(!wallpaper.thumbnailURL.empty());
    CHECK(wallpaper.fitTier == 0 && wallpaper.fitScore > .99);
    CHECK(!Backend::ParseSearchResponse("{}", 1920, 1080, result, error));
    CHECK(!error.empty());
    CHECK(Backend::ParseSearchResponse(R"({"data":[],"meta":{"current_page":1,"last_page":1,"total":0}})",
        1920, 1080, result, error));
    CHECK(result.wallpapers.empty() && result.total == 0);
    Wallpaper fit;
    fit.width = 1280; fit.height = 720;
    Backend::EvaluateFit(fit, 1920, 1080);
    CHECK(fit.fitTier == 3 && fit.fitScore > .29 && fit.fitScore < .31);
    fit.width = 3840; fit.height = 2400;
    Backend::EvaluateFit(fit, 1920, 1080);
    CHECK(fit.fitTier == 1);
    Backend::EvaluateFit(fit, 0, 0);
    CHECK(fit.fitTier == 3 && fit.fitScore == 0);
    puts("PASS: JSON item isolation, valid CDN URLs, empty/invalid responses, paging and fit scoring");
    return wallpaper;
}

static void CheckOffline(Backend& backend, const TemporaryStorage& storage, const Wallpaper& wallpaper)
{
    std::string error, path;
    struct stat infoDirectory;
    Preferences prefs = backend.LoadPreferences();
    CHECK(prefs.categories == "100" && prefs.purity == "100" && prefs.language == "en");
    prefs.language = "zh-Hans"; // Existing explicit Chinese choice survives reload.
    prefs.query = "forest 雪 & rain";
    prefs.apiKey = "test-only-key";
    prefs.strictAspectRatio = true;
    CHECK(backend.SavePreferences(prefs, error));
    CHECK(stat((storage.path + "/settings").c_str(), &infoDirectory) == 0);
    CHECK(S_ISDIR(infoDirectory.st_mode) && (infoDirectory.st_mode & 0777) == 0700);
    Backend nested(storage.path + "/nested/child");
    CHECK(nested.SavePreferences(prefs, error));
    std::string blockedRoot = storage.path + "/blocked";
    CHECK(mkdir(blockedRoot.c_str(), 0700) == 0);
    FILE* blocker = fopen((blockedRoot + "/settings").c_str(), "w");
    CHECK(blocker != NULL); fclose(blocker);
    Backend blocked(blockedRoot);
    CHECK(!blocked.SavePreferences(prefs, error));

    Backend reopened(storage.path);
    Preferences read = reopened.LoadPreferences();
    CHECK(read.language == "zh-Hans");
    CHECK(read.query == prefs.query && read.apiKey == prefs.apiKey && read.strictAspectRatio);
    struct stat info;
    CHECK(stat((storage.path + "/settings/preferences").c_str(), &info) == 0);
    CHECK((info.st_mode & 0777) == 0600);
    CHECK(backend.DownloadDirectory(prefs) == storage.path + "/downloads");
    CHECK(backend.LoadDownloaded(prefs).empty());
    prefs.sorting = "invalid";
    CHECK(!backend.SavePreferences(prefs, error));
    CHECK(reopened.LoadPreferences().sorting == "random");
    CHECK(backend.RecordDisplayed({wallpaper, wallpaper}, error));
    CHECK(backend.RecordSet(wallpaper, error));
    BFile file((storage.path + "/settings/history").c_str(), B_READ_ONLY);
    BMessage history, entry;
    CHECK(history.Unflatten(&file) == B_OK);
    CHECK(history.FindMessage("entries", 0, &entry) == B_OK);
    bool applied = false;
    CHECK(entry.FindBool("applied", &applied) == B_OK && applied);
    CHECK(history.FindMessage("entries", 1, &entry) != B_OK);
    prefs = Preferences();
    prefs.purity = "111";
    SearchResult result;
    CHECK(!backend.Search(prefs, 1280, 800, 1, "", result, error));
    CHECK(error.find("API key") != std::string::npos);
    Wallpaper invalid = wallpaper;
    invalid.id = "../../invalid";
    CHECK(!backend.EnsureThumbnail(invalid, path, error));
    CHECK(!backend.Download(invalid, prefs, path, error));
    puts("PASS: isolated Unicode preferences, 0600 mode, invalid settings, history deduplication, request validation");
}

static void CheckLive(Backend& backend)
{
    Preferences prefs;
    prefs.query = "nature";
    SearchResult result;
    std::string path, error;
    if (!backend.Search(prefs, 1280, 800, 1, "", result, error)) throw std::runtime_error("Live search: " + error);
    CHECK(!result.wallpapers.empty());
    printf("PASS: live search (%zu wallpapers)\n", result.wallpapers.size());
    // Minimize bandwidth while exercising an actual full-resolution image.
    Wallpaper selected = result.wallpapers[0];
    for (const auto& item : result.wallpapers)
        if (item.fileSize > 0 && (selected.fileSize <= 0 || item.fileSize < selected.fileSize)) selected = item;
    if (!backend.EnsureThumbnail(selected, path, error)) throw std::runtime_error("Live thumbnail: " + error);
    if (!backend.Download(selected, prefs, path, error)) throw std::runtime_error("Live original: " + error);
    std::string originalPath = path;
    CHECK(backend.Download(selected, prefs, path, error) && originalPath == path);
    auto local = backend.LoadDownloaded(prefs);
    CHECK(local.size() == 1 && local[0].id == selected.id);
    CHECK(backend.EnsureThumbnail(local[0], path, error) && path == originalPath);
    SearchResult page2;
    if (!backend.Search(prefs, 1280, 800, 2, result.seed, page2, error)) throw std::runtime_error("Live page 2: " + error);
    CHECK(page2.page == 2 && page2.seed == result.seed);
    puts("PASS: live thumbnail/original, cache reuse, offline enumeration, seeded second page");
}

int main(int argc, char** argv)
{
    setbuf(stdout, NULL);
    if (argc > 2 || (argc == 2 && strcmp(argv[1], "--live") != 0)) {
        fprintf(stderr, "Usage: %s [--live]\nDefault: offline isolated regression tests. --live also uses Wallhaven HTTPS.\n", argv[0]);
        return 2;
    }
    try {
        TemporaryStorage storage;
        Backend backend(storage.path);
        Wallpaper wallpaper = CheckParser();
        CheckOffline(backend, storage, wallpaper);
        if (argc == 2) CheckLive(backend);
    } catch (const std::exception& error) {
        fprintf(stderr, "FAIL: %s\n", error.what());
        return 1;
    }
    puts("PASS: all checks; temporary files removed, user data and desktop untouched");
    return 0;
}
