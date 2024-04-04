#include "link.h"
#include "stm_link.h"
#include "wifi_link.h"
#include "boot.h"

void linkProcessPacket(PodtpPacket *packet) {
    switch (packet->type) {
        case PODTP_TYPE_ACK:
            printf("ACK: %s\n", packet->port == PODTP_PORT_OK ? "OK" : "ERROR");
            if (!stmLinkAckQueuePut(packet)) {
                printf("ACK forward to WiFi\n");
                wifiLinkSendPacket(packet);
            }
            break;

        case PODTP_TYPE_COMMAND:
        case PODTP_TYPE_CTRL:
            printf("CO/CT: (%d, %d, %d)\n", packet->type, packet->port, packet->length);
            stmLinkSendPacket(packet);
            break;

        case PODTP_TYPE_LOG:
            printf("LOG: (%d, %d)\n", packet->port, packet->length);
            wifiLinkSendPacket(packet);
            break;

        case PODTP_TYPE_ESP32:
            // packets for ESP32 are not sent to STM32
            if (packet->port == PORT_ECHO) {
                printf("ECHO: (%d, %d)\n", packet->port, packet->length);
                wifiLinkSendPacket(packet);
            } else if (packet->port == PORT_START_STM32_BOOTLOADER) {
                printf("Start STM32 Bootloader\n");
                bootSTM32Bootloader();
            } else if (packet->port == PORT_START_STM32_FIRMWARE) {
                printf("Start STM32 Firmware\n");
                bootSTM32Firmware();
            } else if (packet->port == PORT_ENABLE_STM32) {
                if (packet->data[0]) {
                    printf("Enable STM32\n");
                    bootSTM32Enable();
                } else {
                    printf("Disable STM32\n");
                    bootSTM32Disable();
                }
            } else if (packet->port == PORT_CONFIG_CAMERA) {
                printf("Config Camera");
            } else {
                printf("Unknown ESP32 packet: p=%d, l=%d\n", packet->port, packet->length);
            }
            break;
        case PODTP_TYPE_BOOT_LOADER:
            stmLinkSendReliablePacket(packet, 10);
            wifiLinkSendPacket(packet);
            break;
        default:
            printf("Unknown packet: t=%d, p=%d, l=%d\n", packet->type, packet->port, packet->length);
            break;
    }
    return;
}