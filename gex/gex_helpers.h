//
// Created by MightyPork on 2017/12/19.
//

#ifndef GEX_CLIENT_GEX_HELPERS_H
#define GEX_CLIENT_GEX_HELPERS_H

#include "gex_client_internal.h"
#include "gex_client.h"

/** Delete recursively all GEX callsign look-up table entries */
void destroy_unit_lookup(GexClient *gex);

/** Get lookup entry for unit name */
struct gex_unit_lu *find_unit_by_callsign(GexClient *gex, uint8_t callsign);

/** Get lookup entry for unit name */
struct gex_unit_lu *find_unit_by_name(GexClient *gex, const char *name);

/** Get callsign for unit name */
uint8_t find_callsign_by_name(GexClient *gex, const char *name);

#endif //GEX_CLIENT_GEX_HELPERS_H
