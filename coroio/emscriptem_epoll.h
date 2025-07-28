#ifndef EMSCRIPTEN_EPOLL_H
#define EMSCRIPTEN_EPOLL_H

#ifdef __EMSCRIPTEN__

#include <stdint.h>
#include <sys/types.h>
#include <time.h>
#include <signal.h>
#include <emscripten.h>

#ifdef __cplusplus
extern "C" {
#endif

// epoll event types
#ifndef EPOLLIN
#define EPOLLIN      0x001   // Available for read
#endif
#ifndef EPOLLPRI
#define EPOLLPRI     0x002   // Urgent data available for read
#endif
#ifndef EPOLLOUT
#define EPOLLOUT     0x004   // Available for write
#endif
#ifndef EPOLLRDNORM
#define EPOLLRDNORM  0x040   // Normal data available for read
#endif
#ifndef EPOLLRDBAND
#define EPOLLRDBAND  0x080   // Priority band data available for read
#endif
#ifndef EPOLLWRNORM
#define EPOLLWRNORM  0x100   // Normal data can be written
#endif
#ifndef EPOLLWRBAND
#define EPOLLWRBAND  0x200   // Priority band data can be written
#endif
#ifndef EPOLLMSG
#define EPOLLMSG     0x400   // Message available (unused on Linux)
#endif
#ifndef EPOLLERR
#define EPOLLERR     0x008   // Error condition
#endif
#ifndef EPOLLHUP
#define EPOLLHUP     0x010   // Hang up
#endif
#ifndef EPOLLRDHUP
#define EPOLLRDHUP   0x2000  // Stream socket peer closed connection
#endif
#ifndef EPOLLEXCLUSIVE
#define EPOLLEXCLUSIVE 0x10000000  // Exclusive wakeup mode
#endif
#ifndef EPOLLWAKEUP
#define EPOLLWAKEUP  0x20000000   // Prevent system suspend
#endif
#ifndef EPOLLONESHOT
#define EPOLLONESHOT 0x40000000   // One-shot behavior
#endif
#ifndef EPOLLET
#define EPOLLET      0x80000000   // Edge-triggered behavior
#endif

// epoll_ctl operations
#ifndef EPOLL_CTL_ADD
#define EPOLL_CTL_ADD 1  // Add a file descriptor
#endif
#ifndef EPOLL_CTL_DEL
#define EPOLL_CTL_DEL 2  // Remove a file descriptor
#endif
#ifndef EPOLL_CTL_MOD
#define EPOLL_CTL_MOD 3  // Modify a file descriptor
#endif

// epoll_data union
typedef union epoll_data {
    void *ptr;
    int fd;
    uint32_t u32;
    uint64_t u64;
} epoll_data_t;

// epoll_event structure
struct epoll_event {
    uint32_t events;      // epoll events
    epoll_data_t data;    // User data variable
};

// Function declarations

/**
 * Create an epoll instance
 * @param flags Currently ignored (for compatibility)
 * @return epoll file descriptor on success, -1 on error
 */
int epoll_create1(int flags);

/**
 * Control interface for an epoll descriptor
 * @param epfd epoll file descriptor
 * @param op operation (EPOLL_CTL_ADD, EPOLL_CTL_MOD, EPOLL_CTL_DEL)
 * @param fd file descriptor to operate on
 * @param event epoll event structure
 * @return 0 on success, -1 on error
 */
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);

/**
 * Wait for events on an epoll instance
 * @param epfd epoll file descriptor
 * @param events buffer for returned events
 * @param maxevents maximum number of events to return
 * @param timeout timeout in milliseconds (-1 for infinite)
 * @return number of ready file descriptors, 0 on timeout, -1 on error
 */
int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);

/**
 * Wait for events with signal mask and timespec timeout
 * @param epfd epoll file descriptor
 * @param events buffer for returned events
 * @param maxevents maximum number of events to return
 * @param ts timeout as timespec (NULL for infinite)
 * @param sigmask signal mask (ignored in Emscripten)
 * @return number of ready file descriptors, 0 on timeout, -1 on error
 */
int epoll_pwait2(int epfd, struct epoll_event *events, int maxevents, 
                 const struct timespec *ts, const sigset_t *sigmask);

/**
 * Close and cleanup an epoll instance
 * @param epfd epoll file descriptor to close
 */
void epoll_close(int epfd);

// Legacy compatibility functions

/**
 * Create an epoll instance (legacy version)
 * @param size ignored (for compatibility with older API)
 * @return epoll file descriptor on success, -1 on error
 */
static inline int epoll_create(int size) {
    (void)size;  // Ignore size parameter
    return epoll_create1(0);
}

/**
 * Wait for events with signal mask (original pwait version)
 * @param epfd epoll file descriptor
 * @param events buffer for returned events
 * @param maxevents maximum number of events to return
 * @param timeout timeout in milliseconds
 * @param sigmask signal mask (ignored in Emscripten)
 * @return number of ready file descriptors, 0 on timeout, -1 on error
 */
static inline int epoll_pwait(int epfd, struct epoll_event *events, int maxevents,
                              int timeout, const sigset_t *sigmask) {
    struct timespec ts;
    struct timespec *ts_ptr = NULL;
    
    if (timeout >= 0) {
        ts.tv_sec = timeout / 1000;
        ts.tv_nsec = (timeout % 1000) * 1000000;
        ts_ptr = &ts;
    }
    
    return epoll_pwait2(epfd, events, maxevents, ts_ptr, sigmask);
}

#ifdef __cplusplus
}
#endif

#endif // __EMSCRIPTEN__

#endif // EMSCRIPTEN_EPOLL_H
