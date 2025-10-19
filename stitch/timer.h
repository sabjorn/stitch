#ifdef __linux__
    #include "linux/timer.h"
#elif defined(__APPLE__)
    #include "darwin/timer.h"
#else
    #error "Unsupported platform"
#endif
