#include "timer.h"

#include <cstdint>
#include <cstring>
#include <string>
#include <chrono>

#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

using namespace std;
using namespace std::chrono;

namespace Stitch {

Timer::Timer() : d_running(false), d_should_stop(false), d_repeated(false), d_config_changed(false)
{
    int pipe_fds[2];
    if (pipe(pipe_fds) == -1)
        throw std::runtime_error("'pipe' failed for timer.");
    
    d_read_fd = pipe_fds[0];
    d_write_fd = pipe_fds[1];
    
    // Make both ends non-blocking
    fcntl(d_read_fd, F_SETFL, O_NONBLOCK);
    fcntl(d_write_fd, F_SETFL, O_NONBLOCK);
    
    d_timer_thread = std::thread(&Timer::timer_thread, this);
}

Timer::~Timer()
{
    stop();
    d_should_stop = true;
    d_config_cv.notify_one();
    
    if (d_timer_thread.joinable()) {
        d_timer_thread.join();
    }
    
    close(d_read_fd);
    close(d_write_fd);
}

void Timer::setInterval(const timespec & t, bool repeated)
{
    {
        std::lock_guard<std::mutex> lock(d_config_mutex);
        d_interval = t;
        d_repeated = repeated;
        d_config_changed = true;
        d_running = true;
    }
    d_config_cv.notify_one();
}

void Timer::stop()
{
    {
        std::lock_guard<std::mutex> lock(d_config_mutex);
        d_running = false;
        d_config_changed = true;
    }
    d_config_cv.notify_one();
}

void Timer::clear()
{
    char buffer[256];
    int result;

    // Read all available data to clear the pipe
    do { result = read(d_read_fd, buffer, sizeof(buffer)); }
    while (result > 0 || (result == -1 && errno == EINTR));
}

void Timer::timer_thread()
{
    std::unique_lock<std::mutex> lock(d_config_mutex);
    
    while (!d_should_stop) {
        // Wait for configuration or stop signal
        d_config_cv.wait(lock, [this] { return d_config_changed || d_should_stop; });
        
        if (d_should_stop) break;
        
        if (d_config_changed) {
            d_config_changed = false;
            
            if (!d_running) {
                // Timer was stopped
                continue;
            }
            
            // Start the timer
            timespec interval = d_interval;
            bool repeated = d_repeated;
            
            lock.unlock(); // Release lock during sleep
            
            do {
                // Sleep for the interval
                auto duration = milliseconds(interval.tv_sec * 1000 + interval.tv_nsec / 1000000);
                
                this_thread::sleep_for(duration);
                
                // Check if we should continue
                {
                    std::lock_guard<std::mutex> check_lock(d_config_mutex);
                    if (!d_running || d_config_changed || d_should_stop) {
                        break;
                    }
                }
                
                // Signal timer expiration
                char byte = 1;
                write(d_write_fd, &byte, 1);
                
            } while (repeated && !d_should_stop);
            
            lock.lock(); // Reacquire lock for next iteration
        }
    }
}

Event Timer::event()
{
    Event e;
    e.fd = d_read_fd;
    e.kqueue_filter = EVFILT_READ;
    e.poll_events = POLLIN;
    e.clear = std::bind(&Timer::clear, this);
    return e;
}

}