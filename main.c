#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <assert.h>

#include "gex.h"

static GexClient *gex;

/** ^C handler to close it gracefully */
static void sigintHandler(int sig)
{
    (void)sig;

    if (gex != NULL) {
        GEX_DeInit(gex);
    }
    exit(0);
}

#define LED_CMD_TOGGLE 0x02

int main(void)
{
    // Bind ^C handler for safe shutdown
    signal(SIGINT, sigintHandler);

	gex = GEX_Init("/dev/ttyACM0", 200);
	if (!gex) exit(1);

    GexUnit *led = GEX_Unit(gex, "LED");
    assert(led != NULL);

    GEX_Send(led, LED_CMD_TOGGLE, NULL, 0);

	GEX_DeInit(gex);
	return 0;
}
