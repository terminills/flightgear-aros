#ifndef FGAROS_WRAP_SYS_SOCKET_H
#define FGAROS_WRAP_SYS_SOCKET_H
/* <pthread.h> first: seen after the bsdsocket prototypes it wraps connect(), send() ... in macros. */
#include <pthread.h>
#include_next <sys/socket.h>
#include <fgaros_socket.h>

/* The SDK shows the PF_ names only where BSD extensions are visible: C++ (_GNU_SOURCE) has them, a plain C
   compile does not (<aros/features.h> falls back to _XOPEN_SOURCE 500 there). */
#ifndef PF_UNSPEC
#define PF_UNSPEC AF_UNSPEC
#endif
#ifndef PF_INET
#define PF_INET AF_INET
#endif
#if defined(AF_INET6) && !defined(PF_INET6)
#define PF_INET6 AF_INET6
#endif
#endif
