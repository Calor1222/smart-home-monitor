#include "stm32f4xx.h"
#include "delay.h"
#include "stepmotor.h"

/*
 * 步进电机相序表
 * 使用四相八拍方式驱动
 */
const uint8_t step_sequence[8][4] = {
    {1, 0, 0, 0},
    {1, 1, 0, 0},
    {0, 1, 0, 0},
    {0, 1, 1, 0},
    {0, 0, 1, 0},
    {0, 0, 1, 1},
    {0, 0, 0, 1},
    {1, 0, 0, 1}
};

/*
 * 初始化步进电机
 * 配置四个控制脚为输出
 */
void Stepper_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

    GPIO_InitStruct.GPIO_Pin   = IN1_PIN | IN2_PIN | IN3_PIN | IN4_PIN;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_OUT;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    GPIO_Init(IN1_PORT, &GPIO_InitStruct);

    GPIO_ResetBits(IN1_PORT, IN1_PIN);
    GPIO_ResetBits(IN2_PORT, IN2_PIN);
    GPIO_ResetBits(IN3_PORT, IN3_PIN);
    GPIO_ResetBits(IN4_PORT, IN4_PIN);
}

/*
 * 输出一步
 * 根据相序表设置四相状态
 */
void Stepper_Output(uint8_t step_index)
{
    GPIO_WriteBit(IN1_PORT, IN1_PIN, (BitAction)step_sequence[step_index][0]);
    GPIO_WriteBit(IN2_PORT, IN2_PIN, (BitAction)step_sequence[step_index][1]);
    GPIO_WriteBit(IN3_PORT, IN3_PIN, (BitAction)step_sequence[step_index][2]);
    GPIO_WriteBit(IN4_PORT, IN4_PIN, (BitAction)step_sequence[step_index][3]);
}

/*
 * 顺时针转动步进电机
 * steps控制步数，step_delay_ms控制速度
 */
void Stepper_Rotate_Clockwise(uint32_t steps, uint16_t step_delay_ms)
{
    for(uint32_t i = 0; i < steps; i++)
    {
        for(uint8_t j = 0; j < 8; j++)
        {
            Stepper_Output(j);
            delay_ms(step_delay_ms);
        }
    }

    GPIO_ResetBits(IN1_PORT, IN1_PIN);
    GPIO_ResetBits(IN2_PORT, IN2_PIN);
    GPIO_ResetBits(IN3_PORT, IN3_PIN);
    GPIO_ResetBits(IN4_PORT, IN4_PIN);
}
