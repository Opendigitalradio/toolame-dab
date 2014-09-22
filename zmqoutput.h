#ifndef _ZMQOUTPUT_H_
#define _ZMQOUTPUT_H_

#include <stdint.h>
#include "common.h"

#define ZMQ_ENCODER_TOOLAME 2

struct zmq_frame_header
{
    uint16_t version; // we support version=1 now
    uint16_t encoder; // see ZMQ_ENCODER_XYZ

    /* length of the 'data' field */
    uint32_t datasize;

    /* Audio level, peak, linear PCM */
    int16_t audiolevel_left;
    int16_t audiolevel_right;

    /* Data follows this header */
} __attribute__ ((packed));


int zmqoutput_open(Bit_stream_struc * bs, char* uri);

int zmqoutput_write_byte(Bit_stream_struc *bs, unsigned char data);

void zmqoutput_close(Bit_stream_struc *bs);

void zmqoutput_set_peaks(int left, int right);

#endif

