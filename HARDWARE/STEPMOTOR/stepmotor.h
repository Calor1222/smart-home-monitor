#ifndef __STEPPER_H
#define __STEPPER_H

#include "stm32f4xx.h"

// 引脚定义（可根据实际接线修改）
#define IN1_PORT      GPIOD
#define IN1_PIN       GPIO_Pin_4
#define IN2_PORT      GPIOD
#define IN2_PIN       GPIO_Pin_5
#define IN3_PORT      GPIOD
#define IN3_PIN       GPIO_Pin_6
#define IN4_PORT      GPIOD
#define IN4_PIN       GPIO_Pin_7

// 函数声明
void Stepper_Init(void);                           // 初始化GPIO
void Stepper_Output(uint8_t step_index);           // 输出一拍（0~7）
void Stepper_Rotate_Clockwise(uint32_t steps, uint16_t step_delay_ms);  // 顺时针转动指定步数

#endif