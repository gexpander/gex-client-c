//
// Created by MightyPork on 2017/12/12.
//

#include <malloc.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "TinyFrame.h"

#include "gex_client.h"
#include "serial.h"
#include "gex_client_internal.h"
#include "gex_message_types.h"
#include "utils/payload_parser.h"

/** Callback for ping */
TF_Result connectivityCheckCb(TinyFrame *tf, TF_Msg *msg)
{
    GexClient *gex = tf->userdata;
    gex->connected = true;
    fprintf(stderr, "GEX connected! Version string: %.*s\n", msg->len, msg->data);
    return TF_CLOSE;
}

/** Delete recursively all GEX callsign look-up table entries */
static void destroy_unit_lookup(GexClient *gex)
{
    struct gex_name_lu *next = gex->ulu_head;
    while (next != NULL) {
        struct gex_name_lu *cur = next;
        next = next->next;
        free(cur);
    }
    gex->ulu_head = NULL;
}

/** Get callsign for unit name */
static uint8_t find_callsign_by_name(GexClient *gex, const char *name)
{
    struct gex_name_lu *next = gex->ulu_head;
    while (next != NULL) {
        if (strcmp(next->name, name) == 0) {
            return next->callsign;
        }
        next = next->next;
    }
    return 0;
}

/** Listener for the "list units" query response */
TF_Result listUnitsCb(TinyFrame *tf, TF_Msg *msg)
{
    GexClient *gex = tf->userdata;

    destroy_unit_lookup(gex);

    PayloadParser pp = pp_start((uint8_t*)msg->data, msg->len, NULL);
    uint8_t count = pp_u8(&pp);
    char buf[100];
    struct gex_name_lu *tail = NULL;
    for(int i=0; i< count; i++) {
        uint8_t cs = pp_u8(&pp);
        pp_string(&pp, buf, 100);
        fprintf(stderr, "Available unit \"%s\" @ %d\n", buf, cs);

        // append
        struct gex_name_lu *lu = malloc(sizeof(struct gex_name_lu));
        lu->next = NULL;
        lu->type = "UNKNOWN";
        lu->name = strdup(buf);
        lu->callsign = cs;
        if (tail == NULL) {
            gex->ulu_head = lu;
        } else {
            tail->next = lu;
        }
        tail = lu;
    }

    return TF_CLOSE;
}

/** Create a instance and connect */
GexClient *GEX_Init(const char *device, int timeout_ms)
{
    assert(device != NULL);

    // --- Init the struct ---
    GexClient *gex = calloc(1, sizeof(GexClient));
    assert(gex != NULL);

    // --- Open the device ---
    gex->acm_device = device;
    gex->acm_fd = serial_open(device, false, (timeout_ms + 50) / 100);
    if (gex->acm_fd == -1) {
        free(gex);
        return NULL;
    }

    gex->tf = TF_Init(TF_MASTER);
    gex->tf->userdata = gex;

    // --- Test connectivity ---
    TF_QuerySimple(gex->tf, MSG_PING, /*pld*/ NULL, 0, /*cb*/ connectivityCheckCb, 0);
    GEX_Poll(gex);

    if (!gex->connected) {
        fprintf(stderr, "GEX doesn't respond to ping!\n");
        GEX_DeInit(gex);
        return NULL;
    }

    // --- populate callsign look-up table ---
    TF_QuerySimple(gex->tf, MSG_LIST_UNITS, /*pld*/ NULL, 0, /*cb*/ listUnitsCb, 0);
    GEX_Poll(gex);

    return gex;
}

/** Try to read from the serial port and process any received bytes with TF */
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

/** Free the struct */
void GEX_DeInit(GexClient *gex)
{
    if (gex == NULL) return;
    close(gex->acm_fd);
    destroy_unit_lookup(gex);
    TF_DeInit(gex->tf);
    free(gex);
}

/** Query a unit */
void GEX_QueryUnit(GexClient *gex,
                   const char *unit, uint8_t cmd,
                   uint8_t *payload, uint32_t len,
                   TF_Listener listener)
{
    uint8_t cs = find_callsign_by_name(gex, unit);
    assert(cs != 0);
    uint8_t *pld = malloc(len + 2);
    assert(pld != NULL);

    pld[0] = cs;
    pld[1] = cmd;
    memcpy(pld+2, payload, len);

    TF_Msg msg;
    TF_ClearMsg(&msg);
    msg.type = MSG_UNIT_REQUEST;
    msg.data = pld;
    msg.len = (TF_LEN) (len + 2);
    TF_Query(gex->tf, &msg, listener, 0);
    free(pld);

    GEX_Poll(gex);
}

// TODO add Send command (no query)
// TODO add listener for spontaneous device reports with user configurable handler (per unit)
