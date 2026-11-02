#define WALLHAVEN_REQUEST_TEST
#include "../Backend.cpp"
#include <stdexcept>
#define CHECK(c) do { if (!(c)) throw std::runtime_error(#c); } while (0)
namespace wallhaven { namespace {
std::string observedURL, observedHeader;
int requests = 0;
bool ObserveRequest(CURL* curl, curl_slist* headers, Transfer& transfer, bool, std::string&)
{
    char* url = NULL;
    curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &url);
    observedURL = url ? url : "";
    observedHeader = headers ? headers->data : "";
    CHECK(!headers || !headers->next);
    requests++;
    transfer.body = R"({"data":[],"meta":{"current_page":1,"last_page":1,"total":0}})";
    return true;
}
} }
int main()
{
    using namespace wallhaven;
    try {
        char root[] = "/tmp/wallhaven-request-XXXXXX";
        CHECK(mkdtemp(root));
        Backend backend(root);
        Preferences prefs; SearchResult result; std::string error;
        prefs.apiKey = "test-only-secret";
        CHECK(backend.Search(prefs, 1280, 800, 1, "", result, error));
        CHECK(observedHeader == "X-API-Key: test-only-secret");
        CHECK(observedURL.find("apikey") == std::string::npos);
        CHECK(observedURL.find(prefs.apiKey) == std::string::npos);
        prefs.apiKey.clear();
        CHECK(backend.Search(prefs, 1280, 800, 1, "", result, error));
        CHECK(observedHeader.empty());
        for (auto key : {std::string("bad\r\nInjected: yes"), std::string("bad\0key", 7)}) {
            prefs.apiKey = key; int before = requests;
            CHECK(!backend.Search(prefs, 1280, 800, 1, "", result, error));
            CHECK(requests == before);
        }
        Transfer transfer;
        CHECK(!Request("https://w.wallhaven.cc/full/ab/wallhaven-abc123.jpg", transfer,
            true, 10, error, "test-only-secret"));
        CHECK(!Request("https://wallhaven.cc.evil.invalid/api/v1/search", transfer,
            false, 10, error, "test-only-secret"));
        CHECK(Request("https://w.wallhaven.cc/full/ab/wallhaven-abc123.jpg", transfer,
            true, 10, error));
        CHECK(observedHeader.empty());
        rmdir((std::string(root) + "/settings").c_str());
        rmdir((std::string(root) + "/thumbnails").c_str()); rmdir(root);
        puts("PASS: actual CURL URL/header configuration, anonymous requests, CDN isolation, injection rejection");
    } catch (const std::exception& e) { fprintf(stderr, "FAIL: %s\n", e.what()); return 1; }
}
