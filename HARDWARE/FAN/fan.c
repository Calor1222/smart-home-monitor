#include "fan.h"

void Fan_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;
    
    // 1. 使能相关时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
    
    // 2. 配置PB6为复用功能
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_TIM4);
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    // 3. 配置定时器TIM4（频率50Hz，周期20ms）
    TIM_TimeBaseStructure.TIM_Period = 20000 - 1;
    TIM_TimeBaseStructure.TIM_Prescaler = 84 - 1;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);
    
    // 4. 配置PWM输出通道
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_Pulse = 0;
    TIM_OC1Init(TIM4, &TIM_OCInitStructure);
    
    // 5. 使能预装载寄存器
    TIM_OC1PreloadConfig(TIM4, TIM_OCPreload_Enable);
    TIM_ARRPreloadConfig(TIM4, ENABLE);
    
    // 6. 使能TIM4
    TIM_Cmd(TIM4, ENABLE);
    
    // 7. 初始状态：风扇停止（输出高电平）
    TIM_SetCompare1(TIM4, 20000);
}

// 设置风扇转速（0停止，100全速）
void Fan_SetSpeed(u8 percent)
{
    u32 compare_value;
    
    if(percent > 100) percent = 100;
    
    // 低电平触发：转速与占空比成反比
    // 占空比 = (100 - 转速百分比) %
    compare_value = (100 - percent) * 20000 / 100;
    
    if(compare_value > 20000) compare_value = 20000;
    if(compare_value < 0) compare_value = 0;
    
    TIM_SetCompare1(TIM4, compare_value);
}

// 风扇启动（50%转速）
void Fan_On(void)
{
    Fan_SetSpeed(50);
}

// 风扇停止
void Fan_Off(void)
{
    TIM_SetCompare1(TIM4, 20000);  // 100%占空比 = 持续高电平
}