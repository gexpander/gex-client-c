#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <assert.h>
#include <unistd.h>

#include "gex.h"
#include "utils/hexdump.h"
#include "utils/payload_builder.h"

static GexClient *gex;

/** ^C handler to close it gracefully */
static void sigintHandler(int sig)
{
    (void)sig;
    if (gex != NULL) GEX_DeInit(gex);
    exit(0);
}

/** Main function - test the library */
int main(void)
{
    // Bind ^C handler for safe shutdown - need to release the port
    signal(SIGINT, sigintHandler);

	gex = GEX_Init("/dev/ttyACM0", 200);
	if (!gex) exit(1);

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
