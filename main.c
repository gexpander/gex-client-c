#include <stdio.h>
#include <stdlib.h>
#include "gex_client.h"

int main()
{
	GexClient *gex = GEX_Init("/dev/ttyACM0", 200);
	if (!gex) {
		fprintf(stderr, "FAILED TO CONNECT, ABORTING!\n");
		exit(1);
	}

	printf("Hello, World!\n");

	GEX_DeInit(gex);
	return 0;
}
