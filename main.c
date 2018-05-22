#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <assert.h>
#include <utils/payload_builder.h>
#include <unistd.h>

#include "gex.h"
//#include "utils/hexdump.h"
//#include "utils/payload_builder.h"
//#include "utils/payload_parser.h"

// some examples of usage, for more details see the header files

static GexClient *gex;

/** Main function - test the library */
int main(void)
{
    gex = GEX_Init("/dev/ttyACM0", 200);
    if (!gex) exit(1);

    GexUnit *bus = GEX_GetUnit(gex, "i2c", "I2C");
    assert(NULL != bus);

    sleep(2);

#if 0
    GexUnit *adc = GEX_GetUnit(gex, "adc", "ADC");

    GexMsg msg = GEX_Query(adc, 10, NULL, 0);
    hexDump("Resp1", msg.payload, msg.len);

    fprintf(stderr, "Channels: ");
    for(uint32_t i = 0; i < msg.len; i++) fprintf(stderr, "%d, ", msg.payload[i]);

    fprintf(stderr, "\n");

    msg = GEX_Query(adc, 11, NULL, 0);
    hexDump("Resp2", msg.payload, msg.len);

    PayloadParser pp = pp_start(msg.payload, msg.len, NULL);
    uint32_t fconf = pp_u32(&pp);
    float freal = pp_float(&pp);
    fprintf(stderr, "Freq configured %d Hz, real freq %f Hz", fconf, freal);
#endif

#if 0
    // INI read example
    char buffer[2000];
    uint32_t len = GEX_IniRead(gex, buffer, 2000);
    printf("%s", buffer);

    GEX_IniWrite(gex, buffer);
    printf("Written.\r\n");
#endif

#if 0
    // DO example - there's a 10-LED bargraph connected to this unit
    GexUnit *bg = GEX_GetUnit(gex, "bargraph", "DO");

    struct DO_Set {
        uint16_t bits;
    } __attribute__((packed));

    struct DO_Set cmd_set;

    // animation on the bargraph
    for (int i = 0; i < 100; i++) {
        int n = i%20;
        if(n>=10) n=10-(n-10);
        cmd_set.bits = ~(((1 << (n))-1)<<(10-n));
        GEX_Send(bg, 0x00, (uint8_t *)&cmd_set, sizeof(cmd_set));
        usleep(40000);
    }

    // all dark
    cmd_set.bits = 0x3FF;
    GEX_Send(bg, 0x00, (uint8_t *)&cmd_set, sizeof(cmd_set));
#endif

    GEX_DeInit(gex);
    return 0;
}
