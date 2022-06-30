#include "main.h"

/* »ã±à·ÖÎö1 */
//int a = 123;
//int main(void)
//{
//  volatile int b;
//	b = a;
//	return 0;
//}


/* »ã±à·ÖÎö2 */
//int add_val(int *pa, int *pb)
//{
//	volatile int tmp;
//	tmp = *pa;
//	tmp += *pb;
//	return tmp;
//}

//int main()
//{
//	volatile int a = 1;
//	volatile int b = 2;
//	volatile int c;
//	
//	c = add_val(&a, &b);
//	
//	return 0;
//}

///* »ã±à·ÖÎö3 */
int add_val(volatile int a, volatile int b, volatile int c, volatile int d)
{
	return a+b+c+d;
}

int main()
{
	volatile int a = 1;
	volatile int b = 2;
	volatile int c = 3;
	volatile int d = 4;
	volatile int sum;
	
	sum = add_val(a, b, c, d);
	
	return 0;
}

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
