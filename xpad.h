#ifndef _XPAD_H_
#define _XPAD_H_

#include <stdint.h>

/* Return the number of bytes of x-pad data
 * we have ready
 */
int xpad_len(void);

/* Get one x-pad byte
 */
uint8_t xpad_byte(void);

/* Calculate the two F-PAD bytes */
uint16_t xpad_fpad(void);

#endif

