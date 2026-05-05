#ifndef __ESP8266_H
#define __ESP8266_H

#include "sys.h"

/*
 * ESP8266妯″潡鍙傛暟瀹氫箟
 * 浣跨敤涓插彛3涓庢ā鍧楅€氫俊
 */

// ESP8266宸ヤ綔妯″紡
#define ESP_MODE_STATION     1   //  Station妯″紡
#define ESP_MODE_SOFTAP      2   //  AP妯″紡
#define ESP_MODE_BOTH        3   //  娣峰悎妯″紡

// OneNet閰嶇疆淇℃伅
#define ONENET_SERVER        "183.230.40.96"     // OneNet鏈嶅姟鍣ㄥ湴鍧€
#define ONENET_PORT          "1883"                    // OneNet MQTT绔彛
#define PRODUCT_ID           "gi50cD23hM"              // OneNet浜у搧ID
#define DEVICE_NAME          "esp8266"                 // OneNet璁惧鍚嶇О
#define DEVICE_TOKEN         "YkpTRTlkRDdqclJpN3huQVVTUW1EVkVLb3d0eEJCZ2M="
#define ONENET_MQTT_TOKEN    "version=2018-10-31&res=products%2Fgi50cD23hM%2Fdevices%2Fesp8266&et=1893456000&method=sha1&sign=lk%2Bii7RLBDw%2FuJtd%2BDU3ixJ6KZs%3D"

// 鍏煎鏃у懡鍚?
#define ONENET_DEVICE_ID     DEVICE_NAME
#define ONENET_USERNAME      PRODUCT_ID
#define ONENET_PASSWORD      DEVICE_TOKEN

/*
 * OneNET灞炴€у畾涔?
 */
#define PROPERTY_TEMP        "Temp"
#define PROPERTY_HUM         "Hum"
#define PROPERTY_SMOKE       "Smoke"
#define PROPERTY_PM          "pm"

#define ONENET_PROPERTY_POST_TOPIC "$sys/" PRODUCT_ID "/" DEVICE_NAME "/thing/property/post"
#define ONENET_PROPERTY_POST_REPLY_TOPIC "$sys/" PRODUCT_ID "/" DEVICE_NAME "/thing/property/post/reply"

// WiFi閰嶇疆锛堣淇敼涓轰綘鐨刉iFi淇℃伅锛?
#define WIFI_SSID            "LjjNB"             // WiFi鍚嶇О
#define WIFI_PASSWORD        "Lzd2Lym222"        // WiFi瀵嗙爜

// ESP8266鎸囦护瓒呮椂鏃堕棿(ms)
#define ESP_CMD_TIMEOUT      5000
#define ESP8266_RX_BUFFER_SIZE 2048

/*
 * 澶栭儴鐘舵€佸彉閲?
 */
extern u8 wifi_connected;      // WiFi杩炴帴鐘舵€?
extern u8 onenet_connected;    // OneNet杩炴帴鐘舵€?

/*
 * 瀵瑰鎺ュ彛鍑芥暟
 */
void esp8266_init(void);                                        // ESP8266鍒濆鍖?
u8   esp8266_cmd_send(char *cmd, char *ack, u16 timeout);       // 鍙戦€丄T鎸囦护
u8   esp8266_check_cmd(char *ack);                              // 妫€鏌ユ寚浠よ繑鍥?
void esp8266_connect_wifi(void);                                // 杩炴帴WiFi
void esp8266_connect_onenet(void);                              // 杩炴帴OneNet
void esp8266_send_data(char *data, u16 len);                    // 鍙戦€佹暟鎹?
void esp8266_receive_data(void);                                // 鎺ユ敹鏁版嵁澶勭悊
void esp8266_send_sensor_data(char *datastream, float value);   // 鍙戦€佷紶鎰熷櫒鏁版嵁
void esp8266_send_multi_data(float pm);           // 鍙戦€佸涓暟鎹?
void esp8266_mqtt_publish(const char *topic, const char *payload);
void esp8266_onenet_post_property(float temp, float hum, float smoke, float pm);
void esp8266_subscribe_property_reply(void);
void USART3_SendString(char *str);                              // 涓插彛3鍙戦€佸瓧绗︿覆
void esp8266_data_handle(u8 res);                               // ESP8266鏁版嵁澶勭悊

#endif
