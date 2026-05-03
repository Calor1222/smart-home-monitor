#ifndef __GP2Y_H
#define __GP2Y_H

#include "stm32f4xx.h"

/*
 * GP2Y1010AU引脚定义
 * ILED接PB1
 * AO接PA5
 */

#define GP2Y_ILED_PORT        GPIOB
#define GP2Y_ILED_PIN         GPIO_Pin_1
#define GP2Y_ILED_RCC         RCC_AHB1Periph_GPIOB

#define GP2Y_ADC_PORT         GPIOA
#define GP2Y_ADC_PIN          GPIO_Pin_5
#define GP2Y_ADC_RCC          RCC_AHB1Periph_GPIOA

#define GP2Y_ADC              ADC1
#define GP2Y_ADC_CLK          RCC_APB2Periph_ADC1
#define GP2Y_ADC_CHANNEL      ADC_Channel_5

/*
 * 对外接口函数
 */
void GP2Y_Init(void);
uint16_t GP2Y_ReadRaw(void);
float GP2Y_ReadVoltage(void);
float GP2Y_ReadDust_ugm3(void);
float GP2Y_ReadDustAverage_ugm3(uint8_t times);

#endif
