#ifndef __ESP8266_H
#define __ESP8266_H

#include "sys.h"

/*
 * ESP8266模块参数定义
 * 使用串口3与模块通信
 */

// ESP8266工作模式
#define ESP_MODE_STATION     1   //  Station模式
#define ESP_MODE_SOFTAP      2   //  AP模式
#define ESP_MODE_BOTH        3   //  混合模式

// OneNet配置信息
#define ONENET_SERVER        "mqtts.heclouds.com"     // OneNet服务器地址
#define ONENET_PORT          "1883"                    // OneNet MQTT端口
#define PRODUCT_ID           "gi50cD23hM"              // OneNet产品ID
#define DEVICE_NAME          "esp8266"                 // OneNet设备名称
#define DEVICE_TOKEN         "version=2018-10-31&res=products%2Fgi50cD23hM%2Fdevices%2Fesp8266&et=2192672778&method=md5&sign=QQwNqUaUd0d5I5sOk3ocIg%3D%3D"

// 兼容旧命名
#define ONENET_DEVICE_ID     DEVICE_NAME
#define ONENET_USERNAME      PRODUCT_ID
#define ONENET_PASSWORD      DEVICE_TOKEN

/*
 * OneNET属性定义
 */
#define PROPERTY_TEMP        "Temp"
#define PROPERTY_HUM         "Hum"
#define PROPERTY_SMOKE       "Smoke"
#define PROPERTY_PM          "pm"

#define ONENET_PROPERTY_POST_TOPIC "$sys/" PRODUCT_ID "/" DEVICE_NAME "/thing/property/post"
#define ONENET_PROPERTY_POST_REPLY_TOPIC "$sys/" PRODUCT_ID "/" DEVICE_NAME "/thing/property/post/reply"

// WiFi配置（请修改为你的WiFi信息）
#define WIFI_SSID            "LjjNB"             // WiFi名称
#define WIFI_PASSWORD        "Lzd2Lym222"        // WiFi密码

// ESP8266指令超时时间(ms)
#define ESP_CMD_TIMEOUT      5000
#define ESP8266_RX_BUFFER_SIZE 2048

/*
 * 外部状态变量
 */
extern u8 wifi_connected;      // WiFi连接状态
extern u8 onenet_connected;    // OneNet连接状态

/*
 * 对外接口函数
 */
void esp8266_init(void);                                        // ESP8266初始化
u8   esp8266_cmd_send(char *cmd, char *ack, u16 timeout);       // 发送AT指令
u8   esp8266_check_cmd(char *ack);                              // 检查指令返回
void esp8266_connect_wifi(void);                                // 连接WiFi
void esp8266_connect_onenet(void);                              // 连接OneNet
void esp8266_send_data(char *data, u16 len);                    // 发送数据
void esp8266_receive_data(void);                                // 接收数据处理
void esp8266_send_sensor_data(char *datastream, float value);   // 发送传感器数据
void esp8266_send_multi_data(float pm);           // 发送多个数据
void esp8266_mqtt_publish(const char *topic, const char *payload);
void esp8266_onenet_post_property(float temp, float hum, float smoke, float pm);
void esp8266_subscribe_property_reply(void);
void USART3_SendString(char *str);                              // 串口3发送字符串
void esp8266_data_handle(u8 res);                               // ESP8266数据处理

#endif
