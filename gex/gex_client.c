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

#define GEX_H // to allow including other headers
#include "gex_client.h"
#include "serial.h"
#include "gex_internal.h"
#include "gex_message_types.h"
#include "utils/payload_parser.h"
#include "utils/hexdump.h"


/** Delete recursively all GEX callsign look-up table entries */
static void destroy_unit_lookup(GexClient *gex);

/** Get lookup entry for unit name */
static GexUnit *find_unit_by_callsign(GexClient *gex, uint8_t callsign);

/** Get lookup entry for unit name */
static GexUnit *find_unit_by_name(GexClient *gex, const char *name);

/** Get callsign for unit name */
static uint8_t find_callsign_by_name(GexClient *gex, const char *name);

// --------------------------------

/** Get the system unit */
GexUnit *GEX_SysUnit(GexClient *gex)
{
    return &gex->system_unit;
}

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

    GexUnit *lu = find_unit_by_callsign(gex, callsign);

    GexMsg gexMsg = {
        .payload = (uint8_t *) (msg->data + 2),
        .len = (uint32_t) (msg->len - 2),
        .type = rpt_type,
        .unit = (lu == NULL) ? &gex->system_unit : lu,
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

    destroy_unit_lookup(gex);

    // Parse the payload
    PayloadParser pp = pp_start((uint8_t*)msg->data, msg->len, NULL);
    uint8_t count = pp_u8(&pp);
    char buf[20];
    char buf2[20];
    GexUnit *tail = NULL;
    for(int i = 0; i < count; i++) {
        uint8_t callsign = pp_u8(&pp);
        pp_string(&pp, buf, 20);
        pp_string(&pp, buf2, 20);
        fprintf(stderr, "- Found unit \"%s\" (type %s) @ callsign %d\n", buf, buf2, callsign);

        // append a unit instance
        GexUnit *lu = malloc(sizeof(GexUnit));
        lu->next = NULL;
        lu->type = strdup(buf2);
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

    gex->units_loaded = true;

    return TF_CLOSE;
}

/** Bind report listener */
void GEX_SetUnitReportListener(GexUnit *unit, GexEventListener lst)
{
    unit->report_handler = lst;
}

/** Bind report listener */
void GEX_SetDefaultReportListener(GexClient *gex, GexEventListener lst)
{
    gex->fallback_report_handler = lst;
}

/** Get raw TinyFrame instance */
TinyFrame *GEX_GetTF(GexClient *gex)
{
    return gex->tf;
}

/** Find a unit */
GexUnit *GEX_GetUnit(GexClient *gex, const char *name, const char *type)
{
    GexUnit *u = find_unit_by_name(gex, name);
    if (u == NULL) {
        fprintf(stderr, "!! Unit %s not found!\n", name);
        return NULL;
    }

    if (type != NULL && strcmp(u->type, type) != 0) {
        fprintf(stderr, "!! Unit %s has type %s, expected %s!\n", name, u->type, type);
        return NULL;
    }

    return u;
}

/** Unhandled frame listener - for debug */
static TF_Result hdl_default(TinyFrame *tf, TF_Msg*msg)
{
    (void)tf;
    fprintf(stderr, "TF unhandled msg len %d, type %d, id %d", msg->len, msg->type, msg->frame_id);
    hexDump("payload", msg->data, msg->len);
    return TF_STAY;
}

/** Create a instance and connect */
GexClient *GEX_Init(const char *device, uint32_t timeout_ms)
{
    assert(device != NULL);

    // --- Init the struct ---
    GexClient *gex = calloc(1, sizeof(GexClient));
    assert(gex != NULL);

    // Init the dummy system unit
    gex->system_unit = (GexUnit){
            .name = "SYSTEM",
            .type = "SYSTEM",
            .callsign = 0,
            .gex = gex,
            .next = NULL,
            .report_handler = NULL
    };

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
    TF_QuerySimple(gex->tf, MSG_PING,
                   NULL, 0,
                   connectivity_check_lst, 0);
    GEX_Poll(gex, &gex->connected);

    if (!gex->connected) {
        fprintf(stderr, "GEX doesn't respond to ping!\n");
        GEX_DeInit(gex);
        return NULL;
    }

    // --- populate callsign look-up table ---
    fprintf(stderr, "Loading available units info...\n");
    TF_QuerySimple(gex->tf, MSG_LIST_UNITS,
                   NULL, 0,
                   list_units_lst, 0);
    GEX_Poll(gex, &gex->units_loaded);

    // Bind the catch-all event handler. Will be re-distributed to individual unit listeners if needed.
    TF_AddTypeListener(gex->tf, MSG_UNIT_REPORT, unit_report_lst);

    // Fallback TF listener for reporting unhandled frames
    TF_AddGenericListener(GEX_GetTF(gex), hdl_default);

    return gex;
}

/** Try to read from the serial port and process any received bytes with TF */
void GEX_Poll(GexClient *gex, const volatile bool *done_flag)
{
    static uint8_t pollbuffer[TF_MAX_PAYLOAD_RX];

    assert(gex != NULL);

    bool first = true;

#define MAX_RETRIES 15

    int cycle = 0;
    do {
        // The first read is blocking up to a timeout and waits for data
        if (first) serial_shouldwait(gex->acm_fd, gex->ser_timeout);
        ssize_t len = read(gex->acm_fd, pollbuffer, TF_MAX_PAYLOAD_RX);
        if (first) {
            // Following reads are non-blocking and just grab data from the system buffer
            serial_noblock(gex->acm_fd);
            first = false;
        }

        // abort on error
        if (len < 0) {
            fprintf(stderr, "ERROR %d in GEX Poll: %s\n", errno, strerror(errno));
            break;
        }
        else {
            // nothing received?
            if (len == 0) {
                // keep trying to read if we have a reason to expect more data
                if (gex->tf->state != 0) {
                    if (cycle < MAX_RETRIES) {
                        // start a new cycle, setting 'first' to use a blocking read
                        cycle++;
                        first=true;
                    } else {
                        // tries exhausted
                        break;
                    }
                } else {
                    // nothing more received and TF is in the base state, we're done.
                    if (done_flag != NULL) {
                        if (*done_flag == true) {
                            break;
                        } else {
                            cycle++;
                            first=true;
                            if (cycle >= MAX_RETRIES) break;
                        }
                    } else {
                        break;
                    }
                }
            }
            else {
                // process the received data
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
    destroy_unit_lookup(gex);
    TF_DeInit(gex->tf);
    free(gex);
}

// --------------------------------------------------------


/** Delete recursively all GEX callsign look-up table entries */
static void destroy_unit_lookup(GexClient *gex)
{
    assert(gex != NULL);

    GexUnit *next = gex->ulu_head;
    while (next != NULL) {
        GexUnit *cur = next;
        next = next->next;
        free(cur->name);
        free(cur->type);
        free(cur);
    }
    gex->ulu_head = NULL;
}

/** Get lookup entry for unit name */
static GexUnit *find_unit_by_callsign(GexClient *gex, uint8_t callsign)
{
    assert(gex != NULL);

    GexUnit *next = gex->ulu_head;
    while (next != NULL) {
        if (next->callsign == callsign) {
            return next;
        }
        next = next->next;
    }
    return NULL;
}

/** Get lookup entry for unit name */
static GexUnit *find_unit_by_name(GexClient *gex, const char *name)
{
    assert(gex != NULL);
    assert(name != NULL);

    GexUnit *next = gex->ulu_head;
    while (next != NULL) {
        if (strcmp(next->name, name) == 0) {
            return next;
        }
        next = next->next;
    }
    return NULL;
}

/** Get callsign for unit name */
static uint8_t find_callsign_by_name(GexClient *gex, const char *name)
{
    assert(gex != NULL);

    GexUnit *lu = find_unit_by_name(gex, name);
    return (uint8_t) ((lu == NULL) ? 0 : lu->callsign);
}
