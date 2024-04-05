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

#include "button.h"
#include "boot.h"
#include "wifi_link.h"
#include "stm_link.h"
#include "camera_server.h"

void app_main(void) {
    printf("Hello, world!\n");
    bootInit();
    buttonInit();
    wifiInit(1);
    wifiLinkInit();
    stmLinkInit();
    cameraInit();
    
    bootSTM32Bootloader();
	bootSTM32Firmware();
}
