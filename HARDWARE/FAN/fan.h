#ifndef __FAN_H
#define __FAN_H

#include "sys.h"

// 风扇控制引脚定义
#define FAN_PWM_PIN      GPIO_Pin_6
#define FAN_PWM_PORT     GPIOB
#define FAN_PWM_AF       GPIO_AF_TIM4
#define FAN_TIM          TIM4
#define FAN_TIM_CHANNEL  TIM_Channel_1

// 风扇转速等级（0-100）
void Fan_SetSpeed(u8 percent);
// 风扇启动（默认50%转速）
void Fan_On(void);
// 风扇停止
void Fan_Off(void);
// 风扇初始化
void Fan_Init(void);

#endif