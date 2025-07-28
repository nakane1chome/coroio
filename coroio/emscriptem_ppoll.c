#include <poll.h>

int ppoll(struct pollfd* fds, nfds_t nfds, const struct timespec* ts, const sigset_t* sigmask) {
    int timeout = -1;
    
    if (ts) {
        // Convert timespec to milliseconds
        timeout = ts->tv_sec * 1000 + ts->tv_nsec / 1000000;
    }
    
    // Signal mask handling is not meaningful in WebAssembly/JavaScript context
    // since the threading and signal model is completely different
    (void)sigmask;  // Suppress unused parameter warning
    
    // Use Emscripten's poll() implementation
    // Note: This may have limitations as discussed, but it's the best we can do
    // without reimplementing the entire polling mechanism
    return poll(fds, nfds, timeout);
}  

