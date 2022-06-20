#include "task.h"

const osThreadAttr_t tid_Thread1_attr = {
  .name = "Thread1",
  .priority = (osPriority_t) osPriorityLow3,
  .stack_size = 256 * 4
};


const osThreadAttr_t tid_Thread2_attr = {
  .name = "Thread2",
  .priority = (osPriority_t) osPriorityLow4,
  .stack_size = 512 * 4
};

osThreadId_t Thread1_id= NULL;
osThreadId_t Thread2_id= NULL;

void Thread1(void *argument)
{
	while(1)
	{
		LED2_Toggle();
		osDelay(500);
	}
}

void Thread2(void *argument)
{
	while(1)
	{
		LED3_Toggle();
		osDelay(1000);
	}
}

void Thread_Init(void)
{
	Thread1_id = osThreadNew(Thread1, NULL, &tid_Thread1_attr);

  Thread2_id = osThreadNew(Thread2, NULL, &tid_Thread2_attr);
}