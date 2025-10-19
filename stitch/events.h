#ifdef __linux__
    #include "linux/events.h"
#elif defined(__APPLE__)
    #include "darwin/events.h"
#else
    #error "Unsupported platform"
#endif
