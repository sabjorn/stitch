#ifdef __linux__
    #include "linux/file.h"
#elif defined(__APPLE__)
    #include "darwin/file.h"
#else
    #error "Unsupported platform"
#endif
