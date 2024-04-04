#ifndef __STM_LINK_H__
#define __STM_LINK_H__

#include "podtp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "driver/uart.h"
// class StmLink {
// private:
//     QueueHandle_t ackQueue;
//     HardwareSerial uartSerial;
//     PodtpPacket packetBufferRx;
//     PodtpPacket packetBufferTx;
//     bool uartParsePacket(uint8_t byte);
//     bool waitForAck;
// public:
//     StmLink();
//     void sendPacket(PodtpPacket *packet);
//     void write(uint8_t *data, uint8_t length);
//     bool ackQueuePut(PodtpPacket *packet);
//     bool sendReliablePacket(PodtpPacket *packet, int retry = 10);
//     void rxTask(void *pvParameters);
// };

typedef struct {
    QueueHandle_t ackQueue;
    TaskHandle_t rxTaskHandle;
    PodtpPacket packetBufferRx;
    PodtpPacket packetBufferTx;
    bool waitForAck;
    uart_port_t uartPort;
} StmLink;

void stmLinkInit(StmLink *link);
void stmLinkSendPacket(StmLink *self, PodtpPacket *packet);
void stmLinkRxTask(void *pvParameters);

// extern StmLink *stmLink;

#endif