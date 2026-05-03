#ifndef __FAN_H
#define __FAN_H

#include "sys.h"

/*
 * 风扇控制引脚定义
 * PWM输出接PB6
 * 使用TIM4通道1输出PWM
 */
#define FAN_PWM_PIN      GPIO_Pin_6
#define FAN_PWM_PORT     GPIOB
#define FAN_PWM_AF       GPIO_AF_TIM4
#define FAN_TIM          TIM4
#define FAN_TIM_CHANNEL  TIM_Channel_1

/*
 * 对外接口函数
 */
void Fan_SetSpeed(u8 percent);  // 设置风扇转速
void Fan_On(void);              // 开启风扇
void Fan_Off(void);             // 关闭风扇
void Fan_Init(void);            // 初始化风扇

#endif
