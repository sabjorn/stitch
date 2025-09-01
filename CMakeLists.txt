cmake_minimum_required(VERSION 3.5)

project(stitch)

option(STITCH_BUILD_DOCUMENTATION "Build Stitch documentation." OFF)
option(STITCH_BUILD_TESTS "Build Stitch tests." OFF)
option(STITCH_STATIC_LIB "Build Stitch as a static library." OFF)

add_compile_options(-Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -Werror=return-type)

# Platform-specific source files
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(platform_sources
        stitch/linux/events.cpp
        stitch/linux/signal.cpp
        stitch/linux/timer.cpp
        stitch/linux/file_event.cpp
        stitch/linux/file.cpp
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(platform_sources
        stitch/darwin/events.cpp
        stitch/darwin/signal.cpp
        stitch/darwin/timer.cpp
        stitch/darwin/file_event.cpp
        stitch/darwin/file.cpp
    )
else()
    message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_NAME}")
endif()

set(sources
    stitch/hazard_pointers.cpp
    ${platform_sources}
)

if(STITCH_STATIC_LIB)
    message(STATUS "Stitch building static")
    add_library(stitch STATIC ${sources})
else()
    message(STATUS "Stitch building shared")
    add_library(stitch SHARED ${sources})
endif()

set_property(TARGET stitch PROPERTY CXX_STANDARD 14)

if (STITCH_BUILD_DOCUMENTATION)
    add_subdirectory(doc)
endif()

if (STITCH_BUILD_TESTS)
    add_subdirectory(tests)
endif()
