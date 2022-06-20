#include "kfifo.h"
#include <string.h>
#include <stdlib.h>
#include "stdio.h"

/** linux-2.6.24.y **/

#ifndef __USE__CLZ
	// https://stackoverflow.com/questions/4398711/round-to-the-nearest-power-of-two
	static inline unsigned int roundup_pow_of_two(unsigned int v) {
			v--;
			v |= v >> 1;
			v |= v >> 2;
			v |= v >> 4;
			v |= v >> 8;
			v |= v >> 16;
			v++;
			return v;
	}
	
	// https://blog.csdn.net/dreamispossible/article/details/91162847
	static inline unsigned int rounddown_pow_of_two(unsigned int n) {
		n|=n>>1; n|=n>>2; n|=n>>4; n|=n>>8; n|=n>>16;
		return (n+1) >> 1;
	}
#else
	// http://blog.csdn.net/zhzht19861011/article/details/51418383
	static inline unsigned int roundup_pow_of_two(unsigned int date_roundup_pow_of_two )
	{            
			/* 这里采用 STM32 硬件提供的计算前导零指令 CLZ
			 * 举个例子，假如变量date_roundup_pow_of_two 0x09
			 *（二进制为：0000 0000 0000 0000 0000 0000 0000 1001）, 即bit3和bit0为1
			 * 则__clz( (date_roundup_pow_of_two)的值为28,即最高位1 前面有28个0,32-28 =3 代表最高位1 的 位置
			 * 31UL 表示 无符号 int 数字 31，否则默认为 有符号 int 数字 31
			 */

			return ( 1UL << ( 32UL - ( unsigned int ) __clz( (date_roundup_pow_of_two) ) ) );
	}
#endif

/**
 * kfifo_init - allocates a new FIFO using a preallocated buffer
 * @buffer: the preallocated buffer to be used.
 * @size: the size of the internal buffer, this have to be a power of 2.
 *
 * Do NOT pass the kfifo to kfifo_free() after use! Simply free the
 * &struct kfifo with free().
 */
struct kfifo *kfifo_init(unsigned char *buffer, unsigned int size)
{
	struct kfifo *fifo;

	/* size must be a power of 2 */
	BUG_ON(is_power_of_2(size));

	fifo=(struct kfifo *) malloc(sizeof (struct kfifo));
	if(!fifo)
		return NULL;
	
	fifo->buffer = buffer;
	fifo->size = size;
	fifo->in = fifo->out = 0;
	return fifo;
}

/**
 * kfifo_alloc - allocates a new FIFO and its internal buffer
 * @size: the size of the internal buffer to be allocated.
 *
 * The size will be rounded-up to a power of 2.
 */
struct kfifo *kfifo_alloc(unsigned int size)
{
	unsigned char *buffer;
	struct kfifo *ret;

	/*
	 * round up to the next power of 2, since our 'let the indices
	 * wrap' tachnique works only in this case.
	 */
	if (!is_power_of_2(size)) {
		BUG_ON(!(size > 0x80000000));
		size = roundup_pow_of_two(size);
	}

	buffer = (unsigned char*) malloc(size);
	if (!buffer)
		return NULL; 
	
	ret=(struct kfifo *) malloc(sizeof (struct kfifo));

	ret = kfifo_init(buffer, size);
	if (!ret)
		free(buffer);

	return ret;
}

/**
 * kfifo_free - frees the FIFO
 * @fifo: the fifo to be freed.
 */
void kfifo_free(struct kfifo *fifo)
{
	free(fifo->buffer);
	free(fifo);
}

/**
 * __kfifo_put - puts some data into the FIFO, no locking version
 * @fifo: the fifo to be used.
 * @buffer: the data to be added.
 * @len: the length of the data to be added.
 *
 * This function copies at most @len bytes from the @buffer into
 * the FIFO depending on the free space, and returns the number of
 * bytes copied.
 *
 * Note that with only one concurrent reader and one concurrent
 * writer, you don't need extra locking to use these functions.
 */
unsigned int __kfifo_put(struct kfifo *fifo,
			 unsigned char *buffer, unsigned int len)
{
	unsigned int l;

	len = min(len, fifo->size - fifo->in + fifo->out);

	/*
	 * Ensure that we sample the fifo->out index -before- we
	 * start putting bytes into the kfifo.
	 */

//	smp_mb();

	/* first put the data starting from fifo->in to buffer end */
	l = min(len, fifo->size - (fifo->in & (fifo->size - 1)));
	memcpy(fifo->buffer + (fifo->in & (fifo->size - 1)), buffer, l);

	/* then put the rest (if any) at the beginning of the buffer */
	memcpy(fifo->buffer, buffer + l, len - l);

	/*
	 * Ensure that we add the bytes to the kfifo -before-
	 * we update the fifo->in index.
	 */

//	smp_wmb();

	fifo->in += len;

	return len;
}

/**
 * __kfifo_get - gets some data from the FIFO, no locking version
 * @fifo: the fifo to be used.
 * @buffer: where the data must be copied.
 * @len: the size of the destination buffer.
 *
 * This function copies at most @len bytes from the FIFO into the
 * @buffer and returns the number of copied bytes.
 *
 * Note that with only one concurrent reader and one concurrent
 * writer, you don't need extra locking to use these functions.
 */
unsigned int __kfifo_get(struct kfifo *fifo,
			 unsigned char *buffer, unsigned int len)
{
	unsigned int l;

	len = min(len, fifo->in - fifo->out);

	/*
	 * Ensure that we sample the fifo->in index -before- we
	 * start removing bytes from the kfifo.
	 */

//	smp_rmb();

	/* first get the data from fifo->out until the end of the buffer */
	l = min(len, fifo->size - (fifo->out & (fifo->size - 1)));
	memcpy(buffer, fifo->buffer + (fifo->out & (fifo->size - 1)), l);

	/* then get the rest (if any) from the beginning of the buffer */
	memcpy(buffer + l, fifo->buffer, len - l);

	/*
	 * Ensure that we remove the bytes from the kfifo -before-
	 * we update the fifo->out index.
	 */

//	smp_mb();

	fifo->out += len;

	return len;
}




