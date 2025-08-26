/******************************************************************************
 * File: bsp_mchtmr.h
 * Author: Gloss Tsai
 * Created on: 20241203
 * Description:
 *
 * Copyright (C) 2024 Leetx. All rights reserved.
 *******************************************************************************/
/* Define to prevent recursive inclusion --------------------------------------*/
#ifndef _BSP_MCHTMR_H_
#define _BSP_MCHTMR_H_
#if defined(__cplusplus)
extern "C"
{
#endif
/* Includes -------------------------------------------------------------------*/
#include "hpm_mchtmr_drv.h"
/* Exported constants ---------------------------------------------------------*/
/* Exported macro -------------------------------------------------------------*/
#define MCHTMR_PERIOD_US ((uint64_t)1000000u)
#define MCHTMR_PERIOD_TICK (MCHTMR_PERIOD_US * 24)

#define PERFORMANCE_MONITOR_UNIT_US 0   /* 若使用us为单位，则将该宏置 1 */
/* Exported types -------------------------------------------------------------*/
typedef void (*tmr_callback_t)(void);
/**
 * @note 单位为us或mchtmr_tick，由宏确定
 */
typedef struct
{
    uint64_t entryTs;       /* 开始时间戳 */
    uint64_t exitTs;        /* 结束时间戳 */
    uint32_t exeTime;       /* 执行时间 */

    uint32_t entryPeriod;   /* 执行周期 */
    uint32_t entryTsLast;
    uint32_t entryTsOffset;   /* 基准时间戳偏移 */
    uint32_t entryTsPredict;  /* 入口时间戳偏移 */
    uint32_t entryTsDiff;
    int32_t entryJitter;

    int32_t maxEntryJitter;
    uint32_t maxExeTime;
}PerformanceMonitor_t;

/* Exported variables ---------------------------------------------------------*/
/* Exported functions ---------------------------------------------------------*/
void Bsp_InitMchtmr(tmr_callback_t pF);
uint64_t Bsp_GetMchtmrTimestampMs(void);
uint64_t Bsp_GetMchtmrTimestampUs(void);
void Bsp_GetMchtmrTimestampNs(uint64_t *ts);    /* libcore调用该接口 */
void Bsp_InitPerformanceMonitoring(PerformanceMonitor_t *pInstance, uint64_t period);
void Bsp_EntryPerformanceMonitoring(PerformanceMonitor_t *pInstance);
void Bsp_ExitPerformanceMonitoring(PerformanceMonitor_t *pInstance);
#if defined(__cplusplus)
}
#endif
#endif /* _BSP_MCHTMR_H_ */