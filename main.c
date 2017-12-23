#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <assert.h>
#include <unistd.h>

#include "utils/hexdump.h"
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

#if 1
    // Simple response
    msg = GEX_Query0(test, 0);
    assert(msg.type == MSG_SUCCESS);
    fprintf(stderr, "Cmd \"PING\" OK\n");
#endif

#if 1
    // Test a echo command that returns back what was sent to it as useful payload
    const char *s = "I am \r\nreturning this otherwise good typing paper to you because someone "
            "has printed gibberish all over it and put your name at the top. Read the communist manifesto via bulk transfer. Read the communist manifesto via bulk transfer. Technology is a constand battle between manufacturers producing bigger and "
            "more idiot-proof systems and nature producing bigger and better idiots. END";
    msg = GEX_Query(test, 1, (const uint8_t *) s, (uint32_t) strlen(s));
    fprintf(stderr, "\"ECHO\" cmd resp: %d, len %d, pld: %.*s\n", msg.type, (int) msg.len, (int) msg.len, (char *) msg.payload);
    assert(0==strncmp((char*)msg.payload, s, strlen(s)));
#endif

#if 1
    // Read the communist manifesto via bulk transfer
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
    fprintf(stderr, "%.*s", actuallyRead, buffr);
#endif

    fprintf(stderr, "\n\nALL done, ending.\n");
	GEX_DeInit(gex);
	return 0;
}
