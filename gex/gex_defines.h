//
// Created by MightyPork on 2017/12/19.
//

#ifndef GEX_CLIENT_GEX_DEFINES_H
#define GEX_CLIENT_GEX_DEFINES_H

#include <stdint.h>
#include <stdbool.h>

typedef TF_ID GexSession;

typedef struct gex_client GexClient;
typedef struct gex_unit GexUnit;
typedef struct gex_msg GexMsg;

/** Callback for spontaneous reports from units */
typedef void (*GexEventListener)(GexMsg msg);

/**
 * GEX message, used e.g. as a return value to static query.
 * Contains all needed information to lead a multi-part dialogue.
 */
struct gex_msg {
    GexUnit *unit;
    uint8_t *payload;
    uint32_t len;
    GexSession session;
    union {
        uint8_t cmd;
        uint8_t type;
    };
};

/**
 * GEX unit instance, allocated based on configuration read from the GEX board.
 */
struct gex_unit {
    GexClient *gex;           //!< Client instance
    char *name;               //!< Unit name (loaded, malloc'd)
    char *type;               //!< Unit type (loaded, malloc'd)
    uint8_t callsign;         //!< Unit callsign
    GexEventListener report_handler; //!< Report handling function
    struct gex_unit *next; //!< Pointer to the next entry in this linked list, or NULL if none
};

#endif //GEX_CLIENT_GEX_DEFINES_H
