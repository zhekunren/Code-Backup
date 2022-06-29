/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
struct node_t {
	void *container;
	struct node_t *next;
};

struct person {
	char *name;
	int age;
	struct node_t node;
};

struct list {
	char *name; /* A�� */
	struct node_t head;
};

void InitList(struct list *pList, char *name)
{
	pList->name = name;
	pList->head.next = NULL;
}

void AddItemToList(struct list *pList, struct node_t *new_node)
{
	struct node_t *last = &pList->head;
	
	while (last->next)
	{
		last = last->next;
	}
	
	/* last->next == NULL */
	last->next = new_node;
	new_node->next =NULL;
}

void AddItemAfter(struct node_t *pre, struct node_t *new_node)
{
	new_node->next = pre->next;
	pre->next = new_node;
}

void DelItemFromList(struct list *pList, struct node_t *node)
{
	struct node_t *pre = &pList->head;
	
	/* �ҵ�node */
	while (pre != NULL && pre->next != node)
	{
		pre = pre->next;
	}
	
	/* û�ҵ� */
	if (pre == NULL)
		return;
	else
		pre->next = node->next;
}

int CmpPersonAge(struct node_t *pre, struct node_t *next)
{
	struct person *p;
	struct person *n;
	
	p = pre->container;
	n = next->container;
	
	if (p->age < n->age)
		return -1;
	else
		return 0;
}

void SortList(struct list *pList)
{
	struct node_t *pre1 = &pList->head;
	struct node_t *pre2;
	struct node_t *cur = pre1->next;
	struct node_t *next;
	struct node_t *tmp;
		
	while (cur)
	{
		pre2 = cur;
		next = cur->next;
		while (next)
		{
			if (CmpPersonAge(cur, next) == 0)
			{
				/* �����ڵ� */
				/* 1. del cur */
				DelItemFromList(pList, cur);
				
				/* 2. del next */
				DelItemFromList(pList, next);
				
				/* 3. ��pre1֮�����next */
				AddItemAfter(pre1, next);
				
				/* 4. ��pre2֮�����cur */
				if (pre2 == cur)
					AddItemAfter(next, cur);
				else
					AddItemAfter(pre2, cur);
				
				/* 5. cur/nextָ�뻥�� */
				tmp = cur;
				cur = next;
				next = tmp;				
			}
			
			pre2 = next;
			next = next->next;
		}
		
		pre1 = cur;
		cur = cur->next;
	}
	
}

void PrintList(struct list *pList)
{
	int i = 0;
	
	struct node_t *node = pList->head.next;
	struct person *p;
	
	while (node != NULL)
	{
		p = node->container;
		printf("person %d: %s is %d\r\n", i++, p->name, p->age);
		
		/* ���滹����, �ƶ�����һ�� */
		node = node->next;
	}
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
	struct list a_list;
	int i;

	struct person p[] = {
		{"p1", 10, {NULL, NULL}},
		{"p2", 20, {NULL, NULL}},
		{"p3", 13, {NULL, NULL}},
		{"p4", 41, {NULL, NULL}},
		{"p5", 56, {NULL, NULL}},
		{"p6", 12, {NULL, NULL}},
		{"p7", 9,  {NULL, NULL}},
		{"p8", 21, {NULL, NULL}},
		{"p9", 22, {NULL, NULL}},
		{"p10", 21, {NULL, NULL}},
		{"p11", 20, {NULL, NULL}},
		{NULL, 0, {NULL, NULL}},
	};
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
	InitList(&a_list, "A_class");

	i = 0;
	while (p[i].name != NULL)
	{
		p[i].node.container = &p[i];
		AddItemToList(&a_list, &p[i].node);
		i++;
	}

	printf("add all person:\r\n");
	PrintList(&a_list);
	
	DelItemFromList(&a_list, &p[3].node);

	printf("del person %s:\r\n", p[3].name);
	PrintList(&a_list);

	DelItemFromList(&a_list, &p[0].node);
	
	printf("del person %s:\r\n", p[0].name);
	PrintList(&a_list);
	
	SortList(&a_list);
	printf("sort list, all person:\r\n");
	PrintList(&a_list);
    
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
