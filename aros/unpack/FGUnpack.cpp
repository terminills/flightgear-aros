// FGUnpack [ARCHIVE|URL] [DRAWER]: puts FlightGear's data into DRAWER, by default the drawer "fgdata" beside this
// program, which is where fgfs looks for it. With no argument, or with a http(s) address, the data archive
// (FlightGear-<version>-data.txz) is downloaded and unpacked as it arrives; nothing but the unpacked files is
// written, so the disk needs room for those only. With a file name that archive (.txz, .zip or .tar.gz) is unpacked.
// The archive's own top directory is left out. Uses SimGear's HTTP client and archive reader, the ones fgfs
// fetches and unpacks scenery with. Returns 0 when the whole archive has been unpacked, 20 otherwise.
#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <openssl/evp.h>
#include <simgear/io/HTTPClient.hxx>
#include <simgear/io/HTTPRequest.hxx>
#include <simgear/io/untar.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/sg_dir.hxx>
#include <simgear/debug/logstream.hxx>

extern "C" void fgaros_no_requesters(void);

// The data archive of the FlightGear release this fgfs is, and the SHA-256 of the copy this port was tested with
static const char dataURL[] = "https://download.flightgear.org/release-2024.1/FlightGear-2024.1.7-data.txz";
static const char dataSHA256[] = "46e831109911a067f34b7b14d01f8a2fce8a783c84738e8e203760ceb3406ac4";
// A download that delivers nothing for this long is given up
static const int stallSeconds = 180;

// "Work:Stuff/file" as the C library's Unix path syntax has it ("/Work/Stuff/file"); other names stay
static std::string unixPath(const std::string& name)
{
    const auto colon = name.find(':');
    if (colon == std::string::npos || colon == 0 || name[0] == '/')
        return name;
    std::string r = "/" + name.substr(0, colon);
    if (colon + 1 < name.size())
        r += "/" + name.substr(colon + 1);
    return r;
}

static bool isURL(const std::string& s)
{
    return s.compare(0, 7, "http://") == 0 || s.compare(0, 8, "https://") == 0;
}

// Takes the archive's bytes in order: unpacks them, adds them to the checksum and prints the progress
class Unpacker
{
public:
    explicit Unpacker(const SGPath& root) : _ex(root), _md(EVP_MD_CTX_new())
    {
        _ex.setRemoveTopmostDirectory(true);
        EVP_DigestInit_ex(_md, EVP_sha256(), nullptr);
    }
    ~Unpacker() { EVP_MD_CTX_free(_md); }

    void setTotal(double total) { _total = total; }
    double done() const { return _done; }
    bool hasError() const { return _ex.hasError(); }
    std::string lastFile() const { return _ex.mostRecentExtractedPath().utf8Str(); }

    void feed(const uint8_t* bytes, size_t count)
    {
        if (_ex.hasError())
            return;
        EVP_DigestUpdate(_md, bytes, count);
        _ex.extractBytes(bytes, count);
        _done += count;
        if (_total > 0) {
            const int percent = (int)(_done * 100.0 / _total);
            if (percent != _shown) {
                _shown = percent;
                printf("FGUnpack: %d%%\n", percent);
                fflush(stdout);
            }
        } else if ((int)(_done / 104857600.0) != _shown) {
            _shown = (int)(_done / 104857600.0);
            printf("FGUnpack: %d MB\n", _shown * 100);
            fflush(stdout);
        }
    }

    // true when the archive was complete and every file of it has been written
    bool finish()
    {
        if (_ex.hasError())
            return false;
        _ex.flush();
        return !_ex.hasError() && _ex.isAtEndOfArchive();
    }

    std::string sha256()
    {
        unsigned char d[EVP_MAX_MD_SIZE];
        unsigned int n = 0;
        char hex[2 * EVP_MAX_MD_SIZE + 1] = "";
        EVP_DigestFinal_ex(_md, d, &n);
        for (unsigned int i = 0; i < n; ++i)
            snprintf(hex + 2 * i, 3, "%02x", d[i]);
        return hex;
    }

private:
    simgear::ArchiveExtractor _ex;
    EVP_MD_CTX* _md;
    double _total = 0, _done = 0;
    int _shown = -1;
};

class DataRequest : public simgear::HTTP::Request
{
public:
    DataRequest(const std::string& url, Unpacker& u) : simgear::HTTP::Request(url), _unpacker(u) {}
    bool finished = false;
    time_t lastData = time(nullptr);

protected:
    void gotBodyData(const char* s, int n) override
    {
        lastData = time(nullptr);
        if (responseCode() != 200)
            return;
        if (_unpacker.done() == 0)
            _unpacker.setTotal(responseLength());
        _unpacker.feed((const uint8_t*)s, n);
    }
    void onDone() override { finished = true; }
    void onFail() override { finished = true; }

private:
    Unpacker& _unpacker;
};

static int failed(Unpacker& u, const char* why)
{
    printf("FGUnpack: FAILED after %.0f MB of the archive (%s); last file: %s\n", u.done() / 1048576.0, why,
           u.lastFile().c_str());
    return 20;
}

int main(int argc, char** argv)
{
    fgaros_no_requesters();
    if (argc > 3 || (argc > 1 && !strcmp(argv[1], "?"))) {
        printf("Usage: FGUnpack [ARCHIVE|URL] [DRAWER]\n"
               "Unpacks FlightGear's data archive into DRAWER (default: PROGDIR:fgdata).\n"
               "With no ARCHIVE the archive is downloaded and unpacked as it arrives:\n  %s\n", dataURL);
        return 20;
    }
    const std::string source = argc > 1 ? argv[1] : dataURL;
    const SGPath root(argc == 3 ? unixPath(argv[2]) : std::string("/PROGDIR/fgdata"));

    simgear::Dir dir(root);
    if (!dir.exists() && !dir.create(0755)) {
        printf("FGUnpack: cannot make the drawer %s\n", root.c_str());
        return 20;
    }
    sglog().setLogLevels(SG_ALL, SG_WARN);
    Unpacker unpacker(root);

    if (isURL(source)) {
        printf("FGUnpack: %s into %s\n", source.c_str(), root.c_str());
        fflush(stdout);
        simgear::HTTP::Client client;
        client.setUserAgent("FGUnpack (AROS)");
        SGSharedPtr<DataRequest> req(new DataRequest(source, unpacker));
        client.makeRequest(req);
        const char* why = nullptr;
        while (!req->finished) {
            client.update(200);
            if (unpacker.hasError())
                why = "the archive is damaged or the disk is full";
            else if (time(nullptr) - req->lastData > stallSeconds)
                why = "no data from the server";
            if (why) {
                client.cancelRequest(req, why);
                break;
            }
        }
        if (why)
            return failed(unpacker, why);
        if (req->readyState() != simgear::HTTP::Request::DONE || req->responseCode() != 200) {
            printf("FGUnpack: the download FAILED after %.0f MB: %d %s\n", unpacker.done() / 1048576.0,
                   req->responseCode(), req->responseReason().c_str());
            return 20;
        }
    } else {
        const std::string archive = unixPath(source);
        struct stat st;
        FILE* in = fopen(archive.c_str(), "rb");
        if (!in || stat(archive.c_str(), &st) != 0) {
            printf("FGUnpack: cannot open %s\n", source.c_str());
            return 20;
        }
        unpacker.setTotal(st.st_size > 0 ? (double)st.st_size : 1.0);
        printf("FGUnpack: %s (%.0f MB) into %s\n", source.c_str(), st.st_size / 1048576.0, root.c_str());
        std::vector<uint8_t> buf(1 << 20);
        size_t n;
        while ((n = fread(buf.data(), 1, buf.size(), in)) > 0 && !unpacker.hasError())
            unpacker.feed(buf.data(), n);
        const bool readError = ferror(in) != 0;
        fclose(in);
        if (readError)
            return failed(unpacker, "read error");
    }

    if (!unpacker.finish())
        return failed(unpacker, unpacker.hasError() ? "the archive is damaged or the disk is full"
                                                    : "the archive ends early");
    const std::string sum = unpacker.sha256();
    const bool known = sum == dataSHA256;
    printf("FGUnpack: done, %.0f MB read, SHA-256 %s%s\n", unpacker.done() / 1048576.0, sum.c_str(),
           known ? " (the FlightGear 2024.1.7 data this port was tested with)" : "");
    if (!known && source == dataURL) {
        printf("FGUnpack: that is not the archive this port was tested with (%s)\n", dataSHA256);
        return 20;
    }
    return 0;
}
