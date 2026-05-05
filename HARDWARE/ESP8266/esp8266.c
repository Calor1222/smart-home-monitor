#include "esp8266.h"
#include "usart3.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * 鎺ユ敹缂撳啿鍖?
 * 鐢ㄤ簬淇濆瓨妯″潡瀹屾暣杩斿洖鍐呭
 */
u8 esp_rx_buffer[ESP8266_RX_BUFFER_SIZE];   // ESP8266鎺ユ敹缂撳啿鍖?
u8 esp_rx_flag = 0;                         // 鎺ユ敹瀹屾垚鏍囧織
u16 esp_rx_len = 0;                         // 鎺ユ敹鏁版嵁闀垮害

// 杩炴帴鐘舵€佹爣蹇?
u8 wifi_connected = 0;          // WiFi杩炴帴鐘舵€?
u8 onenet_connected = 0;        // OneNet杩炴帴鐘舵€?

/*
 * 绠€鍗曡蒋浠跺欢鏃?
 * 鐢ㄤ簬AT鍛戒护绛夊緟鏈熼棿
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
 * 娓呯┖鎺ユ敹缂撳啿鍖?
 * 鍙戦€佹柊鍛戒护鍓嶈皟鐢?
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
  * @brief  涓插彛3鍙戦€佸瓧绗︿覆
  * @param  str: 寰呭彂閫佸瓧绗︿覆
  */
void USART3_SendString(char *str)
{
    while(*str)
    {
        while((USART3->SR & 0X40) == 0);  // 绛夊緟鍙戦€佸畬鎴?
        USART3->DR = (u8)(*str++);         // 鍙戦€佹暟鎹?
    }
}

/*
 * 绛夊緟涓や釜鍏抽敭瀛椾腑鐨勪换鎰忎竴涓?
 * 鐢ㄤ簬鍒ゆ柇AT鍛戒护鏄惁鎴愬姛
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
 * 绛夊緟涓変釜鍏抽敭瀛椾腑鐨勪换鎰忎竴涓?
 * 鐢ㄤ簬鍙戝竷鏁版嵁鍚庣殑杩斿洖鍒ゆ柇
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
 * 绛夊緟MQTT杩炴帴缁撴灉
 * 鍚屾椂鍒ゆ柇鎴愬姛鍜屽け璐ユ爣蹇? */
static u8 esp8266_wait_mqttconn(u16 timeout)
{
    u16 elapsed = 0;

    while(elapsed < timeout)
    {
        esp_delay_ms(20);
        elapsed += 20;

        if(strstr((char*)esp_rx_buffer, "+MQTTCONNECTED") != NULL ||
           strstr((char*)esp_rx_buffer, "+MQTTCONN:0,0,0") != NULL ||
           strstr((char*)esp_rx_buffer, "\r\nOK\r\n") != NULL)
        {
            return 1;
        }

        if(strstr((char*)esp_rx_buffer, "+MQTTDISCONNECTED") != NULL ||
           strstr((char*)esp_rx_buffer, "\r\nERROR\r\n") != NULL ||
           strstr((char*)esp_rx_buffer, "\r\nFAIL\r\n") != NULL)
        {
            return 0;
        }
    }

    printf("ESP8266 FULL BUFFER: %s\r\n", esp_rx_buffer);
    return 0;
}

/*
 * 鍙戦€丄T鍛戒护骞剁瓑寰呰繑鍥?
 * 鏀寔涓や釜鎴愬姛鍏抽敭瀛?
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
  * @brief  鍙戦€丄T鍛戒护
  * @param  cmd: 鍛戒护瀛楃涓?
  * @param  ack: 鎴愬姛鍏抽敭瀛?
  * @param  timeout: 瓒呮椂鏃堕棿
  * @retval 1鎴愬姛锛?澶辫触
  */
u8 esp8266_cmd_send(char *cmd, char *ack, u16 timeout)
{
    return esp8266_cmd_send_multi(cmd, ack, NULL, timeout);
}

/**
  * @brief  妫€鏌ョ紦鍐插尯涓槸鍚﹀寘鍚叧閿瓧
  * @param  ack: 鐩爣鍏抽敭瀛?
  * @retval 1鎵惧埌锛?鏈壘鍒?
  */
u8 esp8266_check_cmd(char *ack)
{
    if(strstr((char*)esp_rx_buffer, ack) != NULL)
        return 1;
    return 0;
}

/**
  * @brief  鍒濆鍖朎SP8266妯″潡
  * @note   瀹屾垚AT娴嬭瘯銆佸叧闂洖鏄俱€佽缃ā寮忓拰鏌ヨ鐗堟湰
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
  * @brief  杩炴帴WiFi
  * @note   杩炴帴鎴愬姛鍚庝細鏌ヨ褰撳墠IP
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
  * @brief  杩炴帴OneNET骞冲彴
  * @note   浠QTT杩炴帴鎴愬姛浣滀负鏈€缁堝垽鏂?
  */
void esp8266_connect_onenet(void)
{
    char cmd[512];
    char log_cmd[512];
    u8 mqtt_connected = 0;
    u8 usercfg_ok = 0;
    
    printf("\r\n========== Connect OneNET start ==========\r\n");
    printf("Server: %s:%s\r\n", ONENET_SERVER, ONENET_PORT);
    printf("Device Name: %s\r\n", DEVICE_NAME);
    
    printf("MQTT USERCFG start\r\n");
    sprintf(cmd, "AT+MQTTUSERCFG=0,1,\"%s\",\"%s\",\"%s\",0,0,\"\"", 
            DEVICE_NAME, PRODUCT_ID, ONENET_MQTT_TOKEN);
    sprintf(log_cmd, "AT+MQTTUSERCFG=0,1,\"%s\",\"%s\",\"%.4s****%.4s\",0,0,\"\"",
            DEVICE_NAME, PRODUCT_ID, ONENET_MQTT_TOKEN, ONENET_MQTT_TOKEN + strlen(ONENET_MQTT_TOKEN) - 4);
    printf("MQTT USERCFG CMD: %s\r\n", log_cmd);
    esp_clear_buffer();
    USART3_SendString(cmd);
    USART3_SendString("\r\n");
    usercfg_ok = esp8266_wait_ack("OK", NULL, 10000);
    if(usercfg_ok)
    {
        printf("ESP8266 CMD OK: MQTTUSERCFG\r\n");
        printf("MQTT USERCFG success\r\n");
    }
    else
    {
        printf("ESP8266 CMD ERROR: MQTTUSERCFG\r\n");
        printf("MQTT USERCFG fail\r\n");
    }
    
    esp_delay_ms(500);

    esp8266_cmd_send("AT+CWJAP?", "OK", 3000);
    esp8266_cmd_send("AT+CIPSTATUS", "OK", 3000);
    
    printf("MQTT CONN start\r\n");
    sprintf(cmd, "AT+MQTTCONN=0,\"%s\",%s,1", ONENET_SERVER, ONENET_PORT);
    printf("MQTT CONN CMD: %s\r\n", cmd);

    esp_clear_buffer();
    printf("Send CMD: %s\r\n", cmd);
    USART3_SendString(cmd);
    USART3_SendString("\r\n");

    mqtt_connected = esp8266_wait_mqttconn(15000);
    if(mqtt_connected)
    {
        onenet_connected = 1;
        printf("ESP8266 CMD OK: %s\r\n", cmd);
        printf("MQTT CONN success\r\n");
        printf("Connect OneNET done\r\n");
        esp8266_subscribe_property_reply();
    }
    else
    {
        onenet_connected = 0;
        printf("ESP8266 CMD ERROR: %s\r\n", cmd);
        printf("MQTT CONN fail\r\n");
        printf("MQTT disconnected, stop subscribe and publish\r\n");
        printf("Connect OneNET ERROR\r\n");
        esp_delay_ms(2000);
        return;
    }
}

/**
  * @brief  璁㈤槄灞炴€т笂鎶ュ洖鎵т富棰?
  * @note   鐢ㄤ簬鎺ユ敹OneNET杩斿洖鐨刢ode鍜宮sg
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
  * @brief  鍙戝竷MQTT鍘熷鏁版嵁
  * @param  topic: 鍙戝竷涓婚
  * @param  payload: 鍙戝竷鍐呭
  */
void esp8266_mqtt_publish(const char *topic, const char *payload)
{
    char cmd[256];
    u16 payload_len;

    if(!onenet_connected)
    {
        printf("MQTT not connected, skip publish\r\n");
        return;
    }

    payload_len = strlen(payload);
    sprintf(cmd, "AT+MQTTPUBRAW=0,\"%s\",%d,0,0", topic, payload_len);

    if(!esp8266_cmd_send(cmd, ">", 3000))
        return;

    esp_clear_buffer();
    USART3_SendString((char *)payload);

    if(esp8266_wait_ack3("OK", "+MQTTPUB:OK", "+MQTTSUBRECV", 10000))
        printf("ESP8266 CMD OK: payload\r\n");
    else
        printf("ESP8266 CMD ERROR: payload\r\n");
}

/**
  * @brief  涓婃姤浼犳劅鍣ㄥ睘鎬у埌OneNET
  * @param  temp: 娓╁害鍊?
  * @param  hum: 婀垮害鍊?
  * @param  smoke: 鐑熼浘鍊?
  * @param  pm: PM2.5鍊?
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
  * @brief  鍙戦€佹棫鐗堟暟鎹埌OneNET
  * @param  data: 鏁版嵁鍐呭
  * @param  len: 鏁版嵁闀垮害
  */
void esp8266_send_data(char *data, u16 len)
{
    char cmd[100];
    
    if(!onenet_connected)
    {
        printf("OneNet鏈繛鎺ワ紝璇峰厛杩炴帴!\r\n");
        return;
    }
    
    // 鍙戦€丮QTT鏁版嵁
    sprintf(cmd, "AT+MQTTPUB=0,\"$dp\",\"%s\",0,0", data);
    esp8266_cmd_send(cmd, "OK", 3000);
    
    printf("鏁版嵁鍙戦€佹垚鍔? %s\r\n", data);
}

/**
  * @brief  鍙戦€佸崟涓紶鎰熷櫒鏁版嵁
  * @param  datastream: 鏁版嵁娴佸悕绉?
  * @param  value: 鏁版嵁鍊?
  */
void esp8266_send_sensor_data(char *datastream, float value)
{
    char data[100];
    
    // OneNet鏁版嵁鏍煎紡
    // {"datastreams":[{"id":"temperature","datapoints":[{"value":25.5}]}]}
    sprintf(data, "{\"datastreams\":[{\"id\":\"%s\",\"datapoints\":[{\"value\":%.2f}]}]}", 
            datastream, value);
    
    esp8266_send_data(data, strlen(data));
}

/**
  * @brief  鍙戦€佸涓暟鎹偣
  * @param  pm: PM鏁版嵁
  */
void esp8266_send_multi_data(float pm)
{
    char data[200];
    
    sprintf(data, "{\"id\":\"2573507846\",\"params\":{""\"pm\":{\"value\":%.1f}}}",pm);
    
    esp8266_send_data(data, strlen(data));
}

/**
  * @brief  澶勭悊涓插彛3鎺ユ敹鍒扮殑瀛楄妭
  * @param  res: 褰撳墠鎺ユ敹鍒扮殑瀛楄妭
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
