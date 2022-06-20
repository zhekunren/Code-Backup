#ifndef OS_CONFIG_H
#define OS_CONFIG_H

#include <stdint.h>

#include "stm32f1xx_hal.h"

// unit: MHz
#define CONFIG_SYSCLK 72
// unit: us
#define CONFIG_OS_SYSCLK_DIV 1
#define CONFIG_OS_SYSTICK_CLK (CONFIG_SYSCLK / CONFIG_OS_SYSCLK_DIV)
// 10ms
#define CONFIG_OS_TICK_TIME_US 10000

#define CONFIG_OS_HEAP_SIZE 4096

#endif //OS_CONFIG_H
