/*
 * mmap() for the FlightGear port (libfgaros.a). AROS has no memory-mapped files: the one case served is a
 * private read-only mapping of a file, which is read into allocated memory. Anything else fails with ENOTSUP.
 */
#ifndef FGAROS_SYS_MMAN_H
#define FGAROS_SYS_MMAN_H

#include <sys/types.h>

#define PROT_NONE   0
#define PROT_READ   1
#define PROT_WRITE  2
#define PROT_EXEC   4

#define MAP_SHARED  1
#define MAP_PRIVATE 2

#define MAP_FAILED  ((void *)-1)

#ifdef __cplusplus
extern "C" {
#endif
void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset);
int munmap(void *addr, size_t len);
#ifdef __cplusplus
}
#endif

#endif
