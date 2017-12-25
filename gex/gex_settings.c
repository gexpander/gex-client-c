//
// Created by MightyPork on 2017/12/25.
//

#define GEX_H // to allow including other headers
#include "gex_settings.h"
#include "gex_defines.h"
#include "gex_internal.h"
#include "gex_message_types.h"

uint32_t GEX_SettingsIniRead(GexClient *gex, char *buffer, uint32_t maxlen)
{
    GexBulk br = (GexBulk){
            .buffer = (uint8_t *) buffer,
            .capacity = maxlen,
            .req_cmd = MSG_INI_READ
    };

    uint32_t actuallyRead = GEX_BulkRead(GEX_SysUnit(gex), &br);

    return actuallyRead;
}

bool GEX_SettingsIniWrite(GexClient *gex, const char *buffer)
{
    GexBulk bw = (GexBulk){
            .buffer = (uint8_t *) buffer,
            .len = (uint32_t) strlen(buffer),
            .req_cmd = MSG_INI_WRITE
    };

    return GEX_BulkWrite(GEX_SysUnit(gex), &bw);
}
