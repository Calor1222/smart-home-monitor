#ifndef __STEPPER_H
#define __STEPPER_H

#include "stm32f4xx.h"

/*
 * 步进电机引脚定义
 * 四相控制脚接GPIOD
 * 可根据实际接线调整
 */
#define IN1_PORT      GPIOD
#define IN1_PIN       GPIO_Pin_4
#define IN2_PORT      GPIOD
#define IN2_PIN       GPIO_Pin_5
#define IN3_PORT      GPIOD
#define IN3_PIN       GPIO_Pin_6
#define IN4_PORT      GPIOD
#define IN4_PIN       GPIO_Pin_7

/*
 * 对外接口函数
 */
void Stepper_Init(void);                                        // 初始化步进电机
void Stepper_Output(uint8_t step_index);                        // 输出一步
void Stepper_Rotate_Clockwise(uint32_t steps, uint16_t step_delay_ms); // 顺时针转动

#endif
