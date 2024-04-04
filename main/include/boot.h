#ifndef __BOOT_H__
#define __BOOT_H__

#include "config.h"

void bootSTM32Bootloader();
void bootSTM32Firmware();
void bootSTM32Disable();
void bootSTM32Enable();

#endif // __BOOT_H__