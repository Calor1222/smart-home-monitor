#ifndef __DELAY_H
#define __DELAY_H

#include <sys.h>

/*
 * 延时函数声明
 * 使用SysTick实现us和ms延时
 * 可兼容OS环境
 */
void delay_init(u8 SYSCLK); // 初始化延时
void delay_ms(u16 nms);     // 毫秒延时
void delay_us(u32 nus);     // 微秒延时

#endif
