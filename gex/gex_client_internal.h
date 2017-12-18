//
// Created by MightyPork on 2017/12/15.
//

#ifndef GEX_CLIENT_GEX_CLIENT_INTERNAL_H
#define GEX_CLIENT_GEX_CLIENT_INTERNAL_H

struct gex_name_lu {
    char *name;               //!< Unit name
    char *type;               //!< Unit type
    uint8_t callsign;         //!< Unit callsign byte
    struct gex_name_lu *next; //!< Pointer to the next entry in this linked list, or NULL if none
};

struct gex_client_ {
    TinyFrame *tf;          //!< TinyFrame instance
    const char *acm_device; //!< Comport device name, might be used to reconnect (?)
    int acm_fd;             //!< Open comport file descriptor
    bool connected;         //!< Flag that we're currently connected to the comport

    // synchronous query "hacks"
    bool sync_query_ok;         //!< flag that the query response was received
    TF_Msg sync_query_response; //!< response message, copied here
    uint8_t sync_query_buffer[TF_MAX_PAYLOAD_RX]; //!< buffer for the rx payload to be copied here

    struct gex_name_lu *ulu_head; //!< Units look-up linked list
};

#endif //GEX_CLIENT_GEX_CLIENT_INTERNAL_H
