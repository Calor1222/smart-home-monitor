#ifndef __GP2Y_H
#define __GP2Y_H

#include "stm32f4xx.h"

/* =========================================================
 * GP2Y1010AU 粉尘传感器接口定义
 *
 * ILED：控制红外LED（低电平点亮）
 * AO  ：模拟电压输出（接ADC）
 * =========================================================
 */

// ILED 控制脚（连接到 STM32 PB1）
#define GP2Y_ILED_PORT        GPIOB
#define GP2Y_ILED_PIN         GPIO_Pin_1
#define GP2Y_ILED_RCC         RCC_AHB1Periph_GPIOB

// AO 模拟输入（改为 PA5，因为板子推荐 ADC 口）
#define GP2Y_ADC_PORT         GPIOA
#define GP2Y_ADC_PIN          GPIO_Pin_5
#define GP2Y_ADC_RCC          RCC_AHB1Periph_GPIOA

// 使用 ADC1
#define GP2Y_ADC              ADC1
#define GP2Y_ADC_CLK          RCC_APB2Periph_ADC1

// ?? PA5 对应 ADC 通道 5
#define GP2Y_ADC_CHANNEL      ADC_Channel_5

/* ================= 函数声明 ================= */

// 初始化传感器（GPIO + ADC）
void GP2Y_Init(void);

// 读取一次原始 ADC 值（带时序）
uint16_t GP2Y_ReadRaw(void);

// 转换为电压值（单位：V）
float GP2Y_ReadVoltage(void);

// 转换为粉尘浓度（单位：ug/m3）
float GP2Y_ReadDust_ugm3(void);

// 多次平均（滤波）
float GP2Y_ReadDustAverage_ugm3(uint8_t times);

#endif