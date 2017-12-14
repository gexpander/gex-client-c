#include "TinyFrame.h"
#include "gex_client.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include "gex_client_internal.h"

void TF_WriteImpl(TinyFrame *tf, const uint8_t *buff, size_t len)
{
    GexClient *gc = tf->userdata;
	assert(gc->acm_fd != 0);

	ssize_t rv = write(gc->acm_fd, buff, len);
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
