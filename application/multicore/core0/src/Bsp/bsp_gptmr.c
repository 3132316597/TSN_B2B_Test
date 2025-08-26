/*******************************************************************************
 * File: bsp_gptmr.c
 * Author: Gloss Tsai
 * Created on: 20241203
 * Description:
 *      内核时钟初始化
 * Copyright (C) 2024 Leetx. All rights reserved.
 *******************************************************************************/
/* Includes ------------------------------------------------------------------ */
#include <stdio.h>
#include "board.h"
#include "bsp_gptmr.h"

#include "hpm_soc.h"
#include "hpm_gptmr_drv.h"
#include "hpm_interrupt.h"
/* Private macro ------------------------------------------------------------- */
#define RTTMR_BASE          HPM_GPTMR4
#define RTTMR_CLK_NAME      clock_gptmr4
#define RTTMR_PERIOD_US     (500u)
#define RTTMR_CH            (1u)
#define RTTMR_IRQ           IRQn_GPTMR4
#define RTTMR_IRQ_PRIORITY  (5u)

#define COMMTMR_BASE            HPM_GPTMR5
#define COMMTMR_CLK_NAME        clock_gptmr5
#define COMMTMR_PERIOD_US       (1000u)
#define COMMTMR_CH              (1u)
#define COMMTMR_IRQ             IRQn_GPTMR5
#define COMMTMR_IRQ_PRIORITY    (4u)

// 自定义定时器：200ms周期，用于TSW发送
#define TSW_SEND_TMR_BASE       HPM_GPTMR6          // 选择未使用的GPTMR模块
#define TSW_SEND_TMR_CLK_NAME   clock_gptmr6        // 定时器时钟名（需在board.h中定义）
#define TSW_SEND_TMR_PERIOD_MS  (500u)              // 目标周期：500ms
#define TSW_SEND_TMR_PERIOD_US  (TSW_SEND_TMR_PERIOD_MS * 1000u)  // 转为微秒：500000μs
#define TSW_SEND_TMR_CH         (0u)                // 选择空闲通道（0-3，优先选0）
#define TSW_SEND_TMR_IRQ        IRQn_GPTMR6         // 对应中断号（查HPM6E80手册确认）
#define TSW_SEND_TMR_IRQ_PRI    (3u)                // 中断优先级：低于RTTMR(5)，高于普通任务

/* Private typedef ----------------------------------------------------------- */
/* Private variables --------------------------------------------------------- */
static tmr_callback_t s_rttmrCallback;
static tmr_callback_t s_commtmrCallback;
PerformanceMonitor_t g_pmRttmr;
PerformanceMonitor_t g_pmCommtmr;
/* Private function prototypes ----------------------------------------------- */

/* -------------------------- 发送定时器全局变量 -------------------------- */
static tmr_callback_t s_tswSendTmrCallback;  // 发送定时器回调函数指针
PerformanceMonitor_t g_pmTswSendTmr;        // 发送定时器性能监控实例
/* ----------------------------------------------------------------------------- */
/* Private functions --------------------------------------------------------- */
/**
 * @brief 初始化实时定时器
 */
void Bsp_InitRttmr(tmr_callback_t pF)
{
    uint32_t gptmr_freq;
    gptmr_channel_config_t config;

    gptmr_channel_get_default_config(RTTMR_BASE, &config);

    //clock_add_to_group(RTTMR_CLK_NAME, 0);
    gptmr_freq = clock_get_frequency(RTTMR_CLK_NAME);
    config.reload = gptmr_freq / 1000000 * RTTMR_PERIOD_US ;

    gptmr_channel_config(RTTMR_BASE, RTTMR_CH, &config, false);
    gptmr_start_counter(RTTMR_BASE, RTTMR_CH);
    gptmr_enable_irq(RTTMR_BASE, GPTMR_CH_RLD_IRQ_MASK(RTTMR_CH));
    printf("[BSP] Init rttmr.Interrupt period: %dus\n", RTTMR_PERIOD_US);

    s_rttmrCallback = pF;

    Bsp_InitPerformanceMonitoring(&g_pmRttmr, RTTMR_PERIOD_US);

    intc_m_enable_irq_with_priority(RTTMR_IRQ, RTTMR_IRQ_PRIORITY);
}
void Isr_rttmr(void)
{
    if (gptmr_check_status(RTTMR_BASE, GPTMR_CH_RLD_STAT_MASK(RTTMR_CH))) 
    {
        gptmr_clear_status(RTTMR_BASE, GPTMR_CH_RLD_STAT_MASK(RTTMR_CH));
        if (s_rttmrCallback != NULL)
        {
            Bsp_EntryPerformanceMonitoring(&g_pmRttmr);
            s_rttmrCallback();
            Bsp_ExitPerformanceMonitoring(&g_pmRttmr);
            //printf("rtJ:%5d,E:%5d\n", g_pmRttmr.entryJitter, g_pmRttmr.exeTime); // about 100us
        }
    }
}
SDK_DECLARE_EXT_ISR_M(RTTMR_IRQ, Isr_rttmr);
/**
 * @brief 初始化通信定时器
 */
void Bsp_InitCommtmr(tmr_callback_t pF)
{
    uint32_t gptmr_freq;
    gptmr_channel_config_t config;

    gptmr_channel_get_default_config(COMMTMR_BASE, &config);

    //clock_add_to_group(COMMTMR_CLK_NAME, 0);

    gptmr_freq = clock_get_frequency(COMMTMR_CLK_NAME);
    config.reload = gptmr_freq / 1000000 * COMMTMR_PERIOD_US;

    gptmr_channel_config(COMMTMR_BASE, COMMTMR_CH, &config, false);
    gptmr_start_counter(COMMTMR_BASE, COMMTMR_CH);
    gptmr_enable_irq(COMMTMR_BASE, GPTMR_CH_RLD_IRQ_MASK(COMMTMR_CH));
    printf("[BSP] Init commtmr. Interrupt period: %dus\n", COMMTMR_PERIOD_US);

    s_commtmrCallback = pF;

    Bsp_InitPerformanceMonitoring(&g_pmCommtmr, COMMTMR_PERIOD_US);

    intc_m_enable_irq_with_priority(COMMTMR_IRQ, COMMTMR_IRQ_PRIORITY);
}
void Isr_commtmr(void)
{
    if (gptmr_check_status(COMMTMR_BASE, GPTMR_CH_RLD_STAT_MASK(COMMTMR_CH))) 
    {
        gptmr_clear_status(COMMTMR_BASE, GPTMR_CH_RLD_STAT_MASK(COMMTMR_CH));
        if (s_commtmrCallback != NULL)
        {
            Bsp_EntryPerformanceMonitoring(&g_pmCommtmr);
            s_commtmrCallback();
            //printf("commJ:%5d,E:%5d\n", g_pmCommtmr.entryJitter, g_pmCommtmr.exeTime); // about 100us
            //printf("c");
            Bsp_ExitPerformanceMonitoring(&g_pmCommtmr);
        }
    }
    //gptmr_channel_update_count(COMMTMR_BASE, COMMTMR_CH, 0);
}
SDK_DECLARE_EXT_ISR_M(COMMTMR_IRQ, Isr_commtmr);

static void Isr_TswSendTmr(void)
{
    // 1. 检查定时器重载中断标志（仅处理目标通道的中断）
    if (gptmr_check_status(TSW_SEND_TMR_BASE, GPTMR_CH_RLD_STAT_MASK(TSW_SEND_TMR_CH)))
    {
        // 2. 清除中断标志（必须，否则会重复触发中断）
        gptmr_clear_status(TSW_SEND_TMR_BASE, GPTMR_CH_RLD_STAT_MASK(TSW_SEND_TMR_CH));
        
        // 3. 执行用户回调函数（带性能监控）
        if (s_tswSendTmrCallback != NULL)
        {
            Bsp_EntryPerformanceMonitoring(&g_pmTswSendTmr);  // 记录进入时间
            s_tswSendTmrCallback();                            // 调用发送逻辑
            Bsp_ExitPerformanceMonitoring(&g_pmTswSendTmr);    // 记录退出时间
            
            // （可选）打印性能数据（调试用，稳定后可注释）
            // printf("[TSW Send TMR] Jitter=%lldus, ExecTime=%lldus\n",
            //        g_pmTswSendTmr.entryJitter / 24,  // 24tick=1us（对应24MHz时钟）
            //        g_pmTswSendTmr.exeTime / 24);
        }
    }
}
// 注册中断服务程序（绑定中断号与ISR）
SDK_DECLARE_EXT_ISR_M(TSW_SEND_TMR_IRQ, Isr_TswSendTmr);
/* ----------------------------------------------------------------------------- */

/* -------------------------- 新增：TSW发送定时器初始化函数 -------------------------- */
void Bsp_InitTswSendTmr(tmr_callback_t pF)
{
    uint32_t tmr_freq;          // 定时器时钟频率（Hz）
    gptmr_channel_config_t cfg; // 定时器通道配置结构体

    // 1. 记录用户回调函数
    s_tswSendTmrCallback = pF;
    if (s_tswSendTmrCallback == NULL)
    {
        printf("[BSP] TSW Send TMR: Callback is NULL!\n");
        return;
    }

    // 2. 获取定时器默认配置（基础参数初始化）
    gptmr_channel_get_default_config(TSW_SEND_TMR_BASE, &cfg);

    // 3. 计算定时器时钟频率（需确保clock_gptmr6已在board.h中定义）
    tmr_freq = clock_get_frequency(TSW_SEND_TMR_CLK_NAME);
    if (tmr_freq == 0)
    {
        printf("[BSP] TSW Send TMR: Clock not found!\n");
        return;
    }

    // 4. 计算重载值（周期 = 重载值 / 时钟频率 → 重载值 = 时钟频率 × 周期）
    cfg.reload = (tmr_freq / 1000000u) * TSW_SEND_TMR_PERIOD_US;
    printf("[BSP] TSW Send TMR: Freq=%luHz, Reload=%lu, Period=%dms\n",
           tmr_freq, cfg.reload, TSW_SEND_TMR_PERIOD_MS);

    // 5. 配置定时器通道（初始化硬件寄存器）
    gptmr_channel_config(TSW_SEND_TMR_BASE, TSW_SEND_TMR_CH, &cfg, false);

    // 6. 初始化性能监控（周期单位：微秒）
    Bsp_InitPerformanceMonitoring(&g_pmTswSendTmr, TSW_SEND_TMR_PERIOD_US);

    // 7. 使能定时器中断（外设级：允许定时器产生中断请求）
    gptmr_enable_irq(TSW_SEND_TMR_BASE, GPTMR_CH_RLD_IRQ_MASK(TSW_SEND_TMR_CH));

    // 8. 使能CPU级中断（配置优先级并允许CPU响应中断）
    intc_m_enable_irq_with_priority(TSW_SEND_TMR_IRQ, TSW_SEND_TMR_IRQ_PRI);

    // 9. 启动定时器计数（开始周期运行）
    gptmr_start_counter(TSW_SEND_TMR_BASE, TSW_SEND_TMR_CH);
    printf("[BSP] TSW Send TMR Init Success!\n");
}
/* ----------------------------------------------------------------------------- */