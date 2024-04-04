#ifndef __PODTP_H__
#define __PODTP_H__

#include <stdint.h>

#define PODTP_MAX_DATA_LEN 126

#define PODTP_START_BYTE_1 0xAD
#define PODTP_START_BYTE_2 0x6E

enum {
    PODTP_TYPE_ACK = 0x1,
    PODTP_TYPE_COMMAND = 0x2,
    PODTP_TYPE_LOG = 0x3,
    PODTP_TYPE_CTRL = 0x4,
    PODTP_TYPE_ESP32 = 0xE,
    PODTP_TYPE_BOOT_LOADER = 0xF,
};

enum {
    // PODTP_TYPE_ACK
    PODTP_PORT_ERROR = 0x0,
    PODTP_PORT_OK = 0x1,

    // PODTP_TYPE_COMMAND
    PODTP_PORT_RPYT = 0x0,
    PODTP_PORT_TAKEOFF = 0x1,
    PODTP_PORT_LAND = 0x2,
    PODTP_PORT_HOVER = 0x3,
    PODTP_PORT_POSITION = 0x4,
    
    // PODTP_TYPE_LOG
    PODTP_PORT_STRING = 0x0,

    // PODTP_TYPE_CTRL
    PODTP_PORT_LOCK = 0x0,
    PODTP_PORT_KEEP_ALIVE = 0x1,

    // PODTP_TYPE_ESP32
    PORT_ECHO = 0x0,
    PORT_START_STM32_BOOTLOADER = 0x1,
    PORT_START_STM32_FIRMWARE = 0x2,
    PORT_ENABLE_STM32 = 0x3,
    PORT_CONFIG_CAMERA = 0x4,

    // PODTP_TYPE_BOOT_LOADER
    PORT_LOAD_BUFFER = 0x1,
    PORT_WRITE_FLASH = 0x2,
};

typedef struct {
    uint8_t length;
    union {
        struct {
            union {
                uint8_t header;
                struct {
                    uint8_t port:3;
                    uint8_t ack:1;
                    uint8_t type:4;
                };
            };
            uint8_t data[PODTP_MAX_DATA_LEN];
        } __attribute__((packed));
        uint8_t raw[PODTP_MAX_DATA_LEN + 1];
    } __attribute__((aligned(4)));
} PodtpPacket;

#endif // __PODTP_H__