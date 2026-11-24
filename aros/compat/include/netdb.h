#ifndef FGAROS_WRAP_NETDB_H
#define FGAROS_WRAP_NETDB_H
#include <pthread.h>
#include <sys/socket.h>
#include_next <netdb.h>
#include <fgaros_socket.h>
#undef h_errno
#define h_errno (*fgaros_h_errno())
#endif
