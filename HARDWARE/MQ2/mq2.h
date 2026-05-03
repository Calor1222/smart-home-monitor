#ifndef __MQ2_H
#define __MQ2_H

#include "sys.h"
#include "delay.h"

/*
 * MQ2引脚定义
 * DO接PA0
 * AO接PC1
 */
#define MQ2_DO_GPIO_PORT        GPIOA
#define MQ2_DO_PIN              GPIO_Pin_0

#define MQ2_AO_ADC              ADC1
#define MQ2_AO_ADC_CHANNEL      ADC_Channel_11
#define MQ2_AO_GPIO_PORT        GPIOC
#define MQ2_AO_PIN              GPIO_Pin_1

/*
 * MQ2参数定义
 * 用于校准和浓度计算
 */
#define MQ2_CLEAN_AIR_RATIO     6.5f
#define ADC_REF_VOLTAGE         3.3f
#define ADC_MAX_VALUE           4095.0f
#define MQ2_RL                  5.0f
#define MQ2_VCC                 5.0f
#define MQ2_ALARM_PPM_THRESHOLD 100.0f

/*
 * 对外接口函数
 */
void MQ2_Init(void);             // 初始化MQ2
void MQ2_Calibrate(void);        // 校准MQ2
u8   MQ2_GetDigitalOutput(void); // 读取DO电平
float MQ2_GetVoltage(void);      // 读取AO电压
float MQ2_GetRatio(void);        // 读取Rs/Ro
float MQ2_GetPPM(void);          // 读取浓度
u8   MQ2_GetAlarmStatus(void);   // 读取报警状态

#endif
