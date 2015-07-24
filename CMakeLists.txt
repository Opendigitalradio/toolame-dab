cmake_minimum_required (VERSION 2.6)
project (Toolame-DAB)
add_executable(toolame common.c
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
    vlc_input.c)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -g -fomit-frame-pointer")
set(CMAKE_C_FLAGS "-DGIT_VERSION=")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=native -Wall -DNEWENCODE")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DINLINE=")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DENABLE_INPUT_VLC=0")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DENABLE_INPUT_JACK=0")

target_link_libraries(toolame m zmq)

install (TARGETS toolame DESTINATION bin)
