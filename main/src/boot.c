#include "boot.h"
#include "config.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void bootInit() {
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << STM_ENABLE_PIN) | (1ULL << STM_FLOW_CTRL_PIN);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);
    bootSTM32Disable();
}

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