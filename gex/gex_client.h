//
// Created by MightyPork on 2017/12/12.
//

#ifndef GEX_CLIENT_GEX_CLIENT_H
#define GEX_CLIENT_GEX_CLIENT_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "TinyFrame.h"

struct gex_client_ {
    TinyFrame *tf;
    const char *acm_device;
    int acm_fd;
    bool connected;
};

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
 * @param gc - the allocated client structure
 */
void GEX_DeInit(GexClient *gc);

// --- Internal ---
/**
 * This is accessed by TF_WriteImpl().
 * To be removed once TF supports multiple instances, i.e. without globals
 */
extern int gex_serial_fd;

#endif //GEX_CLIENT_GEX_CLIENT_H
