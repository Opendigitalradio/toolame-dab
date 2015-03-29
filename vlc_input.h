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

struct vlc_buffer* vlc_buffer_new();
void vlc_buffer_free(struct vlc_buffer* node);


// VLC Audio prerender callback
void prepareRender(
        void* p_audio_data,
        uint8_t** pp_pcm_buffer,
        size_t size);

// Audio postrender callback
void handleStream(
        void* p_audio_data,
        uint8_t* p_pcm_buffer,
        unsigned int channels,
        unsigned int rate,
        unsigned int nb_samples,
        unsigned int bits_per_sample,
        size_t size,
        int64_t pts);

// Open the VLC input
int vlc_in_prepare(unsigned verbosity, unsigned int rate, const char* uri);

// Read len audio bytes into buf
ssize_t vlc_in_read(void *buf, size_t len);

#endif

