//
// Created by MightyPork on 2017/12/15.
//

#ifndef GEX_CLIENT_GEX_MESSAGE_TYPES_H
#define GEX_CLIENT_GEX_MESSAGE_TYPES_H

/**
 * Supported message types (TF_TYPE)
 */
enum TF_Types_ {
    // General, low level
    MSG_SUCCESS  = 0x00, //!< Generic success response; used by default in all responses; payload is transaction-specific
    MSG_PING     = 0x01, //!< Ping request (or response), used to test connection
    MSG_ERROR    = 0x02, //!< Generic failure response (when a request fails to execute)
    // Unit messages
    MSG_UNIT_REQUEST  = 0x10, //!< Command addressed to a particular unit
    MSG_UNIT_REPORT   = 0x11, //!< Spontaneous report from a unit
    // System messages
    MSG_LIST_UNITS    = 0x20, //!< Get all unit call-signs and names
};

#endif //GEX_CLIENT_GEX_MESSAGE_TYPES_H
