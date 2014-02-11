#ifndef _ZMQOUTPUT_H_
#define _ZMQOUTPUT_H_

#include "common.h"

int zmqoutput_open(Bit_stream_struc * bs, char* uri);

int zmqoutput_write_byte(Bit_stream_struc *bs, unsigned char data);

void zmqoutput_close(Bit_stream_struc *bs);

#endif

