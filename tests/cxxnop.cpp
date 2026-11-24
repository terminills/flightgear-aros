// cxxnop LABEL: a C++ program that does nothing but what tests/twice does without words: prints the umask it
// starts with and sets 0123. Built three ways by build-tests.sh: cxxnop (plain), cxxnix (-nix), cxxsg (-nix, with
// libfgaros's headers and one SimGear object pulled in, as tests/sgnet is linked). A second word "print" also
// writes a line to stdout.
#include <cstdio>
#include <sys/types.h>
#include <sys/stat.h>
#include <aros/debug.h>
#ifdef WITH_SG
#include <simgear/debug/logstream.hxx>
#endif

int main(int argc, char **argv)
{
    mode_t old = umask(0123);
#ifdef WITH_SG
    if (argc > 5) sglog().setLogLevels(SG_ALL, SG_DEBUG);
#endif
    kprintf("%s: %s: starts with umask %03o%s\n", NAME, argc > 1 ? argv[1] : "?", (unsigned)old,
            old == 0123 ? " = the base of the run before" : "");
    if (argc > 2) { printf("%s: %s: a line on stdout\n", NAME, argv[1]); fflush(stdout); }
    return 0;
}
