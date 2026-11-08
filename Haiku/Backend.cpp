#include "Localization.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Backend"

#include "Backend.h"

#include <Autolock.h>
#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <Json.h>
#include <Locker.h>
#include <Message.h>
#include <NodeInfo.h>
#include <Path.h>
#include <curl/curl.h>

#include <algorithm>
#include <cmath>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <limits>
#include <mutex>
#include <set>
#include <sys/stat.h>
#include <unistd.h>

namespace wallhaven {
namespace {
BLocker sPersistenceLock("Wallhaven persistence");
std::once_flag sCurlOnce;

std::string Directory(directory_which which, const char* fallback)
{
    BPath path;
    return find_directory(which, &path) == B_OK ? path.Path() : fallback;
}

// Haiku create_directory() can report B_FILE_EXISTS for existing ancestors
// inside a chroot. POSIX path traversal works without Storage Kit node refs.
bool EnsureDirectory(const std::string& path, mode_t mode)
{
    if (path.empty()) return false;
    struct stat info;
    if (stat(path.c_str(), &info) == 0) return S_ISDIR(info.st_mode);
    if (errno != ENOENT) return false;
    size_t slash = path.find_last_of('/');
    if (slash != std::string::npos && slash > 0
        && !EnsureDirectory(path.substr(0, slash), mode)) return false;
    if (mkdir(path.c_str(), mode) == 0) return true;
    return errno == EEXIST && stat(path.c_str(), &info) == 0 && S_ISDIR(info.st_mode);
}

bool ValidAPIKey(const std::string& key)
{
    // Reject header injection (including embedded NUL) without printing the key.
    for (unsigned char c : key) if (c < 33 || c > 126) return false;
    return true;
}

std::string String(const BMessage& message, const char* name)
{
    const char* value = NULL;
    return message.FindString(name, &value) == B_OK ? value : "";
}

int64_t Number(const BMessage& message, const char* name)
{
    double value = 0;
    if (message.FindDouble(name, &value) != B_OK || !std::isfinite(value)
        || value < 0 || value > 1000000000000.0)
        return 0;
    return static_cast<int64_t>(value);
}

bool SafeID(const std::string& id)
{
    return !id.empty() && id.size() <= 32
        && id.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789") == std::string::npos;
}

bool ImageURL(const std::string& url)
{
    // Never follow API-provided file or local-network URLs.
    return url.rfind("https://w.wallhaven.cc/", 0) == 0
        || url.rfind("https://th.wallhaven.cc/", 0) == 0;
}

bool ReadMessage(const std::string& path, BMessage& message)
{
    BFile file(path.c_str(), B_READ_ONLY);
    return file.InitCheck() == B_OK && message.Unflatten(&file) == B_OK;
}

bool WriteMessage(const std::string& path, BMessage& message, std::string& error)
{
    std::string temporary = path + ".XXXXXX";
    std::vector<char> name(temporary.begin(), temporary.end());
    name.push_back(0);
    int fd = mkstemp(name.data());
    if (fd < 0) { error = B_TRANSLATE("Cannot create settings file."); return false; }
    fchmod(fd, 0600);
    close(fd);
    BFile file(name.data(), B_WRITE_ONLY);
    status_t status = file.InitCheck();
    if (status == B_OK) status = message.Flatten(&file);
    if (status == B_OK) status = file.Sync();
    file.Unset();
    if (status != B_OK || rename(name.data(), path.c_str()) != 0) {
        unlink(name.data());
        error = B_TRANSLATE("Cannot save settings. Check free space and directory permissions.");
        return false;
    }
    return true;
}

struct Transfer {
    std::string body;
    FILE* file = NULL;
    size_t size = 0;
    size_t limit = 8 * 1024 * 1024;
};

size_t Receive(char* data, size_t size, size_t count, void* user)
{
    Transfer& transfer = *static_cast<Transfer*>(user);
    if (size != 0 && count > std::numeric_limits<size_t>::max() / size) return 0;
    size_t bytes = size * count;
    if (bytes > transfer.limit - transfer.size) return 0;
    if (transfer.file) {
        if (fwrite(data, 1, bytes, transfer.file) != bytes) return 0;
    } else {
        try { transfer.body.append(data, bytes); } catch (...) { return 0; }
    }
    transfer.size += bytes;
    return bytes;
}

#ifdef WALLHAVEN_REQUEST_TEST
bool ObserveRequest(CURL*, curl_slist*, Transfer&, bool, std::string&);
#endif

bool Request(const std::string& url, Transfer& transfer, bool image,
    long timeout, std::string& error, const std::string& apiKey = "")
{
    error.clear();
    std::call_once(sCurlOnce, [] { curl_global_init(CURL_GLOBAL_DEFAULT); });
    CURL* curl = curl_easy_init();
    if (!curl) { error = B_TRANSLATE("Cannot initialize network connection."); return false; }
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "WallhavenHaiku/1.0");
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 12L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 128L);
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 20L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_PROTOCOLS_STR, "https");
    // API/CDN endpoints already use final HTTPS URLs. Redirects are rejected.
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, Receive);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &transfer);
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
    curl_slist* headers = NULL;
    if (!apiKey.empty()) {
        if (!ValidAPIKey(apiKey) || image
            || url.compare(0, 28, "https://wallhaven.cc/api/v1/") != 0) {
            error = B_TRANSLATE("Invalid API authentication request.");
            curl_easy_cleanup(curl);
            return false;
        }
        headers = curl_slist_append(NULL, ("X-API-Key: " + apiKey).c_str());
        if (!headers) {
            error = B_TRANSLATE("Cannot allocate API authentication header.");
            curl_easy_cleanup(curl);
            return false;
        }
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }
#ifdef WALLHAVEN_REQUEST_TEST
    // Compile-time test transport: never present in the application build.
    bool observed = ObserveRequest(curl, headers, transfer, image, error);
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    return observed;
#endif
    CURLcode result = curl_easy_perform(curl);
    long status = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    char* type = NULL;
    curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &type);
    bool imageType = type && strncmp(type, "image/", 6) == 0;
    if (result != CURLE_OK) {
        // curl_easy_strerror describes the failure without exposing the URL/API key.
        BString message(B_TRANSLATE("Network error: %detail%"));
        message.ReplaceAll("%detail%",curl_easy_strerror(result)); error=message.String();
    } else if (status == 401 || status == 403) {
        error = B_TRANSLATE("Access denied. Check the API key and requested purity filter.");
    } else if (status == 429) {
        error = B_TRANSLATE("Wallhaven rate limit reached. Please wait one minute and try again.");
    } else if (status < 200 || status >= 300) {
        BString message(B_TRANSLATE("Wallhaven returned HTTP %status%."));
        message.ReplaceAll("%status%",std::to_string(status).c_str()); error=message.String();
    } else if (transfer.size == 0 || (image && !imageType)) {
        error = B_TRANSLATE("The server did not return a valid image.");
    }
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    return error.empty();
}

std::string Encode(const std::string& text)
{
    static const char hex[] = "0123456789ABCDEF";
    std::string result;
    for (unsigned char c : text) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
            || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~')
            result += c;
        else { result += '%'; result += hex[c >> 4]; result += hex[c & 15]; }
    }
    return result;
}

bool ValidMask(const std::string& value)
{
    return value.size() == 3 && value != "000"
        && value.find_first_not_of("01") == std::string::npos;
}

bool ValidSorting(const std::string& value)
{
    return value == "random" || value == "date_added" || value == "favorites"
        || value == "relevance" || value == "toplist" || value == "views";
}

bool ValidImageFile(const std::string& path)
{
    struct stat info;
    if (lstat(path.c_str(), &info) != 0 || !S_ISREG(info.st_mode) || info.st_size < 12)
        return false;
    FILE* file = fopen(path.c_str(), "rb");
    if (!file) return false;
    unsigned char bytes[12];
    size_t size = fread(bytes, 1, sizeof(bytes), file);
    fclose(file);
    return size == sizeof(bytes) && ((bytes[0] == 0xff && bytes[1] == 0xd8)
        || memcmp(bytes, "\x89PNG\r\n\x1a\n", 8) == 0
        || (memcmp(bytes, "RIFF", 4) == 0 && memcmp(bytes + 8, "WEBP", 4) == 0));
}

bool FetchFile(const std::string& url, const std::string& destination,
    bool thumbnail, std::string& error)
{
    error.clear();
    if (ValidImageFile(destination)) return true;
    if (!ImageURL(url)) { error = B_TRANSLATE("Invalid wallpaper image URL."); return false; }
    std::string temporary = destination + ".XXXXXX";
    std::vector<char> name(temporary.begin(), temporary.end());
    name.push_back(0);
    int fd = mkstemp(name.data());
    if (fd < 0) { error = B_TRANSLATE("Cannot create image file. Check the download directory."); return false; }
    FILE* file = fdopen(fd, "wb");
    if (!file) { close(fd); unlink(name.data()); error = B_TRANSLATE("Cannot open image file."); return false; }
    Transfer transfer;
    transfer.file = file;
    transfer.limit = (thumbnail ? 8 : 150) * 1024 * 1024;
    bool ok = Request(url, transfer, true, thumbnail ? 30 : 120, error);
    if (fclose(file) != 0 && ok) { ok = false; error = B_TRANSLATE("Cannot finish writing image file."); }
    if (ok && !ValidImageFile(name.data())) { ok = false; error = B_TRANSLATE("Downloaded image is invalid or unsupported."); }
    if (ok && rename(name.data(), destination.c_str()) != 0) {
        ok = false; error = B_TRANSLATE("Cannot move the downloaded image to its destination.");
    }
    if (!ok) unlink(name.data());
    else {
        BFile output(destination.c_str(), B_READ_WRITE);
        BNodeInfo node(&output);
        std::string extension = destination.substr(destination.find_last_of('.') + 1);
        node.SetType(extension == "png" ? "image/png" : extension == "webp" ? "image/webp" : "image/jpeg");
    }
    return ok;
}

bool RecordHistory(const std::string& path, const std::vector<Wallpaper>& wallpapers,
    bool applied, std::string& error)
{
    BMessage old, history;
    ReadMessage(path, old);
    std::set<std::string> ids;
    int count = 0;
    for (const Wallpaper& wallpaper : wallpapers) {
        if (!SafeID(wallpaper.id) || !ids.insert(wallpaper.id).second || count >= 240) continue;
        BMessage entry;
        entry.AddString("id", wallpaper.id.c_str());
        entry.AddString("resolution", wallpaper.resolution.c_str());
        entry.AddBool("applied", applied);
        entry.AddInt64("time", time(NULL));
        history.AddMessage("entries", &entry);
        count++;
    }
    for (int32 index = 0; count < 240; ++index) {
        BMessage entry;
        if (old.FindMessage("entries", index, &entry) != B_OK) break;
        if (!ids.insert(String(entry, "id")).second) continue;
        history.AddMessage("entries", &entry);
        count++;
    }
    return WriteMessage(path, history, error);
}
} // anonymous namespace

Backend::Backend()
    : Backend("")
{
}

Backend::Backend(const std::string& storageRoot)
{
    if (storageRoot.empty()) {
        std::string home = Directory(B_USER_DIRECTORY, "/boot/home");
        fSettingsDirectory = Directory(B_USER_SETTINGS_DIRECTORY, "/boot/home/config/settings") + "/Wallhaven";
        fCacheDirectory = Directory(B_USER_CACHE_DIRECTORY, "/boot/home/config/cache") + "/Wallhaven/thumbnails";
        fDefaultDownloadDirectory = home + "/Pictures/Wallhaven";
    } else {
        fSettingsDirectory = storageRoot + "/settings";
        fCacheDirectory = storageRoot + "/thumbnails";
        fDefaultDownloadDirectory = storageRoot + "/downloads";
    }
    if (EnsureDirectory(fSettingsDirectory, 0700))
        chmod(fSettingsDirectory.c_str(), 0700);
    EnsureDirectory(fCacheDirectory, 0700);
}

Preferences Backend::LoadPreferences()
{
    BAutolock lock(&sPersistenceLock);
    Preferences prefs;
    BMessage message;
    if (!ReadMessage(fSettingsDirectory + "/preferences", message)) return prefs;
    prefs.query = String(message, "query");
    prefs.apiKey = String(message, "apiKey");
    prefs.downloadDirectory = String(message, "downloadDirectory");
    std::string value = String(message, "categories");
    if (ValidMask(value)) prefs.categories = value;
    value = String(message, "purity");
    if (ValidMask(value)) prefs.purity = value;
    value = String(message, "sorting");
    if (ValidSorting(value)) prefs.sorting = value;
    value = String(message, "language");
    if (value == "en" || value == "zh-Hans" || value == "system") prefs.language = value;
    message.FindBool("strictAspectRatio", &prefs.strictAspectRatio);
    return prefs;
}

bool Backend::SavePreferences(const Preferences& prefs, std::string& error)
{
    BAutolock lock(&sPersistenceLock);
    error.clear();
    if (!ValidMask(prefs.categories) || !ValidMask(prefs.purity) || !ValidSorting(prefs.sorting)) {
        error = B_TRANSLATE("Invalid search settings."); return false;
    }
    BMessage message;
    message.AddString("query", prefs.query.c_str());
    message.AddString("apiKey", prefs.apiKey.c_str());
    message.AddString("downloadDirectory", prefs.downloadDirectory.c_str());
    message.AddString("categories", prefs.categories.c_str());
    message.AddString("purity", prefs.purity.c_str());
    message.AddString("sorting", prefs.sorting.c_str());
    message.AddString("language", prefs.language.c_str());
    message.AddBool("strictAspectRatio", prefs.strictAspectRatio);
    return WriteMessage(fSettingsDirectory + "/preferences", message, error);
}

void Backend::EvaluateFit(Wallpaper& wallpaper, int displayWidth, int displayHeight)
{
    wallpaper.fitScore = 0;
    wallpaper.fitTier = 3;
    if (wallpaper.width <= 0 || wallpaper.height <= 0 || displayWidth <= 0 || displayHeight <= 0) return;
    double coverage = std::min(double(wallpaper.width) / displayWidth, double(wallpaper.height) / displayHeight);
    double scale = 1 / coverage;
    double area = wallpaper.width * scale * wallpaper.height * scale;
    double loss = std::max(0.0, std::min(1.0, 1.0 - double(displayWidth) * displayHeight / area));
    bool covers = wallpaper.width >= displayWidth && wallpaper.height >= displayHeight;
    wallpaper.fitScore = (1 - loss) * (covers ? 1 : std::min(coverage, 1.0) * 0.45);
    if (covers) wallpaper.fitTier = loss <= 0.03 ? 0 : loss <= 0.18 ? 1 : 2;
}

bool Backend::ParseSearchResponse(const std::string& json, int width, int height,
    SearchResult& result, std::string& error)
{
    error.clear();
    BMessage root, data, meta;
    if (BJson::Parse(json.c_str(), json.size(), root) != B_OK
        || root.FindMessage("data", &data) != B_OK
        || root.FindMessage("meta", &meta) != B_OK || data.what != '_JTA') {
        error = B_TRANSLATE("Wallhaven returned an invalid search response."); return false;
    }
    SearchResult parsed;
    parsed.page = static_cast<int>(Number(meta, "current_page"));
    parsed.lastPage = static_cast<int>(Number(meta, "last_page"));
    parsed.total = static_cast<int>(std::min<int64_t>(Number(meta, "total"), INT32_MAX));
    parsed.seed = String(meta, "seed");
    if (parsed.page < 1 || parsed.lastPage < 1) {
        error = B_TRANSLATE("Wallhaven returned invalid pagination metadata."); return false;
    }
    std::set<std::string> ids;
    for (int index = 0; index < 1000; ++index) {
        BMessage item, thumbs;
        std::string field = std::to_string(index);
        type_code type;
        int32 count;
        if (data.GetInfo(field.c_str(), &type, &count) != B_OK) break;
        if (data.FindMessage(field.c_str(), &item) != B_OK) continue;
        Wallpaper wallpaper;
        wallpaper.id = String(item, "id");
        wallpaper.path = String(item, "path");
        wallpaper.width = static_cast<int>(Number(item, "dimension_x"));
        wallpaper.height = static_cast<int>(Number(item, "dimension_y"));
        if (!SafeID(wallpaper.id) || !ImageURL(wallpaper.path) || wallpaper.width <= 0
            || wallpaper.height <= 0 || wallpaper.width > 100000 || wallpaper.height > 100000
            || !ids.insert(wallpaper.id).second) continue;
        wallpaper.url = "https://wallhaven.cc/w/" + wallpaper.id;
        wallpaper.resolution = std::to_string(wallpaper.width) + "x" + std::to_string(wallpaper.height);
        wallpaper.category = String(item, "category");
        wallpaper.purity = String(item, "purity");
        wallpaper.fileType = String(item, "file_type");
        wallpaper.fileSize = Number(item, "file_size");
        if (item.FindMessage("thumbs", &thumbs) == B_OK) wallpaper.thumbnailURL = String(thumbs, "large");
        if (!ImageURL(wallpaper.thumbnailURL)) wallpaper.thumbnailURL.clear();
        EvaluateFit(wallpaper, width, height);
        parsed.wallpapers.push_back(wallpaper);
    }
    result = parsed;
    return true;
}

bool Backend::Search(const Preferences& prefs, int width, int height, int page,
    const std::string& seed, SearchResult& result, std::string& error)
{
    error.clear();
    if (!ValidMask(prefs.categories) || !ValidMask(prefs.purity) || !ValidSorting(prefs.sorting)
        || page < 1 || width < 1 || height < 1) {
        error = B_TRANSLATE("Invalid search settings or display dimensions."); return false;
    }
    if (prefs.purity[2] == '1' && prefs.apiKey.empty()) {
        error = B_TRANSLATE("An API key is required to include NSFW wallpapers."); return false;
    }
    std::string url = "https://wallhaven.cc/api/v1/search?q=" + Encode(prefs.query)
        + "&categories=" + prefs.categories + "&purity=" + prefs.purity
        + "&sorting=" + prefs.sorting + "&atleast=" + std::to_string(width) + "x" + std::to_string(height)
        + "&page=" + std::to_string(page);
    if (prefs.strictAspectRatio) {
        int a = width, b = height;
        while (b) { int remainder = a % b; a = b; b = remainder; }
        url += "&ratios=" + std::to_string(width / a) + "x" + std::to_string(height / a);
    }
    if (!seed.empty() && prefs.sorting == "random") url += "&seed=" + Encode(seed);
    Transfer transfer;
    if (!Request(url, transfer, false, 35, error, prefs.apiKey)
        || !ParseSearchResponse(transfer.body, width, height, result, error)) return false;
    // Keep the active random-search session even when the server emits a fresh
    // seed in later responses. Only page-one refreshes should choose a new seed.
    if (prefs.sorting == "random" && !seed.empty()) result.seed = seed;
    std::set<std::string> recent;
    {
        BAutolock lock(&sPersistenceLock);
        BMessage history, entry;
        ReadMessage(fSettingsDirectory + "/history", history);
        for (int32 i = 0; history.FindMessage("entries", i, &entry) == B_OK; ++i)
            recent.insert(String(entry, "id"));
    }
    std::stable_sort(result.wallpapers.begin(), result.wallpapers.end(), [&recent](const Wallpaper& a, const Wallpaper& b) {
        bool ar = recent.count(a.id) != 0, br = recent.count(b.id) != 0;
        if (ar != br) return !ar;
        return a.fitTier < b.fitTier;
    });
    return true;
}

bool Backend::EnsureThumbnail(const Wallpaper& wallpaper, std::string& path, std::string& error)
{
    error.clear();
    if (!wallpaper.localPath.empty() && ValidImageFile(wallpaper.localPath)) {
        path = wallpaper.localPath; return true;
    }
    if (!SafeID(wallpaper.id)) { error = B_TRANSLATE("Invalid wallpaper identifier."); return false; }
    path = fCacheDirectory + "/" + wallpaper.id + ".jpg";
    return FetchFile(wallpaper.thumbnailURL, path, true, error);
}

std::string Backend::DownloadDirectory(const Preferences& prefs) const
{
    return prefs.downloadDirectory.empty() ? fDefaultDownloadDirectory : prefs.downloadDirectory;
}

bool Backend::Download(const Wallpaper& wallpaper, const Preferences& prefs,
    std::string& path, std::string& error)
{
    error.clear();
    if (!wallpaper.localPath.empty() && ValidImageFile(wallpaper.localPath)) {
        path = wallpaper.localPath; return true;
    }
    if (!SafeID(wallpaper.id) || wallpaper.width <= 0 || wallpaper.height <= 0) {
        error = B_TRANSLATE("Invalid wallpaper metadata."); return false;
    }
    std::string directory = DownloadDirectory(prefs);
    if (!EnsureDirectory(directory, 0755)) {
        error = B_TRANSLATE("Cannot create the wallpaper download directory."); return false;
    }
    std::string extension = wallpaper.fileType == "image/png" ? "png"
        : wallpaper.fileType == "image/webp" ? "webp" : "jpg";
    path = directory + "/wallhaven-" + wallpaper.id + "-" + std::to_string(wallpaper.width)
        + "x" + std::to_string(wallpaper.height) + "." + extension;
    return FetchFile(wallpaper.path, path, false, error);
}

std::vector<Wallpaper> Backend::LoadDownloaded(const Preferences& prefs)
{
    std::vector<std::pair<int64_t, Wallpaper> > files;
    std::string directory = DownloadDirectory(prefs);
    DIR* dir = opendir(directory.c_str());
    if (!dir) return {};
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        std::string name = entry->d_name;
        if (name.compare(0, 10, "wallhaven-") != 0) continue;
        size_t dash = name.find('-', 10), dot = name.find_last_of('.');
        if (dash == std::string::npos || dot == std::string::npos || dot <= dash) continue;
        Wallpaper wallpaper;
        wallpaper.id = name.substr(10, dash - 10);
        if (!SafeID(wallpaper.id)) continue;
        wallpaper.localPath = directory + "/" + name;
        if (!ValidImageFile(wallpaper.localPath)) continue;
        wallpaper.resolution = name.substr(dash + 1, dot - dash - 1);
        if (sscanf(wallpaper.resolution.c_str(), "%dx%d", &wallpaper.width, &wallpaper.height) != 2) continue;
        wallpaper.url = "https://wallhaven.cc/w/" + wallpaper.id;
        std::string extension = name.substr(dot + 1);
        wallpaper.fileType = extension == "png" ? "image/png" : extension == "webp" ? "image/webp" : "image/jpeg";
        struct stat info;
        if (stat(wallpaper.localPath.c_str(), &info) != 0) continue;
        wallpaper.fileSize = info.st_size;
        files.push_back(std::make_pair(info.st_mtime, wallpaper));
    }
    closedir(dir);
    std::stable_sort(files.begin(), files.end(), [](const std::pair<int64_t, Wallpaper>& a,
        const std::pair<int64_t, Wallpaper>& b) { return a.first > b.first; });
    std::vector<Wallpaper> result;
    for (const auto& file : files) result.push_back(file.second);
    return result;
}

bool Backend::RecordDisplayed(const std::vector<Wallpaper>& wallpapers, std::string& error)
{
    BAutolock lock(&sPersistenceLock);
    error.clear();
    return RecordHistory(fSettingsDirectory + "/history", wallpapers, false, error);
}

bool Backend::RecordSet(const Wallpaper& wallpaper, std::string& error)
{
    BAutolock lock(&sPersistenceLock);
    error.clear();
    return RecordHistory(fSettingsDirectory + "/history", {wallpaper}, true, error);
}
} // namespace wallhaven
