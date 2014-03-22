#include "zmqoutput.h"
#include <zmq.h>
#include <stdlib.h>
#include "common.h"

static void *zmq_context;

// Buffer containing at maximum one frame
unsigned char* zmqbuf;

// The current data length (smaller than allocated
// buffer size)
size_t zmqbuf_len;


int zmqoutput_open(Bit_stream_struc *bs, char* uri)
{
    zmq_context = zmq_ctx_new();
    bs->zmq_sock = zmq_socket(zmq_context, ZMQ_PUB);
    if (bs->zmq_sock == NULL) {
        fprintf(stderr, "Error occurred during zmq_socket: %s\n",
                zmq_strerror(errno));
        return -1;
    }
    if (zmq_connect(bs->zmq_sock, uri) != 0) {
        fprintf(stderr, "Error occurred during zmq_connect: %s\n",
                zmq_strerror(errno));
        return -1;
    }

    zmqbuf = (unsigned char*)malloc(bs->zmq_framesize);
    if (zmqbuf == NULL) {
        fprintf(stderr, "Unable to allocate ZMQ buffer\n");
        exit(0);
    }
    zmqbuf_len = 0;
    return 0;
}

int zmqoutput_write_byte(Bit_stream_struc *bs, unsigned char data)
{
    zmqbuf[zmqbuf_len++] = data;

    if (zmqbuf_len == bs->zmq_framesize) {

        int send_error = zmq_send(bs->zmq_sock, zmqbuf, bs->zmq_framesize,
                ZMQ_DONTWAIT);

        if (send_error < 0) {
            fprintf(stderr, "ZeroMQ send failed! %s\n", zmq_strerror(errno));
        }

        zmqbuf_len = 0;

        return bs->zmq_framesize;
    }

    return 0;

}

void zmqoutput_close(Bit_stream_struc *bs)
{
    if (bs->zmq_sock)
        zmq_close(bs->zmq_sock);

    if (zmq_context)
        zmq_ctx_destroy(zmq_context);

    if (zmqbuf) {
        free(zmqbuf);
        zmqbuf = NULL;
    }
}

