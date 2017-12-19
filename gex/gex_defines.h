//
// Created by MightyPork on 2017/12/19.
//

#ifndef GEX_CLIENT_GEX_DEFINES_H
#define GEX_CLIENT_GEX_DEFINES_H

#include <stdint.h>
#include <stdbool.h>

typedef struct gex_client_ GexClient;

/** Callback for spontaneous reports from units */
typedef void (*GEX_ReportListener)(GexClient *gex, const char *unit, uint8_t code, const uint8_t *payload, uint32_t len);

#endif //GEX_CLIENT_GEX_DEFINES_H
