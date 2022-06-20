#include "delay.h"

static uint32_t fac_us=0;							//us��ʱ������			   
static uint16_t fac_ms=0;							//ms��ʱ������

/* ��ʱ��ʼ��������
	 ���ȣ���ʱ������ʼ��ʱ����Ҫ����SYSTICK��ʱ��Ƶ�ʺ�AHBƵ����ȣ�����HCLK��ȣ�
	 ��SYSTICKʱ��Ƶ�����ã��Ѿ���stm32cubmx������ˣ���ʱ�������ÿ��Կ��������Դ˴����θù��ܴ��롣
	 ���⣬FreeRTOS vTaskStartScheduler�л����vPortSetupTimerInterrupt��������������л�ʹ��SYSTICKʱ�ӡ��жϣ�
	 �ж���Ӧʱ����������õĹ�������ͬ�� ���������������ȿ������ʹ����ʱ�������������������룻
	 Ϊ�˼���������������ﲻ�����Ρ�
	 
	vPortSetupTimerInterrupt����������SYSTICKʱ�ӡ��жϵĴ������£�
	portNVIC_SYSTICK_LOAD_REG = ( configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ ) - 1UL;
	portNVIC_SYSTICK_CTRL_REG = ( portNVIC_SYSTICK_CLK_BIT | portNVIC_SYSTICK_INT_BIT | portNVIC_SYSTICK_ENABLE_BIT );
*/
void delay_init()
{	
#if 0	
	HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);//SysTickƵ��ΪHCLK
#endif
	
	fac_us=SystemCoreClock/1000000;
	fac_ms=1000/configTICK_RATE_HZ;				//����OS������ʱ����С��λ	   

#if 1	//����������ע��
	uint32_t reload;
	reload=SystemCoreClock/1000000;				//ÿ���ӵļ������� ��λΪM  
	reload*=1000000/configTICK_RATE_HZ;			//����configTICK_RATE_HZ�趨���ʱ��																			
																					//reloadΪ24λ�Ĵ���,���ֵ:16777216,��72M��,Լ��0.233s����
	SysTick->CTRL|=SysTick_CTRL_TICKINT_Msk;   	//����SYSTICK�ж�
	SysTick->LOAD=reload; 											//ÿ1/configTICK_RATE_HZ���ж�һ��	
	SysTick->CTRL|=SysTick_CTRL_ENABLE_Msk;   	//����SYSTICK    
#endif
}								    

/* us����ʱ��������������������ȣ���
	 nus:Ҫ��ʱ��us����ȡֵ��Χ0~204522252(���ֵ��2^32/fac_us@fac_us=168)	 
*/
void delay_us(uint32_t nus)
{		
	uint32_t ticks;
	uint32_t told,tnow,tcnt=0;
	uint32_t reload=SysTick->LOAD;				//LOAD��ֵ	    	 
	ticks=nus*fac_us; 						//��Ҫ�Ľ����� 
	told=SysTick->VAL;        				//�ս���ʱ�ļ�����ֵ
	while(1)
	{
		tnow=SysTick->VAL;	
		if(tnow!=told)
		{	    
			if(tnow<told)tcnt+=told-tnow;	//����ע��һ��SYSTICK��һ���ݼ��ļ������Ϳ�����.
			else tcnt+=reload-tnow+told;	    
			told=tnow;
			if(tcnt>=ticks)break;			//ʱ�䳬��/����Ҫ�ӳٵ�ʱ��,���˳�.
		}  
	};										    
} 

/* ms����ʱ������
	 nms:Ҫ��ʱ��ms����ȡֵ��Χ0~65535	 
*/
void delay_ms(uint32_t nms)
{	
	if(xTaskGetSchedulerState()!=taskSCHEDULER_NOT_STARTED)//ϵͳ�Ѿ�����
	{		
		if(nms>=fac_ms)						//��ʱ��ʱ�����OS������ʱ������ 
		{ 
   			vTaskDelay(nms/fac_ms);	 		//FreeRTOS��ʱ
		}
		nms%=fac_ms;						// RTOS�޷��ṩ��ôС����ʱ��,������ͨ��ʽ��ʱ    
	}
	delay_us((uint32_t)(nms*1000));				//��ͨ��ʽ��ʱ
}

/* ms����ʱ��������������������ȣ���
	 nms:Ҫ��ʱ��ms����ȡֵ��Χ0~65535	 
*/
void delay_xms(uint32_t nms)
{
#if 1 //�Զ���
	uint32_t i;
	for(i=0;i<nms;i++) 
		delay_us(1000);
#else //HAL���Դ�
	HAL_Delay(nms);
#endif
	
}



