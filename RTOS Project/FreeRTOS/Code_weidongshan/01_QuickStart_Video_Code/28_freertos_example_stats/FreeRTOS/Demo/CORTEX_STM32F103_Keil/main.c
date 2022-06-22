/*
 * FreeRTOS V202111.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/*
 * Creates all the demo application tasks, then starts the scheduler.  The WEB
 * documentation provides more details of the standard demo application tasks.
 * In addition to the standard demo tasks, the following tasks and tests are
 * defined and/or created within this file:
 *
 * "Fast Interrupt Test" - A high frequency periodic interrupt is generated
 * using a free running timer to demonstrate the use of the
 * configKERNEL_INTERRUPT_PRIORITY configuration constant.  The interrupt
 * service routine measures the number of processor clocks that occur between
 * each interrupt - and in so doing measures the jitter in the interrupt timing.
 * The maximum measured jitter time is latched in the ulMaxJitter variable, and
 * displayed on the LCD by the 'Check' task as described below.  The
 * fast interrupt is configured and handled in the timertest.c source file.
 *
 * "LCD" task - the LCD task is a 'gatekeeper' task.  It is the only task that
 * is permitted to access the display directly.  Other tasks wishing to write a
 * message to the LCD send the message on a queue to the LCD task instead of
 * accessing the LCD themselves.  The LCD task just blocks on the queue waiting
 * for messages - waking and displaying the messages as they arrive.
 *
 * "Check" task -  This only executes every five seconds but has the highest
 * priority so is guaranteed to get processor time.  Its main function is to
 * check that all the standard demo tasks are still operational.  Should any
 * unexpected behaviour within a demo task be discovered the 'check' task will
 * write an error to the LCD (via the LCD task).  If all the demo tasks are
 * executing with their expected behaviour then the check task writes PASS
 * along with the max jitter time to the LCD (again via the LCD task), as
 * described above.
 *
 */

/* Standard includes. */
#include <stdio.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

/* Library includes. */
#include "stm32f10x_it.h"
#include "stm32f10x_tim.h"

/* Demo app includes. */
#include "lcd.h"
#include "LCD_Message.h"
#include "BlockQ.h"
#include "death.h"
#include "integer.h"
#include "blocktim.h"
#include "partest.h"
#include "semtest.h"
#include "PollQ.h"
#include "flash.h"
#include "comtest2.h"
#include "serial.h"

/* Task priorities. */
#define mainQUEUE_POLL_PRIORITY				( tskIDLE_PRIORITY + 2 )
#define mainCHECK_TASK_PRIORITY				( tskIDLE_PRIORITY + 3 )
#define mainSEM_TEST_PRIORITY				( tskIDLE_PRIORITY + 1 )
#define mainBLOCK_Q_PRIORITY				( tskIDLE_PRIORITY + 2 )
#define mainCREATOR_TASK_PRIORITY           ( tskIDLE_PRIORITY + 3 )
#define mainFLASH_TASK_PRIORITY				( tskIDLE_PRIORITY + 1 )
#define mainCOM_TEST_PRIORITY				( tskIDLE_PRIORITY + 1 )
#define mainINTEGER_TASK_PRIORITY           ( tskIDLE_PRIORITY )

/* Constants related to the LCD. */
#define mainMAX_LINE						( 240 )
#define mainROW_INCREMENT					( 24 )
#define mainMAX_COLUMN						( 20 )
#define mainCOLUMN_START					( 319 )
#define mainCOLUMN_INCREMENT 				( 16 )

/* The maximum number of message that can be waiting for display at any one
time. */
#define mainLCD_QUEUE_SIZE					( 3 )

/* The check task uses the sprintf function so requires a little more stack. */
#define mainCHECK_TASK_STACK_SIZE			( configMINIMAL_STACK_SIZE + 50 )

/* Dimensions the buffer into which the jitter time is written. */
#define mainMAX_MSG_LEN						25

/* The time between cycles of the 'check' task. */
#define mainCHECK_DELAY						( ( TickType_t ) 5000 / portTICK_PERIOD_MS )

/* The number of nano seconds between each processor clock. */
#define mainNS_PER_CLOCK ( ( unsigned long ) ( ( 1.0 / ( double ) configCPU_CLOCK_HZ ) * 1000000000.0 ) )

/* Baud rate used by the comtest tasks. */
#define mainCOM_TEST_BAUD_RATE		( 115200 )

/* The LED used by the comtest tasks. See the comtest.c file for more
information. */
#define mainCOM_TEST_LED			( 3 )

/*-----------------------------------------------------------*/

/*
 * Configure the clocks, GPIO and other peripherals as required by the demo.
 */
static void prvSetupHardware( void );

/*
 * Retargets the C library printf function to the USART.
 */
int fputc( int ch, FILE *f );

/*
 * Configures the timers and interrupts for the fast interrupt test as
 * described at the top of this file.
 */
extern void vSetupTimerTest( void );

/*-----------------------------------------------------------*/

#define SYS_FREQ                (72000000)  // 系统频率

#define TIMx                    TIM3        // 目标定时器
#define TIMx_PERIOD             (100-1)       // 定时器的周期，单位：us。 这里是100us=0.1ms
#define TIMx_PRE_LOAD_VALUE     (1024-1)    // 定时器的计数预装载值，范围：0~65535
#define TIMx_PRESCALER          (SYS_FREQ/1000000*TIMx_PERIOD/TIMx_PRE_LOAD_VALUE)           // 定时器预分频系数，范围：0~65535
#define TIMx_DIV                (TIM_CKD_DIV1)
#define TIMx_Clk_Enable()       RCC_APB1PeriphClockCmd( RCC_APB1Periph_TIM3, ENABLE )   // 目标定时器的时钟使能

#define TIMx_IRQ                TIM3_IRQChannel     // 目标中断
#define TIMx_NVIC_PRE_PRIORITY  (0)             // 中断的抢占优先级等级
#define TIMx_NVIC_SUB_PRIORITY  (1)             // 中断的子优先级等级

#define TIMx_IRQHandler         TIM3_IRQHandler

NVIC_InitTypeDef            gTIMx_NVIC_Init;        // 定义一个中断对象结构体
TIM_TimeBaseInitTypeDef     gTIMX_TimeBase_Init;    // 定义一个定时器对象结构体

static vu8 test_cnt = 0;

static uint32_t g_timer_cnt;

void TimerInit(void)
{    
	/* 1. 使能时钟 */
	TIMx_Clk_Enable();
	
	/* 2. 配置定时器对象结构体的成员，设置定时器属性 */
	gTIMX_TimeBase_Init.TIM_Period          = TIMx_PRE_LOAD_VALUE;      // 设置预装载值，定时器会向上从0计数到value或者向下从value计数到0
	gTIMX_TimeBase_Init.TIM_Prescaler       = TIMx_PRESCALER;           // 设置预分频系数
	gTIMX_TimeBase_Init.TIM_ClockDivision   = TIMx_DIV;                 // 设置分频系数     
	gTIMX_TimeBase_Init.TIM_CounterMode     = TIM_CounterMode_Up;       // 设置计数方式：Up/Down
    
    /* 3. 调用库函数初始化定时器 */
	TIM_TimeBaseInit( TIMx, &gTIMX_TimeBase_Init );

    /* 4. 配置中断的属性 */
    gTIMx_NVIC_Init.NVIC_IRQChannel                     = TIMx_IRQ;                 // 选择中断
	gTIMx_NVIC_Init.NVIC_IRQChannelSubPriority          = TIMx_NVIC_SUB_PRIORITY;   // 设置中断的子优先级
	gTIMx_NVIC_Init.NVIC_IRQChannelPreemptionPriority   = TIMx_NVIC_PRE_PRIORITY;   // 设置中断的抢占优先级
	gTIMx_NVIC_Init.NVIC_IRQChannelCmd                  = ENABLE;                   // 使能中断
    
    /* 5. 初始化中断 */
	NVIC_Init( &gTIMx_NVIC_Init );
        
    /* 6. 选择定时器的中断触发方式 */
	TIM_ITConfig( TIMx, TIM_IT_Update, ENABLE );    // 选择为更新触发中断，即向上计数到预装载值或者向下技术到0触发中断
    
    /* 7. 使能定时器 */
    TIM_Cmd( TIMx, ENABLE );
}

vu8 Test_GetCount(void)
{
    return test_cnt;
}


uint32_t TimerGetCount(void)
{
    return g_timer_cnt;
}


void TIMx_IRQHandler(void)
{
    if( TIM_GetITStatus(TIMx, TIM_IT_Update) == SET )   // 判断是否是更新触发中断
    {
				test_cnt = !test_cnt;
				g_timer_cnt++;
				TIM_ClearITPendingBit( TIMx, TIM_IT_Update );   // 清除中断
    }
}

/*-----------------------------------------------------------*/

static TimerHandle_t xMyTimerHandle;
static int flagTimer = 0;

char pcWriteBuffer[200];

void Task1Function(void * param)
{
	volatile int i = 0;

	//xTimerStart(xMyTimerHandle, 0);
	
	while (1)
	{
		//printf("Task1Function ...\r\n");
		//vTaskList(pcWriteBuffer);
		vTaskGetRunTimeStats(pcWriteBuffer);
		printf(pcWriteBuffer);
		vTaskDelay(5000);
	}
}

void Task2Function(void * param)
{
	volatile int i = 0;
	while (1)
	{
	}
}


void MyTimerCallbackFunction( TimerHandle_t xTimer )
{
	static int cnt = 0;
	flagTimer = !flagTimer;
	printf("Get GPIO Key cnt = %d\r\n", cnt++);
}

/*-----------------------------------------------------------*/
void KeyInit(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;//定义结构体
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);// 使能时钟
	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_0;             // 选择IO口   PA0
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;          // 设置成上拉输入
	GPIO_Init(GPIOA, &GPIO_InitStructure);                  // 使用结构体信息进行初始化IO口}
}

void KeyIntInit(void)
{
	EXTI_InitTypeDef EXTI_InitStructure;//定义初始化结构体
	NVIC_InitTypeDef NVIC_InitStructure;//定义结构体

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE); /* 使能AFIO复用时钟 */

	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource0); /* 将GPIO口与中断线映射起来 */


	EXTI_InitStructure.EXTI_Line=EXTI_Line0; // 中断线
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;            // 中断模式
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling; // 双边沿触发

	EXTI_InitStructure.EXTI_LineCmd = ENABLE;

	EXTI_Init(&EXTI_InitStructure); // 初始化	

	NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQChannel;     //使能外部中断所在的通道	
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;  // 抢占优先级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;         // 子优先级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;            // 使能外部中断通道 	
	NVIC_Init(&NVIC_InitStructure); // 初始化 

}

void EXTI0_IRQHandler(void)
{
	static int cnt = 0;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	
	if(EXTI_GetITStatus(EXTI_Line0) != RESET)
	{
		printf("EXTI0_IRQHandler cnt = %d\r\n", cnt++);
		/* 使用定时器消除抖动 */
		//xTimerReset(xMyTimerHandle, 0); /* Tcur + 2000 */
		
		xTimerResetFromISR(xMyTimerHandle, &xHigherPriorityTaskWoken); /* Tcur + 2000 */
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
		
		EXTI_ClearITPendingBit(EXTI_Line0);     //清除中断
	}     
}
/*-----------------------------------------------------------*/

int main( void )
{
	TaskHandle_t xHandleTask1;
		
#ifdef DEBUG
  debug();
#endif

	prvSetupHardware();

	printf("hello world\r\n");
	
	//TimerInit();
	
	KeyInit();
	KeyIntInit();
	
	xMyTimerHandle = xTimerCreate("mytimer", 2000, pdFALSE, NULL, MyTimerCallbackFunction);

	xTaskCreate(Task1Function, "Task1", 100, NULL, 1, &xHandleTask1);
	xTaskCreate(Task2Function, "Task2", 100, NULL, 1, NULL);
	
	/* Start the scheduler. */
	vTaskStartScheduler();

	/* Will only get here if there was not enough heap space to create the
	idle task. */
	return 0;
}
/*-----------------------------------------------------------*/

static void prvSetupHardware( void )
{
	/* Start with the clocks in their expected state. */
	RCC_DeInit();

	/* Enable HSE (high speed external clock). */
	RCC_HSEConfig( RCC_HSE_ON );

	/* Wait till HSE is ready. */
	while( RCC_GetFlagStatus( RCC_FLAG_HSERDY ) == RESET )
	{
	}

	/* 2 wait states required on the flash. */
	*( ( unsigned long * ) 0x40022000 ) = 0x02;

	/* HCLK = SYSCLK */
	RCC_HCLKConfig( RCC_SYSCLK_Div1 );

	/* PCLK2 = HCLK */
	RCC_PCLK2Config( RCC_HCLK_Div1 );

	/* PCLK1 = HCLK/2 */
	RCC_PCLK1Config( RCC_HCLK_Div2 );

	/* PLLCLK = 8MHz * 9 = 72 MHz. */
	RCC_PLLConfig( RCC_PLLSource_HSE_Div1, RCC_PLLMul_9 );

	/* Enable PLL. */
	RCC_PLLCmd( ENABLE );

	/* Wait till PLL is ready. */
	while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET)
	{
	}

	/* Select PLL as system clock source. */
	RCC_SYSCLKConfig( RCC_SYSCLKSource_PLLCLK );

	/* Wait till PLL is used as system clock source. */
	while( RCC_GetSYSCLKSource() != 0x08 )
	{
	}

	/* Enable GPIOA, GPIOB, GPIOC, GPIOD, GPIOE and AFIO clocks */
	RCC_APB2PeriphClockCmd(	RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |RCC_APB2Periph_GPIOC
							| RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOE | RCC_APB2Periph_AFIO, ENABLE );

	/* SPI2 Periph clock enable */
	RCC_APB1PeriphClockCmd( RCC_APB1Periph_SPI2, ENABLE );


	/* Set the Vector Table base address at 0x08000000 */
	NVIC_SetVectorTable( NVIC_VectTab_FLASH, 0x0 );

	NVIC_PriorityGroupConfig( NVIC_PriorityGroup_4 );

	/* Configure HCLK clock as SysTick clock source. */
	SysTick_CLKSourceConfig( SysTick_CLKSource_HCLK );
	
	SerialPortInit();
}
/*-----------------------------------------------------------*/

#ifdef  DEBUG
/* Keep the linker happy. */
void assert_failed( unsigned char* pcFile, unsigned long ulLine )
{
	for( ;; )
	{
	}
}
#endif
