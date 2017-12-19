//
// Created by MightyPork on 2017/12/19.
//

#include <malloc.h>
#include <assert.h>
#include <utils/payload_parser.h>
#include <utils/payload_builder.h>

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
static void GEX_LL_Query(GexUnit *unit,
                         uint8_t cmd,
                         const uint8_t *payload, uint32_t len,
                         GexSession session, bool is_reply,
                         TF_Listener lst, void *userdata2)
{
    assert(unit != NULL);
    assert(unit->gex != NULL);

    GexClient *gex = unit->gex;

    uint8_t callsign = unit->callsign;
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

    GEX_LL_Query(unit, cmd,
                 payload, len,
                 0, false,
                 NULL, NULL);
}

/** Send with no listener, don't wait for response */
void GEX_SendEx(GexUnit *unit, uint8_t cmd,
                const uint8_t *payload, uint32_t len,
                GexSession session, bool is_reply)
{
    assert(unit != NULL);
    assert(unit->gex != NULL);

    GEX_LL_Query(unit, cmd,
                 payload, len,
                 session, is_reply,
                 NULL, NULL);
}

/** listener for the synchronous query functionality */
static TF_Result sync_query_lst(TinyFrame *tf, TF_Msg *msg)
{
    GexClient *gex = tf->userdata;
    assert(gex != NULL);

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
                          GexSession session, bool is_reply)
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

    GEX_LL_Query(unit, cmd,
                 payload, len,
                 session, is_reply,
                 sync_query_lst, NULL);

    GEX_Poll(gex);

    if (!gex->squery_ok) {
        fprintf(stderr, "No response to query of unit %s!", unit->name);
    }

    return gex->squery_msg;
}

/** Static query */
GexMsg GEX_Query(GexUnit *unit, uint8_t cmd, const uint8_t *payload, uint32_t len)
{
    assert(unit != NULL);
    assert(unit->gex != NULL);
    return GEX_QueryEx(unit, cmd, payload, len, 0, false);
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
    GEX_LL_Query(unit, cmd,
                 payload, len,
                 0, false,
                 async_query_lst, lst);
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
    // We ask for the transfer. This is unit specific and can contain information that determines the transferred data
    GexMsg resp0 = GEX_Query(unit, bulk->req_cmd, bulk->req_data, bulk->req_len);

    if (resp0.type == MSG_ERROR) {
        fprintf(stderr, "Bulk read rejected! %.*s", resp0.len, (char*)resp0.payload);
        return 0;
    }

    if (resp0.type != MSG_BULK_READ_OFFER) {
        fprintf(stderr, "Bulk read failed, response not BULK_READ_OFFER!");
        return 0;
    }

    // Check how much data is available
    PayloadParser pp = pp_start(resp0.payload, resp0.len, NULL);
    uint32_t total = pp_u32(&pp);
    assert(pp.ok);

    total = MIN(total, bulk->capacity); // clamp...

    uint32_t at = 0;
    while (at < total) {
        GexMsg resp = GEX_QueryEx(unit, MSG_BULK_READ_POLL,
                                  NULL, 0,
                                  resp0.session, true);

        if (resp.type == MSG_ERROR) {
            fprintf(stderr, "Bulk read failed! %.*s", resp.len, (char *) resp.payload);
            return 0;
        }

        if (resp.type == MSG_BULK_END) {
            // No more data
            return at;
        }

        if (resp.type == MSG_BULK_DATA) {
            memcpy(bulk->buffer+at, resp.payload, resp.len);
            at += resp.len;
        } else {
            fprintf(stderr, "Bulk read failed! Bad response type.");
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
        fprintf(stderr, "Bulk write rejected! %.*s", resp0.len, (char*)resp0.payload);
        return false;
    }

    if (resp0.type != MSG_BULK_WRITE_OFFER) {
        fprintf(stderr, "Bulk write failed, response not MSG_BULK_WRITE_OFFER!");
        return false;
    }

    PayloadParser pp = pp_start(resp0.payload, resp0.len, NULL);
    uint32_t max_size = pp_u32(&pp);
    uint32_t max_chunk = pp_u32(&pp);
    assert(pp.ok);

    if (max_size < bulk->len) {
        fprintf(stderr, "Write not possible, not enough space.");
        // Inform GEX we're not going to do it
        GEX_SendEx(unit, MSG_BULK_ABORT, NULL, 0, resp0.session, true);
        return false;
    }

    uint32_t at = 0;
    while (at < bulk->len) {
        uint32_t chunk = MIN(max_chunk, bulk->len - at);
        GexMsg resp = GEX_QueryEx(unit, MSG_BULK_DATA,
                                  bulk->buffer+at, chunk,
                                  resp0.session, true);
        at += chunk;

        if (resp.type == MSG_ERROR) {
            fprintf(stderr, "Bulk write failed! %.*s", resp.len, (char *) resp.payload);
            return false;
        }

        if (resp.type != MSG_SUCCESS) {
            fprintf(stderr, "Bulk write failed! Bad response type.");
            return false;
        }
    }

    // Conclude the transfer
    GEX_SendEx(unit, MSG_BULK_END, NULL, 0, resp0.session, true);

    return true;
}
