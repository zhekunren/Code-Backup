#ifndef _LINUX_KFIFO_H
#define _LINUX_KFIFO_H

/* Code is added by eric,reference from FreeRTOS kernel source code.This code need to change. CODE BEGIN*/
#include "stm32f1xx_hal.h"
#include "gpio.h"

#define __USE__CLZ              1  //�Ƿ���� STM32 Ӳ���ṩ�ļ���ǰ����ָ�� CLZ

#define configPRIO_BITS         2 // �˺��������� MCU ʹ�ü�λ���ȼ�����ռ���ȼ���
																	// ����FreeRTOS�����ȼ�ѡ��4��4 bits for pre-emption priority��0 bits for subpriority����ʱ����Ϊ4
																	// �ڱ������У���Ϊ����Ϊ2 bits for pre-emption priority��2 bits for subpriority��������Ϊ2
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 1   // �˺���������ϵͳ�ɹ����������ȼ���Ҳ���Ǹ��ڴ����ȼ��Ĳ��ܱ��������Σ���
																												 // �����ֲ��֪�����۰�BASEPRI����Ϊ���٣����޷����������ȼ�Ϊ0���жϡ�
																												 // �����Լ��������ã��˴�����Ϊ1
/* Code is added by eric,reference from FreeRTOS kernel source code.This code need to change. CODE END*/

/* Code is added by eric,reference from FreeRTOS kernel source code.This code don't need to change. CODE BEGIN*/
static inline unsigned int min(unsigned int x,unsigned int y)
{
	unsigned int __min1=x,__min2 = y;
	return __min1 < __min2 ? __min1: __min2;
}
#define is_power_of_2(n)         (n != 0 && ((n & (n - 1)) == 0)) //Ϊ2�Ĵ���,ֵΪ1

#define STR(x)  VAL(x)  
#define VAL(x)  #x 
#define kBUG_ON(char)  printf("BUG_ON Error:%s\r\n",char)
#define BUG_ON(x) ((x)? 0 :kBUG_ON(__FILE__ ":" STR(__LINE__) " " #x"\n" ))
/* Code is added by eric,reference from FreeRTOS kernel source code.This code don't need to change. CODE END*/
struct kfifo {
	unsigned char *buffer;	/* the buffer holding the data */
	unsigned int size;	/* the size of the allocated buffer */
	unsigned int in;	/* data is added at offset (in % size) */
	unsigned int out;	/* data is extracted from off. (out % size) */
//	spinlock_t *lock;	/* protects concurrent modifications */ //lock��Ҫ��֤�κ�ʱ�̶�д������ʣ�stm32ֻ��һ�����ģ����Բ���Ҫ
};


extern struct kfifo *kfifo_init(unsigned char *buffer, unsigned int size);
extern struct kfifo *kfifo_alloc(unsigned int size);
extern void kfifo_free(struct kfifo *fifo);
extern unsigned int __kfifo_put(struct kfifo *fifo,
				unsigned char *buffer, unsigned int len);
extern unsigned int __kfifo_get(struct kfifo *fifo,
				unsigned char *buffer, unsigned int len);

/**
 * __kfifo_reset - removes the entire FIFO contents, no locking version
 * @fifo: the fifo to be emptied.
 */
static inline void __kfifo_reset(struct kfifo *fifo)
{
	fifo->in = fifo->out = 0;
}

/**
 * __kfifo_len - returns the number of bytes available in the FIFO, no locking version
 * @fifo: the fifo to be used.
 */
static inline unsigned int __kfifo_len(struct kfifo *fifo)
{
	return fifo->in - fifo->out;
}

/* Code is added by eric,reference from FreeRTOS kernel source code.This code don't need to change. CODE BEGIN*/

#define portFORCE_INLINE    __forceinline

/* !!!! configMAX_SYSCALL_INTERRUPT_PRIORITY must not be set to zero !!!!*/
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 	( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )

#define portDISABLE_INTERRUPTS()				vPortRaiseBASEPRI()
#define portENABLE_INTERRUPTS()					vPortSetBASEPRI( 0 )

static portFORCE_INLINE void vPortSetBASEPRI( uint32_t ulBASEPRI )
{
	__asm
	{
		/* Barrier instructions are not used as this function is only used to
		lower the BASEPRI value. */
		msr basepri, ulBASEPRI
	}
}

static portFORCE_INLINE void vPortRaiseBASEPRI( void )
{
uint32_t ulNewBASEPRI = configMAX_SYSCALL_INTERRUPT_PRIORITY;

	__asm
	{
		/* Set BASEPRI to the max syscall priority to effect a critical
		section. */
		msr basepri, ulNewBASEPRI
		dsb
		isb
	}
}

#define lock_irqsave()           portDISABLE_INTERRUPTS()
#define unlock_irqrestore()      portENABLE_INTERRUPTS()

/* Code is added by eric,reference from FreeRTOS kernel source code.This code don't need to change. CODE END*/


/**
 * kfifo_reset - removes the entire FIFO contents
 * @fifo: the fifo to be emptied.
 */
static inline void kfifo_reset(struct kfifo *fifo)
{
	lock_irqsave();

	__kfifo_reset(fifo);

	unlock_irqrestore();
}

/**
 * kfifo_put - puts some data into the FIFO
 * @fifo: the fifo to be used.
 * @buffer: the data to be added.
 * @len: the length of the data to be added.
 *
 * This function copies at most @len bytes from the @buffer into
 * the FIFO depending on the free space, and returns the number of
 * bytes copied.
 */
static inline unsigned int kfifo_put(struct kfifo *fifo,
				     unsigned char *buffer, unsigned int len)
{
	unsigned int ret;

	lock_irqsave();

	ret = __kfifo_put(fifo, buffer, len);

	unlock_irqrestore();

	return ret;
}

/**
 * kfifo_get - gets some data from the FIFO
 * @fifo: the fifo to be used.
 * @buffer: where the data must be copied.
 * @len: the size of the destination buffer.
 *
 * This function copies at most @len bytes from the FIFO into the
 * @buffer and returns the number of copied bytes.
 */
static inline unsigned int kfifo_get(struct kfifo *fifo,
				     unsigned char *buffer, unsigned int len)
{
	unsigned int ret;

	lock_irqsave();

	ret = __kfifo_get(fifo, buffer, len);

	/*
	 * optimization: if the FIFO is empty, set the indices to 0
	 * so we don't wrap the next time
	 */
	if (fifo->in == fifo->out)
		fifo->in = fifo->out = 0;

	unlock_irqrestore();

	return ret;
}

/**
 * kfifo_len - returns the number of bytes available in the FIFO
 * @fifo: the fifo to be used.
 */
static inline unsigned int kfifo_len(struct kfifo *fifo)
{
	unsigned int ret;

	lock_irqsave();

	ret = __kfifo_len(fifo);

	unlock_irqrestore();

	return ret;
}


#endif

