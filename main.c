#include <stdio.h>
#include <stdlib.h>
#include "gex_client.h"
#include <signal.h>

static GexClient *gex;

/** ^C handler to close it gracefully */
static void sigintHandler(int sig)
{
    if (gex != NULL) {
        GEX_DeInit(gex);
    }
    exit(0);
}

int main()
{
    // Bind ^C handler for safe shutdown
    signal(SIGINT, sigintHandler);

	gex = GEX_Init("/dev/ttyACM0", 200);
	if (!gex) {
		fprintf(stderr, "FAILED TO CONNECT, ABORTING!\n");
		exit(1);
	}

	printf("Hello, World!\n");

	GEX_DeInit(gex);
	return 0;
}
