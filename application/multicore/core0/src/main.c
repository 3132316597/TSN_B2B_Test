#include "board.h" 
#include "tswLowLevel.h"
#include <stdio.h>
#include "bsp_gptmr.h" 

uint64_t idx = 0;
uint8_t recv_data[1536];
uint16_t len;
uint8_t data_buff[] = {
0x98, 0x2c, 0xbc, 0xb1, 0x9f, 0x17,
0x4e, 0x00, 0x00, 0x00, 0xf0, 0x50,
0x08, 0x06,
0x00, 0x01,
0x08, 0x00,
0x06,
0x04,
0x00, 0x01,
0x98, 0x2c, 0xbc, 0xb1, 0x9f, 0x17,
0xc0, 0xa8, 0x64, 0x0a,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0xc0, 0xa8, 0x64, 0x05,
0xff, 0xff, 0xff, 0xff, 0xff, 0xcc,

};

uint8_t data_buff1[1024] = {
0x98, 0x2c, 0xbc, 0xb1, 0x9f, 0x17,
0x4e, 0x00, 0x00, 0x00, 0xf0, 0x50,
0x08, 0x06,
0x00, 0x01,
0x08, 0x00,
0x06,
0x04,
0x00, 0x01,
0x98, 0x2c, 0xbc, 0xb1, 0x9f, 0x17,
0xc0, 0xa8, 0x64, 0x0a,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0xc0, 0xa8, 0x64, 0x05,
0xff, 0xff, 0xff, 0xff, 0xff, 0xcc,
};
ATTR_PLACE_AT_NONCACHEABLE uint8_t s_FrameTx[1536];

void mac_cpy(uint8_t *Mac,uint8_t *dest_mac)
{
    memcpy(Mac, dest_mac, 6);
}


int DATA_MAKE()
{
    idx++;
    uint32_t num = idx%20;
    uint32_t num_flag = 0;
    uint8_t des_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // 设备ID（6字节）
    uint8_t device_id[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xCC}; // 设备ID（6字节）
    if (num == 19)
    {
        num_flag = 1;
    }else if (num == 0)
    {
        num_flag = 0;
    }else
    {
        num_flag = num_flag;
    }

    mac_cpy(s_FrameTx, des_mac);
    Bsp_GetTswMacAddr(s_FrameTx + 6);

    s_FrameTx[12] = 0x08;
    s_FrameTx[13] = 0x06;

    s_FrameTx[14] = 0x00;
    s_FrameTx[15] = 0x01;

    s_FrameTx[16] = 0x08;
    s_FrameTx[17] = 0x00;
    
    memcpy(&s_FrameTx[42], device_id, 6);
    memcpy(&s_FrameTx[48], &num, sizeof(num));
    memcpy(&s_FrameTx[52], &num_flag, sizeof(num_flag));
    memcpy(&s_FrameTx[56], device_id, 6);
    return 0;
}

static void TswSendCallback_500ms(void)
{
    uint32_t value = 0;
    if (!(tsw_get_link_status(TSW_TSNPORT_PORT1) || tsw_get_link_status(TSW_TSNPORT_PORT2)))
    {
        printf("[TSW Send] No port linked, skip\n");  // 调试用，稳定后可注释
        return;
    }
    DATA_MAKE();    

    g_tsw_queue = 1;        
    send_global_port = 1;  

    Bsp_TransmitTswFrameLowLevel(s_FrameTx, 1024);
    g_tsw_queue = 0; 
    Bsp_TransmitTswFrameLowLevel(s_FrameTx, 62);

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