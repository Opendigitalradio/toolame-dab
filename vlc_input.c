#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "vlc_input.h"


libvlc_instance_t     *m_vlc;
libvlc_media_player_t *m_mp;

unsigned int vlc_rate;
unsigned int vlc_channels;

struct vlc_buffer *head_buffer;

pthread_mutex_t buffer_lock = PTHREAD_MUTEX_INITIALIZER;

struct vlc_buffer* vlc_buffer_new()
{
    struct vlc_buffer* node;
    node = malloc(sizeof(struct vlc_buffer));
    memset(node, 0, sizeof(struct vlc_buffer));
    return node;
}

void vlc_buffer_free(struct vlc_buffer* node)
{
    if (node->buf) {
        free(node->buf);
    }

    free(node);
}

size_t vlc_buffer_totalsize(struct vlc_buffer* node)
{
    size_t totalsize = 0;
    for (; node != NULL; node = node->next) {
        totalsize += node->size;
    }

    return totalsize;
}

// VLC Audio prerender callback, we must allocate a buffer here
void prepareRender(
        void* p_audio_data,
        uint8_t** pp_pcm_buffer,
        size_t size)
{
    *pp_pcm_buffer = malloc(size);
    return;
}



// Audio postrender callback
void handleStream(
        void* p_audio_data,
        uint8_t* p_pcm_buffer,
        unsigned int channels,
        unsigned int rate,
        unsigned int nb_samples,
        unsigned int bits_per_sample,
        size_t size,
        int64_t pts)
{
    assert(channels == vlc_channels);
    assert(rate == vlc_rate);
    assert(bits_per_sample == 16);

    // 16 is a bit arbitrary, if it's too small we might enter
    // a deadlock if toolame asks for too much data
    const size_t max_length = 16 * size;

    for (;;) {
        pthread_mutex_lock(&buffer_lock);

        if (vlc_buffer_totalsize(head_buffer) < max_length) {
            struct vlc_buffer* newbuf = vlc_buffer_new();

            newbuf->buf = p_pcm_buffer;
            newbuf->size = size;

            // Append the new buffer to the end of the linked list
            struct vlc_buffer* tail = head_buffer;
            while (tail->next) {
                tail = tail->next;
            }
            tail->next = newbuf;

            pthread_mutex_unlock(&buffer_lock);
            return;
        }

        pthread_mutex_unlock(&buffer_lock);
        usleep(100);
    }
}

int vlc_in_prepare(
        unsigned verbosity,
        unsigned int rate,
        const char* uri,
        unsigned channels)
{
    fprintf(stderr, "Initialising VLC...\n");

    vlc_rate = rate;
    vlc_channels = channels;

    // VLC options
    char smem_options[512];
    snprintf(smem_options, sizeof(smem_options),
            "#transcode{acodec=s16l,samplerate=%d}:"
            // We are using transcode because smem only support raw audio and
            // video formats
            "smem{"
                "audio-postrender-callback=%lld,"
                "audio-prerender-callback=%lld"
            "}",
            vlc_rate,
            (long long int)(intptr_t)(void*)&handleStream,
            (long long int)(intptr_t)(void*)&prepareRender);

    char verb_options[512];
    snprintf(verb_options, sizeof(verb_options),
            "--verbose=%d", verbosity);

    const char * const vlc_args[] = {
        verb_options,
        "--sout", smem_options // Stream to memory
    };

    // Launch VLC
    m_vlc = libvlc_new(sizeof(vlc_args) / sizeof(vlc_args[0]), vlc_args);

    // Load the media
    libvlc_media_t *m;
    m = libvlc_media_new_location(m_vlc, uri);
    m_mp = libvlc_media_player_new_from_media(m);
    libvlc_media_release(m);

    // Allocate the list
    head_buffer = vlc_buffer_new();

    // Start playing
    int ret = libvlc_media_player_play(m_mp);

    if (ret == 0) {
        libvlc_media_t *media = libvlc_media_player_get_media(m_mp);
        libvlc_state_t st;

        ret = -1;

        int timeout;
        for (timeout = 0; timeout < 100; timeout++) {
            st = libvlc_media_get_state(media);
            usleep(10*1000);
            if (st != libvlc_NothingSpecial) {
                ret = 0;
                break;
            }
        }
    }

    return ret;
}

ssize_t vlc_in_read(void *buf, size_t len)
{
    size_t requested = len;
    for (;;) {
        pthread_mutex_lock(&buffer_lock);

        if (vlc_buffer_totalsize(head_buffer) >= len) {
            while (len >= head_buffer->size) {
                if (head_buffer->buf) {
                    // Get all the data from this list element
                    memcpy(buf, head_buffer->buf, head_buffer->size);

                    buf += head_buffer->size;
                    len -= head_buffer->size;
                }

                if (head_buffer->next) {
                    struct vlc_buffer *next_head = head_buffer->next;
                    vlc_buffer_free(head_buffer);
                    head_buffer = next_head;
                }
                else {
                    vlc_buffer_free(head_buffer);
                    head_buffer = vlc_buffer_new();
                    break;
                }
            }

            if (len > 0) {
                assert(len < head_buffer->size);

                memcpy(buf, head_buffer->buf, len);

                // split the current head into two parts
                size_t remaining = head_buffer->size - len;
                uint8_t *newbuf = malloc(remaining);

                memcpy(newbuf, head_buffer->buf + len, remaining);
                free(head_buffer->buf);
                head_buffer->buf = newbuf;
                head_buffer->size = remaining;
            }

            pthread_mutex_unlock(&buffer_lock);
            return requested;
        }

        pthread_mutex_unlock(&buffer_lock);
        usleep(100);

        libvlc_media_t *media = libvlc_media_player_get_media(m_mp);
        libvlc_state_t st = libvlc_media_get_state(media);
        if (!(st == libvlc_Opening   ||
              st == libvlc_Buffering ||
              st == libvlc_Playing) ) {
            return -1;
        }
    }

    abort();
}

