/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"

#include "esp_camera.h"
#include "esp_wifi.h"

#include "button.h"
#include "config.h"

void app_main(void) {
    printf("Hello, world!\n");
    buttonInit();
    xTaskCreatePinnedToCore(buttonTask, "button_task", 2048, NULL, 5, NULL, 1);
}