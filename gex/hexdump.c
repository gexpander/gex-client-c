//
// Created by MightyPork on 2017/12/04.
//

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <inttypes.h>
#include <stdio.h>
#include "hexdump.h"

void hexDump(const char *restrict desc, const void *restrict addr, uint32_t len)
{
    uint32_t i;
    uint8_t buff[17];
    uint8_t *pc = (unsigned char*)addr;

    // Output description if given.
    if (desc != NULL)
        fprintf(stderr, "%s:\r\n", desc);

    if (len == 0) {
        fprintf(stderr, "  ZERO LENGTH\r\n");
        return;
    }

    // Process every byte in the data.
    for (i = 0; i < len; i++) {
        // Multiple of 16 means new line (with line offset).

        if ((i % 16) == 0) {
            // Just don't print ASCII for the zeroth line.
            if (i != 0)
                fprintf(stderr, "  %s\r\n", buff);

            // Output the offset.
            fprintf(stderr, "  %04"PRIx32" ", i);
        }

        // Now the hex code for the specific character.
        fprintf(stderr, " %02x", pc[i]);

        // And store a printable ASCII character for later.
        if ((pc[i] < 0x20) || (pc[i] > 0x7e))
            buff[i % 16] = '.';
        else
            buff[i % 16] = pc[i];
        buff[(i % 16) + 1] = '\0';
    }

    // Pad out last line if not exactly 16 characters.
    while ((i % 16) != 0) {
        fprintf(stderr, "   ");
        i++;
    }

    // And print the final ASCII bit.
    fprintf(stderr, "  %s\r\n", buff);

    (void)buff;
}
