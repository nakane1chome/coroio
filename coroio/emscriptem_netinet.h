#ifndef EMSCRIPTEN_NETINET_H
#define EMSCRIPTEN_NETINET_H

#ifdef __EMSCRIPTEN__

#include <stdint.h>
#include <netinet/in.h>
#include <endian.h>

#ifdef __cplusplus
extern "C" {
#endif


// Host to network byte order (64-bit) - not always available in standard libraries
static inline uint64_t htonll(uint64_t hostlonglong) {
#if __BYTE_ORDER == __LITTLE_ENDIAN
    return __bswap64(hostlonglong);
#else
    return hostlonglong;
#endif
}

// Network to host byte order (64-bit) - not always available in standard libraries
static inline uint64_t ntohll(uint64_t netlonglong) {
#if __BYTE_ORDER == __LITTLE_ENDIAN
    return __bswap64(netlonglong);
#else
    return netlonglong;
#endif
}

#ifdef __cplusplus
}
#endif

#endif // __EMSCRIPTEN__

#endif // EMSCRIPTEN_NETINET_H

