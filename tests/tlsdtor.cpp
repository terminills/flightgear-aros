// tlsdtor - is a thread_local object still whole when its destructor runs at the end of a thread?
//
// Each thread fills a thread_local object and an object behind a pthread key of its own with a
// pattern; the destructors count the words that still hold it. C++ thread_local is GCC's emulated
// TLS here: its storage is freed by one pthread key destructor, and libstdc++ runs the thread_local
// destructors from another, made later. A third key, made between the two, has a destructor that
// allocates blocks of the storage's size and fills them: if the storage was freed before the
// thread_local destructor runs, that destructor finds the filling.
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WORDS 8
#define PATTERN 0x5a5aa5a5c3c33c3cUL

static int tlsWhole, tlsBroken, keyWhole, keyBroken;
static unsigned long firstBroken[WORDS];

struct Probe {
    unsigned long word[WORDS];
    bool viaKey;

    explicit Probe(bool k = false) : viaKey(k)
    {
        for (int i = 0; i < WORDS; i++)
            word[i] = PATTERN;
    }
    ~Probe()
    {
        int good = 0;
        for (int i = 0; i < WORDS; i++)
            good += word[i] == PATTERN;
        if (good == WORDS)
            (viaKey ? keyWhole : tlsWhole)++;
        else {
            if (!viaKey && !tlsBroken)
                for (int i = 0; i < WORDS; i++)
                    firstBroken[i] = word[i];
            (viaKey ? keyBroken : tlsBroken)++;
        }
    }
};

static thread_local int warm;
static thread_local Probe tlsProbe;
static pthread_key_t key, fillKey;

// takes what was freed just before, if anything was
static void fill(void*)
{
    void* block[64];
    for (int i = 0; i < 64; i++) {
        block[i] = malloc(sizeof(Probe) + sizeof(void*));
        if (block[i])
            memset(block[i], 0xee, sizeof(Probe) + sizeof(void*));
    }
    for (int i = 0; i < 64; i++)
        free(block[i]);
}

static void destroyKeyProbe(void* p) { delete static_cast<Probe*>(p); }

static void* thread(void*)
{
    tlsProbe.word[0] = PATTERN; // first use: constructs it and registers the destructor
    pthread_setspecific(key, new Probe(true));
    pthread_setspecific(fillKey, &fillKey);
    return nullptr;
}

int main()
{
    const int threads = 16;
    warm = 1; // the first thread_local used: the key of the emulated TLS is made here
    pthread_key_create(&fillKey, fill);
    pthread_key_create(&key, destroyKeyProbe);
    for (int i = 0; i < threads; i++) {
        pthread_t t;
        if (pthread_create(&t, nullptr, thread, nullptr) != 0) {
            printf("tlsdtor: pthread_create failed\n");
            return 20;
        }
        pthread_join(t, nullptr);
    }
    printf("tlsdtor: %d threads; thread_local object whole %d, overwritten %d, destructor not run %d\n",
           threads, tlsWhole, tlsBroken, threads - tlsWhole - tlsBroken);
    printf("tlsdtor: object behind its own pthread key whole %d, overwritten %d, destructor not run %d\n",
           keyWhole, keyBroken, threads - keyWhole - keyBroken);
    if (tlsBroken) {
        printf("tlsdtor: first overwritten object:");
        for (int i = 0; i < WORDS; i++)
            printf(" %016lx", firstBroken[i]);
        printf("\n");
    }
    return tlsBroken || keyBroken ? 5 : 0;
}
