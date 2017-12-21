#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <assert.h>
#include <utils/hexdump.h>

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

TF_Result hdl_default(TinyFrame *tf, TF_Msg*msg)
{
    (void)tf;
    fprintf(stderr, "TF unhandled msg len %d, type %d, id %d", msg->len, msg->type, msg->frame_id);
    hexDump("payload", msg->data, msg->len);
    return TF_STAY;
}

int main(void)
{
    // Bind ^C handler for safe shutdown
    signal(SIGINT, sigintHandler);

	gex = GEX_Init("/dev/ttyACM0", 200);
	if (!gex) exit(1);

    TF_AddGenericListener(GEX_GetTF(gex), hdl_default);

    //GexUnit *led = GEX_Unit(gex, "LED");
    //GEX_Send(led, LED_CMD_TOGGLE, NULL, 0);

    GexUnit *test = GEX_Unit(gex, "TEST");
    assert(test != NULL);

    // the "PING" command
    GexMsg msg;

    msg = GEX_Query0(test, 0);
    assert(msg.type == MSG_SUCCESS);
    fprintf(stderr, "\"PING\" cmd: OK.\n");

    const char *s = "BLOCKCHAIN BUT FOR COWS";
    msg = GEX_Query(test, 1, (const uint8_t *) s, (uint32_t) strlen(s));
    fprintf(stderr, "\"ECHO\" cmd resp: %d, len %d, pld: %.*s\n", msg.type, (int) msg.len, (int) msg.len, (char *) msg.payload);
    assert(0==strncmp((char*)msg.payload, s, strlen(s)));

    uint8_t buffr[10000];
    GexBulk br = {
        .buffer = buffr,
        .capacity = 10000,
        .req_cmd = 2,
        .req_data = NULL,
        .req_len = 0,
    };
    uint32_t actuallyRead = GEX_BulkRead(test, &br);
    fprintf(stderr, "Read %d bytes:\n", actuallyRead);
    fprintf(stderr, "%*.s", actuallyRead, buffr);

    fprintf(stderr, "ALL OK, ending.\n");
	GEX_DeInit(gex);
	return 0;
}
