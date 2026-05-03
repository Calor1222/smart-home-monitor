#include "gp2y.h"
#include "delay.h"

#define GP2Y_LED_ON()              GPIO_SetBits(GP2Y_ILED_PORT, GP2Y_ILED_PIN)
#define GP2Y_LED_OFF()             GPIO_ResetBits(GP2Y_ILED_PORT, GP2Y_ILED_PIN)

#define GP2Y_ADC_REF_MV           3300.0f
#define GP2Y_ADC_MAX_VALUE        4096.0f
#define GP2Y_DIVIDER_RATIO        11.0f
#define GP2Y_FILTER_COUNT         10
#define NO_DUST_VOLTAGE           330.0f
#define COV_RATIO                 0.20f

/*
 * 单次ADC采样
 * 只负责读取一次ADC转换值
 */
static uint16_t GP2Y_ADC_ReadOnce(void)
{
    ADC_RegularChannelConfig(GP2Y_ADC,
                             GP2Y_ADC_CHANNEL,
                             1,
                             ADC_SampleTime_480Cycles);

    ADC_ClearFlag(GP2Y_ADC, ADC_FLAG_EOC);
    ADC_SoftwareStartConv(GP2Y_ADC);

    while (ADC_GetFlagStatus(GP2Y_ADC, ADC_FLAG_EOC) == RESET);

    return ADC_GetConversionValue(GP2Y_ADC);
}

/*
 * ADC平均滤波
 * 连续读取多次后取平均值
 */
static uint16_t GP2Y_FilterAdc(uint8_t times)
{
    uint8_t i;
    uint32_t sum = 0;

    if (times == 0)
        times = 1;

    for (i = 0; i < times; i++)
    {
        sum += GP2Y_ADC_ReadOnce();
    }

    return (uint16_t)(sum / times);
}

/**
  * @brief  初始化GP2Y控制引脚和ADC
  * @note   ILED使用高电平点亮，PA5作为模拟输入
  */
void GP2Y_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    ADC_InitTypeDef ADC_InitStructure;
    ADC_CommonInitTypeDef ADC_CommonInitStructure;

    RCC_AHB1PeriphClockCmd(GP2Y_ILED_RCC | GP2Y_ADC_RCC, ENABLE);
    RCC_APB2PeriphClockCmd(GP2Y_ADC_CLK, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GP2Y_ILED_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GP2Y_ILED_PORT, &GPIO_InitStructure);

    GP2Y_LED_OFF();

    GPIO_InitStructure.GPIO_Pin = GP2Y_ADC_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GP2Y_ADC_PORT, &GPIO_InitStructure);

    ADC_CommonStructInit(&ADC_CommonInitStructure);
    ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div4;
    ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
    ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
    ADC_CommonInit(&ADC_CommonInitStructure);

    ADC_StructInit(&ADC_InitStructure);
    ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfConversion = 1;
    ADC_Init(GP2Y_ADC, &ADC_InitStructure);

    ADC_Cmd(GP2Y_ADC, ENABLE);
}

/**
  * @brief  按官方时序读取原始ADC值
  * @note   280us后采样，点亮总宽度0.32ms，周期10ms
  */
uint16_t GP2Y_ReadRaw(void)
{
    uint16_t raw;

    GP2Y_LED_ON();
    delay_us(280);
    raw = GP2Y_FilterAdc(GP2Y_FILTER_COUNT);
    delay_us(40);
    GP2Y_LED_OFF();
    delay_us(9680);

    return raw;
}

/**
  * @brief  获取当前输出电压
  * @retval 返回补偿分压后的电压值，单位mV
  */
float GP2Y_ReadVoltage(void)
{
    uint16_t raw = GP2Y_ReadRaw();

    return raw * GP2Y_ADC_REF_MV / GP2Y_ADC_MAX_VALUE * GP2Y_DIVIDER_RATIO;
}

/**
  * @brief  获取PM2.5浓度
  * @retval 返回浓度值，单位ug/m3
  */
float GP2Y_ReadDust_ugm3(void)
{
    float voltage_mv;
    float pm;

    voltage_mv = GP2Y_ReadVoltage();

    if (voltage_mv > NO_DUST_VOLTAGE)
        pm = (voltage_mv - NO_DUST_VOLTAGE) * COV_RATIO;
    else
        pm = 0;

    return pm;
}

/**
  * @brief  获取平均PM2.5浓度
  * @param  times: 平均次数
  * @retval 返回平均后的浓度值
  */
float GP2Y_ReadDustAverage_ugm3(uint8_t times)
{
    uint8_t i;
    float sum = 0;

    if (times == 0)
        times = 1;

    for (i = 0; i < times; i++)
    {
        sum += GP2Y_ReadDust_ugm3();
    }

    return sum / times;
}
