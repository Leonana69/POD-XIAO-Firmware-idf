#ifndef __WIFI_LINK_H__
#define __WIFI_LINK_H__

#include "podtp.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "sys/socket.h"

#define MAX_PACKETS 20

typedef struct {
    PodtpPacket raw[MAX_PACKETS];
    uint8_t head;
    uint8_t tail;
} PacketBuffer;

// Initialize the PacketBuffer
void PacketBufferInit(PacketBuffer* buffer);
// Push a packet into the PacketBuffer
void PacketBufferPush(PacketBuffer* buffer, PodtpPacket *packet);
// Pop a packet from the PacketBuffer
PodtpPacket* PacketBufferPop(PacketBuffer* buffer);
// Check if the PacketBuffer is empty
bool PacketBufferEmpty(PacketBuffer* buffer);

typedef struct {
    int socket;
    int client_socket;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len;
    bool connected;
    TaskHandle_t rx_task_handle;
    PacketBuffer tx_buffer;
    PodtpPacket rx_packet;
    // Add other members as needed
} WifiLink;

void wifiInit(int8_t configIndex);
void wifiLinkInit();
void wifiLinkSendPacket(PodtpPacket *packet);
void wifiLinkResetStreamLink();
void wifiLinkSendImage(uint8_t *data, uint32_t length);

#endif // __WIFI_LINK_H__