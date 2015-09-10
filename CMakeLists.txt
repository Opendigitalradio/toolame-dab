########################################################################
# Project setup
########################################################################

cmake_minimum_required(VERSION 2.8)
project(Toolame-DAB C)

# Select the release build type by default to get optimization flags
if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE "Release")
   message(STATUS "Build type not specified: defaulting to release.")
endif(NOT CMAKE_BUILD_TYPE)
set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/Modules)


########################################################################
# Version information
########################################################################

set(VERSION_INFO_MAJOR  0)
set(VERSION_INFO_MINOR  0)
set(VERSION_INFO_PATCH  0)

if(NOT DEFINED VERSION_INFO_EXTRA)
    set(VERSION_INFO_EXTRA "git")
endif()
include(Version)

set(VERSION "${VERSION_INFO}")


########################################################################
# Compiler specific setup
########################################################################

set(CMAKE_CFLAGS "${CMAKE_C_FLAGS} -W -Wall")
add_definitions(-fomit-frame-pointer)
add_definitions(-march=native)
add_definitions(-DGIT_VERSION="${VERSION}")
add_definitions(-DINLINE=)
add_definitions(-DNEWENCODE)


########################################################################
# Find build dependencies
########################################################################

find_package(PkgConfig)

# libm
find_library(M_LIB m REQUIRED)

# threads
find_package(Threads REQUIRED)

# libzmq
pkg_check_modules(ZMQ libzmq>=4.0 REQUIRED)
if(NOT ZMQ_FOUND)
    message(FATAL_ERROR "libzmq required to compile Toolame-DAB \n")
endif()
include_directories(${ZMQ_INCLUDE_DIRS})

# libjack
pkg_check_modules(JACK jack)

# libvlc
pkg_check_modules(VLC libvlc)


########################################################################
# Options
########################################################################

# vlc input
option(ENABLE_INPUT_VLC
    "libvlc input plugin" ${VLC_FOUND})

# jack input
option(ENABLE_INPUT_JACK
    "Jack input plugin" ${JACK_FOUND})


if(ENABLE_INPUT_VLC)
    if(NOT VLC_FOUND)
        message(FATAL_ERROR "libvlc required to compile Toolame-DAB with ENABLE_INPUT_VLC \n")
    endif()
    add_definitions(-DVLC_INPUT)
    include_directories(${VLC_INCLUDE_DIRS})
    list(APPEND other_libs ${VLC_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
endif()


if(ENABLE_INPUT_JACK)
    if(NOT JACK_FOUND)
        message(FATAL_ERROR "libjack required to compile Toolame-DAB with ENABLE_INPUT_JACK \n")
    endif()
    add_definitions(-DJACK_INPUT)
    include_directories(${JACK_INCLUDE_DIRS})
    list(APPEND other_libs ${JACK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
endif()


########################################################################
# Setup apps
########################################################################

list(APPEND toolame_sources
    common.c
    encode.c
    ieeefloat.c
    toolame.c
    portableio.c
    psycho_n1.c
    psycho_0.c
    psycho_1.c
    psycho_2.c
    psycho_3.c
    psycho_4.c
    fft.c
    subband.c
    audio_read.c
    bitstream.c
    mem.c
    crc.c
    tables.c
    availbits.c
    ath.c
    encode_new.c
    zmqoutput.c
    utils.c
    xpad.c
    vlc_input.c
    )

add_executable(toolame ${toolame_sources})
set_target_properties(toolame PROPERTIES OUTPUT_NAME toolame-dab)
target_link_libraries(toolame ${M_LIB} ${ZMQ_LIBRARIES} ${other_libs})

install(TARGETS toolame DESTINATION bin)


########################################################################
# Create uninstall target
########################################################################

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)


