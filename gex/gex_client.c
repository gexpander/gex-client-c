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
#include "gex_internal.h"
#include "gex_message_types.h"
#include "gex_helpers.h"
#include "utils/payload_parser.h"

/** Callback for ping */
static TF_Result connectivity_check_lst(TinyFrame *tf, TF_Msg *msg)
{
    GexClient *gex = tf->userdata;
    gex->connected = true;
    fprintf(stderr, "GEX connected! Version string: %.*s\n", msg->len, msg->data);
    return TF_CLOSE;
}

/** Callback for ping */
static TF_Result unit_report_lst(TinyFrame *tf, TF_Msg *msg)
{
    GexClient *gex = tf->userdata;

    // payload must start by callsign and report type.
    if (msg->len < 2) goto done;
    uint8_t callsign = msg->data[0];
    uint8_t rpt_type = msg->data[1];

    struct gex_unit_lu *lu = gex_find_unit_by_callsign(gex, callsign);
    if (lu && lu->report_handler) {
        lu->report_handler(gex, lu->name, rpt_type,
                           msg->data+2, (uint32_t) (msg->len - 2));
    } else if (gex->fallback_report_handler) {
        gex->fallback_report_handler(gex, (lu ? lu->name : "UNKNOWN"), rpt_type,
                                     msg->data+2, (uint32_t) (msg->len - 2));
    }

done:
    return TF_STAY;
}

/** Listener for the "list units" query response */
static TF_Result list_units_lst(TinyFrame *tf, TF_Msg *msg)
{
    GexClient *gex = tf->userdata;

    gex_destroy_unit_lookup(gex);

    PayloadParser pp = pp_start((uint8_t*)msg->data, msg->len, NULL);
    uint8_t count = pp_u8(&pp);
    char buf[100];
    struct gex_unit_lu *tail = NULL;
    for(int i = 0; i < count; i++) {
        uint8_t callsign = pp_u8(&pp);
        pp_string(&pp, buf, 100);
        fprintf(stderr, "- Found unit \"%s\" @ callsign %d\n", buf, callsign);

        // append
        struct gex_unit_lu *lu = malloc(sizeof(struct gex_unit_lu));
        lu->next = NULL;
        lu->type = "UNKNOWN";
        lu->name = strdup(buf);
        lu->callsign = callsign;
        lu->report_handler = NULL;
        if (tail == NULL) {
            gex->ulu_head = lu;
        } else {
            tail->next = lu;
        }
        tail = lu;
    }

    return TF_CLOSE;
}

/** Bind report listener */
void GEX_OnReport(GexClient *gex, const char *unit_name, GEX_ReportListener lst)
{
    if (!unit_name) {
        gex->fallback_report_handler = lst;
    }
    else {
        struct gex_unit_lu *lu = gex_find_unit_by_name(gex, unit_name);
        if (!lu) {
            fprintf(stderr, "No unit named \"%s\", can't bind listener!", unit_name);
        }
        else {
            lu->report_handler = lst;
        }
    }
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
    fprintf(stderr, "Testing connection...\n");
    TF_QuerySimple(gex->tf, MSG_PING, /*pld*/ NULL, 0, /*cb*/ connectivity_check_lst, 0);
    GEX_Poll(gex);

    if (!gex->connected) {
        fprintf(stderr, "GEX doesn't respond to ping!\n");
        GEX_DeInit(gex);
        return NULL;
    }

    // --- populate callsign look-up table ---
    fprintf(stderr, "Loading available units info...\n");
    TF_QuerySimple(gex->tf, MSG_LIST_UNITS, /*pld*/ NULL, 0, /*cb*/ list_units_lst, 0);
    GEX_Poll(gex);

    // Bind the catch-all event handler. Will be re-distributed to individual unit listeners if needed.
    TF_AddTypeListener(gex->tf, MSG_UNIT_REPORT, unit_report_lst);

    return gex;
}


/** Try to read from the serial port and process any received bytes with TF */
void GEX_Poll(GexClient *gex)
{
    uint8_t pollbuffer[TF_MAX_PAYLOAD_RX];

    assert(gex != NULL);

    ssize_t len = read(gex->acm_fd, pollbuffer, TF_MAX_PAYLOAD_RX);
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
    gex_destroy_unit_lookup(gex);
    TF_DeInit(gex->tf);
    free(gex);
}


/** Query a unit */
void GEX_Query(GexClient *gex,
               const char *unit, uint8_t cmd,
               uint8_t *payload, uint32_t len,
               TF_Listener listener)
{
    uint8_t callsign = gex_find_callsign_by_name(gex, unit);
    assert(callsign != 0);
    uint8_t *pld = malloc(len + 2);
    assert(pld != NULL);
    {
        // prefix the actual payload with the callsign and command bytes.
        // TODO provide TF API for sending the payload externally in smaller chunks? Will avoid the malloc here
        pld[0] = callsign;
        pld[1] = cmd;
        memcpy(pld + 2, payload, len);

        TF_Msg msg;
        TF_ClearMsg(&msg);
        msg.type = MSG_UNIT_REQUEST;
        msg.data = pld;
        msg.len = (TF_LEN) (len + 2);
        TF_Query(gex->tf, &msg, listener, 0);
    }
    free(pld);

    if (NULL != listener) {
        GEX_Poll(gex);
    }
}


/** listener for the synchronous query functionality */
static TF_Result sync_query_lst(TinyFrame *tf, TF_Msg *msg)
{
    GexClient *gex = tf->userdata;
    // clone the message
    memcpy(&gex->sync_query_response, msg, sizeof(TF_Msg));
    // clone the buffer
    if (msg->len > 0) memcpy(gex->sync_query_buffer, msg->data, msg->len);
    // re-link the buffer
    gex->sync_query_response.data = gex->sync_query_buffer;
    gex->sync_query_ok = true;
}


/** Query a unit. The response is expected to be relatively short. */
TF_Msg *GEX_SyncQuery(GexClient *gex,
                   const char *unit, uint8_t cmd,
                   uint8_t *payload, uint32_t len)
{
    gex->sync_query_ok = false;
    memset(&gex->sync_query_response, 0, sizeof(TF_Msg));
    GEX_Query(gex, unit, cmd, payload, len, sync_query_lst);
    return gex->sync_query_ok ? &gex->sync_query_response : NULL;
}

/** Command a unit (same like query, but without listener and without polling) */
void GEX_Send(GexClient *gex,
              const char *unit, uint8_t cmd,
              uint8_t *payload, uint32_t len)
{
    GEX_Query(gex, unit, cmd, payload, len, NULL);
}
