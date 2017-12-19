//
// Created by MightyPork on 2017/12/19.
//

#ifndef GEX_CLIENT_GEX_UNIT_H
#define GEX_CLIENT_GEX_UNIT_H

#ifndef GEX_H
#error "Include gex.h instead!"
#endif

#include "gex_defines.h"
#include <stdint.h>
#include <stdbool.h>

/** Send a command with no listener */
void GEX_Send(GexUnit *unit, uint8_t cmd, const uint8_t *payload, uint32_t len);

/** Send a command with no payload or listener */
static inline void GEX_Send0(GexUnit *unit, uint8_t cmd)
{
	GEX_Send(unit, cmd, NULL, 0);
}

/** Static query, send a command and wait for the response */
GexMsg GEX_Query(GexUnit *unit, uint8_t cmd, const uint8_t *payload, uint32_t len);

/** Asynchronous query with an async listener */
void GEX_QueryAsync(GexUnit *unit, uint8_t cmd, const uint8_t *payload, uint32_t len, GexEventListener lst);


/**
 * Bulk read from a unit, synchronous
 *
 * @param unit - the unit to target
 * @param cmd - initial request command
 * @param payload - initial request payload
 * @param len - initial request payload length
 * @param buffer - destination buffer
 * @param capacity - size of the buffer, max nr of bytes to receive
 * @return actual number of bytes received.
 */
uint32_t GEX_BulkRead(GexUnit *unit, uint8_t cmd,
                      const uint8_t *payload, uint32_t len,
                      uint8_t *buffer, uint32_t capacity);


/**
 * Bulk write to a unit, synchronous
 *
 * @param unit - the unit to target
 * @param cmd - initial request command
 * @param payload - initial request payload
 * @param len - initial request payload length
 * @param buffer - destination buffer
 * @param capacity - size of the buffer, max nr of bytes to receive
 * @return true on success
 */
bool GEX_BulkWrite(GexUnit *unit, uint8_t cmd,
                   const uint8_t *payload, uint32_t len,
                   const uint8_t *buffer, uint32_t capacity);

#endif //GEX_CLIENT_GEX_UNIT_H
