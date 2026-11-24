/*
 * POSIX socket calls as real functions over bsdsocket.library: see include/fgaros_socket.h.
 *
 * This file is compiled against the SDK's own headers (not the wrappers in include/), takes the library call
 * macros from <defines/bsdsocket.h> and drops the POSIX-named ones it defines as functions.
 */
#define __CONFIG_ROADSHOW__  /* the getaddrinfo() family and inet_ntop()/inet_pton() of the library */
#include <pthread.h>
#include <exec/types.h>
#include <exec/libraries.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <bsdsocket/socketbasetags.h>
#include <libraries/bsdsocket.h>
#include <defines/bsdsocket.h>

#undef socket
#undef bind
#undef connect
#undef listen
#undef accept
#undef shutdown
#undef send
#undef recv
#undef sendto
#undef recvfrom
#undef setsockopt
#undef getsockopt
#undef getsockname
#undef getpeername
#undef gethostname
#undef select
#undef gethostbyname
#undef gethostbyaddr
#undef getservbyname
#undef getprotobyname
#undef getaddrinfo
#undef freeaddrinfo
#undef gai_strerror
#undef inet_addr
#undef inet_ntoa
#undef inet_aton
#undef inet_ntop
#undef inet_pton
#undef SocketBaseTagList
#undef h_errno

struct fgsock {
    struct Library *base;
    int err;        /* the base reports here; copied to the caller's errno */
    LONG herr;
};

static pthread_key_t key;
static pthread_once_t once = PTHREAD_ONCE_INIT;
static int no_herr;

static void drop(void *p)
{
    struct fgsock *s = p;

    if (s == NULL)
        return;
    if (s->base)
        CloseLibrary(s->base);
    free(s);
}

/* the initial task of a program is not ended by the pthread library, so its destructor never runs */
static void drop_caller(void)
{
    struct fgsock *s = pthread_getspecific(key);

    if (s) {
        pthread_setspecific(key, NULL);
        drop(s);
    }
}

static void make_key(void)
{
    pthread_key_create(&key, drop);
    atexit(drop_caller);
}

/* A thread without a network stack keeps a state with no base, so the library is looked for once. */
static struct fgsock *self(void)
{
    struct fgsock *s;

    pthread_once(&once, make_key);
    s = pthread_getspecific(key);
    if (s == NULL) {
        s = calloc(1, sizeof(*s));
        if (s == NULL)
            return NULL;
        s->base = OpenLibrary((CONST_STRPTR)"bsdsocket.library", 4);
        if (s->base) {
            struct TagItem tags[] = {
                { SBTM_SETVAL(SBTC_ERRNOPTR(sizeof(int))), (IPTR)&s->err },
                { SBTM_SETVAL(SBTC_HERRNOLONGPTR), (IPTR)&s->herr },
                { TAG_DONE, 0 }
            };
            __SocketBaseTagList_WB(s->base, tags);
        }
        pthread_setspecific(key, s);
    }
    return s;
}

#define BASE(fail) \
    struct fgsock *s = self(); \
    if (s == NULL || s->base == NULL) { errno = ENETDOWN; return fail; }

static long done(struct fgsock *s, long r)
{
    if (r < 0)
        errno = s->err;
    return r;
}

int socket(int domain, int type, int protocol)
{
    BASE(-1)
    return done(s, __socket_WB(s->base, domain, type, protocol));
}

int bind(int fd, const struct sockaddr *name, socklen_t namelen)
{
    BASE(-1)
    return done(s, __bind_WB(s->base, fd, (struct sockaddr *)name, namelen));
}

int connect(int fd, const struct sockaddr *name, socklen_t namelen)
{
    BASE(-1)
    return done(s, __connect_WB(s->base, fd, (struct sockaddr *)name, namelen));
}

int listen(int fd, int backlog)
{
    BASE(-1)
    return done(s, __listen_WB(s->base, fd, backlog));
}

int accept(int fd, struct sockaddr *addr, socklen_t *addrlen)
{
    BASE(-1)
    return done(s, __accept_WB(s->base, fd, addr, addrlen));
}

int shutdown(int fd, int how)
{
    BASE(-1)
    return done(s, __shutdown_WB(s->base, fd, how));
}

int send(int fd, const void *msg, int len, int flags)
{
    BASE(-1)
    return done(s, __send_WB(s->base, fd, (void *)msg, len, flags));
}

int recv(int fd, void *buf, int len, int flags)
{
    BASE(-1)
    return done(s, __recv_WB(s->base, fd, buf, len, flags));
}

int sendto(int fd, const void *msg, int len, int flags, const struct sockaddr *to, socklen_t tolen)
{
    BASE(-1)
    return done(s, __sendto_WB(s->base, fd, (void *)msg, len, flags, (struct sockaddr *)to, tolen));
}

int recvfrom(int fd, void *buf, int len, int flags, struct sockaddr *from, socklen_t *fromlen)
{
    BASE(-1)
    return done(s, __recvfrom_WB(s->base, fd, buf, len, flags, from, fromlen));
}

int setsockopt(int fd, int level, int optname, const void *optval, socklen_t optlen)
{
    BASE(-1)
    return done(s, __setsockopt_WB(s->base, fd, level, optname, (void *)optval, optlen));
}

int getsockopt(int fd, int level, int optname, void *optval, socklen_t *optlen)
{
    BASE(-1)
    return done(s, __getsockopt_WB(s->base, fd, level, optname, optval, optlen));
}

int getsockname(int fd, struct sockaddr *name, socklen_t *namelen)
{
    BASE(-1)
    return done(s, __getsockname_WB(s->base, fd, name, namelen));
}

int getpeername(int fd, struct sockaddr *name, socklen_t *namelen)
{
    BASE(-1)
    return done(s, __getpeername_WB(s->base, fd, name, namelen));
}

int gethostname(char *name, size_t len)
{
    BASE(-1)
    return done(s, __gethostname_WB(s->base, name, len));
}

/* Sockets only: the descriptors are those of the calling thread's library base. */
int select(int nfds, fd_set *rfds, fd_set *wfds, fd_set *efds, struct timeval *timeout)
{
    BASE(-1)
    return done(s, __WaitSelect_WB(s->base, nfds, rfds, wfds, efds, timeout, NULL));
}

struct hostent *gethostbyname(const char *name)
{
    BASE(NULL)
    return __gethostbyname_WB(s->base, (char *)name);
}

struct hostent *gethostbyaddr(const void *addr, int len, int type)
{
    BASE(NULL)
    return __gethostbyaddr_WB(s->base, (char *)addr, len, type);
}

struct servent *getservbyname(const char *name, const char *proto)
{
    BASE(NULL)
    return __getservbyname_WB(s->base, (char *)name, (char *)proto);
}

struct protoent *getprotobyname(const char *name)
{
    BASE(NULL)
    return __getprotobyname_WB(s->base, (char *)name);
}

int getaddrinfo(const char *host, const char *serv, const struct addrinfo *hints, struct addrinfo **res)
{
    BASE(EAI_FAIL)
    return __getaddrinfo_WB(s->base, host, serv, hints, res);
}

void freeaddrinfo(struct addrinfo *res)
{
    struct fgsock *s = self();

    if (s && s->base)
        __freeaddrinfo_WB(s->base, res);
}

const char *gai_strerror(int code)
{
    BASE("no network stack")
    return (const char *)__gai_strerror_WB(s->base, code);
}

ULONG inet_addr(const char *cp)
{
    BASE(INADDR_NONE)
    return __inet_addr_WB(s->base, (char *)cp);
}

char *inet_ntoa(struct in_addr in)
{
    BASE("")
    return (char *)__Inet_NtoA_WB(s->base, in.s_addr);
}

LONG inet_aton(CONST_STRPTR cp, struct in_addr *in)
{
    BASE(0)
    return __inet_aton_WB(s->base, cp, in);
}

const char *inet_ntop(int af, const void *src, char *dst, socklen_t size)
{
    BASE(NULL)
    return (const char *)__inet_ntop_WB(s->base, af, (void *)src, dst, size);
}

int inet_pton(int af, const char *src, void *dst)
{
    BASE(-1)
    return __inet_pton_WB(s->base, af, (char *)src, dst);
}

int fgaros_closesocket(int fd)
{
    BASE(-1)
    return done(s, __CloseSocket_WB(s->base, fd));
}

int fgaros_socket_nonblocking(int fd, int on)
{
    long arg = on ? 1 : 0;

    BASE(-1)
    return done(s, __IoctlSocket_WB(s->base, fd, FIONBIO, (char *)&arg));
}

int *fgaros_h_errno(void)
{
    struct fgsock *s = self();

    return s ? (int *)&s->herr : &no_herr;
}

/* <netdb.h> of the SDK calls this by name in an inline function */
ULONG SocketBaseTagList(struct TagItem *tags)
{
    BASE((ULONG)-1)
    return __SocketBaseTagList_WB(s->base, tags);
}

/* The name servers the network stack uses, separated by spaces (the form of udns's $NAMESERVERS); the number
   of them. */
int fgaros_nameservers(char *buf, size_t size)
{
    struct List *list;
    struct DomainNameServerNode *n;
    size_t used = 0;
    int count = 0;
    BASE(0)

    if (size == 0)
        return 0;
    buf[0] = '\0';
    list = __ObtainDomainNameServerList_WB(s->base);
    if (list == NULL)
        return 0;
    for (n = (struct DomainNameServerNode *)list->lh_Head; n->dnsn_MinNode.mln_Succ;
         n = (struct DomainNameServerNode *)n->dnsn_MinNode.mln_Succ) {
        size_t len = n->dnsn_Address ? strlen((const char *)n->dnsn_Address) : 0;

        if (len == 0 || used + len + 2 > size)
            continue;
        if (count)
            buf[used++] = ' ';
        memcpy(buf + used, n->dnsn_Address, len + 1);
        used += len;
        count++;
    }
    __ReleaseDomainNameServerList_WB(s->base, list);
    return count;
}
