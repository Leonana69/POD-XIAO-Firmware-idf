#include "boot.h"
#include "config.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void bootSTM32Bootloader() {
    gpio_set_level(STM_ENABLE_PIN, LOW);
    gpio_set_level(STM_FLOW_CTRL_PIN, HIGH);
    vTaskDelay(100);
    gpio_set_level(STM_ENABLE_PIN, HIGH);
}

void bootSTM32Firmware() {
    gpio_set_level(STM_ENABLE_PIN, LOW);
    gpio_set_level(STM_FLOW_CTRL_PIN, LOW);
    vTaskDelay(100);
    gpio_set_level(STM_ENABLE_PIN, HIGH);
}

void bootSTM32Disable() {
    gpio_set_level(STM_ENABLE_PIN, LOW);
}

void bootSTM32Enable() {
    gpio_set_level(STM_ENABLE_PIN, HIGH);
}