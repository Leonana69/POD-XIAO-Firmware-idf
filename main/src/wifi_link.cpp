#include "wifi_link.h"
#include "link.h"
#include "camera_server.h"
#include "debug.h"

WifiConfig wifiConfigs[2] = {
    { "YECL-tplink", "08781550" },
    { "LEONA", "64221771" }
};

WifiLink *dataLink;
WifiLink *streamLink;
bool WifiLink::connected = false;
WifiLink::WifiLink(WifiConfig config, uint16_t port): server(port), client() {
    if (!WifiLink::connected) {
        WiFi.begin(config.ssid, config.password);
        type = port == 80 ? DATA : STREAM;
        DEBUG_PRINT("Connecting to WiFi");
        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            DEBUG_PRINT(".");
        }
        DEBUG_PRINT("\nWiFi connected with IP: %s\n", WiFi.localIP().toString().c_str());
        WifiLink::connected = true;
    }
	server.begin(port);
    server.setNoDelay(true);
    client.setNoDelay(true);
}

void WifiLink::sendPacket(PodtpPacket *packet) {
    packetBuffer.push(packet);
    if (!client || !client.connected()) {
        return;
    }

    while (!packetBuffer.empty()) {
        PodtpPacket *p = packetBuffer.pop();
        tx(p);
    }
}

void WifiLink::sendData(uint8_t *data, uint32_t length) {
    if (!client || !client.connected()) {
        client = server.available();
    }

    if (!client || !client.connected()) {
        return;
    }

    static uint8_t buffer[6] = { CAMERA_START_BYTE_1, CAMERA_START_BYTE_2, 0 };
    *((uint32_t *) &buffer[2]) = length;
    client.write(buffer, 6);
    client.write(data, length);
}

void WifiLink::tx(PodtpPacket *packet) {
    static uint8_t buffer[PODTP_MAX_DATA_LEN + 4] = { PODTP_START_BYTE_1, PODTP_START_BYTE_2, 0 };
    buffer[2] = packet->length;
    for (uint8_t i = 0; i < packet->length; i++) {
        buffer[i + 3] = packet->raw[i];
    }
    client.write(buffer, packet->length + 3);
}

void WifiLink::rxTask() {
	while (true) {
		if (!client || !client.connected()) {
			client = server.available();
		}
   		// check for new packet
		if (client && client.connected()) {
            if (client.available())
                while (client.available()) {
                    if (wifiParsePacket((uint8_t) client.read())) {
                        linkProcessPacket(&packetBufferRx);
                    }
                }
            // dump packet buffer
            if (!packetBuffer.empty()) {
                PodtpPacket *packet = packetBuffer.pop();
                tx(packet);
            }
		}
		vTaskDelay(10);
	}
}

typedef enum {
    PODTP_STATE_START_1,
    PODTP_STATE_START_2,
    PODTP_STATE_LENGTH,
    PODTP_STATE_RAW_DATA
} WifiLinkState;

bool WifiLink::wifiParsePacket(uint8_t byte) {
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
                packetBufferRx.length = 0;
                state = PODTP_STATE_RAW_DATA;
            }
            break;
        case PODTP_STATE_RAW_DATA:
            packetBufferRx.raw[packetBufferRx.length++] = byte;
            if (packetBufferRx.length >= length) {
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

void wifiLinkRxTask(void *pvParameters) {
    WifiLink *link = (WifiLink *) pvParameters;
    link->rxTask();
}