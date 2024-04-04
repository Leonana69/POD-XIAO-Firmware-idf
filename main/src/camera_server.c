#include "camera_server.h"
#include "wifi_link.h"

#include "esp_camera.h"
#define CAMERA_MODEL_XIAO_ESP32S3 // Has PSRAM
#include "camera_pins.h"

static uint8_t header[6] = { CAMERA_START_BYTE_1, CAMERA_START_BYTE_2, 0 };

void cameraServerTask(void *pvParameters) {
    camera_fb_t * fb = NULL;
    int count = 0;
    while (true) {
        fb = esp_camera_fb_get();
        if (!fb) {
            printf("Camera capture [FAILED]\n");
            continue;
        }

        *((uint32_t *) &header[2]) = fb->len;
        wifiLinkSendImage(header, 6);
        wifiLinkSendImage(fb->buf, fb->len);

        if (fb) {
            esp_camera_fb_return(fb);
            fb = NULL;
        }
        count++;
    }
}

void cameraInit() {
    printf("Camera init [START]\n");
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.frame_size = FRAMESIZE_UXGA;
    config.pixel_format = PIXFORMAT_JPEG; // for streaming
    // config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.jpeg_quality = 12;
    config.fb_count = 1;
    if (config.pixel_format == PIXFORMAT_JPEG) {
        // Limit the frame size when PSRAM is not available
        config.frame_size = FRAMESIZE_SVGA;
        config.fb_location = CAMERA_FB_IN_DRAM;
    } else {
        // Best option for face detection/recognition
        config.frame_size = FRAMESIZE_240X240;
        #if CONFIG_IDF_TARGET_ESP32S3
        config.fb_count = 2;
        #endif
    }
    // camera init
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        printf("Camera init [FAILED]: 0x%x\n", err);
        return;
    }

    sensor_t * s = esp_camera_sensor_get();
    // initial sensors are flipped vertically and colors are a bit saturated
    printf("Camera id: 0x%x\n", s->id.PID);
    // s->set_vflip(s, 1); // flip it back
    // s->set_brightness(s, 1); // up the brightness just a bit
    // s->set_saturation(s, -2); // lower the saturation
    // drop down frame size for higher initial frame rate
    if (config.pixel_format == PIXFORMAT_JPEG) {
        s->set_framesize(s, FRAMESIZE_VGA);
    }

    xTaskCreatePinnedToCore(cameraServerTask, "camera_server_task", 4096, NULL, 15, NULL, 0);
}