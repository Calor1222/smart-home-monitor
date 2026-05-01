#include "gp2y.h"
#include "delay.h"

/* =========================================================
 * 关键参数（建议后期根据实际环境校准）
 * =========================================================
 */

// ADC参考电压
#define GP2Y_ADC_REF_VOLTAGE      3.3f

// 12位ADC最大值
#define GP2Y_ADC_MAX_VALUE        4095.0f

// ?? 无尘电压（建议后期实际测量）
#define GP2Y_CLEAN_AIR_VOLTAGE    0.9f

// 灵敏度：0.5V / (0.1 mg/m3)
#define GP2Y_SENSITIVITY          0.5f


/* =========================================================
 * 函数：单次ADC采样
 * =========================================================
 */
static uint16_t GP2Y_ADC_ReadOnce(void)
{
    // 设置 ADC 通道（PA5 = Channel 5）
    ADC_RegularChannelConfig(GP2Y_ADC,
                             GP2Y_ADC_CHANNEL,
                             1,
                             ADC_SampleTime_480Cycles);

    // 清除转换完成标志
    ADC_ClearFlag(GP2Y_ADC, ADC_FLAG_EOC);

    // 启动 ADC 转换
    ADC_SoftwareStartConv(GP2Y_ADC);

    // 等待转换完成
    while (ADC_GetFlagStatus(GP2Y_ADC, ADC_FLAG_EOC) == RESET);

    return ADC_GetConversionValue(GP2Y_ADC);
}


/* =========================================================
 * 初始化函数
 * =========================================================
 */
void GP2Y_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    ADC_InitTypeDef ADC_InitStructure;
    ADC_CommonInitTypeDef ADC_CommonInitStructure;

    // 开启 GPIO 和 ADC 时钟
    RCC_AHB1PeriphClockCmd(GP2Y_ILED_RCC | GP2Y_ADC_RCC, ENABLE);
    RCC_APB2PeriphClockCmd(GP2Y_ADC_CLK, ENABLE);

    /* ===== ILED 控制引脚 ===== */
    GPIO_InitStructure.GPIO_Pin = GP2Y_ILED_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GP2Y_ILED_PORT, &GPIO_InitStructure);

    // 默认关闭LED（高电平）
    GPIO_SetBits(GP2Y_ILED_PORT, GP2Y_ILED_PIN);

    /* ===== ADC 输入引脚（PA5）===== */
    GPIO_InitStructure.GPIO_Pin = GP2Y_ADC_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GP2Y_ADC_PORT, &GPIO_InitStructure);

    /* ===== ADC 公共配置 ===== */
    ADC_CommonStructInit(&ADC_CommonInitStructure);
    ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div4;
    ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
    ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
    ADC_CommonInit(&ADC_CommonInitStructure);

    /* ===== ADC 配置 ===== */
    ADC_StructInit(&ADC_InitStructure);
    ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfConversion = 1;
    ADC_Init(GP2Y_ADC, &ADC_InitStructure);

    ADC_Cmd(GP2Y_ADC, ENABLE);
}


/* =========================================================
 * 核心函数：按手册时序读取
 *
 * 官方推荐：
 * LED ON → 280us → 采样 → 40us → LED OFF → 10ms周期
 * =========================================================
 */
uint16_t GP2Y_ReadRaw(void)
{
    uint16_t raw;

    // 1. 点亮LED（低电平）
    GPIO_ResetBits(GP2Y_ILED_PORT, GP2Y_ILED_PIN);

    // 2. 等待 280us（光信号稳定）
    delay_us(280);

    // 3. 采样（必须在LED亮期间）
    raw = GP2Y_ADC_ReadOnce();

    // 4. 补足 LED 点亮时间（280 + 40 = 320us）
    delay_us(40);

    // 5. 关闭 LED
    GPIO_SetBits(GP2Y_ILED_PORT, GP2Y_ILED_PIN);

    // ?? 必须保证 10ms 周期，否则数据会乱
    delay_us(9680);

    return raw;
}


/* =========================================================
 * 转电压
 * =========================================================
 */
float GP2Y_ReadVoltage(void)
{
    uint16_t raw = GP2Y_ReadRaw();

    // ADC 转电压公式
    return raw * GP2Y_ADC_REF_VOLTAGE / GP2Y_ADC_MAX_VALUE;
}


/* =========================================================
 * 转浓度（ug/m3）
 * =========================================================
 */
float GP2Y_ReadDust_ugm3(void)
{
    float voltage;
    float dust_mg;
    float dust_ug;

    voltage = GP2Y_ReadVoltage();

    /*
     * 转换公式：
     * ΔV = Vout - Voc
     * mg/m3 = ΔV / K * 0.1
     */
    dust_mg = (voltage - GP2Y_CLEAN_AIR_VOLTAGE) / GP2Y_SENSITIVITY * 0.1f;

    if (dust_mg < 0)
        dust_mg = 0;

    // mg/m3 → ug/m3
    dust_ug = dust_mg * 1000.0f;

    return dust_ug;
}


/* =========================================================
 * 多次平均滤波
 * =========================================================
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