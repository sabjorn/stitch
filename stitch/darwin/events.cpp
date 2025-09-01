#include "events.h"

#include <list>
#include <algorithm>
#include <cstring>

#include <unistd.h>
#include <poll.h>
#include <sys/event.h>

using namespace std;

namespace Stitch {

void wait(const Event & e)
{
    pollfd data;
    data.fd = e.fd;
    data.events = e.poll_events;

    int result;

    do { result = poll(&data, 1, -1); }
    while(result == -1 && errno == EINTR);

    if (result == -1)
        throw std::runtime_error("'poll' failed.");

    e.clear();
}

Event_Reactor::Event_Reactor()
{
    d_kqueue_fd = kqueue();

    if (d_kqueue_fd == -1)
        throw std::runtime_error("'kqueue' failed.");

    d_ready_events.resize(5);
}

Event_Reactor::~Event_Reactor()
{
    close(d_kqueue_fd);
}

void Event_Reactor::subscribe(const Event & event, Callback cb)
{
    d_watched_events.emplace_back();

    auto & watched_event = d_watched_events.back();
    watched_event.clear = event.clear;
    watched_event.cb = cb;
    watched_event.ident = reinterpret_cast<uintptr_t>(&watched_event);

    struct kevent kev;
    EV_SET(&kev, event.fd, event.kqueue_filter, EV_ADD | EV_ENABLE, 0, 0, &watched_event);

    if (kevent(d_kqueue_fd, &kev, 1, NULL, 0, NULL) == -1)
    {
        d_watched_events.pop_back();
        throw std::runtime_error("'kevent' failed.");
    }
}

void Event_Reactor::run(Mode mode)
{
    d_running = true;

    do
    {
        struct timespec timeout = {0, 0};
        struct timespec *timeout_ptr = (mode == NoWait) ? &timeout : NULL;
        int result;

        do {
            result = kevent(d_kqueue_fd, NULL, 0, d_ready_events.data(), d_ready_events.size(), timeout_ptr);
        } while (result == -1 && errno == EINTR);

        if (result < 0)
            throw std::runtime_error("'kevent' failed.");

        for (int i = 0; i < result && d_running; ++i)
        {
            auto & kev = d_ready_events[i];
            auto data = reinterpret_cast<Event_Data*>(kev.udata);
            data->clear();
            data->cb();
        }
    }
    while(mode == WaitUntilQuit && d_running);
}

void Event_Reactor::quit()
{
    d_running = false;
}

}