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
#include "loco.h"

void app_main(void) {
    printf("System Starting...\n");
    bootInit();
    buttonInit();
    stmLinkInit();
    wifiInit(-1);
    wifiLinkInit();
    cameraInit();
    // dw1000_init();
    
	bootSTM32Firmware();
}
