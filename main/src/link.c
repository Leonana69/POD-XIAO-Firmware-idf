#include "link.h"
#include "stm_link.h"
#include "wifi_link.h"
#include "boot.h"
#include "camera_server.h"

void linkProcessPacket(PodtpPacket *packet) {
    switch (packet->type) {
        case PODTP_TYPE_ACK:
            printf("ACK: %s\n", packet->port == PORT_ACK_OK ? "OK" : "ERROR");
            if (!stmLinkAckQueuePut(packet)) {
                printf("ACK forward to WiFi\n");
                wifiLinkSendPacket(packet);
            }
            break;

        case PODTP_TYPE_COMMAND:
        case PODTP_TYPE_CTRL:
            printf("CO/CT: (%d, %d, %d)\n", packet->type, packet->port, packet->length);
            if (packet->ack == true) {
                stmLinkSendReliablePacket(packet, 5);
                wifiLinkSendPacket(packet);
            } else
                stmLinkSendPacket(packet);
            break;

        case PODTP_TYPE_LOG:
            printf("LOG: (%d, %d)\n", packet->port, packet->length);
            wifiLinkSendPacket(packet);
            break;

        case PODTP_TYPE_ESP32:
            // packets for ESP32 are not sent to STM32
            if (packet->port == PORT_ESP32_ECHO) {
                printf("ECHO: (%d, %d)\n", packet->port, packet->length);
                wifiLinkSendPacket(packet);
            } else if (packet->port == PORT_ESP32_START_STM32_BOOTLOADER) {
                if (packet->data[0] == 1) {
                    printf("Start STM32 Bootloader\n");
                    bootSTM32Bootloader();
                } else {
                    printf("Start STM32 Firmware\n");
                    bootSTM32Firmware();
                }
            } else if (packet->port == PORT_ESP32_ENABLE_STM32) {
                if (packet->data[0]) {
                    printf("Enable STM32\n");
                    bootSTM32Enable();
                } else {
                    printf("Disable STM32\n");
                    bootSTM32Disable();
                }
            } else if (packet->port == PORT_ESP32_CONFIG_CAMERA) {
                printf("Config Camera ");
                if (packet->length - 1 != sizeof(_camera_config_t)) {
                    printf(" [FAILED]\n");
                } else {
                    printf("[OK]\n");
                    cameraConfig((_camera_config_t *) &packet->data[0]);
                }
            } else if (packet->port == PORT_ESP32_RESET_STREAM_LINK) {
                printf("Reset Stream Link\n");
                wifiLinkResetStreamLink();
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