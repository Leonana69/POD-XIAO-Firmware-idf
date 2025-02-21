#include "wifi_link.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "sys/socket.h"
#include "lwip/netdb.h"
#include "lwip/err.h"
#include "freertos/event_groups.h"
#include "link.h"

static WifiLink controlLink;
static WifiLink streamLink;
static bool wifiConnected = false;

// Initialize the PacketBuffer
void PacketBufferInit(PacketBuffer* buffer) {
    buffer->head = 0;
    buffer->tail = 0;
}

// Push a packet into the PacketBuffer
void PacketBufferPush(PacketBuffer* buffer, PodtpPacket *packet) {
    buffer->raw[buffer->head] = *packet;
    buffer->head = (buffer->head + 1) % MAX_PACKETS;
}

// Pop a packet from the PacketBuffer
PodtpPacket* PacketBufferPop(PacketBuffer* buffer) {
    if (buffer->head == buffer->tail) {
        return NULL;
    }
    PodtpPacket* packet = &buffer->raw[buffer->tail];
    buffer->tail = (buffer->tail + 1) % MAX_PACKETS;
    return packet;
}

// Check if the PacketBuffer is empty
bool PacketBufferEmpty(PacketBuffer* buffer) {
    return buffer->head == buffer->tail;
}

wifi_config_t wifiConfigs[3] = {
    {
        .sta = {
            .ssid = "YECL-DEMO",
            .password = "64221771",
        },
    },
    {
        .sta = {
            .ssid = "YECL-tplink",
            .password = "08781550",
        },
    },
    {
        .sta = {
            .ssid = "LEONA",
            .password = "64221771",
        },
    }
};

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;
static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < 10) {
            esp_wifi_connect();
            s_retry_num++;
            printf("Retry to connect to the AP\n");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        vTaskDelay(10);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        printf("Get IP: " IPSTR "\n", IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);

        // Get the signal strength (RSSI)
        wifi_ap_record_t ap_info;
        if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
            printf("Signal Strength (RSSI): %d dBm\n", ap_info.rssi);
        } else {
            printf("Failed to get signal strength\n");
        }
    }
}

int8_t wifiScan() {
    int8_t configIndex = -1;
    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = false
    };

    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true));
    uint16_t ap_count = 0;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
    wifi_ap_record_t *ap_list = (wifi_ap_record_t *)malloc(sizeof(wifi_ap_record_t) * ap_count);
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_count, ap_list));

    for (int j = 0; j < 3; j++) {
        bool found = false;
        for (int i = 0; i < ap_count; i++) {
            if (strcmp((char *) ap_list[i].ssid, (char *) wifiConfigs[j].sta.ssid) == 0) {
                configIndex = j;
                found = true;
                break;
            }
        }
        if (found)
            break;
    }
    
    free(ap_list);
    return configIndex;
}

void wifiInit(int8_t configIndex) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE)); // Disable power saving mode
    
    ESP_ERROR_CHECK(esp_wifi_start());
    // ESP_ERROR_CHECK(esp_wifi_set_channel(8, WIFI_SECOND_CHAN_NONE));
    ESP_ERROR_CHECK(esp_wifi_set_bandwidth(ESP_IF_WIFI_STA, WIFI_BW_HT20));
    
    if (configIndex < 0 || configIndex > 2) {
        configIndex = wifiScan();
    }

    if (configIndex < 0) {
        printf("Cannot find any known AP\n");
        return;
    }

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));
    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifiConfigs[configIndex]));
    ESP_ERROR_CHECK(esp_wifi_start());
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);
    if (bits & WIFI_CONNECTED_BIT) {
        printf("Connecting to %s [OK]\n", (char *) wifiConfigs[configIndex].sta.ssid);
        wifiConnected = true;
    } else if (bits & WIFI_FAIL_BIT) {
        printf("Connecting to %s [FAILED]\n", (char *) wifiConfigs[configIndex].sta.ssid);
    } else {
        printf("UNEXPECTED EVENT\n");
    }
}

typedef enum {
    PODTP_STATE_START_1,
    PODTP_STATE_START_2,
    PODTP_STATE_LENGTH,
    PODTP_STATE_RAW_DATA
} WifiLinkState;

static bool wifiParsePacket(uint8_t byte) {
    static WifiLinkState state = PODTP_STATE_START_1;
    static uint8_t length = 0;

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
                controlLink.rx_packet.length = 0;
                state = PODTP_STATE_RAW_DATA;
            }
            break;
        case PODTP_STATE_RAW_DATA:
            controlLink.rx_packet.raw[controlLink.rx_packet.length++] = byte;
            if (controlLink.rx_packet.length >= length) {
                state = PODTP_STATE_START_1;
                return true;
            }
            break;
        default:
            // reset state
            state = PODTP_STATE_START_1;
            break;
    }
    return false;
}

static void wifiLinkSendData(WifiLink *self, uint8_t *data, uint32_t length) {
    if (!self->enabled) {
        return;
    }
    send(self->client_socket, data, length, 0);
}

void wifiLinkSendPacket(PodtpPacket *packet) {
    PacketBufferPush(&controlLink.tx_buffer, packet);
}

void wifiLinkTxTask(void* pvParameters) {
    static uint8_t buffer[PODTP_MAX_DATA_LEN + 4]; // Reserve space for start bytes, length, and data
    WifiLink *self = (WifiLink *)pvParameters;
    
    while (true) {
        if (!PacketBufferEmpty(&self->tx_buffer)) {
            PodtpPacket *packet = PacketBufferPop(&self->tx_buffer);
            
            // Construct the packet directly in the buffer
            buffer[0] = PODTP_START_BYTE_1;
            buffer[1] = PODTP_START_BYTE_2;
            buffer[2] = packet->length;
            
            // Copy the packet data directly into the buffer
            memcpy(&buffer[3], packet->raw, packet->length);
            
            // Send the entire buffer
            wifiLinkSendData(self, buffer, packet->length + 3);
        }
        vTaskDelay(1);
    }
}

void wifiLinkEnableStream(bool enable) {
    streamLink.enabled = enable;
    streamLink.client_addr.sin_addr.s_addr = controlLink.client_addr.sin_addr.s_addr;
}

static uint16_t count = 0;
#define IMAGE_PACKET_SIZE 1024
#define IMAGE_HEADER_SIZE 8
#define IMAGE_PAYLOAD_SIZE (IMAGE_PACKET_SIZE - IMAGE_HEADER_SIZE)

typedef struct {
    union {
        struct {
            uint16_t seq;
            uint16_t index;
            uint16_t total;
            uint16_t size;
        };
        uint8_t raw[IMAGE_HEADER_SIZE];
    };
    uint8_t data[IMAGE_PAYLOAD_SIZE];
} ImagePacket;

static ImagePacket image_packet;

void wifiLinkSendImage(uint8_t *data, uint32_t length) {
    if (!streamLink.enabled) {
        return;
    }

    int packet_count = (length + IMAGE_PAYLOAD_SIZE - 1) / IMAGE_PAYLOAD_SIZE;
    for (int i = 0; i < packet_count; i++) {
        image_packet.seq = count;
        image_packet.index = i;
        image_packet.total = packet_count;
        image_packet.size = (i == packet_count - 1) ? length % IMAGE_PAYLOAD_SIZE : IMAGE_PAYLOAD_SIZE;
        memcpy(image_packet.data, data + i * IMAGE_PAYLOAD_SIZE, image_packet.size);
        sendto(streamLink.socket, (const char *)&image_packet, image_packet.size + IMAGE_HEADER_SIZE, 0, (struct sockaddr *)&streamLink.client_addr, streamLink.client_addr_len);
    }

    count += 1;

    if (count % 30 == 0) {
        wifi_ap_record_t ap_info;
        esp_wifi_sta_get_ap_info(&ap_info);
        printf("RSSI: %d\n", ap_info.rssi);
    }
}

void wifiLinkRxTask(void* pvParameters) {
    WifiLink *self = (WifiLink *)pvParameters;
    char rx_buffer[128];

    while (true) {
        int client_sock = accept(self->socket, (struct sockaddr *)&(self->client_addr), &(self->client_addr_len));
        if (client_sock < 0) {
            printf("Accept failed\n");
            continue;
        }
        self->enabled = true;
        self->client_socket = client_sock;

        while (self->enabled) {
            int len = recv(client_sock, rx_buffer, sizeof(rx_buffer) - 1, 0);

            if (len < 0) {
                printf("Receive failed\n");
                self->enabled = false;
                break;
            } else if (len == 0) {
                printf("Connection closed\n");
                self->enabled = false;
                break;
            } else {
                for (int i = 0; i < len; i++) {
                    if (wifiParsePacket(rx_buffer[i])) {
                        linkProcessPacket(&self->rx_packet);
                    }
                }
            }
        }
        close(client_sock);
    }
}

bool tcpLinkInit(WifiLink *self, uint16_t port) {
    self->socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (self->socket < 0) {
        printf("Cannot open socket");
        return false;
    }

    // Bind the socket to the port
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    if (bind(self->socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("Socket bind failed");
        close(self->socket);
        return false;
    }

    self->client_addr_len = sizeof(self->client_addr);
    // Listen for incoming connections
    if (listen(self->socket, 1) < 0) {
        printf("Socket listen failed");
        close(self->socket);
        return false;
    }
    return true;
}

bool udpLinkInit(WifiLink *self, uint16_t port) {
    self->socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (self->socket < 0) {
        printf("Cannot open socket");
        return false;
    }

    self->client_addr.sin_family = AF_INET;
    self->client_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    self->client_addr.sin_port = htons(port);
    self->client_addr_len = sizeof(self->client_addr);
    return true;
}

void wifiRSSITask() {
    while (true) {
        if (wifiConnected) {
            wifi_ap_record_t ap_info;
            esp_wifi_sta_get_ap_info(&ap_info);
            printf("RSSI: %d\n", ap_info.rssi);
        }
        vTaskDelay(1000);
    }
}

void wifiLinkInit() {
    if (!wifiConnected)
        return;

    if (tcpLinkInit(&controlLink, 80)) {
        // Create the Rx and Tx task
        xTaskCreatePinnedToCore(wifiLinkRxTask, "control_link_rx_task", 4096, &controlLink, 5, &controlLink.rx_task_handle, 1);
        xTaskCreatePinnedToCore(wifiLinkTxTask, "control_link_tx_task", 4096, &controlLink, 5, &controlLink.tx_task_handle, 1);
        // xTaskCreatePinnedToCore(wifiRSSITask, "wifi_rssi_task", 4096, NULL, 5, NULL, 1);
    } else {
        printf("Create Control TCP [FAILED]\n");
    }

    if (!udpLinkInit(&streamLink, 81)) {
        printf("Create Stream UDP [FAILED]\n");
    }
}