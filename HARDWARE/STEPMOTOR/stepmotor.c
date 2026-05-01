#include "stm32f4xx.h"
#include "delay.h"
#include "stepmotor.h"

// 步进电机四相八拍正转（顺时针）相序表
// 数组元素：{IN1, IN2, IN3, IN4} 电平状态 (1=高电平, 0=低电平)
// 注意：ULN2003 是低电平有效，但这里我们按正常逻辑写 1/0，
// 实际输出时用 GPIO_SetBits / GPIO_ResetBits 控制，无需反向。
const uint8_t step_sequence[8][4] = {
    {1, 0, 0, 0},   // A
    {1, 1, 0, 0},   // AB
    {0, 1, 0, 0},   // B
    {0, 1, 1, 0},   // BC
    {0, 0, 1, 0},   // C
    {0, 0, 1, 1},   // CD
    {0, 0, 0, 1},   // D
    {1, 0, 0, 1}    // DA
};

// 初始化步进电机 GPIO
void Stepper_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    // 使能 GPIOD 时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

    GPIO_InitStruct.GPIO_Pin   = IN1_PIN | IN2_PIN | IN3_PIN | IN4_PIN;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_OUT;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;      // 推挽输出
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    GPIO_Init(IN1_PORT, &GPIO_InitStruct);

    // 初始全部置低电平（电机不转）
    GPIO_ResetBits(IN1_PORT, IN1_PIN);
    GPIO_ResetBits(IN2_PORT, IN2_PIN);
    GPIO_ResetBits(IN3_PORT, IN3_PIN);
    GPIO_ResetBits(IN4_PORT, IN4_PIN);
}

// 输出一步（根据相序表）
void Stepper_Output(uint8_t step_index)
{
    // step_index 范围 0~7
    GPIO_WriteBit(IN1_PORT, IN1_PIN, (BitAction)step_sequence[step_index][0]);
    GPIO_WriteBit(IN2_PORT, IN2_PIN, (BitAction)step_sequence[step_index][1]);
    GPIO_WriteBit(IN3_PORT, IN3_PIN, (BitAction)step_sequence[step_index][2]);
    GPIO_WriteBit(IN4_PORT, IN4_PIN, (BitAction)step_sequence[step_index][3]);
}

// 顺时针转动固定步数（每步的延时决定转速）
// steps：步数（1步对应八拍中的一拍，实际电机转动的机械角度取决于减速比）
// step_delay_ms：每步之间的延时（毫秒），建议 2~10ms
void Stepper_Rotate_Clockwise(uint32_t steps, uint16_t step_delay_ms)
{
    for(uint32_t i = 0; i < steps; i++)
    {
        for(uint8_t j = 0; j < 8; j++)      // 循环输出八拍
        {
            Stepper_Output(j);
            delay_ms(step_delay_ms);
        }
    }
    // 停止时释放所有线圈（可选）
    GPIO_ResetBits(IN1_PORT, IN1_PIN);
    GPIO_ResetBits(IN2_PORT, IN2_PIN);
    GPIO_ResetBits(IN3_PORT, IN3_PIN);
    GPIO_ResetBits(IN4_PORT, IN4_PIN);
}
