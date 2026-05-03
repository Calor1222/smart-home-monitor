#ifndef __LED_H
#define __LED_H

#include "sys.h"

/*
 * LED引脚定义
 * DS0接PF9
 * DS1接PF10
 */
#define LED0 PFout(9)   // DS0控制脚
#define LED1 PFout(10)  // DS1控制脚

/*
 * 对外接口函数
 */
void LED_Init(void);    // 初始化LED

#endif
