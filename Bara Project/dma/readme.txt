博客：https://blog.csdn.net/weixin_60824769/article/details/121629863
1、DMA
DMA 串口收发数据（接收数据定长、单缓存区）
2、DMA_Double
DMA 串口收发数据（接收数据定长、双缓存区）
3、DMA_IDLE_NL
DMA 串口收发数据（接收数据不定长、单缓存区）
4、DMA_IDLE_NL_Double
DMA 串口收发数据（接收数据不定长、双缓存区）
5、DMA_MEM
DMA 存储器到存储器数据传输(MEM_TO_MEM)
6、DMA_IDLE_NL_Double_TC（缓冲区太小会数据丢失的问题）
DMA 串口收发数据（接收数据不定长、双缓存区），其中使用了串口TC中断。

博客：https://blog.csdn.net/weixin_60824769/article/details/121829615
1、UART_KFIFO
UART串口收发数据采用KFIFO
2、UART_LWRB
UART串口收发数据采用LWRB
