#include "esp8266.h"
#include "usart3.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * 接收缓冲区
 * 用于保存模块完整返回内容
 */
u8 esp_rx_buffer[ESP8266_RX_BUFFER_SIZE];   // ESP8266接收缓冲区
u8 esp_rx_flag = 0;                         // 接收完成标志
u16 esp_rx_len = 0;                         // 接收数据长度

// 连接状态标志
u8 wifi_connected = 0;          // WiFi连接状态
u8 onenet_connected = 0;        // OneNet连接状态

/*
 * 简单软件延时
 * 用于AT命令等待期间
 */
void esp_delay_ms(u16 nms)
{
    u32 i;
    while(nms--)
    {
        i = 10000;
        while(i--);
    }
}

/*
 * 清空接收缓冲区
 * 发送新命令前调用
 */
void esp_clear_buffer(void)
{
    u16 i;
    for(i = 0; i < ESP8266_RX_BUFFER_SIZE; i++)
        esp_rx_buffer[i] = 0;
    esp_rx_len = 0;
    esp_rx_flag = 0;
}

/**
  * @brief  串口3发送字符串
  * @param  str: 待发送字符串
  */
void USART3_SendString(char *str)
{
    while(*str)
    {
        while((USART3->SR & 0X40) == 0);  // 等待发送完成
        USART3->DR = (u8)(*str++);         // 发送数据
    }
}

/*
 * 等待两个关键字中的任意一个
 * 用于判断AT命令是否成功
 */
static u8 esp8266_wait_ack(const char *ack1, const char *ack2, u16 timeout)
{
    u16 elapsed = 0;

    while(elapsed < timeout)
    {
        esp_delay_ms(20);
        elapsed += 20;

        if((ack1 && strstr((char*)esp_rx_buffer, ack1) != NULL) ||
           (ack2 && strstr((char*)esp_rx_buffer, ack2) != NULL))
        {
            return 1;
        }

        if(strstr((char*)esp_rx_buffer, "\r\nERROR\r\n") != NULL ||
           strstr((char*)esp_rx_buffer, "\r\nFAIL\r\n") != NULL)
            return 0;
    }

    printf("ESP8266 FULL BUFFER: %s\r\n", esp_rx_buffer);

    return 0;
}

/*
 * 等待三个关键字中的任意一个
 * 用于发布数据后的返回判断
 */
static u8 esp8266_wait_ack3(const char *ack1, const char *ack2, const char *ack3, u16 timeout)
{
    u16 elapsed = 0;

    while(elapsed < timeout)
    {
        esp_delay_ms(20);
        elapsed += 20;

        if((ack1 && strstr((char*)esp_rx_buffer, ack1) != NULL) ||
           (ack2 && strstr((char*)esp_rx_buffer, ack2) != NULL) ||
           (ack3 && strstr((char*)esp_rx_buffer, ack3) != NULL))
        {
            return 1;
        }

        if(strstr((char*)esp_rx_buffer, "\r\nERROR\r\n") != NULL ||
           strstr((char*)esp_rx_buffer, "\r\nFAIL\r\n") != NULL)
            return 0;
    }

    printf("ESP8266 FULL BUFFER: %s\r\n", esp_rx_buffer);

    return 0;
}

/*
 * 发送AT命令并等待返回
 * 支持两个成功关键字
 */
static u8 esp8266_cmd_send_multi(char *cmd, const char *ack1, const char *ack2, u16 timeout)
{
    u8 ok;

    esp_clear_buffer();

    printf("Send CMD: %s\r\n", cmd);
    USART3_SendString(cmd);
    USART3_SendString("\r\n");

    ok = esp8266_wait_ack(ack1, ack2, timeout);
    if(ok)
        printf("ESP8266 CMD OK: %s\r\n", cmd);
    else
        printf("ESP8266 CMD ERROR: %s\r\n", cmd);

    return ok;
}

/**
  * @brief  发送AT命令
  * @param  cmd: 命令字符串
  * @param  ack: 成功关键字
  * @param  timeout: 超时时间
  * @retval 1成功，0失败
  */
u8 esp8266_cmd_send(char *cmd, char *ack, u16 timeout)
{
    return esp8266_cmd_send_multi(cmd, ack, NULL, timeout);
}

/**
  * @brief  检查缓冲区中是否包含关键字
  * @param  ack: 目标关键字
  * @retval 1找到，0未找到
  */
u8 esp8266_check_cmd(char *ack)
{
    if(strstr((char*)esp_rx_buffer, ack) != NULL)
        return 1;
    return 0;
}

/**
  * @brief  初始化ESP8266模块
  * @note   完成AT测试、关闭回显、设置模式和查询版本
  */
void esp8266_init(void)
{
    printf("\r\n========== ESP8266 init start ==========\r\n");
    
    esp8266_cmd_send("AT", "OK", 3000);
    esp_delay_ms(500);
    
    esp8266_cmd_send("ATE0", "OK", 3000);
    esp_delay_ms(500);
    
    esp8266_cmd_send("AT+CWMODE=1", "OK", 3000);
    esp_delay_ms(500);
    
    esp8266_cmd_send("AT+RST", "ready", 5000);
    esp_delay_ms(3000);
    
    esp8266_cmd_send("AT+GMR", "OK", 3000);
    
    printf("========== ESP8266 init done ==========\r\n");
}

/**
  * @brief  连接WiFi
  * @note   连接成功后会查询当前IP
  */
void esp8266_connect_wifi(void)
{
    char cmd[256];
    
    printf("\r\n========== WiFi connect start ==========\r\n");
    printf("SSID: %s\r\n", WIFI_SSID);
    printf("PWD:  %s\r\n", WIFI_PASSWORD);
    
    sprintf(cmd, "AT+CWJAP=\"%s\",\"%s\"", WIFI_SSID, WIFI_PASSWORD);
    if(esp8266_cmd_send_multi(cmd, "WIFI GOT IP", "OK", 20000))
        wifi_connected = 1;
    
    esp8266_cmd_send("AT+CIFSR", "OK", 2000);
    
    printf("WiFi connect done\r\n");
}

/**
  * @brief  连接OneNET平台
  * @note   以MQTT连接成功作为最终判断
  */
void esp8266_connect_onenet(void)
{
    char cmd[512];
    u8 mqtt_connected = 0;
    
    printf("\r\n========== Connect OneNET start ==========\r\n");
    printf("Server: %s:%s\r\n", ONENET_SERVER, ONENET_PORT);
    printf("Device Name: %s\r\n", DEVICE_NAME);
    
    sprintf(cmd, "AT+MQTTUSERCFG=0,1,\"%s\",\"%s\",\"%s\",0,0,\"\"", 
            DEVICE_NAME, PRODUCT_ID, DEVICE_TOKEN);
    esp8266_cmd_send(cmd, "OK", 5000);
    
    esp_delay_ms(500);
    
    sprintf(cmd, "AT+MQTTCONN=0,\"%s\",%s,1", ONENET_SERVER, ONENET_PORT);
    mqtt_connected = esp8266_cmd_send_multi(cmd, "+MQTTCONNECTED", "OK", 10000);
    if(mqtt_connected)
        onenet_connected = 1;
    
    printf("Connect OneNET done\r\n");
    esp8266_subscribe_property_reply();
}

/**
  * @brief  订阅属性上报回执主题
  * @note   用于接收OneNET返回的code和msg
  */
void esp8266_subscribe_property_reply(void)
{
    char cmd[256];

    sprintf(cmd, "AT+MQTTSUB=0,\"%s\",0", ONENET_PROPERTY_POST_REPLY_TOPIC);
    if(esp8266_cmd_send(cmd, "OK", 10000))
        printf("Subscribe reply topic OK: %s\r\n", ONENET_PROPERTY_POST_REPLY_TOPIC);
    else
        printf("Subscribe reply topic ERROR: %s\r\n", ONENET_PROPERTY_POST_REPLY_TOPIC);

    delay_ms(1000);
}

/**
  * @brief  发布MQTT原始数据
  * @param  topic: 发布主题
  * @param  payload: 发布内容
  */
void esp8266_mqtt_publish(const char *topic, const char *payload)
{
    char cmd[256];

    sprintf(cmd, "AT+MQTTPUBRAW=0,\"%s\",%d,0,0", topic, strlen(payload));

    esp8266_cmd_send(cmd, ">", 3000);
    esp_clear_buffer();
    USART3_SendString((char *)payload);

    if(esp8266_wait_ack3("OK", "+MQTTPUB:OK", "+MQTTSUBRECV", 10000))
        printf("ESP8266 CMD OK: payload\r\n");
    else
        printf("ESP8266 CMD ERROR: payload\r\n");
}

/**
  * @brief  上报传感器属性到OneNET
  * @param  temp: 温度值
  * @param  hum: 湿度值
  * @param  smoke: 烟雾值
  * @param  pm: PM2.5值
  */
void esp8266_onenet_post_property(float temp, float hum, float smoke, float pm)
{
    char payload[320];
    int smoke_value = (int)(smoke + 0.5f);
    float pm_upload = ((int)(pm * 10 + 0.5f)) / 10.0f;

    sprintf(payload,
        "{\"id\":\"1\",\"version\":\"1.0\",\"params\":{"
        "\"Temp\":{\"value\":%.2f},"
        "\"Hum\":{\"value\":%.2f},"
        "\"Smoke\":{\"value\":%d},"
        "\"%s\":{\"value\":%.1f}"
        "}}",
        temp, hum, smoke_value,
        PROPERTY_PM, pm_upload);

    esp8266_mqtt_publish(ONENET_PROPERTY_POST_TOPIC, payload);
    delay_ms(5000);
    printf("ESP8266 AFTER PUBLISH BUFFER: %s\r\n", esp_rx_buffer);
}

/**
  * @brief  发送旧版数据到OneNET
  * @param  data: 数据内容
  * @param  len: 数据长度
  */
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

/**
  * @brief  发送单个传感器数据
  * @param  datastream: 数据流名称
  * @param  value: 数据值
  */
void esp8266_send_sensor_data(char *datastream, float value)
{
    char data[100];
    
    // OneNet数据格式
    // {"datastreams":[{"id":"temperature","datapoints":[{"value":25.5}]}]}
    sprintf(data, "{\"datastreams\":[{\"id\":\"%s\",\"datapoints\":[{\"value\":%.2f}]}]}", 
            datastream, value);
    
    esp8266_send_data(data, strlen(data));
}

/**
  * @brief  发送多个数据点
  * @param  pm: PM数据
  */
void esp8266_send_multi_data(float pm)
{
    char data[200];
    
    sprintf(data, "{\"id\":\"2573507846\",\"params\":{""\"pm\":{\"value\":%.1f}}}",pm);
    
    esp8266_send_data(data, strlen(data));
}

/**
  * @brief  处理串口3接收到的字节
  * @param  res: 当前接收到的字节
  */
void esp8266_data_handle(u8 res)
{
    if(esp_rx_len < sizeof(esp_rx_buffer) - 1)
    {
        esp_rx_buffer[esp_rx_len++] = res;
        esp_rx_buffer[esp_rx_len] = '\0';

        if(res == '\n')
            esp_rx_flag = 1;
    }
    else
    {
        esp_rx_buffer[esp_rx_len] = '\0';
        esp_rx_flag = 1;
    }
}
