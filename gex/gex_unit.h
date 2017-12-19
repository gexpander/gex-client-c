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

/** Send with no listener, don't wait for response */
void GEX_Send(GexUnit *unit, uint8_t cmd, uint8_t *payload, uint32_t len);

/** Static query */
GexMsg GEX_Query(GexUnit *unit, uint8_t cmd, uint8_t *payload, uint32_t len);

/** Sync query, without poll */
void GEX_QueryAsync(GexUnit *unit, uint8_t cmd, uint8_t *payload, uint32_t len, GexEventListener lst);


#endif //GEX_CLIENT_GEX_UNIT_H
