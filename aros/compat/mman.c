/* mmap() as far as the FlightGear port needs it: see include/sys/mman.h. */
#include <sys/mman.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset)
{
    char *p;
    off_t was;
    size_t got = 0;

    if (len == 0) {
        errno = EINVAL;
        return MAP_FAILED;
    }
    if (addr != NULL || prot != PROT_READ || flags != MAP_PRIVATE || fd < 0) {
        errno = ENOTSUP;
        return MAP_FAILED;
    }
    was = lseek(fd, 0, SEEK_CUR);
    if (was == (off_t)-1 || lseek(fd, offset, SEEK_SET) == (off_t)-1)
        return MAP_FAILED;
    p = malloc(len);
    if (p == NULL) {
        errno = ENOMEM;
        return MAP_FAILED;
    }
    while (got < len) {
        ssize_t n = read(fd, p + got, len - got);
        if (n < 0) {
            int e = errno;
            free(p);
            lseek(fd, was, SEEK_SET);
            errno = e;
            return MAP_FAILED;
        }
        if (n == 0)
            break;
        got += n;
    }
    /* past the end of the file a mapping reads as zero */
    memset(p + got, 0, len - got);
    lseek(fd, was, SEEK_SET);
    return p;
}

int munmap(void *addr, size_t len)
{
    (void)len;
    if (addr == NULL || addr == MAP_FAILED) {
        errno = EINVAL;
        return -1;
    }
    free(addr);
    return 0;
}
