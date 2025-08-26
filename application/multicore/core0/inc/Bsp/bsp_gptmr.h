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

/* -------------------------- ������500ms TSW���Ͷ�ʱ������ -------------------------- */
// ���Ͷ�ʱ�����ܼ��ʵ�����ⲿ�ɷ��ʣ����ڵ��ԣ�
extern PerformanceMonitor_t g_pmTswSendTmr;
// ��ʼ��TSW���Ͷ�ʱ��������500ms��pFΪ���ͻص�������
void Bsp_InitTswSendTmr(tmr_callback_t pF);
/* ----------------------------------------------------------------------------------- */

#if defined(__cplusplus)
}
#endif
#endif /* _BSP_GPTMR_H_ */