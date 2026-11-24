/*
 * POSIX socket calls for the FlightGear port, as real functions (libfgaros.a) over bsdsocket.library.
 *
 * The AROS SDK reaches bsdsocket.library through function-like macros named socket(), bind(), select() ...,
 * which rewrite C++ members and std::bind. The port is compiled with BSDSOCKET_NOLIBDEFINES, which leaves the
 * prototypes and no function behind them; libfgaros.a has the functions. The wrapper headers next to this
 * file bring the prototypes in where POSIX programs expect them.
 *
 * Every thread has its own library base (the stack ties a base and its sockets to the task that opened it),
 * so a socket has to be used on the thread that created it.
 */
#ifndef FGAROS_SOCKET_H
#define FGAROS_SOCKET_H

#include <sys/types.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <exec/types.h>
#include <sys/cdefs.h>
#include <aros/libcall.h>

#ifdef __cplusplus
extern "C" {
#endif

/* the SDK's prototypes have no linkage specification of their own */
#include <proto/bsdsocket.h>

/* <clib/bsdsocket_protos.h> defines these two whatever is asked for */
#undef select
#undef inet_ntoa

struct addrinfo;

/* socket(), bind(), send() ... gethostbyname(), inet_addr(): declared by the SDK (<clib/bsdsocket_protos.h>,
   with the library's own types), defined in libfgaros.a. These are the ones the SDK has under other names,
   or (inet_aton) only for programs that ask for the whole Roadshow interface, as fgsock.c does. */
int getaddrinfo(const char *host, const char *serv, const struct addrinfo *hints, struct addrinfo **res);
void freeaddrinfo(struct addrinfo *res);
const char *gai_strerror(int code);
char *inet_ntoa(struct in_addr in);
const char *inet_ntop(int af, const void *src, char *dst, socklen_t size);
int inet_pton(int af, const char *src, void *dst);
#ifndef __CONFIG_ROADSHOW__
int inet_aton(const char *cp, struct in_addr *in);
#endif

/* Sockets are not C library file descriptors on AROS: close() and fcntl() do not know them. */
int fgaros_closesocket(int s);
int fgaros_socket_nonblocking(int s, int on);
int *fgaros_h_errno(void);

#ifdef __cplusplus
}
#endif

#endif /* FGAROS_SOCKET_H */

/* <clib/bsdsocket_protos.h> defines these two whatever is asked; select() is the function of libfgaros.a
   (sockets only), declared by <sys/select.h>. */
#undef select
#undef inet_ntoa
