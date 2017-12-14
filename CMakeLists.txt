cmake_minimum_required(VERSION 3.10)
project(gex_client)

set(CMAKE_C_STANDARD 99)

set(SOURCE_FILES
        main.c
        gex/serial/serial.c
        gex/serial/serial.h
        gex/gex_client.c
        gex/gex_client.h
        gex/gex_client_internal.h
        gex/gex_message_types.h
        gex/utils/hexdump.c
        gex/utils/hexdump.h
        gex/TF_Integration.c
        gex/utils/payload_builder.c
        gex/utils/payload_builder.h
        gex/utils/payload_parser.c
        gex/utils/payload_parser.h
        gex/protocol/TF_Config.h
        gex/protocol/TinyFrame.c
        gex/protocol/TinyFrame.h
        gex/utils/type_coerce.h
)

include_directories(
        gex
        gex/protocol
        gex/serial
)

add_executable(gex_client ${SOURCE_FILES})
