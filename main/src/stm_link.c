#include "stm_link.h"
#include "config.h"
#include "podtp.h"
#include "link.h"
#include "driver/gpio.h"
#include <string.h>

static StmLink stmLink;

#define UART_RX_BUFFER_LENGTH 512
bool stmLinkUartParsePacket(uint8_t byte);
void stmLinkRxTask(void *pvParameters) {
    StmLink *self = (StmLink *)pvParameters;
    size_t length;
    uint8_t buffer[UART_RX_BUFFER_LENGTH];
    while (true) {
        uart_get_buffered_data_len(self->uartPort, &length);
        if (length > 0) {
            int len = uart_read_bytes(self->uartPort, buffer, UART_RX_BUFFER_LENGTH, 1);
            for (int i = 0; i < len; i++) {
                if (stmLinkUartParsePacket(buffer[i])) {
                    linkProcessPacket(&self->rxPacket);
                }
            }
        }
        vTaskDelay(1);
    }
}

void stmLinkTxTask(void *pvParameters) {
    StmLink *self = (StmLink *)pvParameters;
    static uint8_t buffer[2 + 1 + 1 + PODTP_MAX_DATA_LEN + 2 + 5] = { PODTP_START_BYTE_1, PODTP_START_BYTE_2, 0 };
    PodtpPacket *packet = (PodtpPacket *) &buffer[2];
    uint8_t check_sum[2] = { 0 };

    while (true) {
        if (xQueueReceive(self->txQueue, packet, portMAX_DELAY) == pdTRUE) {
            if (packet->length > PODTP_MAX_DATA_LEN) {
                printf("Error: Packet length exceeds maximum allowed size\n");
            } else {
                // uartSendPacket(&packet);
                check_sum[0] = check_sum[1] = packet->length;
                for (uint8_t i = 0; i < packet->length; i++) {
                    check_sum[0] += packet->raw[i];
                    check_sum[1] += check_sum[0];
                }
                buffer[packet->length + 3] = check_sum[0];
                buffer[packet->length + 4] = check_sum[1];
                // add tail to reset the state machine
                *(uint32_t *) &buffer[packet->length + 5] = 0x0A0D0A0D;
                uart_write_bytes(self->uartPort, (const char *)buffer, packet->length + 9);
            }
        }
    }
}

void stmLinkInit() {
    stmLink.ackQueue = xQueueCreate(3, sizeof(PodtpPacket));
    stmLink.waitForAck = false;
    uart_config_t uart_config = {
        .baud_rate = 1000000,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_XTAL,
    };
    uart_param_config(UART_NUM_0, &uart_config);
    uart_driver_install(UART_NUM_0, UART_RX_BUFFER_LENGTH, 0, 0, NULL, 0);
    uart_set_pin(UART_NUM_0, STM_TX_PIN, STM_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    stmLink.uartPort = UART_NUM_0;

    stmLink.txQueue = xQueueCreate(10, sizeof(PodtpPacket));
    xTaskCreatePinnedToCore(stmLinkRxTask, "stmLinkRxTask", 4096, &stmLink, 7, &stmLink.rxTaskHandle, 1);
    xTaskCreatePinnedToCore(stmLinkTxTask, "stmLinkTxTask", 4096, &stmLink, 7, &stmLink.txTaskHandle, 1);
}

void stmLinkSendPacket(PodtpPacket *packet) {
    xQueueSend(stmLink.txQueue, packet, 0);
}

bool stmLinkAckQueuePut(PodtpPacket *packet) {
    bool ret = false;
    if (stmLink.waitForAck) {
        xQueueSend(stmLink.ackQueue, packet, 0);
        ret = true;
    }
    return ret;
}

bool stmLinkSendReliablePacket(PodtpPacket *packet, int retry) {
    bool success = false;
    PodtpPacket ackPacket;
    stmLink.waitForAck = true;

    for (int i = 0; i < retry; i++) {
        // printf("SR [%d]: p=%d, l=%d\n", i, packet->port, packet->length);
        stmLinkSendPacket(packet);
        xQueueReceive(stmLink.ackQueue, &ackPacket, 1000);
        if (ackPacket.port == PORT_ACK_OK) {
            success = true;
            break;
        }
    }

    stmLink.waitForAck = false;
    packet->type = PODTP_TYPE_ACK;
    packet->port = success ? PORT_ACK_OK : PORT_ACK_ERROR;
    packet->length = 1;
    return success;
}

typedef enum {
    PODTP_STATE_START_1,
    PODTP_STATE_START_2,
    PODTP_STATE_LENGTH,
    PODTP_STATE_RAW_DATA,
    PODTP_STATE_CHECK_SUM_1,
    PODTP_STATE_CHECK_SUM_2,
} UartLinkState;

bool stmLinkUartParsePacket(uint8_t byte) {
    static UartLinkState state = PODTP_STATE_START_1;
    static uint8_t length = 0;
    static uint8_t check_sum[2] = { 0 };

    switch (state) {
        case PODTP_STATE_START_1:
            if (byte == PODTP_START_BYTE_1) {
                state = PODTP_STATE_START_2;
            }
            break;
        case PODTP_STATE_START_2:
            state = (byte == PODTP_START_BYTE_2) ? PODTP_STATE_LENGTH : PODTP_STATE_START_1;
            break;
        case PODTP_STATE_LENGTH:
            length = byte;
            if (length > PODTP_MAX_DATA_LEN || length == 0) {
                state = PODTP_STATE_START_1;
            } else {
                stmLink.rxPacket.length = 0;
                check_sum[0] = check_sum[1] = length;
                state = PODTP_STATE_RAW_DATA;
            }
            break;
        case PODTP_STATE_RAW_DATA:
            stmLink.rxPacket.raw[stmLink.rxPacket.length++] = byte;
            check_sum[0] += byte;
            check_sum[1] += check_sum[0];
            if (stmLink.rxPacket.length == length) {
                state = PODTP_STATE_CHECK_SUM_1;
            }
            break;
        case PODTP_STATE_CHECK_SUM_1:
            state = (check_sum[0] == byte) ? PODTP_STATE_CHECK_SUM_2 : PODTP_STATE_START_1;
            break;
        case PODTP_STATE_CHECK_SUM_2:
            state = PODTP_STATE_START_1;
            if (check_sum[1] == byte) {
                return true;
            }
            break;
    }
    return false;
}
