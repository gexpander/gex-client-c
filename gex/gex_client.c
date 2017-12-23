//
// Created by MightyPork on 2017/12/12.
//

#include <malloc.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <utils/hexdump.h>
#include "TinyFrame.h"

#define GEX_H // to allow including other headers
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

    struct gex_unit *lu = gex_find_unit_by_callsign(gex, callsign);

    // NULL object pattern - we use a fake unit if no unit matched.
    GexUnit fbu = {
        .callsign = 0,
        .report_handler = NULL,
        .type = "NONE",
        .name = "FALLBACK",
        .gex = gex, // gex must be available here - this is why we can't have this static or const.
        .next = NULL,
    };

    GexMsg gexMsg = {
        .payload = (uint8_t *) (msg->data + 2),
        .len = (uint32_t) (msg->len - 2),
        .type = rpt_type,
        .unit = (lu == NULL) ? &fbu : lu,
    };

    if (lu && lu->report_handler) {
        lu->report_handler(gexMsg);
    } else if (gex->fallback_report_handler) {
        gex->fallback_report_handler(gexMsg);
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
    struct gex_unit *tail = NULL;
    for(int i = 0; i < count; i++) {
        uint8_t callsign = pp_u8(&pp);
        pp_string(&pp, buf, 100);
        fprintf(stderr, "- Found unit \"%s\" @ callsign %d\n", buf, callsign);

        // append
        struct gex_unit *lu = malloc(sizeof(struct gex_unit));
        lu->next = NULL;
        lu->type = strdup("UNKNOWN"); // TODO
        lu->name = strdup(buf);
        lu->callsign = callsign;
        lu->gex = gex;
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
void GEX_OnReport(GexClient *gex, GexUnit *unit, GexEventListener lst)
{
    if (!unit) {
        gex->fallback_report_handler = lst;
    }
    else {
        unit->report_handler = lst;
    }
}

TinyFrame *GEX_GetTF(GexClient *gex)
{
    return gex->tf;
}

/** Find a unit */
GexUnit *GEX_Unit(GexClient *gex, const char *name)
{
    GexUnit *u = gex_find_unit_by_name(gex, name);
    if (u == NULL) {
        fprintf(stderr, "!! Unit %s not found!\n", name);
    }
    return u;
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
    gex->ser_timeout = timeout_ms;
    gex->acm_fd = serial_open(device);
    if (gex->acm_fd == -1) {
        free(gex);
        fprintf(stderr, "FAILED TO CONNECT TO %s!\n", device);
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
    static uint8_t pollbuffer[TF_MAX_PAYLOAD_RX];

    assert(gex != NULL);

    bool first = true;

#define MAX_RETRIES 10

    int cycle = 0;
    do {
        if (first) serial_shouldwait(gex->acm_fd, gex->ser_timeout);
        ssize_t len = read(gex->acm_fd, pollbuffer, TF_MAX_PAYLOAD_RX);
        if (first) {
            serial_noblock(gex->acm_fd);
            first = false;
        }

        if (len < 0) {
            fprintf(stderr, "ERROR %d in GEX Poll: %s\n", errno, strerror(errno));
            break;
        }
        else {
            if (len == 0) {
                fprintf(stderr,"No more data to read.\n");
                if (gex->tf->state != 0) {
                    if (cycle < MAX_RETRIES) {
                        cycle++;
                        first=true;
                    } else {
                        break;
                    }
                } else {
                    break;
                }
            }
            else {
                fprintf(stderr, "rx %d bytes\n", (int) len);
                hexDump("TF_Receive", pollbuffer, (uint32_t) len);

                TF_Accept(gex->tf, pollbuffer, (size_t) len);
            }
        }
    } while(1);
}

/** Free the struct */
void GEX_DeInit(GexClient *gex)
{
    if (gex == NULL) return;
    fsync(gex->acm_fd);
    gex_destroy_unit_lookup(gex);
    TF_DeInit(gex->tf);
    free(gex);
}
