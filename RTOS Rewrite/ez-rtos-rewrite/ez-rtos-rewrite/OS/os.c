#include "os.h"

#define SYSTICK_LOAD (CONFIG_OS_TICK_TIME_US * CONFIG_OS_SYSTICK_CLK - 1)

void PendSV_init() {
	/*Configure the PendSV priority */
  HAL_NVIC_SetPriority(PendSV_IRQn, (1<<__NVIC_PRIO_BITS) - 1 ,0); //0xff
}

void SysTick_init() {
    SysTick->LOAD  = SYSTICK_LOAD;
    SysTick->VAL   = 0;
    SysTick->CTRL  |=  SysTick_CTRL_CLKSOURCE_Msk | // 0=AHB/8£»1=AHB
                      SysTick_CTRL_TICKINT_Msk   |
                      SysTick_CTRL_ENABLE_Msk	|
                      0; 
    NVIC_SetPriority(SysTick_IRQn, (1<<__NVIC_PRIO_BITS) - 1); //0xff
}

void os_init() {	
    init_task();
    init_memory();
//    init_io();
}

void os_start() {
    PendSV_init();
    SysTick_init();
}

void delay(uint32_t us) {
    uint8_t old_switch_task_enable = task_switch_enable;
    uint32_t old_SysTick_val = SysTick->VAL;
    task_switch_enable = 0;

    const uint32_t load = us * CONFIG_OS_SYSTICK_CLK - 1;
    SysTick->LOAD = load;
    
    while (SysTick->VAL) {
    }
    
    task_switch_enable = old_switch_task_enable;
    SysTick->LOAD = SYSTICK_LOAD;
    SysTick->VAL = 0;
}
