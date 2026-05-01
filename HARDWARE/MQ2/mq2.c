/**
  ******************************************************************************
  * @file    mq2.c
  * @author  MQ-2 Driver
  * @version V1.0
  * @date    2026-04-16
  * @brief   MQ-2 烟雾传感器驱动源文件
  ******************************************************************************
  */

#include "mq2.h"
#include "stm32f4xx_adc.h"
#include <math.h>

/* 存储校准后的 Ro 值（传感器在洁净空气中的内阻）*/
static float Ro = 1.0f;

/* MQ-2 传感器特性曲线参数 (浓度与 Rs/Ro 比值的关系) */
/* 公式: ppm = a * (Rs/Ro)^b */
#define MQ2_CURVE_A             1000.0f
#define MQ2_CURVE_B             -1.5f

/**
  * @brief  ADC1 初始化
  * @param  无
  * @retval 无
  */
static void MQ2_ADC_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    ADC_InitTypeDef ADC_InitStruct;
    
    /* 1. 使能时钟 */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
    
    /* 2. 配置 PC1 为模拟输入模式 */
    GPIO_InitStruct.GPIO_Pin = MQ2_AO_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AN;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(MQ2_AO_GPIO_PORT, &GPIO_InitStruct);
    
    /* 3. ADC 配置 */
    ADC_InitStruct.ADC_Resolution = ADC_Resolution_12b;
    ADC_InitStruct.ADC_ScanConvMode = DISABLE;
    ADC_InitStruct.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStruct.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
    ADC_InitStruct.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1;
    ADC_InitStruct.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStruct.ADC_NbrOfConversion = 1;
    ADC_Init(ADC1, &ADC_InitStruct);
    
    /* 4. 使能 ADC1 */
    ADC_Cmd(ADC1, ENABLE);
}

/**
  * @brief  MQ-2 初始化函数 (包含 GPIO 和 ADC)
  * @param  无
  * @retval 无
  */
void MQ2_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    /* 1. 使能 GPIO 时钟 */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOC, ENABLE);
    
    /* 2. 初始化 DO 引脚 (PA0 - 输入模式) */
    GPIO_InitStruct.GPIO_Pin = MQ2_DO_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_DOWN;  /* 下拉，防止悬空误触发 */
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(MQ2_DO_GPIO_PORT, &GPIO_InitStruct);
    
    /* 3. 初始化 ADC (用于 AO 引脚) */
    MQ2_ADC_Init();
}

/**
  * @brief  获取 DO 引脚电平 (硬件数字输出)
  * @param  无
  * @retval 0: 浓度低于阈值, 1: 浓度超过阈值
  */
u8 MQ2_GetDigitalOutput(void)
{
    return GPIO_ReadInputDataBit(MQ2_DO_GPIO_PORT, MQ2_DO_PIN);
}

/**
  * @brief  读取 ADC 值（带中值滤波）
  * @param  num_samples: 采样次数 (建议 5-15)
  * @retval ADC 原始值 (0-4095)
  */
static u32 MQ2_ReadADC_Filtered(u8 num_samples)
{
    u32 values[15];
    u32 temp;
    u8 i, j;
    u32 sum = 0;
    u8 count = 0;
    
    if(num_samples > 15) num_samples = 15;
    if(num_samples < 3) num_samples = 3;
    
    /* 采样 */
    for(i = 0; i < num_samples; i++)
    {
        /* 配置通道，使用最长采样时间提高稳定性 */
        ADC_RegularChannelConfig(ADC1, MQ2_AO_ADC_CHANNEL, 1, ADC_SampleTime_480Cycles);
        
        /* 启动转换 */
        ADC_SoftwareStartConv(ADC1);
        
        /* 等待转换完成 */
        while(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);
        
        /* 读取值 */
        values[i] = ADC_GetConversionValue(ADC1);
        
        delay_ms(5);
    }
    
    /* 冒泡排序 */
    for(i = 0; i < num_samples - 1; i++)
    {
        for(j = i + 1; j < num_samples; j++)
        {
            if(values[i] > values[j])
            {
                temp = values[i];
                values[i] = values[j];
                values[j] = temp;
            }
        }
    }
    
    /* 去掉最大最小值，取平均 */
    for(i = 1; i < num_samples - 1; i++)
    {
        sum += values[i];
        count++;
    }
    
    if(count == 0) return values[0];
    return sum / count;
}

/**
  * @brief  获取 AO 引脚电压值（带滑动平均滤波）
  * @param  无
  * @retval 电压值 (单位: V)
  */
float MQ2_GetVoltage(void)
{
    u32 adc_value;
    static float voltage_history[5] = {0};
    static u8 index = 0;
    static u8 filled = 0;
    float sum = 0;
    u8 i;
    
    /* 读取滤波后的 ADC 值，采样10次 */
    adc_value = MQ2_ReadADC_Filtered(10);
    
    /* 转换为电压值 */
    float voltage = (float)adc_value / ADC_MAX_VALUE * ADC_REF_VOLTAGE;
    
    /* 滑动平均滤波 */
    voltage_history[index] = voltage;
    index++;
    if(index >= 5) index = 0;
    
    if(index == 0) filled = 1;
    
    if(filled)
    {
        for(i = 0; i < 5; i++) sum += voltage_history[i];
        return sum / 5.0f;
    }
    else
    {
        for(i = 0; i <= index; i++) sum += voltage_history[i];
        return sum / (index + 1);
    }
}

/**
  * @brief  获取传感器内阻与洁净空气内阻的比值 (Rs/Ro)
  * @param  无
  * @retval Rs/Ro 比值
  * @note   需要先调用 MQ2_Calibrate() 获取 Ro 值
  */
float MQ2_GetRatio(void)
{
    float voltage = MQ2_GetVoltage();
    float Rs;
    
    /* 避免除零 */
    if(voltage < 0.01f) voltage = 0.01f;
    
    /* 计算 Rs = RL * (Vcc/Vout - 1) */
    Rs = MQ2_RL * (MQ2_VCC / voltage - 1.0f);
    
    return Rs / Ro;
}

/**
  * @brief  传感器校准 - 在洁净空气中获取 Ro 值
  * @param  无
  * @retval 无
  * @note   使用前确保传感器已预热 15 秒以上
  */
void MQ2_Calibrate(void)
{
    float voltage_sum = 0.0f;
    u8 i;
    
    /* 取 30 次采样平均值 */
    for(i = 0; i < 30; i++)
    {
        voltage_sum += MQ2_GetVoltage();
        delay_ms(50);
    }
    
    float voltage_avg = voltage_sum / 30.0f;
    
    /* 避免除零 */
    if(voltage_avg < 0.01f) voltage_avg = 0.01f;
    
    /* 计算洁净空气中的 Rs */
    float Rs_clean = MQ2_RL * (MQ2_VCC / voltage_avg - 1.0f);
    
    /* 洁净空气中 Rs/Ro = MQ2_CLEAN_AIR_RATIO */
    Ro = Rs_clean / MQ2_CLEAN_AIR_RATIO;
}

/**
  * @brief  获取当前气体浓度 (PPM)
  * @param  无
  * @retval 浓度值 (单位: PPM)
  * @note   需要先调用 MQ2_Calibrate() 进行校准
  */
float MQ2_GetPPM(void)
{
    float ratio = MQ2_GetRatio();
    float ppm;
    
    if(ratio <= 0.01f) ratio = 0.01f;
    
    /* 使用幂函数公式: ppm = A * (ratio)^B */
    ppm = MQ2_CURVE_A * powf(ratio, MQ2_CURVE_B);
    
    /* 限制输出范围 */
    if(ppm < 0) ppm = 0;
    if(ppm > 10000) ppm = 10000;
    
    return ppm;
}

/**
  * @brief  获取报警状态 (基于 PPM 浓度)
  * @param  无
  * @retval 0: 正常, 1: 报警 (PPM > 100)
  */
u8 MQ2_GetAlarmStatus(void)
{
    float ppm = MQ2_GetPPM();
    
    if(ppm > MQ2_ALARM_PPM_THRESHOLD)
        return 1;
    else
        return 0;
}