#include "link.h"
// #include "stm_link.h"
#include "wifi_link.h"
#include "boot.h"

void linkProcessPacket(PodtpPacket *packet) {
    switch (packet->type) {
        case PODTP_TYPE_ACK:
            printf("ACK: %s\n", packet->port == PODTP_PORT_OK ? "OK" : "ERROR");
            // if (!stmLink->ackQueuePut(packet)) {
            //     printf("ACK forward to WiFi\n");
            //     dataLink->sendPacket(packet);
            // }
            break;

        case PODTP_TYPE_COMMAND:
        case PODTP_TYPE_CTRL:
            printf("CO/CT: (%d, %d, %d)\n", packet->type, packet->port, packet->length);
            // stmLink->sendPacket(packet);
            break;

        case PODTP_TYPE_LOG:
            printf("LOG: (%d, %d)\n", packet->port, packet->length);
            wifiLinkSendPacket(&wifiLink, packet);
            break;

        case PODTP_TYPE_ESP32:
            // packets for ESP32 are not sent to STM32
            if (packet->port == PORT_ECHO) {
                printf("ECHO: (%d, %d)\n", packet->port, packet->length);
                wifiLinkSendPacket(&wifiLink, packet);
            } else if (packet->port == PORT_START_STM32_BOOTLOADER) {
                printf("Start STM32 Bootloader");
                bootSTM32Bootloader();
            } else if (packet->port == PORT_START_STM32_FIRMWARE) {
                printf("Start STM32 Firmware");
                bootSTM32Firmware();
            } else if (packet->port == PORT_ENABLE_STM32) {
                if (packet->data[0]) {
                    printf("Enable STM32");
                    bootSTM32Enable();
                } else {
                    printf("Disable STM32");
                    bootSTM32Disable();
                }
            } else if (packet->port == PORT_CONFIG_CAMERA) {
                printf("Config Camera");
            } else {
                printf("Unknown ESP32 packet: p=%d, l=%d\n", packet->port, packet->length);
            }
            break;
        case PODTP_TYPE_BOOT_LOADER:
            // stmLink->sendReliablePacket(packet);
            wifiLinkSendPacket(&wifiLink, packet);
            break;
        default:
            printf("Unknown packet: t=%d, p=%d, l=%d\n", packet->type, packet->port, packet->length);
            break;
    }
    return;
}