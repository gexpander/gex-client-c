//
// Created by MightyPork on 2017/12/25.
//

#define GEX_H // to allow including other headers

#include <utils/payload_builder.h>
#include "gex_settings.h"
#include "gex_defines.h"
#include "gex_internal.h"
#include "gex_message_types.h"

uint32_t GEX_IniRead(GexClient *gex, char *buffer, uint32_t maxlen)
{
    GexBulk br = (GexBulk){
            .buffer = (uint8_t *) buffer,
            .capacity = maxlen,
            .req_cmd = MSG_INI_READ
    };

    uint32_t actuallyRead = GEX_BulkRead(GEX_SysUnit(gex), &br);

    if (actuallyRead == maxlen) {
        actuallyRead--; // lose last char in favor of terminator
    }

    buffer[actuallyRead] = 0; // terminate
    return actuallyRead;
}

bool GEX_IniWrite(GexClient *gex, const char *buffer)
{
    uint8_t buf[8];
    PayloadBuilder pb = pb_start(buf, 8, NULL);
    pb_u32(&pb, (uint32_t) strlen(buffer));

    GexBulk bw = (GexBulk){
            .buffer = (uint8_t *) buffer,
            .len = (uint32_t) strlen(buffer),
            .req_cmd = MSG_INI_WRITE,
            .req_data = buf,
            .req_len = (uint32_t) pb_length(&pb),
    };

    return GEX_BulkWrite(GEX_SysUnit(gex), &bw);
}

bool GEX_IniPersist(GexClient *gex)
{
    return TF_SendSimple(gex->tf, MSG_PERSIST_SETTINGS, NULL, 0);
}
