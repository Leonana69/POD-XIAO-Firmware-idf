#include "boot.h"

void bootSTM32Bootloader() {
    digitalWrite(STM_ENABLE_PIN, LOW);
    digitalWrite(STM_FLOW_CTRL_PIN, HIGH);
    delay(100);
    digitalWrite(STM_ENABLE_PIN, HIGH);
}

void bootSTM32Firmware() {
    digitalWrite(STM_ENABLE_PIN, LOW);
    digitalWrite(STM_FLOW_CTRL_PIN, LOW);
    delay(100);
    digitalWrite(STM_ENABLE_PIN, HIGH);
}

void bootSTM32Disable() {
    digitalWrite(STM_ENABLE_PIN, LOW);
}

void bootSTM32Enable() {
    digitalWrite(STM_ENABLE_PIN, HIGH);
}