#include "TinyFrame.h"
#include "gex_client.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

void TF_WriteImpl(const uint8_t *buff, size_t len)
{
	assert(gex_serial_fd != 0);

    ssize_t rv = write(gex_serial_fd, buff, len);
	if (rv != len) {
		fprintf(stderr, "ERROR %d in TF write: %s\n", errno, strerror(errno));
	}
}

/** Claim the TX interface before composing and sending a frame */
void TF_ClaimTx(void)
{
    //
}

/** Free the TX interface after composing and sending a frame */
void TF_ReleaseTx(void)
{
    //
}
