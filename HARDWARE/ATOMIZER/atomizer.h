#ifndef __ATOMIZER_H
#define __ATOMIZER_H

#include "stm32f4xx.h"

/*
 * 雾化模块控制引脚定义
 * S 引脚接 PB0
 * 使用宏定义方便后期修改引脚
 */
#define ATOMIZER_GPIO_PORT        GPIOB
#define ATOMIZER_GPIO_PIN         GPIO_Pin_0
#define ATOMIZER_GPIO_CLK         RCC_AHB1Periph_GPIOB

/*
 * 对外接口函数
 */
void Atomizer_Init(void);         // 初始化GPIO
void Atomizer_On(void);           // 开启雾化
void Atomizer_Off(void);          // 关闭雾化
void Atomizer_Set(uint8_t state); // 设置状态
uint8_t Atomizer_GetState(void);  // 获取状态
void Atomizer_Toggle(void);       // 翻转状态

#endif