/*
 * xmmcheck [seconds] [avx|both] [debug] [wait SECONDS]: loads the sixteen vector registers with fixed patterns and spins, storing and
 * comparing them. Nothing in the loop writes them, so a register that differs was changed under the program:
 * by an interrupt that did not put it back, or by a task switch. Prints every difference and a count every
 * ten seconds. With "avx" the 256-bit registers are checked instead of the 128-bit ones; with "both" a
 * round of each in turn; with "debug" the lines also go to the debug output (for a copy started with Run);
 * "wait" sleeps first.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdarg.h>
#include <aros/debug.h>

static int debugout;
/* to the debug output as well: a program started with Run has no output there */
static char line[1024];
static int linelen;
static void out(const char *fmt, ...)
{
    va_list a;
    va_start(a, fmt);
    linelen += vsnprintf(line + linelen, sizeof(line) - linelen, fmt, a);
    va_end(a);
    if (linelen >= (int)sizeof(line)) linelen = sizeof(line) - 1;
    if (linelen && line[linelen - 1] == '\n') {
        fputs(line, stdout); fflush(stdout);
        if (debugout) kprintf("%s", line);
        linelen = 0;
    }
}

#define ROUND 2000000UL

static uint8_t pat[16][32] __attribute__((aligned(32)));
static uint8_t got[16][32] __attribute__((aligned(32)));

/* returns the iterations that were left when a difference was seen, 0 for a clean round */
static unsigned long spin_sse(unsigned long n)
{
    __asm__ volatile(
        "movdqa    0(%[p]), %%xmm0\n  movdqa  32(%[p]), %%xmm1\n  movdqa  64(%[p]), %%xmm2\n  movdqa  96(%[p]), %%xmm3\n"
        "movdqa  128(%[p]), %%xmm4\n  movdqa 160(%[p]), %%xmm5\n  movdqa 192(%[p]), %%xmm6\n  movdqa 224(%[p]), %%xmm7\n"
        "movdqa  256(%[p]), %%xmm8\n  movdqa 288(%[p]), %%xmm9\n  movdqa 320(%[p]), %%xmm10\n movdqa 352(%[p]), %%xmm11\n"
        "movdqa  384(%[p]), %%xmm12\n movdqa 416(%[p]), %%xmm13\n movdqa 448(%[p]), %%xmm14\n movdqa 480(%[p]), %%xmm15\n"
        "1:\n"
        "movdqa %%xmm0,    0(%[g])\n  movdqa %%xmm1,   32(%[g])\n  movdqa %%xmm2,   64(%[g])\n  movdqa %%xmm3,   96(%[g])\n"
        "movdqa %%xmm4,  128(%[g])\n  movdqa %%xmm5,  160(%[g])\n  movdqa %%xmm6,  192(%[g])\n  movdqa %%xmm7,  224(%[g])\n"
        "movdqa %%xmm8,  256(%[g])\n  movdqa %%xmm9,  288(%[g])\n  movdqa %%xmm10, 320(%[g])\n  movdqa %%xmm11, 352(%[g])\n"
        "movdqa %%xmm12, 384(%[g])\n  movdqa %%xmm13, 416(%[g])\n  movdqa %%xmm14, 448(%[g])\n  movdqa %%xmm15, 480(%[g])\n"
        "xor %%ecx, %%ecx\n"
        "2:\n"
        "mov (%[p],%%rcx,8), %%rax\n"
        "cmp (%[g],%%rcx,8), %%rax\n"
        "jne 3f\n"
        "mov 8(%[p],%%rcx,8), %%rax\n"
        "cmp 8(%[g],%%rcx,8), %%rax\n"
        "jne 3f\n"
        "add $4, %%ecx\n"
        "cmp $64, %%ecx\n"
        "jne 2b\n"
        "dec %[n]\n"
        "jnz 1b\n"
        "3:\n"
        : [n] "+r" (n)
        : [p] "r" (pat), [g] "r" (got)
        : "rax", "rcx", "cc", "memory",
          "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7",
          "xmm8", "xmm9", "xmm10", "xmm11", "xmm12", "xmm13", "xmm14", "xmm15");
    return n;
}

static unsigned long spin_avx(unsigned long n)
{
    __asm__ volatile(
        "vmovdqa   0(%[p]), %%ymm0\n  vmovdqa  32(%[p]), %%ymm1\n  vmovdqa  64(%[p]), %%ymm2\n  vmovdqa  96(%[p]), %%ymm3\n"
        "vmovdqa 128(%[p]), %%ymm4\n  vmovdqa 160(%[p]), %%ymm5\n  vmovdqa 192(%[p]), %%ymm6\n  vmovdqa 224(%[p]), %%ymm7\n"
        "vmovdqa 256(%[p]), %%ymm8\n  vmovdqa 288(%[p]), %%ymm9\n  vmovdqa 320(%[p]), %%ymm10\n vmovdqa 352(%[p]), %%ymm11\n"
        "vmovdqa 384(%[p]), %%ymm12\n vmovdqa 416(%[p]), %%ymm13\n vmovdqa 448(%[p]), %%ymm14\n vmovdqa 480(%[p]), %%ymm15\n"
        "1:\n"
        "vmovdqa %%ymm0,    0(%[g])\n vmovdqa %%ymm1,   32(%[g])\n vmovdqa %%ymm2,   64(%[g])\n vmovdqa %%ymm3,   96(%[g])\n"
        "vmovdqa %%ymm4,  128(%[g])\n vmovdqa %%ymm5,  160(%[g])\n vmovdqa %%ymm6,  192(%[g])\n vmovdqa %%ymm7,  224(%[g])\n"
        "vmovdqa %%ymm8,  256(%[g])\n vmovdqa %%ymm9,  288(%[g])\n vmovdqa %%ymm10, 320(%[g])\n vmovdqa %%ymm11, 352(%[g])\n"
        "vmovdqa %%ymm12, 384(%[g])\n vmovdqa %%ymm13, 416(%[g])\n vmovdqa %%ymm14, 448(%[g])\n vmovdqa %%ymm15, 480(%[g])\n"
        "xor %%ecx, %%ecx\n"
        "2:\n"
        "mov (%[p],%%rcx,8), %%rax\n"
        "cmp (%[g],%%rcx,8), %%rax\n"
        "jne 3f\n"
        "inc %%ecx\n"
        "cmp $64, %%ecx\n"
        "jne 2b\n"
        "dec %[n]\n"
        "jnz 1b\n"
        "3:\n"
        "vzeroupper\n"
        : [n] "+r" (n)
        : [p] "r" (pat), [g] "r" (got)
        : "rax", "rcx", "cc", "memory",
          "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7",
          "xmm8", "xmm9", "xmm10", "xmm11", "xmm12", "xmm13", "xmm14", "xmm15");
    return n;
}

static void hex(const uint8_t *b, int n)
{
    for (int i = n - 1; i >= 0; i--) out("%02x", b[i]);
}

int main(int argc, char **argv)
{
    long seconds = argc > 1 ? atol(argv[1]) : 600, wait = 0;
    int mode = 0;       /* 0: xmm, 1: ymm, 2: a round of each in turn */
    unsigned long rounds = 0, bad = 0;
    unsigned perreg[2][16] = {{0}};
    static const char *const name[2] = { "xmm", "ymm" };

    for (int i = 2; i < argc; i++) {
        if (!strcmp(argv[i], "avx")) mode = 1;
        if (!strcmp(argv[i], "both")) mode = 2;
        if (!strcmp(argv[i], "debug")) debugout = 1;
        if (!strcmp(argv[i], "wait") && i + 1 < argc) wait = atol(argv[++i]);
    }
    /* a pattern no code would leave behind by chance: register number and byte position in every byte */
    for (int r = 0; r < 16; r++)
        for (int i = 0; i < 32; i++)
            pat[r][i] = (uint8_t)(0xA0 + r) ^ (uint8_t)(i * 7 + 1);

    out("xmmcheck: %s, %ld s after %ld s\n", mode == 2 ? "xmm0-15 and ymm0-15 in turn" : mode ? "ymm0-15" : "xmm0-15",
        seconds, wait);
    if (wait) sleep(wait);
    time_t start = time(NULL), last = start;
    for (;;) {
        int avx = mode == 2 ? (int)(rounds & 1) : mode, width = avx ? 32 : 16;
        unsigned long left = avx ? spin_avx(ROUND) : spin_sse(ROUND);
        time_t now = time(NULL);
        rounds++;
        if (left) {
            bad++;
            for (int r = 0; r < 16; r++) {
                if (!memcmp(pat[r], got[r], width)) continue;
                perreg[avx][r]++;
                if (bad <= 12) {
                    out("xmmcheck: CHANGED at %ld s: %s%d was ", (long)(now - start), name[avx], r);
                    hex(pat[r], width); out(" is "); hex(got[r], width); out("\n");
                }
            }
        }
        if (now - last >= 10 || now - start >= seconds) {
            out("xmmcheck: %ld s, %lu rounds, %lu with a changed register", (long)(now - start), rounds, bad);
            for (int m = 0; m < 2; m++)
                for (int r = 0; r < 16; r++) if (perreg[m][r]) out(" %s%d:%u", name[m], r, perreg[m][r]);
            out("\n");
            last = now;
        }
        if (now - start >= seconds) break;
    }
    out("xmmcheck: done, %lu rounds, %lu with a changed register\n", rounds, bad);
    return bad ? 5 : 0;
}
