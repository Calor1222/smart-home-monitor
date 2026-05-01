#include "sys.h"
#include "usart3.h"
#include "esp8266.h"  // 添加ESP8266头文件

//////////////////////////////////////////////////////////////////////////////////
// 如果使用ucos,则包括下面的头文件即可.
#if SYSTEM_SUPPORT_OS
#include "includes.h"
#endif
//////////////////////////////////////////////////////////////////////////////////

#if EN_USART3_RX
u8 USART3_RX_BUF[USART3_REC_LEN];
u16 USART3_RX_STA = 0;
#endif

/**
 * 串口3初始化函数
 * bound: 波特率
 */
void uart3_init(u32 bound)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    // 使能GPIOB和USART3时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);

    // 串口3对应引脚复用映射: PB10->USART3_TX, PB11->USART3_RX
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource10, GPIO_AF_USART3);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource11, GPIO_AF_USART3);

    // USART3端口配置
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;    // 复用功能
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;  // 推挽复用输出
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;    // 上拉
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // USART3 初始化设置
    USART_InitStructure.USART_BaudRate = bound;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART3, &USART_InitStructure);

    USART_Cmd(USART3, ENABLE);  // 使能串口3

#if EN_USART3_RX
    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE); // 开启接收中断

    // USART3 NVIC 配置
    NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
#endif
}

#if EN_USART3_RX
/**
 * 串口3中断服务程序
 */
void USART3_IRQHandler(void)
{
    u8 Res;

#if SYSTEM_SUPPORT_OS
    OSIntEnter();
#endif

    if (USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)
    {
        Res = USART_ReceiveData(USART3);
        
        // 原有的串口3数据接收处理
        if ((USART3_RX_STA & 0x8000) == 0)
        {
            if (USART3_RX_STA & 0x4000)
            {
                if (Res != 0x0a)
                    USART3_RX_STA = 0;
                else
                    USART3_RX_STA |= 0x8000;
            }
            else
            {
                if (Res == 0x0d)
                {
                    USART3_RX_STA |= 0x4000;
                }
                else
                {
                    USART3_RX_BUF[USART3_RX_STA & 0x3FFF] = Res;
                    USART3_RX_STA++;
                    if (USART3_RX_STA > (USART3_REC_LEN - 1))
                        USART3_RX_STA = 0;
                }
            }
        }
        
        // 添加ESP8266数据处理
        esp8266_data_handle(Res);
    }

#if SYSTEM_SUPPORT_OS
    OSIntExit();
#endif
}
#endif