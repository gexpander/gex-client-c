//
// Created by MightyPork on 2017/12/12.
//

#ifndef GEX_CLIENT_GEX_CLIENT_H
#define GEX_CLIENT_GEX_CLIENT_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "TinyFrame.h"

typedef struct gex_client_ GexClient;

/**
 * Initialize the GEX client
 *
 * @param device - device, e.g. /dev/ttyACM0
 * @param timeout_ms - read timeout in ms (allowed only multiples of 100 ms, others are rounded)
 * @return an allocated client instance
 */
GexClient *GEX_Init(const char *device, int timeout_ms);

/**
 * Poll for new messages
 * @param gex - client
 */
void GEX_Poll(GexClient *gex);

/**
 * Safely release all resources used up by GEX_Init()
 *
 * @param gex - the allocated client structure
 */
void GEX_DeInit(GexClient *gex);

/**
 * Query a unit
 *
 * @param gex - client instance
 * @param unit - unit name
 * @param cmd - command (hex)
 * @param payload - payload for the unit
 * @param len - number of bytes in the payload
 * @param listener - TF listener function called for the response
 */
void GEX_Query(GexClient *gex,
               const char *unit, uint8_t cmd,
               uint8_t *payload, uint32_t len,
               TF_Listener listener);

/**
 * Query a unit and return the response. The resulting message (including payload) is
 * copied to a internal holder variable. Do not attempt to free it!
 *
 * @attention Calling `GEX_SyncQuery` destroys the previous sync query message and payload.
 *
 * @param gex - client instance
 * @param unit - unit name
 * @param cmd - command (hex)
 * @param payload - payload for the unit
 * @param len - number of bytes in the payload
 * @return a clone of the response, or NULL if none arrived in time.
*/
TF_Msg *GEX_SyncQuery(GexClient *gex,
                      const char *unit, uint8_t cmd,
                      uint8_t *payload, uint32_t len);

/**
 * Send a command with response no listener
 *
 * @param gex - client instance
 * @param unit - unit name
 * @param cmd - command (hex)
 * @param payload - payload for the unit
 * @param len - number of bytes in the payload
 */
void GEX_Send(GexClient *gex,
              const char *unit, uint8_t cmd,
              uint8_t *payload, uint32_t len);

#endif //GEX_CLIENT_GEX_CLIENT_H
