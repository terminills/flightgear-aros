/* Declarations of the functions libfgaros.a adds for the FlightGear port: what SimGear and FlightGear expect
   from a POSIX C library and AROS's does not have. Force-included by the build scripts. */
#ifndef FGAROS_H
#define FGAROS_H

#include <stddef.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

time_t timegm(struct tm *tm);
int fgaros_nameservers(char *buf, size_t size);

#ifdef __cplusplus
}
#endif

#endif
