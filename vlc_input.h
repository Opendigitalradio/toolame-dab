#ifndef __VLC_INPUT_H_
#define __VLC_INPUT_H_

#include <stdint.h>
#include <vlc/vlc.h>


// A linked list structure for the incoming buffers
struct vlc_buffer {
    uint8_t                *buf;
    size_t                  size;
    struct vlc_buffer *next;
};

// Open the VLC input
int vlc_in_prepare(
        unsigned verbosity,
        unsigned int rate,
        const char* uri,
        unsigned channels);

// Read len audio bytes into buf
ssize_t vlc_in_read(void *buf, size_t len);

#endif

