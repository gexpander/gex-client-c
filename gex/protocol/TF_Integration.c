#include "TinyFrame.h"
#include "gex_client.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

void TF_WriteImpl(TinyFrame *tf, const uint8_t *buff, size_t len)
{
	assert(gex_serial_fd != 0); // TODO update after TF has instances

	ssize_t rv = write(gex_serial_fd, buff, len);
	if (rv != len) {
		fprintf(stderr, "ERROR %d in TF write: %s\n", errno, strerror(errno));
	}
}

/** Claim the TX interface before composing and sending a frame */
void TF_ClaimTx(TinyFrame *tf)
{
    //
}

/** Free the TX interface after composing and sending a frame */
void TF_ReleaseTx(TinyFrame *tf)
{
    //
}
