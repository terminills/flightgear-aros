/*
 * The static OpenSSL of the SDK refers to a global SocketBase from its own socket BIOs. Nothing in FlightGear
 * uses those: curl hands OpenSSL a BIO of its own and keeps a bsdsocket.library base per thread. The base
 * stays NULL, so a call through it fails at once instead of using another task's base.
 */
struct Library;
struct Library *SocketBase;
