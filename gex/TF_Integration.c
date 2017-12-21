#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include "TinyFrame.h"
#include "utils/hexdump.h"

#include "gex.h"
#include "gex_internal.h"

void TF_WriteImpl(TinyFrame *tf, const uint8_t *buff, size_t len)
{
    GexClient *gc = tf->userdata;
	assert(gc->acm_fd != 0);

//    hexDump("TF_Write", buff, (uint32_t)len);

	ssize_t rv = write(gc->acm_fd, buff, len);
	if (rv != (ssize_t)len) {
		fprintf(stderr, "ERROR %d in TF write: %s\n", errno, strerror(errno));
	}
}

/** Claim the TX interface before composing and sending a frame */
void TF_ClaimTx(TinyFrame *tf)
{
    (void)tf;
}

/** Free the TX interface after composing and sending a frame */
void TF_ReleaseTx(TinyFrame *tf)
{
    (void)tf;
}
