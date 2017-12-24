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
 * @param cmd - command byte, or TYPE if raw query is used
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
                GexSession session, bool is_reply, bool raw_pld)
{
    assert(unit != NULL);
    assert(unit->gex != NULL);

    GEX_LL_Query(unit, cmd, payload, len, session, is_reply, NULL, NULL, raw_pld);
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
GexMsg GEX_QueryEx(GexUnit *unit, uint8_t cmd,
                          const uint8_t *payload, uint32_t len,
                          GexSession session, bool is_reply,
                          bool raw_pld) // FIXME this is horrible, split to multiple functions or use a struct
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
