// sgnet [lookup] [naptr] [url URL]... [thread] [dns NAMESERVER] [debug]: the network calls FlightGear's TerraSync
// makes, through the same code: lookup = a host lookup, naptr = SimGear's DNS client (NAPTR records of
// terrasync.flightgear.org), url = SimGear's HTTP client (libcurl); thread = all of it again in a second thread,
// as TerraSync runs in one. Each step prints one line that starts with "sgnet:"; the program returns the number
// of failed steps. It also prints the umask it starts with and sets 0123, as tests/twice does.
#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <simgear/io/DNSClient.hxx>
#include <simgear/io/HTTPClient.hxx>
#include <simgear/io/HTTPMemoryRequest.hxx>
#include <simgear/threads/SGThread.hxx>
#include <simgear/debug/logstream.hxx>

static std::string nameserver;
static std::vector<std::string> urls;
static bool do_lookup, do_naptr, do_thread;

static int lookup(const char *who, const char *host)
{
    struct hostent *h = gethostbyname(host);
    if (!h || !h->h_addr_list[0]) {
        printf("sgnet: %s: lookup %s FAILED h_errno %d\n", who, host, h_errno);
        return 1;
    }
    struct in_addr a;
    memcpy(&a, h->h_addr_list[0], sizeof a);
    printf("sgnet: %s: lookup %s = %s\n", who, host, inet_ntoa(a));
    return 0;
}

static int naptr(const char *who)
{
    using namespace simgear;
    DNS::NAPTRRequest *q = new DNS::NAPTRRequest("terrasync.flightgear.org");
    q->qservice = "ws20";
    q->qflags = "U";
    DNS::Request_ptr r(q);
    DNS::Client client{nameserver};
    time_t t0 = time(NULL);
    client.makeRequest(r);
    while (!r->isComplete() && !r->isTimeout() && time(NULL) - t0 < 40)
        client.update(0);
    if (q->entries.empty()) {
        printf("sgnet: %s: NAPTR FAILED after %ld s (complete %d, timeout %d)\n", who, (long)(time(NULL) - t0),
               (int)r->isComplete(), (int)r->isTimeout());
        return 1;
    }
    for (const auto& e : q->entries)
        printf("sgnet: %s: NAPTR %d %d %s %s %s\n", who, e->order, e->preference, e->flags.c_str(),
               e->service.c_str(), e->regexp.c_str());
    return 0;
}

static int get(const char *who, const std::string& url)
{
    using namespace simgear;
    HTTP::Client client;
    client.setUserAgent("sgnet-aros");
    HTTP::MemoryRequestRef r(new HTTP::MemoryRequest(url));
    time_t t0 = time(NULL);
    client.makeRequest(r);
    while (!r->isComplete() && time(NULL) - t0 < 60)
        client.update(100);
    const std::string& body = r->responseBody();
    bool ok = r->isComplete() && r->responseCode() == 200 && !body.empty();
    printf("sgnet: %s: GET %s %s: code %d (%s), %lu bytes, %ld s\n", who, url.c_str(), ok ? "ok" : "FAILED",
           r->responseCode(), r->responseReason().c_str(), (unsigned long)body.size(), (long)(time(NULL) - t0));
    if (ok) {
        std::string first = body.substr(0, body.find('\n', body.find('\n') + 1));
        for (auto& c : first) if (c == '\n') c = '|';
        printf("sgnet: %s: body starts \"%s\"\n", who, first.c_str());
    }
    return ok ? 0 : 1;
}

static int all(const char *who)
{
    int bad = 0;
    if (do_lookup) bad += lookup(who, "terrasync.b-cdn.net");
    if (do_naptr) bad += naptr(who);
    for (const auto& u : urls)
        bad += get(who, u);
    return bad;
}

class Worker : public SGThread
{
public:
    int bad = -1;
    void run() override { bad = all("thread"); }
};

int main(int argc, char **argv)
{
    mode_t old = umask(0123);
    setvbuf(stdout, NULL, _IONBF, 0);
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "dns") && i + 1 < argc) nameserver = argv[++i];
        else if (!strcmp(argv[i], "url") && i + 1 < argc) urls.push_back(argv[++i]);
        else if (!strcmp(argv[i], "lookup")) do_lookup = true;
        else if (!strcmp(argv[i], "naptr")) do_naptr = true;
        else if (!strcmp(argv[i], "thread")) do_thread = true;
        else if (!strcmp(argv[i], "debug")) sglog().setLogLevels(SG_ALL, SG_DEBUG);
    }
    printf("sgnet: starts with umask %03o%s; lookup %d, naptr %d, %d url(s), thread %d, nameserver \"%s\"\n",
           (unsigned)old, old == 0123 ? " = the base of the run before" : "", (int)do_lookup, (int)do_naptr,
           (int)urls.size(), (int)do_thread, nameserver.c_str());
    int bad = all("main");
    if (do_thread) {
        Worker w;
        w.start();
        w.join();
        bad += w.bad < 0 ? 1 : w.bad;
    }
    printf("sgnet: done, %d failed\n", bad);
    return bad;
}
