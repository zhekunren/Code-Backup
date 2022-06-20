#ifndef __OS_H_
#define __OS_H_

#include "config.h"
#include "task.h"
#include "memory.h"

void os_init(void);
void os_start(void);

void delay(uint32_t us);

#endif
