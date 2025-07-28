// Simple epoll emulation using poll() for Emscripten
#include <poll.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "emscriptem_epoll.h"

// Simple epoll descriptor tracking
struct emscripten_epoll_fd {
    int fd;
    uint32_t events;
    union epoll_data data;
};

struct emscripten_epoll {
    struct emscripten_epoll_fd* fds;
    int count;
    int capacity;
};

static struct emscripten_epoll* epoll_descriptors[256] = {0};
static int next_epfd = 10; // Start after standard fds

int epoll_create1(int flags) {
    (void)flags; // Ignore flags for simplicity
    
    struct emscripten_epoll* ep = malloc(sizeof(struct emscripten_epoll));
    if (!ep) {
        errno = ENOMEM;
        return -1;
    }
    
    ep->fds = NULL;
    ep->count = 0;
    ep->capacity = 0;
    
    int epfd = next_epfd++;
    if (epfd < 256) {
        epoll_descriptors[epfd] = ep;
        return epfd;
    }
    
    free(ep);
    errno = EMFILE;
    return -1;
}

int epoll_ctl(int epfd, int op, int fd, struct epoll_event* event) {
    if (epfd < 0 || epfd >= 256 || !epoll_descriptors[epfd]) {
        errno = EBADF;
        return -1;
    }
    
    struct emscripten_epoll* ep = epoll_descriptors[epfd];
    
    // Find existing fd
    int found_idx = -1;
    for (int i = 0; i < ep->count; i++) {
        if (ep->fds[i].fd == fd) {
            found_idx = i;
            break;
        }
    }
    
    switch (op) {
        case EPOLL_CTL_ADD:
            if (found_idx >= 0) {
                errno = EEXIST;
                return -1;
            }
            
            // Grow array if needed
            if (ep->count >= ep->capacity) {
                int new_cap = ep->capacity ? ep->capacity * 2 : 8;
                struct emscripten_epoll_fd* new_fds = realloc(ep->fds, 
                    new_cap * sizeof(struct emscripten_epoll_fd));
                if (!new_fds) {
                    errno = ENOMEM;
                    return -1;
                }
                ep->fds = new_fds;
                ep->capacity = new_cap;
            }
            
            ep->fds[ep->count].fd = fd;
            ep->fds[ep->count].events = event->events;
            ep->fds[ep->count].data = event->data;
            ep->count++;
            break;
            
        case EPOLL_CTL_MOD:
            if (found_idx < 0) {
                errno = ENOENT;
                return -1;
            }
            ep->fds[found_idx].events = event->events;
            ep->fds[found_idx].data = event->data;
            break;
            
        case EPOLL_CTL_DEL:
            if (found_idx < 0) {
                errno = ENOENT;
                return -1;
            }
            // Move last element to found position
            if (found_idx < ep->count - 1) {
                ep->fds[found_idx] = ep->fds[ep->count - 1];
            }
            ep->count--;
            break;
            
        default:
            errno = EINVAL;
            return -1;
    }
    
    return 0;
}

int epoll_wait(int epfd, struct epoll_event* events, int maxevents, int timeout) {
    if (epfd < 0 || epfd >= 256 || !epoll_descriptors[epfd]) {
        errno = EBADF;
        return -1;
    }
    
    if (maxevents <= 0) {
        errno = EINVAL;
        return -1;
    }
    
    struct emscripten_epoll* ep = epoll_descriptors[epfd];
    if (ep->count == 0) {
        // No fds to poll, just sleep for timeout
        if (timeout > 0) {
            emscripten_sleep(timeout);
        }
        return 0;
    }
    
    // Convert epoll events to poll events
    struct pollfd* pollfds = malloc(ep->count * sizeof(struct pollfd));
    if (!pollfds) {
        errno = ENOMEM;
        return -1;
    }
    
    for (int i = 0; i < ep->count; i++) {
        pollfds[i].fd = ep->fds[i].fd;
        pollfds[i].events = 0;
        pollfds[i].revents = 0;
        
        // Convert epoll events to poll events
        if (ep->fds[i].events & EPOLLIN) pollfds[i].events |= POLLIN;
        if (ep->fds[i].events & EPOLLOUT) pollfds[i].events |= POLLOUT;
        if (ep->fds[i].events & EPOLLPRI) pollfds[i].events |= POLLPRI;
    }
    
    int result = poll(pollfds, ep->count, timeout);
    if (result <= 0) {
        free(pollfds);
        return result;
    }
    
    // Convert results back to epoll events
    int event_count = 0;
    for (int i = 0; i < ep->count && event_count < maxevents; i++) {
        if (pollfds[i].revents != 0) {
            events[event_count].events = 0;
            events[event_count].data = ep->fds[i].data;
            
            // Convert poll events back to epoll events
            if (pollfds[i].revents & POLLIN) events[event_count].events |= EPOLLIN;
            if (pollfds[i].revents & POLLOUT) events[event_count].events |= EPOLLOUT;
            if (pollfds[i].revents & POLLPRI) events[event_count].events |= EPOLLPRI;
            if (pollfds[i].revents & POLLERR) events[event_count].events |= EPOLLERR;
            if (pollfds[i].revents & POLLHUP) events[event_count].events |= EPOLLHUP;
            
            event_count++;
        }
    }
    
    free(pollfds);
    return event_count;
}

int epoll_pwait2(int epfd, struct epoll_event* events, int maxevents, const struct timespec* ts, const sigset_t* sigmask) {
    int timeout = -1;
    
    if (ts) {
        // Convert timespec to milliseconds
        timeout = ts->tv_sec * 1000 + ts->tv_nsec / 1000000;
    }
    
    // Signal mask handling is not meaningful in WebAssembly/JavaScript context
    (void)sigmask;  // Suppress unused parameter warning
    
    return epoll_wait(epfd, events, maxevents, timeout);
}

// Cleanup function - should be called when epoll fd is closed
void epoll_close(int epfd) {
    if (epfd >= 0 && epfd < 256 && epoll_descriptors[epfd]) {
        struct emscripten_epoll* ep = epoll_descriptors[epfd];
        free(ep->fds);
        free(ep);
        epoll_descriptors[epfd] = NULL;
    }
}
