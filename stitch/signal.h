#ifdef __linux__
    #include "linux/signal.h"
#elif defined(__APPLE__)
    #include "darwin/signal.h"
#else
    #error "Unsupported platform"
#endif
