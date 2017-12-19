//
// Created by MightyPork on 2017/12/19.
//

#include <malloc.h>
#include <assert.h>

#define GEX_H // to allow including other headers
#include "gex_defines.h"
#include "gex_helpers.h"
#include "gex_message_types.h"

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
static void GEX_LL_Query(GexUnit *unit, uint8_t cmd, uint8_t *payload, uint32_t len, TF_Listener lst, void *userdata2)
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
        TF_Query(gex->tf, &msg, lst, 0);
    }
    free(pld);
}

/** Send with no listener, don't wait for response */
void GEX_Send(GexUnit *unit, uint8_t cmd, uint8_t *payload, uint32_t len)
{
    assert(unit != NULL);
    assert(unit->gex != NULL);

    GEX_LL_Query(unit, cmd, payload, len, NULL, NULL);
}

/** listener for the synchronous query functionality */
static TF_Result sync_query_lst(TinyFrame *tf, TF_Msg *msg)
{
    GexClient *gex = tf->userdata;
    assert(gex != NULL);

    // clone the message
    gex->sync_query_response.len = msg->len;
    gex->sync_query_response.session = msg->frame_id;
    gex->sync_query_response.unit = msg->userdata;
    gex->sync_query_response.type = msg->type;
    // clone the buffer
    if (msg->len > 0) memcpy(gex->sync_query_buffer, msg->data, msg->len);
    // re-link the buffer
    gex->sync_query_response.payload = gex->sync_query_buffer;
    gex->sync_query_ok = true;
}

/** Static query */
GexMsg GEX_Query(GexUnit *unit, uint8_t cmd, uint8_t *payload, uint32_t len)
{
    assert(unit != NULL);
    assert(unit->gex != NULL);

    GexClient *gex = unit->gex;

    gex->sync_query_ok = false;

    // Default response that will be used if nothing is received
    gex->sync_query_response.unit = unit;
    gex->sync_query_response.session = 0;
    gex->sync_query_response.type = MSG_ERROR;
    sprintf((char *) gex->sync_query_buffer, "TIMEOUT");
    gex->sync_query_response.len = (uint32_t) strlen("TIMEOUT");
    gex->sync_query_response.payload = gex->sync_query_buffer;

    GEX_LL_Query(unit, cmd, payload, len, sync_query_lst, NULL);
    GEX_Poll(gex);

    if (!gex->sync_query_ok) {
        fprintf(stderr, "No response to query of unit %s!", unit->name);
    }

    return gex->sync_query_response;
}


/** listener for the synchronous query functionality */
static TF_Result async_query_lst(TinyFrame *tf, TF_Msg *msg)
{
    GexMsg gexMsg;

    // clone the message
    gexMsg.len = msg->len;
    gexMsg.session = msg->frame_id;
    gexMsg.unit = msg->userdata; // gex is accessible via the unit
    gexMsg.type = msg->type;
    gexMsg.payload = (uint8_t *) msg->data;

    GexEventListener lst = msg->userdata2;
    assert(lst != NULL);

    lst(gexMsg);
}

/** Sync query, without poll */
void GEX_QueryAsync(GexUnit *unit, uint8_t cmd, uint8_t *payload, uint32_t len, GexEventListener lst)
{
    assert(unit != NULL);
    assert(unit->gex != NULL);

    GexClient *gex = unit->gex;
    gex->sync_query_ok = false;
    memset(&gex->sync_query_response, 0, sizeof(GexMsg));

    // Default response that will be used if nothing is received
    gex->sync_query_response.type = MSG_ERROR;
    sprintf((char *) gex->sync_query_buffer, "TIMEOUT");
    gex->sync_query_response.len = (uint32_t) strlen("TIMEOUT");

    GEX_LL_Query(unit, cmd, payload, len, async_query_lst, lst);
}
