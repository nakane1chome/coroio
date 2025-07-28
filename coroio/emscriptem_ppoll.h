#ifndef EMSCRIPTEN_PPOLL_H
#define EMSCRIPTEN_PPOLL_H

#ifdef __EMSCRIPTEN__

#include <sys/types.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <emscripten.h>

#ifdef __cplusplus
extern "C" {
#endif

// Poll event flags (standard POSIX values)
#ifndef POLLIN
#define POLLIN      0x001   // Data available for reading
#endif
#ifndef POLLPRI
#define POLLPRI     0x002   // Priority data available
#endif
#ifndef POLLOUT
#define POLLOUT     0x004   // Ready for writing
#endif
#ifndef POLLERR
#define POLLERR     0x008   // Error condition
#endif
#ifndef POLLHUP
#define POLLHUP     0x010   // Hang up
#endif
#ifndef POLLNVAL
#define POLLNVAL    0x020   // Invalid request
#endif

// Additional flags
#ifndef POLLRDNORM
#define POLLRDNORM  0x040   // Normal data available
#endif
#ifndef POLLRDBAND
#define POLLRDBAND  0x080   // Priority band data available
#endif
#ifndef POLLWRNORM
#define POLLWRNORM  0x100   // Normal data can be written
#endif
#ifndef POLLWRBAND
#define POLLWRBAND  0x200   // Priority band data can be written
#endif

// pollfd structure
struct pollfd {
    int fd;         // File descriptor
    short events;   // Requested events
    short revents;  // Returned events
};

// Function prototypes
int ppoll(struct pollfd *fds, nfds_t nfds, const struct timespec *timeout_ts, const sigset_t *sigmask);
int poll(struct pollfd *fds, nfds_t nfds, int timeout);

// Helper function to check if fd is valid and what operations are possible
static inline int _check_fd_status(int fd, short events) {
    // In Emscripten, most file descriptors are either:
    // - stdin (0), stdout (1), stderr (2) 
    // - or virtual file system descriptors
    
    if (fd < 0) {
        return POLLNVAL;
    }
    
    int result = 0;
    
    // Check if we can read from this fd
    if (events & (POLLIN | POLLRDNORM)) {
        // For stdin or regular files, assume readable
        if (fd == STDIN_FILENO) {
            // stdin is typically always ready in web environment
            result |= (events & (POLLIN | POLLRDNORM));
        } else {
            // For other fds, we'll assume they're ready
            result |= (events & (POLLIN | POLLRDNORM));
        }
    }
    
    // Check if we can write to this fd  
    if (events & (POLLOUT | POLLWRNORM)) {
        if (fd == STDOUT_FILENO || fd == STDERR_FILENO) {
            // stdout/stderr are typically always ready
            result |= (events & (POLLOUT | POLLWRNORM));
        } else {
            // For other fds, assume writable
            result |= (events & (POLLOUT | POLLWRNORM));
        }
    }
    
    return result;
}

// Convert timespec to milliseconds
static inline int _timespec_to_ms(const struct timespec *ts) {
    if (!ts) return -1;  // Infinite timeout
    if (ts->tv_sec < 0 || ts->tv_nsec < 0 || ts->tv_nsec >= 1000000000) {
        return -1;  // Invalid timespec
    }
    
    long long ms = (long long)ts->tv_sec * 1000 + ts->tv_nsec / 1000000;
    if (ms > INT_MAX) return INT_MAX;
    return (int)ms;
}

// Main ppoll implementation
static inline int ppoll(struct pollfd *fds, nfds_t nfds, const struct timespec *timeout_ts, const sigset_t *sigmask) {
    if (!fds && nfds > 0) {
        errno = EFAULT;
        return -1;
    }
    
    if (nfds < 0) {
        errno = EINVAL;
        return -1;
    }
    
    // In WebAssembly, signal handling is limited, so we largely ignore sigmask
    // In a real implementation, you'd temporarily set the signal mask here
    (void)sigmask;  // Suppress unused parameter warning
    
    int ready_count = 0;
    int timeout_ms = _timespec_to_ms(timeout_ts);
    
    // Record start time for timeout calculation
    double start_time = emscripten_get_now();
    
    do {
        ready_count = 0;
        
        // Check each file descriptor
        for (nfds_t i = 0; i < nfds; i++) {
            fds[i].revents = 0;
            
            if (fds[i].fd < 0) {
                // Negative fd is ignored
                continue;
            }
            
            // Check the status of this fd
            int status = _check_fd_status(fds[i].fd, fds[i].events);
            
            if (status & POLLNVAL) {
                fds[i].revents = POLLNVAL;
                ready_count++;
            } else if (status) {
                fds[i].revents = status;
                ready_count++;
            }
        }
        
        // If we found ready descriptors, return immediately
        if (ready_count > 0) {
            break;
        }
        
        // Check for timeout
        if (timeout_ms >= 0) {
            double elapsed = emscripten_get_now() - start_time;
            if (elapsed >= timeout_ms) {
                break;  // Timeout occurred
            }
            
            // Sleep for a short time to avoid busy waiting
            // In a real async environment, you'd yield to the event loop
            emscripten_sleep(1);  // Sleep for 1ms
        } else {
            // Infinite timeout - in WebAssembly this is problematic
            // We'll do a short sleep and continue
            emscripten_sleep(10);
        }
        
    } while (timeout_ms < 0 || (emscripten_get_now() - start_time) < timeout_ms);
    
    return ready_count;
}

// Standard poll implementation in terms of ppoll
static inline int poll(struct pollfd *fds, nfds_t nfds, int timeout) {
    struct timespec ts;
    struct timespec *timeout_ptr = NULL;
    
    if (timeout >= 0) {
        ts.tv_sec = timeout / 1000;
        ts.tv_nsec = (timeout % 1000) * 1000000;
        timeout_ptr = &ts;
    }
    
    return ppoll(fds, nfds, timeout_ptr, NULL);
}

#ifdef __cplusplus
}
#endif

#endif // __EMSCRIPTEN__

#endif // EMSCRIPTEN_PPOLL_H
