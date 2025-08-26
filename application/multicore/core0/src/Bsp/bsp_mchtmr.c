/*******************************************************************************
 * File: bsp_mchtmr.c
 * Author: Gloss Tsai
 * Created on: 20241203
 * Description:
 *      内核时钟初始化
 * Copyright (C) 2024 Leetx. All rights reserved.
 *******************************************************************************/
/* Includes ------------------------------------------------------------------ */
#include <stdio.h>
#include "board.h"
#include "hpm_soc.h"
#include "bsp_mchtmr.h"
/* Private macro ------------------------------------------------------------- */
#ifndef MCHTMR_CLK_NAME
#define MCHTMR_CLK_NAME (clock_mchtmr0)
#endif
/* Private typedef ----------------------------------------------------------- */
/* Private variables --------------------------------------------------------- */
static uint32_t s_freq = 0;
static tmr_callback_t s_callback;
/* Private function prototypes ----------------------------------------------- */
static uint64_t GetMchtmrTimestamp(void);
/* Private functions --------------------------------------------------------- */
ATTR_RAMFUNC
static void DelayTick(uint64_t tick)
{
    mchtmr_delay(HPM_MCHTMR, tick);
}
static uint64_t GetMchtmrTimestamp()
{
#if defined(PERFORMANCE_MONITOR_UNIT_US) && PERFORMANCE_MONITOR_UNIT_US
    return (mchtmr_get_count(HPM_MCHTMR) / 24);
#else
    return mchtmr_get_count(HPM_MCHTMR);
#endif
}

uint64_t get_ms()
{
    return (mchtmr_get_count(HPM_MCHTMR) / (24 * 1000));
}

uint64_t Bsp_GetMchtmrTimestampMs()
{
    return (mchtmr_get_count(HPM_MCHTMR) / (24 * 1000));
}
uint64_t Bsp_GetMchtmrTimestampUs()
{
    return (mchtmr_get_count(HPM_MCHTMR) / 24);
}
void Bsp_GetMchtmrTimestampNs(uint64_t *ts)
{
    uint64_t ts_ns = (mchtmr_get_count(HPM_MCHTMR) * 42);   /* 41.6 = 1000 / 24MHz */
    memcpy(ts, &ts_ns, sizeof(uint64_t));
}

void Isr_Mchtmr(void)
{
    if (s_callback != NULL)
    {
        s_callback();
    }
    DelayTick(MCHTMR_PERIOD_TICK); /* 重新设置mchtmr的比较器及寄存器值 */
}
SDK_DECLARE_MCHTMR_ISR(Isr_Mchtmr)

/**
 * @brief 使用机器（内核）定时器产生1s定时中断
 */
void Bsp_InitMchtmr(tmr_callback_t pF)
{
    /* make sure mchtmr will not be gated on "wfi" */
    board_ungate_mchtmr_at_lp_mode();
    s_freq = clock_get_frequency(MCHTMR_CLK_NAME);
    printf("[BSP] Init mchtmr period interrupt. Timestamp:%lld\n", mchtmr_get_count(HPM_MCHTMR));
    enable_mchtmr_irq();
    DelayTick(MCHTMR_PERIOD_TICK); /* 重新设置mchtmr的比较器及寄存器值，引发中断 */

    s_callback = pF; /* 若f为NULL，则不会调用 */
}

/**
 * @param period 用于计算入口时间抖动，单位us。当period的值为0时，表示这不是一个周期调用的函数，不测试入口时间抖动。
 * @note 首次计算得出的executedTimeElapse与entryJitter不可信，需等待稳定
 */
void Bsp_InitPerformanceMonitoring(PerformanceMonitor_t *pInstance, uint64_t period)
{
#if defined(PERFORMANCE_MONITOR_UNIT_US) && PERFORMANCE_MONITOR_UNIT_US
    pInstance->entryPeriod = period;
#else
    pInstance->entryPeriod = period * 24;
#endif
}
/**
 * @warning 函数Bsp_EntryPerformanceMonitoring必须与函数Bsp_ExitPerformanceMonitoring形成调用对
 */
ATTR_RAMFUNC
void Bsp_EntryPerformanceMonitoring(PerformanceMonitor_t *pInstance)
{
    pInstance->entryTs = GetMchtmrTimestamp();
    if(pInstance->entryTsLast == 0) 
    {
        pInstance->entryTsLast = pInstance->entryTs;
        pInstance->entryTsOffset = pInstance->entryTs % pInstance->entryPeriod;
        return;
    }
    pInstance->entryTsPredict = pInstance->entryTs % pInstance->entryPeriod;
    pInstance->entryJitter =  pInstance->entryTs -pInstance->entryTsLast;
    //printf("[PM] Predict:%u, entryTs:%u, entryCnt:%u\n", pInstance->entryTsPredict, pInstance->entryTs, pInstance->entryCnt); //耗时十几毫秒不要打开
    pInstance->entryTsLast = pInstance->entryTs;
    if(pInstance->maxEntryJitter < pInstance->entryJitter)
    {
        pInstance->maxEntryJitter = pInstance->entryJitter;
    }
}
/**
 * @warning 函数Bsp_EntryPerformanceMonitoring必须与函数Bsp_ExitPerformanceMonitoring形成调用对
 */
ATTR_RAMFUNC
void Bsp_ExitPerformanceMonitoring(PerformanceMonitor_t *pInstance)
{
    pInstance->exitTs = GetMchtmrTimestamp();
    pInstance->exeTime = pInstance->exitTs - pInstance->entryTs;
    if(pInstance->maxExeTime < pInstance->exeTime)
    {
        pInstance->maxExeTime = pInstance->exeTime;
    }
}