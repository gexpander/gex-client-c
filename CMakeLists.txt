cmake_minimum_required(VERSION 3.10)
project(gex_client)

set(CMAKE_C_STANDARD 99)

set(SOURCE_FILES
        main.c
        gex/serial/serial.c
        gex/serial/serial.h
        gex/gex_client.c
        gex/gex_client.h
        gex/hexdump.c
        gex/hexdump.h
        gex/TF_Integration.c
        gex/protocol/payload_builder.c
        gex/protocol/payload_builder.h
        gex/protocol/payload_parser.c
        gex/protocol/payload_parser.h
        gex/protocol/TF_Config.h
        gex/protocol/TinyFrame.c
        gex/protocol/TinyFrame.h
        gex/protocol/type_coerce.h
)

include_directories(
        gex
        gex/protocol
        gex/serial
)

add_executable(gex_client ${SOURCE_FILES})
