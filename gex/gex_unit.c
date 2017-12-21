//
// Created by MightyPork on 2017/12/19.
//

#include <malloc.h>
#include <assert.h>
#include "utils/payload_parser.h"
#include "utils/payload_builder.h"

#define GEX_H // to allow including other headers
#include "gex_defines.h"
#include "gex_helpers.h"
#include "gex_message_types.h"
#include "gex_unit.h"
#include "gex_internal.h"

/**
 * Low level query
 *
 * @param unit - GEX unit to address. The unit is available in userdata1 of the response message, if any
 * @param cmd - command byte
 * @param payload - payload buffer
 * @param len - payload length
 * @param lst - TF listener to handle the response, can be NULL
 * @param userdata2 userdata2 argument for the TF listener's message
 */
static void GEX_LL_Query(GexUnit *unit, uint8_t cmd,
                         const uint8_t *payload, uint32_t len,
                         GexSession session, bool is_reply,
                         TF_Listener lst, void *userdata2,
                         bool raw_pld)
{
    assert(unit != NULL);
    assert(unit->gex != NULL);

//    fprintf(stderr, "raw pld? %d\n", raw_pld);

    GexClient *gex = unit->gex;

    uint8_t callsign = 0;
    uint8_t *pld = NULL;

    if (!raw_pld) {
        callsign = unit->callsign;
        assert(callsign != 0);
        pld = malloc(len + 2);
    } else {
        pld = malloc(len);
    }

    assert(pld != NULL);
    {
        if (!raw_pld) {
            // prefix the actual payload with the callsign and command bytes.
            // TODO provide TF API for sending the payload externally in smaller chunks? Will avoid the malloc here
            pld[0] = callsign;
            pld[1] = cmd;
            memcpy(pld + 2, payload, len);
        } else {
            memcpy(pld, payload, len);
        }

        TF_Msg msg;
        TF_ClearMsg(&msg);
        msg.type = (raw_pld ? cmd : (uint8_t) MSG_UNIT_REQUEST);
        msg.data = pld;
        msg.len = (TF_LEN) (len + (raw_pld?0:2));
        msg.userdata = unit;
        msg.userdata2 = userdata2;
        msg.frame_id = session;
        msg.is_response = is_reply; // This ensures the frame_id is not re-generated
        TF_Query(gex->tf, &msg, lst, 0);
    }
    free(pld);
}

/** Send with no listener, don't wait for response */
void GEX_Send(GexUnit *unit, uint8_t cmd, const uint8_t *payload, uint32_t len)
{
    assert(unit != NULL);
    assert(unit->gex != NULL);

    GEX_LL_Query(unit, cmd, payload, len, 0, false, NULL, NULL, false);
}

/** Send with no listener, don't wait for response */
void GEX_SendEx(GexUnit *unit, uint8_t cmd,
                const uint8_t *payload, uint32_t len,
                GexSession session, bool is_reply)
{
    assert(unit != NULL);
    assert(unit->gex != NULL);

    GEX_LL_Query(unit, cmd, payload, len, session, is_reply, NULL, NULL, false);
}

/** listener for the synchronous query functionality */
static TF_Result sync_query_lst(TinyFrame *tf, TF_Msg *msg)
{
    GexClient *gex = tf->userdata;
    assert(gex != NULL);

//    fprintf(stderr, "sync query lst called <-\n");

    // clone the message
    gex->squery_msg.len = msg->len;
    gex->squery_msg.unit = msg->userdata;
    gex->squery_msg.type = msg->type;
    gex->squery_msg.session = msg->frame_id;
    // clone the buffer
    if (msg->len > 0) memcpy(gex->squery_buffer, msg->data, msg->len);
    // re-link the buffer
    gex->squery_msg.payload = gex->squery_buffer;
    gex->squery_ok = true;
    return TF_CLOSE;
}


/** Static query */
static GexMsg GEX_QueryEx(GexUnit *unit, uint8_t cmd,
                          const uint8_t *payload, uint32_t len,
                          GexSession session, bool is_reply,
                          bool raw_pld)
{
    assert(unit != NULL);
    assert(unit->gex != NULL);

    GexClient *gex = unit->gex;

    gex->squery_ok = false;

    // Default response that will be used if nothing is received
    gex->squery_msg.unit = unit;
    gex->squery_msg.type = MSG_ERROR;
    gex->squery_msg.session = 0;
    sprintf((char *) gex->squery_buffer, "TIMEOUT");
    gex->squery_msg.len = (uint32_t) strlen("TIMEOUT");
    gex->squery_msg.payload = gex->squery_buffer;

    GEX_LL_Query(unit, cmd, payload, len, session, is_reply, sync_query_lst, NULL, raw_pld);

    GEX_Poll(gex);

    if (!gex->squery_ok) {
        fprintf(stderr, "No response to query of unit %s!\n", unit->name);
    }

    return gex->squery_msg;
}

/** Static query */
GexMsg GEX_Query(GexUnit *unit, uint8_t cmd, const uint8_t *payload, uint32_t len)
{
    assert(unit != NULL);
    assert(unit->gex != NULL);
    return GEX_QueryEx(unit, cmd, payload, len, 0, false, false);
}


/** listener for the synchronous query functionality */
static TF_Result async_query_lst(TinyFrame *tf, TF_Msg *msg)
{
    (void)tf;
    GexMsg gexMsg;

    // clone the message
    gexMsg.len = msg->len;
    gexMsg.unit = msg->userdata; // Unit passed to "userdata" in GEX_LL_Query
    gexMsg.type = msg->type;
    gexMsg.session = msg->frame_id;
    gexMsg.payload = (uint8_t *) msg->data;

    assert( gexMsg.unit != NULL);

    GexEventListener lst = msg->userdata2;
    assert(lst != NULL);

    lst(gexMsg);
    return TF_CLOSE;
}

/** Sync query, without poll */
void GEX_QueryAsync(GexUnit *unit, uint8_t cmd, const uint8_t *payload, uint32_t len, GexEventListener lst)
{
    assert(unit != NULL);
    assert(unit->gex != NULL);

    // Async query does not poll, instead the user listener is attached to the message
    GEX_LL_Query(unit, cmd, payload, len, 0, false, async_query_lst, lst, false);
}

// ---------------------------- BULK ----------------------------


/**
 * Bulk read from a unit, synchronous
 *
 * @param unit - the unit to target
 * @param bulk - the bulk transport info struct
 * @return actual number of bytes received.
 */
uint32_t GEX_BulkRead(GexUnit *unit, GexBulk *bulk)
{
//    fprintf(stderr, "Ask to read:\n");
    // We ask for the transfer. This is unit specific and can contain information that determines the transferred data
    GexMsg resp0 = GEX_Query(unit, bulk->req_cmd, bulk->req_data, bulk->req_len);

    if (resp0.type == MSG_ERROR) {
        fprintf(stderr, "Bulk read rejected! %.*s\n", resp0.len, (char*)resp0.payload);
        return 0;
    }

    if (resp0.type != MSG_BULK_READ_OFFER) {
        fprintf(stderr, "Bulk read failed, response not BULK_READ_OFFER!\n");
        return 0;
    }

//    fprintf(stderr, "BR, got offer!\n");

    // Check how much data is available
    PayloadParser pp = pp_start(resp0.payload, resp0.len, NULL);
    uint32_t total = pp_u32(&pp);
    assert(pp.ok);

    total = MIN(total, bulk->capacity); // clamp...
//    fprintf(stderr, "Total is %d\n", total);

    uint32_t at = 0;
    while (at < total+1) { // +1 makes sure we read one past end and trigger the END OF DATA msg
        uint8_t buf[10];
        PayloadBuilder pb = pb_start(buf, 10, NULL);
        pb_u32(&pb, 120); // This selects the chunk size.
        // FIXME Something is wrong in the poll function, or the transport in general,
        // because chunks larger than e.g. 128 bytes seem to get stuck half-transferred.
        // This isn't an issue for events and regular sending, only query responses like here.
        // This may also cause problems when receiving events while reading a bulk.
        // It may be best to get rid of the comport and use libusb, after all.

//        fprintf(stderr, "Poll read:\n");
        GexMsg resp = GEX_QueryEx(unit, MSG_BULK_READ_POLL, buf,
                                  (uint32_t) pb_length(&pb), resp0.session, true, true);

        if (resp.type == MSG_ERROR) {
            fprintf(stderr, "Bulk read failed! %.*s\n", resp.len, (char *) resp.payload);
            return 0;
        }

        if (resp.type == MSG_BULK_END) {
            // No more data
            fprintf(stderr, "Bulk read OK, closed.\n");
            return at;
        }

        if (resp.type == MSG_BULK_DATA) {
            memcpy(bulk->buffer+at, resp.payload, resp.len);
            at += resp.len;
        } else {
            fprintf(stderr, "Bulk read failed! Bad response type.\n");
            return 0;
        }
    }

    return at;
}


/**
 * Bulk write to a unit, synchronous
 *
 * @param unit - the unit to target
 * @param bulk - the bulk transport info struct
 * @return true on success
 */
bool GEX_BulkWrite(GexUnit *unit, GexBulk *bulk)
{
    // We ask for the transfer. This is unit specific
    GexMsg resp0 = GEX_Query(unit, bulk->req_cmd, bulk->req_data, bulk->req_len);

    if (resp0.type == MSG_ERROR) {
        fprintf(stderr, "Bulk write rejected! %.*s\n", resp0.len, (char*)resp0.payload);
        return false;
    }

    if (resp0.type != MSG_BULK_WRITE_OFFER) {
        fprintf(stderr, "Bulk write failed, response not MSG_BULK_WRITE_OFFER!\n");
        return false;
    }

    PayloadParser pp = pp_start(resp0.payload, resp0.len, NULL);
    uint32_t max_size = pp_u32(&pp);
    uint32_t max_chunk = pp_u32(&pp);
    assert(pp.ok);

    if (max_size < bulk->len) {
        fprintf(stderr, "Write not possible, not enough space.\n");
        // Inform GEX we're not going to do it
        GEX_SendEx(unit, MSG_BULK_ABORT, NULL, 0, resp0.session, true);
        return false;
    }

    uint32_t at = 0;
    while (at < bulk->len) {
        uint32_t chunk = MIN(max_chunk, bulk->len - at);
//        fprintf(stderr, "Query at %d, len %d\n", (int)at, (int)chunk);
        GexMsg resp = GEX_QueryEx(unit, MSG_BULK_DATA, bulk->buffer + at, chunk,
                                  resp0.session, true, true);
        at += chunk;

        if (resp.type == MSG_ERROR) {
            fprintf(stderr, "Bulk write failed! %.*s\n", resp.len, (char *) resp.payload);
            return false;
        }

        if (resp.type != MSG_SUCCESS) {
            fprintf(stderr, "Bulk write failed! Bad response type.\n");
            return false;
        }
    }

    // Conclude the transfer
    GEX_SendEx(unit, MSG_BULK_END, NULL, 0, resp0.session, true);

    return true;
}
