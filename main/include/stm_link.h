#ifndef __STM_LINK_H__
#define __STM_LINK_H__

#include "main.h"
#include "podtp.h"

class StmLink {
private:
    QueueHandle_t ackQueue;
    HardwareSerial uartSerial;
    PodtpPacket packetBufferRx;
    PodtpPacket packetBufferTx;
    bool uartParsePacket(uint8_t byte);
    bool waitForAck;
public:
    StmLink();
    void sendPacket(PodtpPacket *packet);
    void write(uint8_t *data, uint8_t length);
    bool ackQueuePut(PodtpPacket *packet);
    bool sendReliablePacket(PodtpPacket *packet, int retry = 10);
    void rxTask(void *pvParameters);
};

void stmLinkRxTask(void *pvParameters);

extern StmLink *stmLink;

#endif