/******************************************************************************
 * File: main.h
 * Author: Gloss Tsai
 * Created on: 20241203
 * Description:
 *
 * Copyright (C) 2024 Leetx. All rights reserved.
 *******************************************************************************/
/* Define to prevent recursive inclusion --------------------------------------*/
#ifndef _TSW_LOW_LEVEL_H_
#define _TSW_LOW_LEVEL_H_
#if defined(__cplusplus)
extern "C"
{
#endif
/* Includes -------------------------------------------------------------------*/
#include <stdint.h>
#include "hpm_tsw_drv.h"
/* Exported constants ---------------------------------------------------------*/
/* Exported macro -------------------------------------------------------------*/
/* Exported types -------------------------------------------------------------*/
typedef struct
{
    uint8_t index;
    uint16_t length[TSW_RECV_DESC_COUNT];
    uint8_t *buffer[TSW_RECV_DESC_COUNT];
} tsn_frame_t;

typedef struct
{
    uint64_t mac[128];
    uint8_t mac_port[128];
    uint32_t depth;
} tsn_mac_table_t;

typedef struct {
    // 存储帧数据
    uint8_t data[1536]; 
    // 帧实际长度
    uint32_t len;                
} FrameData;

typedef enum {
    SOURCE_UNKNOWN = 0,  // 未知来源
    SOURCE_PC,           // PC来源
    SOURCE_BOARD1,       // 板子1来源（ID1）
    SOURCE_BOARD2        // 板子2来源（ID2）
} FrameSource;

/* Exported variables ---------------------------------------------------------*/
/* Exported functions ---------------------------------------------------------*/
int Bsp_InitTsw(void);
int Bsp_TransmitTswFrameLowLevel(uint8_t *payload, int len);
int Bsp_ReceiveTswFrameLowLevel(uint8_t *payload, int toReadLen);
void Bsp_GetTswMacAddr(uint8_t *pMac);
bool tsw_get_link_status(uint8_t index);
hpm_stat_t reset_tsw_phy_port1(void);
hpm_stat_t reset_tsw_phy_port2(void);
hpm_stat_t reset_tsw_phy_port3(void);

extern uint8_t g_tsw_queue;
extern bool rx_flag;
extern uint32_t g_tsw_headerBuf[4];
extern uint32_t irq_frame_cnt;

#if defined(__cplusplus)
}
#endif
#endif /* _TSW_LOW_LEVEL_H_ */