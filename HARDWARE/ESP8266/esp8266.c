#include "esp8266.h"
#include "usart3.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

//////////////////////////////////////////////////////////////////////////////////
// 全局变量
//////////////////////////////////////////////////////////////////////////////////
u8 esp_rx_buffer[300];          // ESP8266接收缓冲区
u8 esp_rx_flag = 0;             // 接收完成标志
u16 esp_rx_len = 0;             // 接收数据长度

// 连接状态标志
u8 wifi_connected = 0;          // WiFi连接状态
u8 onenet_connected = 0;        // OneNet连接状态

//////////////////////////////////////////////////////////////////////////////////
// 延时函数(简单延时，ms)
//////////////////////////////////////////////////////////////////////////////////
void esp_delay_ms(u16 nms)
{
    u32 i;
    while(nms--)
    {
        i = 10000;
        while(i--);
    }
}

//////////////////////////////////////////////////////////////////////////////////
// 清空接收缓冲区
//////////////////////////////////////////////////////////////////////////////////
void esp_clear_buffer(void)
{
    u16 i;
    for(i = 0; i < 300; i++)
        esp_rx_buffer[i] = 0;
    esp_rx_len = 0;
    esp_rx_flag = 0;
}

//////////////////////////////////////////////////////////////////////////////////
// 串口3发送字符串函数（修正版）
//////////////////////////////////////////////////////////////////////////////////
void USART3_SendString(char *str)
{
    while(*str)
    {
        while((USART3->SR & 0X40) == 0);  // 等待发送完成
        USART3->DR = (u8)(*str++);         // 发送数据
    }
}

//////////////////////////////////////////////////////////////////////////////////
// 发送AT指令
//////////////////////////////////////////////////////////////////////////////////
void esp8266_cmd_send(char *cmd, char *ack, u16 timeout)
{
    u8 retry = 0;
    
    esp_clear_buffer();
    
    // 发送指令
    printf("Send CMD: %s\r\n", cmd);
    USART3_SendString(cmd);
    USART3_SendString("\r\n");
    
    // 等待响应
    while(timeout--)
    {
        esp_delay_ms(1);
        if(esp_rx_flag == 1)    // 收到数据
        {
            esp_rx_flag = 0;
            if(strstr((char*)esp_rx_buffer, ack) != NULL)  // 找到期望的应答
            {
                printf("ESP8266 CMD OK: %s\r\n", cmd);
                return;
            }
        }
    }
    
    printf("ESP8266 CMD ERROR: %s\r\n", cmd);
}

//////////////////////////////////////////////////////////////////////////////////
// 检查AT指令是否成功
//////////////////////////////////////////////////////////////////////////////////
u8 esp8266_check_cmd(char *ack)
{
    if(strstr((char*)esp_rx_buffer, ack) != NULL)
        return 1;
    return 0;
}

//////////////////////////////////////////////////////////////////////////////////
// ESP8266初始化
//////////////////////////////////////////////////////////////////////////////////
void esp8266_init(void)
{
    printf("\r\n========== ESP8266初始化开始 ==========\r\n");
    
    // 1. 测试AT指令
    esp8266_cmd_send("AT", "OK", 2000);
    esp_delay_ms(500);
    
    // 2. 关闭回显
    esp8266_cmd_send("ATE0", "OK", 2000);
    esp_delay_ms(500);
    
    // 3. 设置为Station模式
    esp8266_cmd_send("AT+CWMODE=1", "OK", 2000);
    esp_delay_ms(500);
    
    // 4. 重启模块
    esp8266_cmd_send("AT+RST", "ready", 5000);
    esp_delay_ms(2000);
    
    // 5. 查询模块版本
    esp8266_cmd_send("AT+GMR", "OK", 2000);
    
    printf("========== ESP8266初始化完成 ==========\r\n");
}

//////////////////////////////////////////////////////////////////////////////////
// 连接WiFi
//////////////////////////////////////////////////////////////////////////////////
void esp8266_connect_wifi(void)
{
    char cmd[100];
    
    printf("\r\n========== 连接WiFi ==========\r\n");
    printf("SSID: %s\r\n", WIFI_SSID);
    printf("PWD:  %s\r\n", WIFI_PASSWORD);
    
    // 发送连接WiFi指令
    sprintf(cmd, "AT+CWJAP=\"%s\",\"%s\"", WIFI_SSID, WIFI_PASSWORD);
    esp8266_cmd_send(cmd, "OK", 15000);
    
    // 查询IP地址
    esp8266_cmd_send("AT+CIFSR", "OK", 2000);
    
    wifi_connected = 1;
    printf("WiFi连接成功!\r\n");
}

//////////////////////////////////////////////////////////////////////////////////
// 配置MQTT连接并连接OneNet
//////////////////////////////////////////////////////////////////////////////////
void esp8266_connect_onenet(void)
{
    char cmd[300];
    
    printf("\r\n========== 连接OneNet云平台 ==========\r\n");
    printf("Server: %s:%s\r\n", ONENET_SERVER, ONENET_PORT);
    printf("Device Name: %s\r\n", DEVICE_NAME);
    
    // 1. 配置MQTT版本和连接方式
    sprintf(cmd, "AT+MQTTUSERCFG=0,1,\"%s\",\"%s\",\"%s\",0,0,\"\"", 
            DEVICE_NAME, PRODUCT_ID, DEVICE_TOKEN);
    esp8266_cmd_send(cmd, "OK", 3000);
    
    esp_delay_ms(500);
    
    // 2. 设置MQTT服务器地址和端口
    sprintf(cmd, "AT+MQTTCONN=0,\"%s\",%s,1", ONENET_SERVER, ONENET_PORT);
    esp8266_cmd_send(cmd, "CONNECT", 10000);
    
    onenet_connected = 1;
    printf("OneNet连接成功!\r\n");
}

void esp8266_mqtt_publish(const char *topic, const char *payload)
{
    char cmd[160];

    sprintf(cmd, "AT+MQTTPUBRAW=\"%s\",%d", topic, strlen(payload));
    esp8266_cmd_send(cmd, ">", 2000);
    esp8266_cmd_send((char *)payload, "OK", 5000);
}

void esp8266_onenet_post_property(float temp, float hum, float smoke)
{
    char payload[256];

    sprintf(payload,
        "{\"id\":\"123\",\"version\":\"1.0\",\"params\":{"
        "\"%s\":{\"value\":%.2f},"
        "\"%s\":{\"value\":%.2f},"
        "\"%s\":{\"value\":%.2f}"
        "}}",
        PROPERTY_TEMP, temp,
        PROPERTY_HUM, hum,
        PROPERTY_SMOKE, smoke
    );

    esp8266_mqtt_publish(ONENET_PROPERTY_POST_TOPIC, payload);
}

//////////////////////////////////////////////////////////////////////////////////
// 发送数据到OneNet
//////////////////////////////////////////////////////////////////////////////////
void esp8266_send_data(char *data, u16 len)
{
    char cmd[100];
    
    if(!onenet_connected)
    {
        printf("OneNet未连接，请先连接!\r\n");
        return;
    }
    
    // 发送MQTT数据
    sprintf(cmd, "AT+MQTTPUB=0,\"$dp\",\"%s\",0,0", data);
    esp8266_cmd_send(cmd, "OK", 3000);
    
    printf("数据发送成功: %s\r\n", data);
}

//////////////////////////////////////////////////////////////////////////////////
// 发送传感器数据到OneNet (格式: datastream,value)
//////////////////////////////////////////////////////////////////////////////////
void esp8266_send_sensor_data(char *datastream, float value)
{
    char data[100];
    
    // OneNet数据格式
    // {"datastreams":[{"id":"temperature","datapoints":[{"value":25.5}]}]}
    sprintf(data, "{\"datastreams\":[{\"id\":\"%s\",\"datapoints\":[{\"value\":%.2f}]}]}", 
            datastream, value);
    
    esp8266_send_data(data, strlen(data));
}

//////////////////////////////////////////////////////////////////////////////////
// 发送多个数据点
//////////////////////////////////////////////////////////////////////////////////
void esp8266_send_multi_data(float pm)
{
    char data[200];
    
    sprintf(data, "{\"id\":\"2573507846\",\"params\":{""\"pm\":{\"value\":%.1f}}}",pm);
    
    esp8266_send_data(data, strlen(data));
}

//////////////////////////////////////////////////////////////////////////////////
// ESP8266数据处理（在串口3中断中调用）
//////////////////////////////////////////////////////////////////////////////////
void esp8266_data_handle(u8 res)
{
    if(esp_rx_len < 300)
    {
        esp_rx_buffer[esp_rx_len++] = res;
        
        // 检测到换行或超时后认为一帧数据结束
        if(res == '\n')
        {
            esp_rx_flag = 1;
        }
    }
    else
    {
        esp_clear_buffer();
    }
}
