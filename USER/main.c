#include "stm32f4xx.h"
#include "delay.h"
#include "usart.h"
#include "usart3.h"
#include "esp8266.h"
#include "dht11.h"
#include "mq2.h"
#include "gp2y.h"
#include "stepmotor.h"
#include "fan.h"
#include "atomizer.h"
#include "lcd.h"
#include <stdio.h>

#define LCD_TEXT_X         10
#define LCD_VALUE_X        92
#define LCD_TEXT_WIDTH     220
#define LCD_LINE_HEIGHT    16
#define LCD_FONT_SIZE      16
#define LCD_LINE_GAP       22
#define LCD_VALUE_WIDTH    180
#define LCD_BG_COLOR       WHITE
#define LCD_FG_COLOR       BLACK

#define MOTOR_OFF          0
#define MOTOR_ON           1
#define MOTOR_ON_TEMP      30
#define MOTOR_OFF_TEMP     28
#define MOTOR_STEP_COUNT   1
#define MOTOR_STEP_DELAY   1

#define FAN_OFF_STATE      0
#define FAN_ON_STATE       1
#define FAN_ON_PM          75.0f
#define FAN_OFF_PM         40.0f

/*
 * 刷新LCD一行内容
 * 先清除旧内容再显示新数据
 */
static void LCD_ShowLineValue(u16 y, const char *value)
{
    LCD_Fill(LCD_VALUE_X, y, LCD_VALUE_X + LCD_VALUE_WIDTH, y + LCD_LINE_HEIGHT, LCD_BG_COLOR);
    LCD_ShowString(LCD_VALUE_X, y, LCD_VALUE_WIDTH, LCD_LINE_HEIGHT, LCD_FONT_SIZE, (u8 *)value);
}

/*
 * 初始化LCD界面
 * 显示固定标题和字段名
 */
static void LCD_ShowMainFrame(void)
{
    POINT_COLOR = LCD_FG_COLOR;
    BACK_COLOR = LCD_BG_COLOR;

    LCD_Init();
    LCD_Clear(LCD_BG_COLOR);

    LCD_ShowString(LCD_TEXT_X, 10, LCD_TEXT_WIDTH, LCD_LINE_HEIGHT, LCD_FONT_SIZE, (u8 *)"WiFi :");
    LCD_ShowString(LCD_TEXT_X, 10 + LCD_LINE_GAP, LCD_TEXT_WIDTH, LCD_LINE_HEIGHT, LCD_FONT_SIZE, (u8 *)"MQTT :");
    LCD_ShowString(LCD_TEXT_X, 10 + LCD_LINE_GAP * 2, LCD_TEXT_WIDTH, LCD_LINE_HEIGHT, LCD_FONT_SIZE, (u8 *)"Motor:");
    LCD_ShowString(LCD_TEXT_X, 10 + LCD_LINE_GAP * 3, LCD_TEXT_WIDTH, LCD_LINE_HEIGHT, LCD_FONT_SIZE, (u8 *)"Fan  :");
    LCD_ShowString(LCD_TEXT_X, 10 + LCD_LINE_GAP * 4, LCD_TEXT_WIDTH, LCD_LINE_HEIGHT, LCD_FONT_SIZE, (u8 *)"Temp :");
    LCD_ShowString(LCD_TEXT_X, 10 + LCD_LINE_GAP * 5, LCD_TEXT_WIDTH, LCD_LINE_HEIGHT, LCD_FONT_SIZE, (u8 *)"Hum  :");
    LCD_ShowString(LCD_TEXT_X, 10 + LCD_LINE_GAP * 6, LCD_TEXT_WIDTH, LCD_LINE_HEIGHT, LCD_FONT_SIZE, (u8 *)"Smoke:");
    LCD_ShowString(LCD_TEXT_X, 10 + LCD_LINE_GAP * 7, LCD_TEXT_WIDTH, LCD_LINE_HEIGHT, LCD_FONT_SIZE, (u8 *)"PM2.5:");
}

/*
 * 刷新网络状态
 * 显示当前连接阶段
 */
static void LCD_UpdateNetStatusText(const char *wifi_text, const char *mqtt_text)
{
    LCD_ShowLineValue(10, wifi_text);
    LCD_ShowLineValue(10 + LCD_LINE_GAP, mqtt_text);
}

/*
 * 刷新网络状态
 * 根据连接结果显示状态
 */
static void LCD_UpdateNetStatus(void)
{
    LCD_ShowLineValue(10, wifi_connected ? "OK" : "FAIL");
    LCD_ShowLineValue(10 + LCD_LINE_GAP, onenet_connected ? "OK" : "FAIL");
}

/*
 * 刷新电机状态
 * 显示当前电机开关状态
 */
static void LCD_UpdateMotorStatus(u8 motor_status)
{
    LCD_ShowLineValue(10 + LCD_LINE_GAP * 2, motor_status == MOTOR_ON ? "ON" : "OFF");
}

/*
 * 刷新风扇状态
 * 显示当前风扇开关状态
 */
static void LCD_UpdateFanStatus(u8 fan_status)
{
    LCD_ShowLineValue(10 + LCD_LINE_GAP * 3, fan_status == FAN_ON_STATE ? "ON" : "OFF");
}

/*
 * 刷新传感器数据
 * 显示温湿度烟雾和PM2.5
 */
static void LCD_UpdateSensorData(u8 temp, u8 hum, float smoke, float pm)
{
    char line[32];

    sprintf(line, "%d C", temp);
    LCD_ShowLineValue(10 + LCD_LINE_GAP * 4, line);

    sprintf(line, "%d %%", hum);
    LCD_ShowLineValue(10 + LCD_LINE_GAP * 5, line);

    sprintf(line, "%.2f ppm", smoke);
    LCD_ShowLineValue(10 + LCD_LINE_GAP * 6, line);

    if (pm > 150.0f)
        LCD_ShowLineValue(10 + LCD_LINE_GAP * 7, "DANGER");
    else if (pm > 75.0f)
        LCD_ShowLineValue(10 + LCD_LINE_GAP * 7, "HIGH");
    else
    {
        sprintf(line, "%.1f ug/m3", pm);
        LCD_ShowLineValue(10 + LCD_LINE_GAP * 7, line);
    }
}

/*
 * 更新电机控制状态
 * 温度过高开启 温度恢复后关闭
 */
static void Motor_UpdateStatus(u8 temp, u8 *motor_status)
{
    if (*motor_status == MOTOR_OFF && temp >= MOTOR_ON_TEMP)
    {
        *motor_status = MOTOR_ON;
        printf("Motor ON: high temperature\r\n");
    }
    else if (*motor_status == MOTOR_ON && temp < MOTOR_OFF_TEMP)
    {
        *motor_status = MOTOR_OFF;
        printf("Motor OFF: temperature normal\r\n");
    }
}

/*
 * 更新风扇控制状态
 * PM2.5过高开启 恢复后关闭
 */
static void Fan_UpdateStatus(float pm, u8 *fan_status)
{
    if (*fan_status == FAN_OFF_STATE && pm > FAN_ON_PM)
    {
        *fan_status = FAN_ON_STATE;
        Fan_On();
        printf("Fan ON: PM2.5 high\r\n");
    }
    else if (*fan_status == FAN_ON_STATE && pm < FAN_OFF_PM)
    {
        *fan_status = FAN_OFF_STATE;
        Fan_Off();
        printf("Fan OFF: PM2.5 normal\r\n");
    }
}

/*
 * 驱动步进电机转动
 * 每轮只转少量步数避免阻塞
 */
static void Motor_RunIfNeeded(u8 motor_status)
{
    if (motor_status == MOTOR_ON)
        Stepper_Rotate_Clockwise(MOTOR_STEP_COUNT, MOTOR_STEP_DELAY);
}

/*
 * 主函数流程
 * 负责采集温湿度烟雾和PM2.5并上传OneNET
 */
int main(void)
{
    u8 temp = 0;                 // 温度值
    u8 hum = 0;                  // 湿度值
    float smoke = 0.0f;          // 烟雾浓度
    float pm = 0.0f;             // PM2.5浓度
    u16 gp2y_raw = 0;            // GP2Y原始ADC值
    float gp2y_voltage = 0.0f;   // GP2Y输出电压
    u8 dht11_status;             // DHT11初始化结果
    u8 motor_status = MOTOR_OFF; // 电机状态
    u8 fan_status = FAN_OFF_STATE;// 风扇状态

    /*
     * 初始化基础外设
     * 串口1用于调试输出 串口3用于ESP8266通信
     */
    delay_init(168);
    uart_init(115200);
    uart3_init(115200);
    esp8266_init();
    Atomizer_Init();
    Stepper_Init();
    Fan_Init();
    Fan_Off();
    LCD_ShowMainFrame();
    LCD_UpdateNetStatusText("Connecting", "Connecting");
    LCD_UpdateMotorStatus(motor_status);
    LCD_UpdateFanStatus(fan_status);

    /*
     * 初始化传感器
     */
    dht11_status = DHT11_Init();
    MQ2_Init();
    GP2Y_Init();

    printf("\r\nSystem start\r\n");
    if (dht11_status == 0)
        printf("DHT11 init success\r\n");
    else
        printf("DHT11 init failed\r\n");

    /*
     * 连接网络
     */
    LCD_UpdateNetStatusText("Connecting", "Connecting");
    esp8266_connect_wifi();
    LCD_ShowLineValue(10, wifi_connected ? "OK" : "FAIL");
    LCD_ShowLineValue(10 + LCD_LINE_GAP, "Connecting");
    esp8266_connect_onenet();
    LCD_UpdateNetStatus();

    /*
     * MQ2需要预热后再校准
     */
    printf("MQ2 warm up...\r\n");
    delay_ms(20000);
    printf("MQ2 calibrating...\r\n");
    MQ2_Calibrate();
    printf("MQ2 calibration done\r\n");

    while (1)
    {
        /*
         * 刷新网络状态
         */
        LCD_UpdateNetStatus();

        /*
         * DHT11读取失败时跳过本轮上传
         */
        if (DHT11_Read_Data(&temp, &hum) != 0)
        {
            printf("DHT11 read failed, skip upload\r\n");
            delay_ms(5000);
            continue;
        }

        /*
         * 更新电机状态
         */
        Motor_UpdateStatus(temp, &motor_status);
        LCD_UpdateMotorStatus(motor_status);

        /*
         * 读取烟雾和PM2.5数据
         */
        smoke = MQ2_GetPPM();
        gp2y_raw = GP2Y_ReadRaw();
        gp2y_voltage = gp2y_raw * 3300.0f / 4096.0f * 11.0f;
        pm = GP2Y_ReadDustAverage_ugm3(5);

        /*
         * 更新风扇状态
         */
        Fan_UpdateStatus(pm, &fan_status);
        LCD_UpdateFanStatus(fan_status);

        /*
         * 打印当前采集结果
         */
        printf("Temp=%d C, Hum=%d %%, Smoke=%.2f ppm, PM=%.2f ug/m3\r\n",
               temp, hum, smoke, pm);
        printf("GP2Y RAW=%4d, V=%.0f mV, PM=%.2f ug/m3\r\n",
               gp2y_raw, gp2y_voltage, pm);

        /*
         * 刷新LCD数据
         */
        LCD_UpdateSensorData(temp, hum, smoke, pm);

        /*
         * 上传到OneNET
         */
        esp8266_onenet_post_property((float)temp, (float)hum, smoke, pm);

        /*
         * 电机开启时执行降温动作
         */
        Motor_RunIfNeeded(motor_status);

        delay_ms(5000);
    }
}
