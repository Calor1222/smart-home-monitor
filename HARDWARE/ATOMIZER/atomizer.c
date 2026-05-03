#include "atomizer.h"

/*
 * 软件状态记录
 * 用于避免直接读取GPIO带来的不确定性
 */
static uint8_t Atomizer_State = 0;

/**
  * @brief  初始化雾化模块控制引脚
  * @note   PB0 输出控制 S 引脚，高电平开启，低电平关闭
  */
void Atomizer_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /*
     * 开启 GPIOB 时钟
     * 若不使能时钟，GPIO 配置无效
     */
    RCC_AHB1PeriphClockCmd(ATOMIZER_GPIO_CLK, ENABLE);

    /*
     * 配置 PB0 为推挽输出
     * 用于输出稳定的高低电平控制模块
     */
    GPIO_InitStructure.GPIO_Pin = ATOMIZER_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    /*
     * 下拉配置
     * 上电默认低电平，防止误触发雾化器
     */
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;

    GPIO_Init(ATOMIZER_GPIO_PORT, &GPIO_InitStructure);

    /*
     * 默认关闭设备，保证安全启动
     */
    Atomizer_Off();
}

/**
  * @brief  开启雾化器
  * @note   高电平触发模块工作
  */
void Atomizer_On(void)
{
    GPIO_SetBits(ATOMIZER_GPIO_PORT, ATOMIZER_GPIO_PIN);
    Atomizer_State = 1;
}

/**
  * @brief  关闭雾化器
  * @note   低电平关闭模块
  */
void Atomizer_Off(void)
{
    GPIO_ResetBits(ATOMIZER_GPIO_PORT, ATOMIZER_GPIO_PIN);
    Atomizer_State = 0;
}

/**
  * @brief  设置雾化器状态
  * @param  state: 0关闭，非0开启
  */
void Atomizer_Set(uint8_t state)
{
    if (state)
    {
        Atomizer_On();
    }
    else
    {
        Atomizer_Off();
    }
}

/**
  * @brief  获取当前状态
  * @retval 0关闭，1开启
  */
uint8_t Atomizer_GetState(void)
{
    return Atomizer_State;
}

/**
  * @brief  状态翻转
  */
void Atomizer_Toggle(void)
{
    if (Atomizer_State)
    {
        Atomizer_Off();
    }
    else
    {
        Atomizer_On();
    }
}