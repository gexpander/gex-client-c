//
// Created by MightyPork on 2017/12/19.
//

#ifndef GEX_CLIENT_GEX_DEFINES_H
#define GEX_CLIENT_GEX_DEFINES_H

#ifndef GEX_H
#error "Include gex.h instead!"
#endif

#include <stdint.h>
#include <stdbool.h>

#include "TinyFrame.h"

typedef TF_ID GexSession;

typedef struct gex_client GexClient;
typedef struct gex_unit GexUnit;
typedef struct gex_msg GexMsg;
typedef struct gex_bulk GexBulk;

/** Callback for spontaneous reports from units */
typedef void (*GexEventListener)(GexMsg msg);

/**
 * GEX message, used e.g. as a return value to static query.
 * Contains all needed information to lead a multi-part dialogue.
 */
struct gex_msg {
    GexUnit *unit;       //!< Unit this message belongs to
    GexSession session;  //!< TinyFrame session used in the message
    uint8_t *payload;    //!< Useful payload
    uint32_t len;        //!< Payload length
    uint8_t type;        //!< Message type (e.g. MSG_ERROR), or report type in event handler
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

/**
 * Bulk transport info struct
 */
struct gex_bulk {
    uint8_t req_cmd;          //!< Bulk request command
    const uint8_t *req_data;  //!< Bulk request data
    uint32_t req_len;         //!< Bulk request data len

    uint8_t *buffer;          //!< Transfer buffer (rx or tx)
    // those are in a union to make it more obvious they shouldn't be used at once
    union {
        uint32_t capacity;    //!< Receive buffer capacity
        uint32_t len;         //!< Transfer buffer length
    };
};

#endif //GEX_CLIENT_GEX_DEFINES_H
