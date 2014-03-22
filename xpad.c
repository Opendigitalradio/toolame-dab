#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>

#include "xpad.h"

static int xpad_fd = 0;

uint16_t xpad_fpad() {
    uint16_t fpad = 0x2; // CI flag

    if (xpad_len()) {
        fpad |= 1<<13; // variable length X-PAD
    }

    return fpad;
}

int xpad_len() {
    if (xpad_fd == 0) {
        xpad_fd = open("/home/bram/dab/mot-slideshow.file", O_RDONLY);
        if (xpad_fd < 0) {
            perror("Failed to open xpad file");
            exit(1);
        }
    }
    return 1;
}

uint8_t xpad_byte(void) {
    uint8_t dat;

    assert(xpad_fd != 0);
    ssize_t num_read = read(xpad_fd, &dat, 1);

    if (num_read == 0) {
        fprintf(stderr, "xpad rewind\n");
        lseek(xpad_fd, 0, SEEK_SET);
        num_read = read(xpad_fd, &dat, 1);

        assert(num_read == 1);
    }

    return dat;
}

