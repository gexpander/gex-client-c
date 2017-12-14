//
// Created by MightyPork on 2017/12/15.
//

#ifndef GEX_CLIENT_GEX_CLIENT_INTERNAL_H
#define GEX_CLIENT_GEX_CLIENT_INTERNAL_H

struct gex_name_lu {
    char *name;
    char *type;
    uint8_t callsign;
    struct gex_name_lu *next;
};

struct gex_client_ {
    TinyFrame *tf;          //!< TinyFrame instance
    const char *acm_device; //!< Comport device name, might be used to reconnect
    int acm_fd;             //!< Open comport file descriptor
    bool connected;         //!< Flag that we're currently connected to the comport

    struct gex_name_lu *ulu_head; //!< Units look-up
};

#endif //GEX_CLIENT_GEX_CLIENT_INTERNAL_H
