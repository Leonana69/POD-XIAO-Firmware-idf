#include "stm_link.h"
#include "config.h"
#include "podtp.h"
#include "link.h"
#include "driver/gpio.h"

static StmLink stmLink;

#define UART_RX_BUFFER_LENGTH 512
bool stmLinkUartParsePacket(uint8_t byte);
void stmLinkRxTask(void *pvParameters) {
    size_t length;
    uint8_t buffer[UART_RX_BUFFER_LENGTH];
    while (true) {
        uart_get_buffered_data_len(stmLink.uartPort, &length);
        if (length > 0) {
            int len = uart_read_bytes(stmLink.uartPort, buffer, UART_RX_BUFFER_LENGTH, 1);
            for (int i = 0; i < len; i++) {
                if (stmLinkUartParsePacket(buffer[i])) {
                    linkProcessPacket(&stmLink.packetBufferRx);
                }
            }
        }
        vTaskDelay(1);
    }
}

void stmLinkInit() {
    stmLink.ackQueue = xQueueCreate(3, sizeof(PodtpPacket));
    stmLink.waitForAck = false;
    stmLink.rxTaskHandle = NULL;
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
    xTaskCreatePinnedToCore(stmLinkRxTask, "stmLinkRxTask", 4096, NULL, 10, &stmLink.rxTaskHandle, 1);
}

void stmLinkSendPacket(PodtpPacket *packet) {
    static uint8_t buffer[PODTP_MAX_DATA_LEN + 2 + 2 + 2 + 4] = { PODTP_START_BYTE_1, PODTP_START_BYTE_2, 0 };
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
    // add tail to reset the state machine
    *(uint32_t *) &buffer[packet->length + 5] = 0x0A0D0A0D;
    uart_write_bytes(stmLink.uartPort, (const char *)buffer, packet->length + 9);
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
    stmLink.packetBufferTx = *packet;
    packet->type = PODTP_TYPE_ACK;
    packet->length = 1;
    stmLink.waitForAck = true;
    for (int i = 0; i < retry; i++) {
        // printf("SR [%d]: p=%d, l=%d\n", i, packet->port, packet->length);
        stmLinkSendPacket(&stmLink.packetBufferTx);
        xQueueReceive(stmLink.ackQueue, packet, 1000);
        if (packet->port == PORT_ACK_OK) {
            stmLink.waitForAck = false;
            return true;
        }
    }
    stmLink.waitForAck = false;
    return false;
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
                stmLink.packetBufferRx.length = 0;
                check_sum[0] = check_sum[1] = length;
                state = PODTP_STATE_RAW_DATA;
            }
            break;
        case PODTP_STATE_RAW_DATA:
            stmLink.packetBufferRx.raw[stmLink.packetBufferRx.length++] = byte;
            check_sum[0] += byte;
            check_sum[1] += check_sum[0];
            if (stmLink.packetBufferRx.length == length) {
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
