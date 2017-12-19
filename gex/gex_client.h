//
// Created by MightyPork on 2017/12/12.
//

#ifndef GEX_CLIENT_GEX_CLIENT_H
#define GEX_CLIENT_GEX_CLIENT_H

#ifndef GEX_H
#error "Include gex.h instead!"
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "TinyFrame.h"
#include "gex_defines.h"

/**
 * Bind a report listener. The listener is called with a message object when
 * a spontaneous report is received. If no known unit matched the report,
 * a dummy unit is provided to avoid NULL access.
 *
 * @param gex - client
 * @param unit - the handled unit, NULL to bind a fallback listener (fallback may receive events from all unhandled units)
 * @param lst - the listener
 */
void GEX_OnReport(GexClient *gex, GexUnit *unit, GexEventListener lst);

/**
 * Get a unit, or NULL
 *
 * @param gex - gex
 * @param name - unit name
 */
GexUnit *GEX_Unit(GexClient *gex, const char *name);

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

#endif //GEX_CLIENT_GEX_CLIENT_H
