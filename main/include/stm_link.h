#ifndef __STM_LINK_H__
#define __STM_LINK_H__

#include "podtp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "driver/uart.h"

typedef struct {
    QueueHandle_t ackQueue;
    TaskHandle_t rxTaskHandle;
    PodtpPacket packetBufferRx;
    PodtpPacket packetBufferTx;
    bool waitForAck;
    uart_port_t uartPort;
} StmLink;

void stmLinkInit();
void stmLinkSendPacket(PodtpPacket *packet);
bool stmLinkSendReliablePacket(PodtpPacket *packet, int retry);
bool stmLinkAckQueuePut(PodtpPacket *packet);
void stmLinkRxTask(void *pvParameters);

// extern StmLink *stmLink;

#endif