#include "file_event.h"

#include <cstring>

#include <sys/event.h>
#include <poll.h>

using namespace std;

namespace Stitch {

File_Event::File_Event(int fd_, Type type)
{
    fd = fd_;

    switch(type)
    {
    case Read_Ready:
        kqueue_filter = EVFILT_READ;
        poll_events = POLLIN;
        break;
    case Write_Ready:
        kqueue_filter = EVFILT_WRITE;
        poll_events = POLLOUT;
        break;
    }

    clear = [](){};
}

}