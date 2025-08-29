#include "board.h" 
#include "tswLowLevel.h"
#include <stdio.h>
#include "bsp_gptmr.h" 
#include "data_u.h"

uint64_t idx = 0;
uint8_t recv_data[1536];
uint16_t len;

ATTR_PLACE_AT_NONCACHEABLE uint8_t s_FrameTx[1536];

ATTR_PLACE_AT_NONCACHEABLE TswFrame_t s_tsw_frame = {
    .eth_type = {0x08, 0x06}, 
    .reserved = {0},
    .device_id = {0},
    .num = 0,
    .num_flag = 0,
    .device_id2 = {0},
    .payload = {0}
};
void mac_cpy(uint8_t *Mac,uint8_t *dest_mac)
{
    memcpy(Mac, dest_mac, 6);
}

int DATA_MAKE()
{
    idx++;
    uint32_t num = idx % 20;
    uint32_t num_flag = 0;
    num_flag = (num == 19) ? 1 : (num == 0 ? 0 : num_flag); 

    uint8_t des_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    memcpy(s_tsw_frame.dest_mac, des_mac, 6);
    Bsp_GetTswMacAddr(s_tsw_frame.src_mac); // 源MAC仅需初始化一次（若不变）
    
    uint8_t device_id[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xCC};
    memcpy(s_tsw_frame.device_id, device_id, 6);
    s_tsw_frame.num = num;
    s_tsw_frame.num_flag = num_flag;
    memcpy(s_tsw_frame.device_id2, device_id, 6);
    memcpy(s_tsw_frame.payload, data_buff, 64);
    return 0;
}

static void TswSendCallback_500ms(void) {
    static uint64_t last_timestamp = 0;
    uint32_t value = 0;
    
    uint64_t current_timestamp = Bsp_GetMchtmrTimestampMs();
    if (last_timestamp != 0) {
        uint32_t actual_period = current_timestamp - last_timestamp;
        if (abs((int)(actual_period - 500)) > 10) {
            printf("[Warning] TSW period deviation: %dms (expected 500ms)\n", actual_period);
        }
    }
    last_timestamp = current_timestamp;

    if (!(tsw_get_link_status(TSW_TSNPORT_PORT1) || tsw_get_link_status(TSW_TSNPORT_PORT2))) {
        printf("[TSW Send] No port linked, skip\n");
        return;
    }

    DATA_MAKE();    
    g_tsw_queue = 1;        
    send_global_port = 1;  
    Bsp_TransmitTswFrameLowLevel((uint8_t*)&s_tsw_frame, 1024);
    g_tsw_queue = 0; 
    Bsp_TransmitTswFrameLowLevel((uint8_t*)&s_tsw_frame, 62);

    tsw_fpe_get_mms_statistics_counter(HPM_TSW, TSW_TSNPORT_PORT1, tsw_fpe_mms_fragment_tx_counter, &value);
    printf("FPE MMS Fragment Tx Counter: %d\n", value);
    printf("=========================================================\n");
}


int main(void)
{
    board_init();
    printf("TSW Frame Demo Start !\n");
    if (Bsp_InitTsw() != status_success) {
        printf("TSW init failed !\n");
        while (1) {
        }
    }
    ;;;
    
    Bsp_InitTswSendTmr(TswSendCallback_500ms);
    while (1) {
        if(tsw_get_link_status(TSW_TSNPORT_PORT1))
        {
            if(rx_flag)
            {
                len = Bsp_ReceiveTswFrameLowLevel(recv_data, sizeof(recv_data)); 
                printf("Recv Frame Len : %d\n", len);
            }
        }
    }
}