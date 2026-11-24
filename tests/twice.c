/*
 * twice LABEL [thread] [pw] [tpw] [sock] [tsock] [lib NAME]: does a program get a C library state of its own when it is the
 * second one run from the same Shell?
 * Prints the umask the program starts with (022 in a new posixc.library base) and sets it to 0123, so a second run
 * that prints 123 was given the base of the first. The words choose what the run does before it ends:
 *   thread  a pthread that only returns       pw    getpwuid() in the program's task   tpw    getpwuid() in a pthread
 *   sock    opens and closes bsdsocket.library in the program's task                   tsock  the same in a pthread
 *   lib NAME  opens and closes the library NAME in the program's task (the first open after boot loads it)
 * Output goes to the debug log (kprintf), so the program can be started with Run.
 */
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <pthread.h>
#include <proto/exec.h>
#include <aros/debug.h>

static int has(int argc, char **argv, const char *w)
{
    for (int i = 2; i < argc; i++)
        if (!strcmp(argv[i], w)) return 1;
    return 0;
}

static void pw(const char *who)
{
    struct passwd *p = getpwuid(getuid());
    kprintf("twice:   %s: getpwuid %s\n", who, p ? p->pw_name : "(none)");
}

static void sock(const char *who)
{
    struct Library *b = OpenLibrary((CONST_STRPTR)"bsdsocket.library", 4);
    kprintf("twice:   %s: bsdsocket.library %s\n", who, b ? "opened and closed" : "not there");
    if (b) CloseLibrary(b);
}

static int t_pw, t_sock;

static void *work(void *arg)
{
    if (t_pw) pw("thread");
    if (t_sock) sock("thread");
    return arg;
}

int main(int argc, char **argv)
{
    mode_t old = umask(0123);
    char what[128] = "";

    for (int i = 2; i < argc; i++) { strcat(what, " "); strncat(what, argv[i], 20); }
    kprintf("twice: %s: starts with umask %03o%s; does:%s\n", argc > 1 ? argv[1] : "?", (unsigned)old,
            old == 0123 ? " = the base of the run before" : "", what[0] ? what : " nothing");
    if (has(argc, argv, "pw")) pw("main");
    if (has(argc, argv, "sock")) sock("main");
    for (int i = 2; i + 1 < argc; i++)
        if (!strcmp(argv[i], "lib")) {
            struct Library *b = OpenLibrary((CONST_STRPTR)argv[i + 1], 0);
            kprintf("twice:   main: %s %s\n", argv[i + 1], b ? "opened and closed" : "not there");
            if (b) CloseLibrary(b);
        }
    t_pw = has(argc, argv, "tpw"); t_sock = has(argc, argv, "tsock");
    if (has(argc, argv, "thread") || t_pw || t_sock) {
        pthread_t t;
        if (pthread_create(&t, NULL, work, NULL) == 0) pthread_join(t, NULL);
        else kprintf("twice:   no thread\n");
    }
    return 0;
}
