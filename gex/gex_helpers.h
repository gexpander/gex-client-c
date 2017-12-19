//
// Created by MightyPork on 2017/12/19.
//

#ifndef GEX_CLIENT_GEX_HELPERS_H
#define GEX_CLIENT_GEX_HELPERS_H

#ifndef GEX_H
#error "Include gex.h instead!"
#endif

#include "gex_internal.h"
#include "gex_defines.h"

/** Delete recursively all GEX callsign look-up table entries */
void gex_destroy_unit_lookup(GexClient *gex);

/** Get lookup entry for unit name */
struct gex_unit *gex_find_unit_by_callsign(GexClient *gex, uint8_t callsign);

/** Get lookup entry for unit name */
struct gex_unit *gex_find_unit_by_name(GexClient *gex, const char *name);

/** Get callsign for unit name */
uint8_t gex_find_callsign_by_name(GexClient *gex, const char *name);

#endif //GEX_CLIENT_GEX_HELPERS_H
