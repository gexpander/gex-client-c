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

const char *longtext = "A sharper perspective on this matter is particularly important to feminist\r\n"
        "thought today, because a major tendency in feminism has constructed the\r\n"
        "problem of domination as a drama of female vulnerability victimized by male\r\n"
        "aggression.  Even the more sophisticated feminist thinkers frequently shy\r\n"
        "away from the analysis of submission, for fear that in admitting woman's\r\n"
        "participation in the relationship of domination, the onus of responsibility\r\n"
        "will appear to shift from men to women, and the moral victory from women to\r\n"
        "men.  More generally, this has been a weakness of radical politics: to\r\n"
        "idealize the oppressed, as if their politics and culture were untouched by\r\n"
        "the system of domination, as if people did not participate in their own\r\n"
        "submission.  To reduce domination to a simple relation of doer and done-to\r\n"
        "is to substitute moral outrage for analysis.\r\n"
        "\t\t-- Jessica Benjamin, \"The Bonds of Love\""
        "A sharper perspective on this matter is particularly important to feminist\r\n"
        "thought today, because a major tendency in feminism has constructed the\r\n"
        "problem of domination as a drama of female vulnerability victimized by male\r\n"
        "aggression.  Even the more sophisticated feminist thinkers frequently shy\r\n"
        "away from the analysis of submission, for fear that in admitting woman's\r\n"
        "participation in the relationship of domination, the onus of responsibility\r\n"
        "will appear to shift from men to women, and the moral victory from women to\r\n"
        "men.  More generally, this has been a weakness of radical politics: to\r\n"
        "idealize the oppressed, as if their politics and culture were untouched by\r\n"
        "the system of domination, as if people did not participate in their own\r\n"
        "submission.  To reduce domination to a simple relation of doer and done-to\r\n"
        "is to substitute moral outrage for analysis.\r\n"
        "\t\t-- Jessica Benjamin, \"The Bonds of Love\""
        "A sharper perspective on this matter is particularly important to feminist\r\n"
        "thought today, because a major tendency in feminism has constructed the\r\n"
        "problem of domination as a drama of female vulnerability victimized by male\r\n"
        "aggression.  Even the more sophisticated feminist thinkers frequently shy\r\n"
        "away from the analysis of submission, for fear that in admitting woman's\r\n"
        "participation in the relationship of domination, the onus of responsibility\r\n"
        "will appear to shift from men to women, and the moral victory from women to\r\n"
        "men.  More generally, this has been a weakness of radical politics: to\r\n"
        "idealize the oppressed, as if their politics and culture were untouched by\r\n"
        "the system of domination, as if people did not participate in their own\r\n"
        "submission.  To reduce domination to a simple relation of doer and done-to\r\n"
        "is to substitute moral outrage for analysis.\r\n"
        "\t\t-- Jessica Benjamin, \"The Bonds of Love\" \r\nEND OF TEXT";

int main(void)
{
    GexBulk br;

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
    // Load settings to a buffer as INI
    uint8_t inifile[10000];
    br = (GexBulk){
            .buffer = inifile,
            .capacity = 10000,
            .req_cmd = MSG_INI_READ,
            .req_data = NULL,
            .req_len = 0,
    };
    uint32_t actuallyRead = GEX_BulkRead(GEX_SystemUnit(gex), &br);
    fprintf(stderr, "Read %d bytes of INI:\n", actuallyRead);
    fprintf(stderr, "%.*s", actuallyRead, inifile);

    // And send it back...
    br.len = actuallyRead;
    br.req_cmd = MSG_INI_WRITE;
    GEX_BulkWrite(GEX_SystemUnit(gex), &br);
#endif

#if 0
    // Test a echo command that returns back what was sent to it as useful payload
    const char *s = "I am \r\nreturning this otherwise good typing paper to you because someone "
            "has printed gibberish all over it and put your name at the top. Read the communist manifesto via bulk transfer. Read the communist manifesto via bulk transfer. Technology is a constand battle between manufacturers producing bigger and "
            "more idiot-proof systems and nature producing bigger and better idiots. END";
    msg = GEX_Query(test, 1, (const uint8_t *) s, (uint32_t) strlen(s));
    fprintf(stderr, "\"ECHO\" cmd resp: %d, len %d, pld: %.*s\n", msg.type, (int) msg.len, (int) msg.len, (char *) msg.payload);
    assert(0==strncmp((char*)msg.payload, s, strlen(s)));
#endif

#if 0
    // Read the communist manifesto via bulk transfer
    uint8_t buffr[10000];
    br = (GexBulk){
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

#if 0
    // Read the communist manifesto via bulk transfer
    br = (GexBulk){
            .buffer = (uint8_t *) longtext,
            .len = (uint32_t) strlen(longtext),
            .req_cmd = 3,
            .req_data = NULL,
            .req_len = 0,
    };
    assert(true == GEX_BulkWrite(test, &br));
#endif

    fprintf(stderr, "\n\nALL done, ending.\n");
	GEX_DeInit(gex);
	return 0;
}
