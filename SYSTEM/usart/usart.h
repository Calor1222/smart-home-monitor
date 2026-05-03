#ifndef __USART_H
#define __USART_H

#include "stdio.h"
#include "stm32f4xx_conf.h"
#include "sys.h"

/*
 * 串口1参数定义
 * 用于printf调试输出
 * 同时支持串口接收
 */
#define USART_REC_LEN  200 // 接收缓冲长度
#define EN_USART1_RX   1   // 使能串口1接收

/*
 * 外部变量
 * 用于保存串口1接收状态
 */
extern u8  USART_RX_BUF[USART_REC_LEN]; // 接收缓冲区
extern u16 USART_RX_STA;                // 接收状态

/*
 * 对外接口函数
 */
void uart_init(u32 bound);              // 初始化串口1

#endif
