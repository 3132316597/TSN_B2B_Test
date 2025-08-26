#ifndef MAC_TABLE_H
#define MAC_TABLE_H

/* Includes ------------------------------------------------------------------*/
#include "board.h"
#include "hpm_tsw_drv.h"

#define UTAG_TB_FRAME   1
#define UTAG_IN_FRAME   2
#define UTAG_BC_FRAME   3
#define UTAG_NN_FRAME   4

#define TSW_OK     0
#define TSW_FAIL  -1
#define TSW_MAC_ADDR_LEN  (6U)
#define TSW_MAC_TABLE_MAX (128U)
#define TSW_DEFAULT_VID   (1U)
#define TSW_MAC_AGING_S   (0xffffff00)
//#define TSW_MAC_AGING_S   (0x60)

#define TSW_HEAD_LEN      (16U)
#define TSW_MAC_LEN       (6U)
#define TSW_TYPE_LEN      (2U)

#define HPM_MCHTMR_FREQ (24000000UL)

#define GET_TSW_TYPE(buff, offset)  \
                    (((uint16_t)(buff[offset + 0]) << 8) | \
                    ((uint16_t)(buff[offset + 1])))

typedef struct{
    uint32_t hitmem;
    uint8_t  mac[TSW_MAC_ADDR_LEN];
    uint8_t  vid;
    uint8_t  port;
} tsn_mac_t;


void tsw_set_mac_table(TSW_Type *ptr, uint16_t entry_num, uint8_t dest_port, uint8_t* dest_mac, uint8_t vid);

void tsw_clr_mac_table(TSW_Type *ptr, uint16_t entry_num, uint8_t dest_port, uint8_t* dest_mac, uint8_t vid);

void tsw_set_cam_vid(TSW_Type *ptr, uint8_t dest_port, uint16_t vid);

void tsw_clr_cam_vid(TSW_Type *ptr, uint8_t dest_port, uint16_t vid);

int tsw_mac_table_init(uint8_t* mac, uint16_t vid);

void tsw_clear_hitmem_table(void);

void tsw_mac_table_learn(uint8_t* frame_buff, uint32_t frame_len);

#endif /* MAC_TABLE_H */