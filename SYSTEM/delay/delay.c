#include "delay.h"
#include "sys.h"

/*
 * OS支持头文件
 * 使用OS时需要包含
 */
#if SYSTEM_SUPPORT_OS
#include "includes.h"
#endif

/*
 * 延时系数
 * fac_us用于us延时
 * fac_ms用于ms延时
 */
static u8  fac_us = 0; // us延时系数
static u16 fac_ms = 0; // ms延时系数

#if SYSTEM_SUPPORT_OS
/*
 * OS状态宏定义
 * 适配UCOSII和UCOSIII
 */
#ifdef OS_CRITICAL_METHOD
#define delay_osrunning      OSRunning
#define delay_ostickspersec  OS_TICKS_PER_SEC
#define delay_osintnesting   OSIntNesting
#endif

#ifdef CPU_CFG_CRITICAL_METHOD
#define delay_osrunning      OSRunning
#define delay_ostickspersec  OSCfg_TickRate_Hz
#define delay_osintnesting   OSIntNestingCtr
#endif

/*
 * 锁定任务调度
 * 防止us延时被打断
 */
void delay_osschedlock(void)
{
#ifdef CPU_CFG_CRITICAL_METHOD
    OS_ERR err;
    OSSchedLock(&err);
#else
    OSSchedLock();
#endif
}

/*
 * 恢复任务调度
 * us延时结束后调用
 */
void delay_osschedunlock(void)
{
#ifdef CPU_CFG_CRITICAL_METHOD
    OS_ERR err;
    OSSchedUnlock(&err);
#else
    OSSchedUnlock();
#endif
}

/*
 * 调用OS延时函数
 * 用于较长时间延时
 */
void delay_ostimedly(u32 ticks)
{
#ifdef CPU_CFG_CRITICAL_METHOD
    OS_ERR err;
    OSTimeDly(ticks, OS_OPT_TIME_PERIODIC, &err);
#else
    OSTimeDly(ticks);
#endif
}

/*
 * SysTick中断处理
 * 使用OS时用于时钟节拍
 */
void SysTick_Handler(void)
{
    if(delay_osrunning == 1)
    {
        OSIntEnter();
        OSTimeTick();
        OSIntExit();
    }
}
#endif

/*
 * 初始化延时模块
 * 根据系统时钟设置延时系数
 */
void delay_init(u8 SYSCLK)
{
#if SYSTEM_SUPPORT_OS
    u32 reload;
#endif

    SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);
    fac_us = SYSCLK / 8;

#if SYSTEM_SUPPORT_OS
    reload = SYSCLK / 8;
    reload *= 1000000 / delay_ostickspersec;
    fac_ms = 1000 / delay_ostickspersec;
    SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
    SysTick->LOAD = reload;
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
#else
    fac_ms = (u16)fac_us * 1000;
#endif
}

#if SYSTEM_SUPPORT_OS
/*
 * 微秒延时
 * 使用OS时会临时锁定调度
 */
void delay_us(u32 nus)
{
    u32 ticks;
    u32 told, tnow, tcnt = 0;
    u32 reload = SysTick->LOAD;

    ticks = nus * fac_us;
    delay_osschedlock();
    told = SysTick->VAL;

    while(1)
    {
        tnow = SysTick->VAL;
        if(tnow != told)
        {
            if(tnow < told)
                tcnt += told - tnow;
            else
                tcnt += reload - tnow + told;
            told = tnow;
            if(tcnt >= ticks)
                break;
        }
    }

    delay_osschedunlock();
}

/*
 * 毫秒延时
 * 优先使用OS延时
 */
void delay_ms(u16 nms)
{
    if(delay_osrunning && delay_osintnesting == 0)
    {
        if(nms >= fac_ms)
        {
            delay_ostimedly(nms / fac_ms);
        }
        nms %= fac_ms;
    }
    delay_us((u32)(nms * 1000));
}
#else
/*
 * 微秒延时
 * 非OS环境直接使用SysTick
 */
void delay_us(u32 nus)
{
    u32 temp;

    SysTick->LOAD = nus * fac_us;
    SysTick->VAL = 0x00;
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
    do
    {
        temp = SysTick->CTRL;
    }while((temp & 0x01) && !(temp & (1 << 16)));
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
    SysTick->VAL = 0X00;
}

/*
 * 毫秒延时基础函数
 * 单次延时不能过长
 */
void delay_xms(u16 nms)
{
    u32 temp;

    SysTick->LOAD = (u32)nms * fac_ms;
    SysTick->VAL = 0x00;
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
    do
    {
        temp = SysTick->CTRL;
    }while((temp & 0x01) && !(temp & (1 << 16)));
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
    SysTick->VAL = 0X00;
}

/*
 * 毫秒延时
 * 大延时拆分为多次执行
 */
void delay_ms(u16 nms)
{
    u8 repeat = nms / 540;
    u16 remain = nms % 540;

    while(repeat)
    {
        delay_xms(540);
        repeat--;
    }
    if(remain)
        delay_xms(remain);
}
#endif
