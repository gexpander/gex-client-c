//
// Created by MightyPork on 2017/12/25.
//

#ifndef GEX_CLIENT_GEX_SETTINGS_H
#define GEX_CLIENT_GEX_SETTINGS_H

#ifndef GEX_H
#error "Include gex.h instead!"
#endif

#include "gex_defines.h"
#include "gex_unit.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * Read the settings INI file via TinyFrame
 *
 * @param gex - client
 * @param buffer - target buffer
 * @param maxlen - buffer size
 * @return number of bytes read, 0 on error
 */
uint32_t GEX_IniRead(GexClient *gex, char *buffer, uint32_t maxlen);

/**
 * Write settings INI file via TinyFrame
 *
 * @param gex - client
 * @param buffer - buffer with the settings file
 * @return success
 */
bool GEX_IniWrite(GexClient *gex, const char *buffer);

/**
 * Persiste the settings loaded via GEX_SettingsIniWrite() to Flash
 * This can be used with GEX instances built without the MSC support
 * as a substitute for the LOCK jumper.
 *
 * @param gex - client
 * @return send success
 */
bool GEX_IniPersist(GexClient *gex);

#endif //GEX_CLIENT_GEX_SETTINGS_H
