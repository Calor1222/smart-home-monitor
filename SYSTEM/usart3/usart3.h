#ifndef __USART3_H
#define __USART3_H

#include "sys.h"
#include <stdio.h>

//////////////////////////////////////////////////////////////////////////////////
// 串口3驱动代码 - 基于STM32F407
// TX: PB10, RX: PB11
//////////////////////////////////////////////////////////////////////////////////

#define EN_USART3_RX 1          // 使能串口3接收
#define USART3_REC_LEN 200      // 定义最大接收字节数

extern u8  USART3_RX_BUF[USART3_REC_LEN]; // 接收缓冲
extern u16 USART3_RX_STA;                 // 接收状态标记

void uart3_init(u32 bound);               // 串口3初始化函数

#endif