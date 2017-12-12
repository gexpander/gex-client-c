//
// Created by MightyPork on 2017/12/12.
//

#include <malloc.h>
#include <assert.h>
#include <unistd.h>
#include <signal.h>
#include  <stdlib.h>
#include <protocol/TinyFrame.h>
#include <string.h>
#include <errno.h>

#include "gex_client.h"
#include "serial.h"
#include "hexdump.h"

int gex_serial_fd = -1;


static void sigintHandler(int sig)
{
	if (gex_serial_fd != -1) {
		close(gex_serial_fd);
	}
	exit(0);
}

TF_Result connectivityCheckCb(TF_Msg *msg)
{
	GexClient *gc = msg->userdata;
	gc->connected = true;
	fprintf(stderr, "GEX connected! Version string: %.*s\n", msg->len, msg->data);

	msg->userdata = NULL;
	return TF_CLOSE;
}

GexClient *GEX_Init(const char *device, int timeout_ms)
{
	assert(device != NULL);

	GexClient *gc = calloc(1, sizeof(GexClient));
	assert(gc != NULL);

	// Bind ^C handler for safe shutdown
	signal(SIGINT, sigintHandler);

	// Open the device
	gc->acm_device = device;
	gc->acm_fd = serial_open(device, false, (timeout_ms+50)/100);
	if (gc->acm_fd == -1) {
		free(gc);
		return NULL;
	}

	gex_serial_fd = gc->acm_fd;

	// Test connectivity
	TF_Msg msg;
	TF_ClearMsg(&msg);
	msg.type = 0x01; // TODO use constant
	msg.userdata = gc;
	TF_Query(&msg, connectivityCheckCb, 0);
	GEX_Poll(gc);

	if (!gc->connected) {
		fprintf(stderr, "GEX doesn't respond to ping!\n");
		GEX_DeInit(gc);
		return NULL;
	}

	// TODO load and store unit callsigns + names

	return gc;
}


void GEX_Poll(GexClient *gc)
{
	static uint8_t pollbuffer[4096];

	assert(gc != NULL);

	ssize_t len = read(gc->acm_fd, pollbuffer, 4096);
	if (len < 0) {
		fprintf(stderr, "ERROR %d in GEX Poll: %s\n", errno, strerror(errno));
	} else {
//		hexDump("Received", pollbuffer, (uint32_t) len);
		TF_Accept(pollbuffer, (size_t) len);
	}
}


void GEX_DeInit(GexClient *gc)
{
	if (gc == NULL) return;

	close(gc->acm_fd);
	gex_serial_fd = -1;
	free(gc);
}
