//
// Created by MightyPork on 2017/12/19.
//

#ifndef GEX_CLIENT_GEX_BULK_H
#define GEX_CLIENT_GEX_BULK_H

#ifndef GEX_H
#error "Include gex.h instead!"
#endif

#include "gex_defines.h"
#include "gex_unit.h"
#include <stdint.h>
#include <stdbool.h>

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

#endif //GEX_CLIENT_GEX_BULK_H
