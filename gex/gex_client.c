//
// Created by MightyPork on 2017/12/12.
//

#include <malloc.h>
#include <assert.h>
#include <unistd.h>
#include  <stdlib.h>
#include <string.h>
#include <errno.h>

#include "TinyFrame.h"
#include "gex_client.h"
#include "serial.h"

TF_Result connectivityCheckCb(TinyFrame *tf, TF_Msg *msg)
{
    GexClient *gex = tf->userdata;
    gex->connected = true;
    fprintf(stderr, "GEX connected! Version string: %.*s\n", msg->len, msg->data);
    return TF_CLOSE;
}

GexClient *GEX_Init(const char *device, int timeout_ms)
{
    assert(device != NULL);

    GexClient *gex = calloc(1, sizeof(GexClient));
    assert(gex != NULL);

    // Open the device
    gex->acm_device = device;
    gex->acm_fd = serial_open(device, false, (timeout_ms + 50) / 100);
    if (gex->acm_fd == -1) {
        free(gex);
        return NULL;
    }

    gex->tf = TF_Init(TF_MASTER);
    gex->tf->userdata = gex;

    // Test connectivity
    TF_Msg msg;
    TF_ClearMsg(&msg);
    msg.type = 0x01; // TODO use constant
    TF_Query(gex->tf, &msg, connectivityCheckCb, 0);
    GEX_Poll(gex);

    if (!gex->connected) {
        fprintf(stderr, "GEX doesn't respond to ping!\n");
        GEX_DeInit(gex);
        return NULL;
    }

    // TODO load and store unit callsigns + names

    return gex;
}


void GEX_Poll(GexClient *gex)
{
    uint8_t pollbuffer[1024];

    assert(gex != NULL);

    ssize_t len = read(gex->acm_fd, pollbuffer, 1024);
    if (len < 0) {
        fprintf(stderr, "ERROR %d in GEX Poll: %s\n", errno, strerror(errno));
    } else {
        //hexDump("Received", pollbuffer, (uint32_t) len);
        TF_Accept(gex->tf, pollbuffer, (size_t) len);
    }
}


void GEX_DeInit(GexClient *gc)
{
    if (gc == NULL) return;
    close(gc->acm_fd);
    TF_DeInit(gc->tf);
    free(gc);
}
