#include "delay.h"

static uint32_t fac_us=0;							//us延时倍乘数			   
static uint16_t fac_ms=0;							//ms延时倍乘数

/* 延时初始化函数。
	 首先，延时函数初始化时，需要设置SYSTICK的时钟频率和AHB频率相等，即和HCLK相等，
	 而SYSTICK时钟频率配置，已经在stm32cubmx中完成了，从时钟树配置可以看出，所以此处屏蔽该功能代码。
	 此外，FreeRTOS vTaskStartScheduler中会调用vPortSetupTimerInterrupt函数，这个函数中会使能SYSTICK时钟、中断，
	 中断响应时间和这里设置的功能是相同， 所以如果在任务调度开启后才使用延时函数则可以屏蔽下面代码；
	 为了兼容所有情况，这里不作屏蔽。
	 
	vPortSetupTimerInterrupt函数中设置SYSTICK时钟、中断的代码如下：
	portNVIC_SYSTICK_LOAD_REG = ( configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ ) - 1UL;
	portNVIC_SYSTICK_CTRL_REG = ( portNVIC_SYSTICK_CLK_BIT | portNVIC_SYSTICK_INT_BIT | portNVIC_SYSTICK_ENABLE_BIT );
*/
void delay_init()
{	
#if 0	
	HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);//SysTick频率为HCLK
#endif
	
	fac_us=SystemCoreClock/1000000;
	fac_ms=1000/configTICK_RATE_HZ;				//代表OS可以延时的最小单位	   

#if 1	//描述见上面注释
	uint32_t reload;
	reload=SystemCoreClock/1000000;				//每秒钟的计数次数 单位为M  
	reload*=1000000/configTICK_RATE_HZ;			//根据configTICK_RATE_HZ设定溢出时间																			
																					//reload为24位寄存器,最大值:16777216,在72M下,约合0.233s左右
	SysTick->CTRL|=SysTick_CTRL_TICKINT_Msk;   	//开启SYSTICK中断
	SysTick->LOAD=reload; 											//每1/configTICK_RATE_HZ秒中断一次	
	SysTick->CTRL|=SysTick_CTRL_ENABLE_Msk;   	//开启SYSTICK    
#endif
}								    

/* us级延时函数（不会引起任务调度）。
	 nus:要延时的us数，取值范围0~204522252(最大值即2^32/fac_us@fac_us=168)	 
*/
void delay_us(uint32_t nus)
{		
	uint32_t ticks;
	uint32_t told,tnow,tcnt=0;
	uint32_t reload=SysTick->LOAD;				//LOAD的值	    	 
	ticks=nus*fac_us; 						//需要的节拍数 
	told=SysTick->VAL;        				//刚进入时的计数器值
	while(1)
	{
		tnow=SysTick->VAL;	
		if(tnow!=told)
		{	    
			if(tnow<told)tcnt+=told-tnow;	//这里注意一下SYSTICK是一个递减的计数器就可以了.
			else tcnt+=reload-tnow+told;	    
			told=tnow;
			if(tcnt>=ticks)break;			//时间超过/等于要延迟的时间,则退出.
		}  
	};										    
} 

/* ms级延时函数。
	 nms:要延时的ms数，取值范围0~65535	 
*/
void delay_ms(uint32_t nms)
{	
	if(xTaskGetSchedulerState()!=taskSCHEDULER_NOT_STARTED)//系统已经运行
	{		
		if(nms>=fac_ms)						//延时的时间大于OS的最少时间周期 
		{ 
   			vTaskDelay(nms/fac_ms);	 		//FreeRTOS延时
		}
		nms%=fac_ms;						// RTOS无法提供这么小的延时了,采用普通方式延时    
	}
	delay_us((uint32_t)(nms*1000));				//普通方式延时
}

/* ms级延时函数（不会引起任务调度）。
	 nms:要延时的ms数，取值范围0~65535	 
*/
void delay_xms(uint32_t nms)
{
#if 1 //自定义
	uint32_t i;
	for(i=0;i<nms;i++) 
		delay_us(1000);
#else //HAL库自带
	HAL_Delay(nms);
#endif
	
}



