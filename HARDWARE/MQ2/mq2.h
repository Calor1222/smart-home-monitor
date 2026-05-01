/**
  ******************************************************************************
  * @file    mq2.h
  * @author  MQ-2 Driver
  * @version V1.0
  * @date    2026-04-16
  * @brief   MQ-2 烟雾传感器驱动头文件
  ******************************************************************************
  */

#ifndef __MQ2_H
#define __MQ2_H

#include "sys.h"
#include "delay.h"

/* 引脚定义 - 可根据实际接线修改 */
#define MQ2_DO_GPIO_PORT        GPIOA
#define MQ2_DO_PIN              GPIO_Pin_0

#define MQ2_AO_ADC              ADC1
#define MQ2_AO_ADC_CHANNEL      ADC_Channel_11   /* PC1 对应 ADC123_IN11 */
#define MQ2_AO_GPIO_PORT        GPIOC
#define MQ2_AO_PIN              GPIO_Pin_1

/* 传感器校准参数 - 洁净空气中 Rs/Ro 典型值约 6.5 */
#define MQ2_CLEAN_AIR_RATIO     6.5f

/* ADC 配置 */
#define ADC_REF_VOLTAGE         3.3f    /* STM32F407 参考电压 3.3V */
#define ADC_MAX_VALUE           4095.0f /* 12位 ADC 最大值 */

/* 模块硬件参数 (可根据实际模块调整) */
#define MQ2_RL                  5.0f    /* 模块负载电阻 (kΩ) */
#define MQ2_VCC                 5.0f    /* 模块供电电压 (V) */

/* 报警阈值 - PPM > 100 时报警 */
#define MQ2_ALARM_PPM_THRESHOLD 100.0f

/* 函数声明 */
void MQ2_Init(void);                    /* 初始化 GPIO 和 ADC */
void MQ2_Calibrate(void);               /* 校准传感器，获取 Ro 值 */
u8   MQ2_GetDigitalOutput(void);        /* 获取 DO 引脚电平 (硬件报警) */
float MQ2_GetVoltage(void);             /* 获取 AO 引脚电压 (V) */
float MQ2_GetRatio(void);               /* 获取 Rs/Ro 比值 */
float MQ2_GetPPM(void);                 /* 获取气体浓度 (PPM) */
u8   MQ2_GetAlarmStatus(void);          /* 获取报警状态 (PPM > 100) */

#endif /* __MQ2_H */