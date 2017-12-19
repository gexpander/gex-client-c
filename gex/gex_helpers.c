//
// Created by MightyPork on 2017/12/19.
//

#include <malloc.h>

#include "gex_defines.h"
#include "gex_helpers.h"

/** Delete recursively all GEX callsign look-up table entries */
void destroy_unit_lookup(GexClient *gex)
{
    struct gex_unit_lu *next = gex->ulu_head;
    while (next != NULL) {
        struct gex_unit_lu *cur = next;
        next = next->next;
        free(cur);
    }
    gex->ulu_head = NULL;
}

/** Get lookup entry for unit name */
struct gex_unit_lu *find_unit_by_callsign(GexClient *gex, uint8_t callsign)
{
    struct gex_unit_lu *next = gex->ulu_head;
    while (next != NULL) {
        if (next->callsign == callsign) {
            return next;
        }
        next = next->next;
    }
    return NULL;
}

/** Get lookup entry for unit name */
struct gex_unit_lu *find_unit_by_name(GexClient *gex, const char *name)
{
    struct gex_unit_lu *next = gex->ulu_head;
    while (next != NULL) {
        if (strcmp(next->name, name) == 0) {
            return next;
        }
        next = next->next;
    }
    return NULL;
}

/** Get callsign for unit name */
uint8_t find_callsign_by_name(GexClient *gex, const char *name)
{
    struct gex_unit_lu *lu = find_unit_by_name(gex, name);
    return (uint8_t) ((lu == NULL) ? 0 : lu->callsign);
}
