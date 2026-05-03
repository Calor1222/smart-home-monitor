#include "dht11.h"
#include "delay.h"

/*
 * 复位DHT11
 * 先拉低总线再拉高
 */
void DHT11_Rst(void)
{
    DHT11_IO_OUT();
    DHT11_DQ_OUT = 0;
    delay_ms(20);
    DHT11_DQ_OUT = 1;
    delay_us(30);
}

/*
 * 检查DHT11响应
 * 返回0表示存在
 */
u8 DHT11_Check(void)
{
    u8 retry = 0;

    DHT11_IO_IN();
    while (DHT11_DQ_IN && retry < 100)
    {
        retry++;
        delay_us(1);
    }
    if(retry >= 100) return 1;

    retry = 0;
    while (!DHT11_DQ_IN && retry < 100)
    {
        retry++;
        delay_us(1);
    }
    if(retry >= 100) return 1;

    return 0;
}

/*
 * 读取一个位
 * 根据高电平宽度判断0和1
 */
u8 DHT11_Read_Bit(void)
{
    u8 retry = 0;

    while(DHT11_DQ_IN && retry < 100)
    {
        retry++;
        delay_us(1);
    }
    retry = 0;
    while(!DHT11_DQ_IN && retry < 100)
    {
        retry++;
        delay_us(1);
    }
    delay_us(40);

    if(DHT11_DQ_IN) return 1;
    else return 0;
}

/*
 * 读取一个字节
 * 连续读取8个位
 */
u8 DHT11_Read_Byte(void)
{
    u8 i,dat;
    dat = 0;

    for (i = 0; i < 8; i++)
    {
        dat <<= 1;
        dat |= DHT11_Read_Bit();
    }
    return dat;
}

/*
 * 读取温湿度数据
 * 成功返回0，失败返回1
 */
u8 DHT11_Read_Data(u8 *temp,u8 *humi)
{
    u8 buf[5];
    u8 i;

    DHT11_Rst();
    if(DHT11_Check() == 0)
    {
        for(i = 0; i < 5; i++)
        {
            buf[i] = DHT11_Read_Byte();
        }
        if((buf[0] + buf[1] + buf[2] + buf[3]) == buf[4])
        {
            *humi = buf[0];
            *temp = buf[2];
        }
    }
    else return 1;
    return 0;
}

/*
 * 初始化DHT11
 * 配置PG9并检测模块
 */
u8 DHT11_Init(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOG, &GPIO_InitStructure);

    DHT11_Rst();
    return DHT11_Check();
}
