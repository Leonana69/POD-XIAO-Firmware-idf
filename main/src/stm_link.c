#include "stm_link.h"
#include "config.h"
#include "podtp.h"
#include "link.h"
#include "driver/gpio.h"

StmLink *stmLink;

void stmLinkInit(StmLink *self) {
    self->ackQueue = xQueueCreate(3, sizeof(PodtpPacket));
    self->waitForAck = false;
    self->rxTaskHandle = NULL;
    uart_config_t uart_config = {
        .baud_rate = 1000000,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NUM_0, &uart_config);
    uart_driver_install(UART_NUM_0, 256, 0, 0, NULL, 0);
    uart_set_pin(UART_NUM_0, STM_TX_PIN, STM_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    self->uartPort = UART_NUM_0;
}

void stmLinkSendPacket(StmLink *self, PodtpPacket *packet) {
    static uint8_t buffer[PODTP_MAX_DATA_LEN + 5] = { PODTP_START_BYTE_1, PODTP_START_BYTE_2, 0 };
    uint8_t check_sum[2] = { 0 };
    check_sum[0] = check_sum[1] = packet->length;
    buffer[2] = packet->length;
    for (uint8_t i = 0; i < packet->length; i++) {
        check_sum[0] += packet->raw[i];
        check_sum[1] += check_sum[0];
        buffer[i + 3] = packet->raw[i];
    }
    buffer[packet->length + 3] = check_sum[0];
    buffer[packet->length + 4] = check_sum[1];
    uart_write_bytes(self->uartPort, (const char *)buffer, packet->length + 5);
}

// void StmLink::write(uint8_t *data, uint8_t length) {
//     uartSerial.write(data, length);
// }

bool stmLinkAckQueuePut(StmLink *self, PodtpPacket *packet) {
    bool ret = false;
    if (self->waitForAck) {
        xQueueSend(self->ackQueue, packet, 0);
        ret = true;
    }
    return ret;
}

bool stmLinkSendReliablePacket(StmLink *self, PodtpPacket *packet, int retry) {
    self->packetBufferTx = *packet;
    packet->type = PODTP_TYPE_ACK;
    packet->length = 1;
    self->waitForAck = true;
    for (int i = 0; i < retry; i++) {
        // DEBUG_PRINT("SR [%d]: p=%d, l=%d\n", i, packet->port, packet->length);
        stmLinkSendPacket(self, &self->packetBufferTx);
        xQueueReceive(self->ackQueue, packet, 1000);
        if (packet->port == PODTP_PORT_OK) {
            self->waitForAck = false;
            return true;
        }
    }
    self->waitForAck = false;
    return false;
}

// void stmLinkRxTask(void *pvParameters) {
//     stmLink->rxTask(pvParameters);
// }

// void StmLink::rxTask(void *pvParameters) {
//     while (true) {
//         while (uartSerial.available()) {
//             if (uartParsePacket(uartSerial.read())) {
//                 linkProcessPacket(&packetBufferRx);
//             }
//         }
//         vTaskDelay(1);
//     }
// }

// typedef enum {
//     PODTP_STATE_START_1,
//     PODTP_STATE_START_2,
//     PODTP_STATE_LENGTH,
//     PODTP_STATE_RAW_DATA,
//     PODTP_STATE_CHECK_SUM_1,
//     PODTP_STATE_CHECK_SUM_2,
// } UartLinkState;

// bool StmLink::uartParsePacket(uint8_t byte) {
//     static UartLinkState state = PODTP_STATE_START_1;
//     static uint8_t length = 0;
//     static uint8_t check_sum[2] = { 0 };

//     switch (state) {
//         case PODTP_STATE_START_1:
//             if (byte == PODTP_START_BYTE_1) {
//                 state = PODTP_STATE_START_2;
//             }
//             break;
//         case PODTP_STATE_START_2:
//             state = (byte == PODTP_START_BYTE_2) ? PODTP_STATE_LENGTH : PODTP_STATE_START_1;
//             break;
//         case PODTP_STATE_LENGTH:
//             length = byte;
//             if (length > PODTP_MAX_DATA_LEN || length == 0) {
//                 state = PODTP_STATE_START_1;
//             } else {
//                 packetBufferRx.length = 0;
//                 check_sum[0] = check_sum[1] = length;
//                 state = PODTP_STATE_RAW_DATA;
//             }
//             break;
//         case PODTP_STATE_RAW_DATA:
//             packetBufferRx.raw[packetBufferRx.length++] = byte;
//             check_sum[0] += byte;
//             check_sum[1] += check_sum[0];
//             if (packetBufferRx.length == length) {
//                 state = PODTP_STATE_CHECK_SUM_1;
//             }
//             break;
//         case PODTP_STATE_CHECK_SUM_1:
//             state = (check_sum[0] == byte) ? PODTP_STATE_CHECK_SUM_2 : PODTP_STATE_START_1;
//             break;
//         case PODTP_STATE_CHECK_SUM_2:
//             state = PODTP_STATE_START_1;
//             if (check_sum[1] == byte) {
//                 return true;
//             }
//             break;
//     }
//     return false;
// }
