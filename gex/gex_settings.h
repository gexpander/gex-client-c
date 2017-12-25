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

uint32_t GEX_SettingsIniRead(GexClient *gex, char *buffer, uint32_t maxlen);

bool GEX_SettingsIniWrite(GexClient *gex, const char *buffer);

#endif //GEX_CLIENT_GEX_SETTINGS_H
