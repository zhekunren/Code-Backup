#ifndef _LED_H
#define _LED_H
#include "main.h"

//LED端口定义
#define LED2(n)		(n?HAL_GPIO_WritePin(GPIOE,GPIO_PIN_5,GPIO_PIN_SET):HAL_GPIO_WritePin(GPIOE,GPIO_PIN_5,GPIO_PIN_RESET))
#define LED2_Toggle() (HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_5)) //LED0输出电平翻转
#define LED3(n)		(n?HAL_GPIO_WritePin(GPIOB,GPIO_PIN_5,GPIO_PIN_SET):HAL_GPIO_WritePin(GPIOE,GPIO_PIN_5,GPIO_PIN_RESET))
#define LED3_Toggle() (HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_5)) //LED1输出电平翻转

void LED_Init(void); 

#endif
