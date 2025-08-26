/*
 * @Author: Joe jie.huang@leetx.com
 * @Date: 2025-03-26 16:30:03
 * @LastEditors: Joe jie.huang@leetx.com
 * @LastEditTime: 2025-03-27 14:21:16
 * @FilePath: \Controller-ToolV2-HPM\application\multicore\core0\bsp\inc\bsp_gptmr.h
 * @Description: 
 * 
 * Copyright (c) 2025 by Leetx, All Rights Reserved. 
 */
/******************************************************************************
 * File: bsp_gptmr.h
 * Author: Gloss Tsai
 * Created on: 20241218
 * Description:
 *
 * Copyright (C) 2024 Leetx. All rights reserved.
 *******************************************************************************/
/* Define to prevent recursive inclusion --------------------------------------*/
#ifndef _BSP_GPTMR_H_
#define _BSP_GPTMR_H_
#if defined(__cplusplus)
extern "C"
{
#endif
/* Includes -------------------------------------------------------------------*/
#include "bsp_mchtmr.h"
/* Exported constants ---------------------------------------------------------*/
extern PerformanceMonitor_t g_pmRttmr;

/* Exported macro -------------------------------------------------------------*/
/* Exported types -------------------------------------------------------------*/
/* Exported variables ---------------------------------------------------------*/
/* Exported functions ---------------------------------------------------------*/
void Bsp_InitRttmr(tmr_callback_t pF);
void Bsp_InitCommtmr(tmr_callback_t pF);

/* -------------------------- 新增：500ms TSW发送定时器声明 -------------------------- */
// 发送定时器性能监控实例（外部可访问，用于调试）
extern PerformanceMonitor_t g_pmTswSendTmr;
// 初始化TSW发送定时器（周期500ms，pF为发送回调函数）
void Bsp_InitTswSendTmr(tmr_callback_t pF);
/* ----------------------------------------------------------------------------------- */

#if defined(__cplusplus)
}
#endif
#endif /* _BSP_GPTMR_H_ */