#ifndef __DHT11_H
#define __DHT11_H

#include "sys.h"

/*
 * DHT11引脚定义
 * 数据脚接PG9
 * 通过输入输出切换完成时序通信
 */
#define DHT11_IO_IN()  {GPIOG->MODER &= ~(3 << (9 * 2)); GPIOG->MODER |= 0 << (9 * 2);}
#define DHT11_IO_OUT() {GPIOG->MODER &= ~(3 << (9 * 2)); GPIOG->MODER |= 1 << (9 * 2);}
#define DHT11_DQ_OUT   PGout(9)   // 数据输出脚
#define DHT11_DQ_IN    PGin(9)    // 数据输入脚

/*
 * 对外接口函数
 */
u8 DHT11_Init(void);                 // 初始化DHT11
u8 DHT11_Read_Data(u8 *temp,u8 *humi); // 读取温湿度
u8 DHT11_Read_Byte(void);            // 读取一个字节
u8 DHT11_Read_Bit(void);             // 读取一个位
u8 DHT11_Check(void);                // 检查模块响应
void DHT11_Rst(void);                // 复位DHT11

#endif
