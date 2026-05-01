#include "stm32f4xx.h"
#include "delay.h"
#include "usart.h"
#include "usart3.h"
#include "esp8266.h"
#include "dht11.h"
#include "mq2.h"
#include <stdio.h>

int main(void)
{
    u8 temp = 0;
    u8 hum = 0;
    float smoke = 0.0f;
    u8 dht11_status;

    delay_init(168);
    uart_init(115200);
    uart3_init(115200);
    esp8266_init();

    dht11_status = DHT11_Init();
    MQ2_Init();

    printf("\r\nSystem start\r\n");
    if (dht11_status == 0)
        printf("DHT11 init success\r\n");
    else
        printf("DHT11 init failed\r\n");

    esp8266_connect_wifi();
    esp8266_connect_onenet();

    printf("MQ2 warm up...\r\n");
    delay_ms(20000);
    printf("MQ2 calibrating...\r\n");
    MQ2_Calibrate();
    printf("MQ2 calibration done\r\n");

    while (1)
    {
        if (DHT11_Read_Data(&temp, &hum) != 0)
        {
            printf("DHT11 read failed, skip upload\r\n");
            delay_ms(5000);
            continue;
        }

        smoke = MQ2_GetPPM();

        printf("Temp=%d C, Hum=%d %%, Smoke=%.2f ppm\r\n",
               temp, hum, smoke);

        esp8266_onenet_post_property((float)temp, (float)hum, smoke);

        delay_ms(5000);
    }
}
