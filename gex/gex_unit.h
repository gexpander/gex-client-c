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

// TODO proper descriptions

/** Send a command with no listener */
void GEX_Send(GexUnit *unit, uint8_t cmd, const uint8_t *payload, uint32_t len);

/** Send a command with no payload or listener */
static inline void GEX_Send0(GexUnit *unit, uint8_t cmd)
{
	GEX_Send(unit, cmd, NULL, 0);
}

/** Static query, send a command and wait for the response */
GexMsg GEX_Query(GexUnit *unit, uint8_t cmd, const uint8_t *payload, uint32_t len);

/** Send a query with no payload */
static inline GexMsg GEX_Query0(GexUnit *unit, uint8_t cmd)
{
	return GEX_Query(unit, cmd, NULL, 0);
}

/** Asynchronous query with an async listener */
void GEX_QueryAsync(GexUnit *unit, uint8_t cmd, const uint8_t *payload, uint32_t len, GexEventListener lst);

/**
 * Bulk read from a unit, synchronous
 *
 * @param unit - the unit to target
 * @param bulk - the bulk transport info struct
 * @return actual number of bytes received.
 */
uint32_t GEX_BulkRead(GexUnit *unit, GexBulk *bulk);

/**
 * Bulk write to a unit, synchronous
 *
 * @param unit - the unit to target
 * @param bulk - the bulk transport info struct
 * @return true on success
 */
bool GEX_BulkWrite(GexUnit *unit, GexBulk *bulk);


// extended low level stuff

/** Static query */
GexMsg GEX_QueryEx(GexUnit *unit, uint8_t cmd,
				   const uint8_t *payload, uint32_t len,
				   GexSession session, bool is_reply,
				   bool raw_pld);

void GEX_SendEx(GexUnit *unit, uint8_t cmd,
				const uint8_t *payload, uint32_t len,
				GexSession session, bool is_reply);

#endif //GEX_CLIENT_GEX_UNIT_H
