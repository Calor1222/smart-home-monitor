#include "mq2.h"
#include "stm32f4xx_adc.h"
#include <math.h>

/*
 * 校准后的Ro值
 * 用于气体浓度换算
 */
static float Ro = 1.0f;

/*
 * 曲线参数
 * 用于PPM计算
 */
#define MQ2_CURVE_A 1000.0f
#define MQ2_CURVE_B -1.5f

/*
 * 初始化ADC输入
 * AO脚使用PC1
 */
static void MQ2_ADC_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    ADC_InitTypeDef ADC_InitStruct;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

    GPIO_InitStruct.GPIO_Pin = MQ2_AO_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AN;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(MQ2_AO_GPIO_PORT, &GPIO_InitStruct);

    ADC_InitStruct.ADC_Resolution = ADC_Resolution_12b;
    ADC_InitStruct.ADC_ScanConvMode = DISABLE;
    ADC_InitStruct.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStruct.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
    ADC_InitStruct.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1;
    ADC_InitStruct.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStruct.ADC_NbrOfConversion = 1;
    ADC_Init(ADC1, &ADC_InitStruct);

    ADC_Cmd(ADC1, ENABLE);
}

/*
 * 初始化MQ2
 * 包括DO和AO引脚
 */
void MQ2_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOC, ENABLE);

    GPIO_InitStruct.GPIO_Pin = MQ2_DO_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_DOWN;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(MQ2_DO_GPIO_PORT, &GPIO_InitStruct);

    MQ2_ADC_Init();
}

/*
 * 读取DO电平
 * 用于硬件报警判断
 */
u8 MQ2_GetDigitalOutput(void)
{
    return GPIO_ReadInputDataBit(MQ2_DO_GPIO_PORT, MQ2_DO_PIN);
}

/*
 * 读取ADC并滤波
 * 去掉最大最小值后取平均
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

    for(i = 0; i < num_samples; i++)
    {
        ADC_RegularChannelConfig(ADC1, MQ2_AO_ADC_CHANNEL, 1, ADC_SampleTime_480Cycles);
        ADC_SoftwareStartConv(ADC1);
        while(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);
        values[i] = ADC_GetConversionValue(ADC1);
        delay_ms(5);
    }

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

    for(i = 1; i < num_samples - 1; i++)
    {
        sum += values[i];
        count++;
    }

    if(count == 0) return values[0];
    return sum / count;
}

/*
 * 读取AO电压
 * 带滑动平均滤波
 */
float MQ2_GetVoltage(void)
{
    u32 adc_value;
    static float voltage_history[5] = {0};
    static u8 index = 0;
    static u8 filled = 0;
    float sum = 0;
    u8 i;
    float voltage;

    adc_value = MQ2_ReadADC_Filtered(10);
    voltage = (float)adc_value / ADC_MAX_VALUE * ADC_REF_VOLTAGE;

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

/*
 * 读取Rs/Ro比值
 * 需要先完成校准
 */
float MQ2_GetRatio(void)
{
    float voltage = MQ2_GetVoltage();
    float Rs;

    if(voltage < 0.01f) voltage = 0.01f;

    Rs = MQ2_RL * (MQ2_VCC / voltage - 1.0f);
    return Rs / Ro;
}

/*
 * 校准MQ2
 * 在洁净空气中获取Ro
 */
void MQ2_Calibrate(void)
{
    float voltage_sum = 0.0f;
    u8 i;
    float voltage_avg;
    float Rs_clean;

    for(i = 0; i < 30; i++)
    {
        voltage_sum += MQ2_GetVoltage();
        delay_ms(50);
    }

    voltage_avg = voltage_sum / 30.0f;

    if(voltage_avg < 0.01f) voltage_avg = 0.01f;

    Rs_clean = MQ2_RL * (MQ2_VCC / voltage_avg - 1.0f);
    Ro = Rs_clean / MQ2_CLEAN_AIR_RATIO;
}

/*
 * 读取烟雾浓度
 * 返回PPM值
 */
float MQ2_GetPPM(void)
{
    float ratio = MQ2_GetRatio();
    float ppm;

    if(ratio <= 0.01f) ratio = 0.01f;

    ppm = MQ2_CURVE_A * powf(ratio, MQ2_CURVE_B);

    if(ppm < 0) ppm = 0;
    if(ppm > 10000) ppm = 10000;

    return ppm;
}

/*
 * 读取报警状态
 * 超过阈值时返回1
 */
u8 MQ2_GetAlarmStatus(void)
{
    float ppm = MQ2_GetPPM();

    if(ppm > MQ2_ALARM_PPM_THRESHOLD)
        return 1;
    else
        return 0;
}
