#ifndef FGAROS_WRAP_UNISTD_H
#define FGAROS_WRAP_UNISTD_H
#include_next <unistd.h>
#ifdef __cplusplus
extern "C" {
#endif
int gethostname(char *name, size_t len);
#ifdef __cplusplus
}
#endif
#endif
