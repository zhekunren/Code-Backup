/**
  ******************************************************************************
  * @file    gpio.h
  * @brief   This file contains all the function prototypes for
  *          the gpio.c file
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __GPIO_H__
#define __GPIO_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* USER CODE BEGIN Private defines */
#define LED2(n)		    (n?HAL_GPIO_WritePin(GPIOE,GPIO_PIN_5,GPIO_PIN_SET):HAL_GPIO_WritePin(GPIOE,GPIO_PIN_5,GPIO_PIN_RESET))
#define LED2_Toggle() (HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_5)) //LED0输出电平翻转
#define LED3(n)		    (n?HAL_GPIO_WritePin(GPIOB,GPIO_PIN_5,GPIO_PIN_SET):HAL_GPIO_WritePin(GPIOB,GPIO_PIN_5,GPIO_PIN_RESET))
#define LED3_Toggle() (HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_5)) //LED1输出电平翻转

#define KEY1()        HAL_GPIO_ReadPin(GPIOE,GPIO_PIN_4) //KEY1按键
#define KEY2()        HAL_GPIO_ReadPin(GPIOE,GPIO_PIN_3) //KEY2按键
#define KEY3()        HAL_GPIO_ReadPin(GPIOE,GPIO_PIN_2) //KEY3按键
#define WK_UP()       HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_0) //WKUP按键
/* USER CODE END Private defines */

void MX_GPIO_Init(void);

/* USER CODE BEGIN Prototypes */

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif
#endif /*__ GPIO_H__ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
