#include "signal.h"

#include <cstdint>
#include <mutex>
#include <list>

#include <unistd.h>
#include <sys/event.h>
#include <poll.h>
#include <fcntl.h>

using namespace std;

namespace Stitch {

Signal::Signal()
{
    int pipe_fds[2];
    if (pipe(pipe_fds) == -1)
        throw std::runtime_error("'pipe' failed.");
    
    d_read_fd = pipe_fds[0];
    d_write_fd = pipe_fds[1];
    
    // Make both ends non-blocking
    fcntl(d_read_fd, F_SETFL, O_NONBLOCK);
    fcntl(d_write_fd, F_SETFL, O_NONBLOCK);
}

Signal::~Signal()
{
    close(d_read_fd);
    close(d_write_fd);
}

void Signal::notify()
{
    // Write multiple bytes to ensure multiple readers can be woken up
    char bytes[64];
    memset(bytes, 1, sizeof(bytes));
    int result;

    do { result = write(d_write_fd, bytes, sizeof(bytes)); }
    while (result == -1 && errno == EINTR);
}

void Signal::clear()
{
    char buffer[256];
    int result;

    // Read all available data to clear the pipe
    do { result = read(d_read_fd, buffer, sizeof(buffer)); }
    while (result > 0 || (result == -1 && errno == EINTR));
}

Event Signal::event()
{
    Event e;
    e.fd = d_read_fd;
    e.kqueue_filter = EVFILT_READ;
    e.poll_events = POLLIN;
    e.clear = std::bind(&Signal::clear, this);
    return e;
}

Detail::SignalChannel::SignalChannel()
{
    int pipe_fds[2];
    if (pipe(pipe_fds) == -1)
        throw std::runtime_error("'pipe' failed.");
    
    fd = pipe_fds[0];
    write_fd = pipe_fds[1];
    
    // Make both ends non-blocking
    fcntl(fd, F_SETFL, O_NONBLOCK);
    fcntl(write_fd, F_SETFL, O_NONBLOCK);
}

Detail::SignalChannel::~SignalChannel()
{
    close(fd);
    close(write_fd);
}

void Detail::SignalChannel::notify()
{
    // Write multiple bytes to ensure multiple readers can be woken up
    char bytes[64];
    memset(bytes, 1, sizeof(bytes));
    int result;

    do { result = write(write_fd, bytes, sizeof(bytes)); }
    while (result == -1 && errno == EINTR);
}

void Detail::SignalChannel::clear()
{
    char buffer[256];
    int result;

    // Read all available data to clear the pipe
    do { result = read(fd, buffer, sizeof(buffer)); }
    while (result > 0 || (result == -1 && errno == EINTR));
}

Event Signal_Receiver::event()
{
    Event e;
    e.fd = data().fd;
    e.kqueue_filter = EVFILT_READ;
    e.poll_events = POLLIN;
    e.clear = std::bind(&Detail::SignalChannel::clear, &data());

    return e;
}

}