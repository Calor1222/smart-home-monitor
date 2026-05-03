#ifndef __USART3_H
#define __USART3_H

#include "sys.h"
#include <stdio.h>

/*
 * 串口3引脚定义
 * TX接PB10
 * RX接PB11
 */
#define EN_USART3_RX 1          // 使能串口3接收
#define USART3_REC_LEN 200      // 串口3接收长度

/*
 * 外部变量
 * 用于保存串口3接收状态
 */
extern u8  USART3_RX_BUF[USART3_REC_LEN]; // 接收缓冲区
extern u16 USART3_RX_STA;                 // 接收状态

/*
 * 对外接口函数
 */
void uart3_init(u32 bound);               // 初始化串口3

#endif
