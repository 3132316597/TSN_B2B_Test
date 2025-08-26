/*******************************************************************************
 * File: bsp_tswLowLevel.c
 * Author: Gloss Tsai
 * Created on: 20241216
 * Description:
 *      HPM6E80EVK TSW��������������Դ�����������
 *      ���ܣ�ʵ��TSW��������ʼ����֡����/���ա�PHY����֡��Դ�������
 * Copyright (C) 2024 Leetx. All rights reserved.
 *******************************************************************************/

/* Includes ------------------------------------------------------------------ */
#include <stdio.h>
#include <string.h>
#include "board.h"
#include "tswLowLevel.h"
#include "hpm_tsw_drv.h"
#include "hpm_gpio_drv.h"
#include "hpm_yt8531.h"
#include "RingBuffer.h"
#include <math.h>
#include "mac_table.h" 

/* Private macro ------------------------------------------------------------- */
#define TSW_RX_HDR_LEN       sizeof(rx_hdr_desc_t)  // ����֡ͷ���ȣ�rx_hdr_desc_t�ṹ���С��
#define FPE_FRAG_CACHE_MAX   8                      // FPE��Ƭ�������ֵ
#define PATTERN_PC           "\xff\xff\xff\xff\xff\xff"  // PC�豸֡�����루6�ֽڹ㲥��ַ��
#define PATTERN_BOARD_ID1    "\xff\xff\xff\xff\xff\xee"  // ����1�����루6�ֽڣ�
#define PATTERN_BOARD_ID2    "\xff\xff\xff\xff\xff\xcc"  // ����2�����루6�ֽڣ�
#define PATTERN_LEN          6                           // �����볤�ȣ�6�ֽڣ�
#define FRAME_DATA_OFFSET    42                          // ��������֡�е�ƫ��������֡ͷ��ʼ���㣩

/* Private typedef ----------------------------------------------------------- */
/* Private variables --------------------------------------------------------- */
static uint32_t sys_tick = 0;                          // ϵͳ�δ������
uint32_t send_fail_time = 0;                           // ����ʧ�ܼ���
uint32_t send_success_time = 0, FPE_0=0, FPE_1=0, recv_fail_time = 0;      // ���ͳɹ�������FPE֡���ͼ���
uint8_t g_tsw_queue = 0;                   // TSW����
uint32_t g_tsw_headerBuf[4] = {0};                     // TSWͷ������
bool rx_flag = false;                      // ���ձ�־
static tsw_phy_status_t phy1_last_status = {0};        // PHY1�ϴ�״̬����
static tsw_phy_status_t phy2_last_status = {0};        // PHY2�ϴ�״̬����
static tsw_phy_status_t phy3_last_status = {0};        // PHY3�ϴ�״̬����
uint32_t irq_frame_cnt = 0;                      // �жϼ�����
uint8_t recv_global_port = 0, send_global_port = 1;  // ȫ���շ��˿ڱ�ʶ
static tsw_frame_t frame[16] = {0};                     // TSW֡�ṹ����
// �ǻ��������壨���ջ�������
ATTR_PLACE_AT_FAST_RAM_WITH_ALIGNMENT(TSW_SOC_DATA_BUS_WIDTH)
__RW uint8_t tsw_rx_buff[TSW_RECV_DESC_COUNT][TSW_RECV_BUFF_LEN];

// �ǻ��������壨���ͻ�������
ATTR_PLACE_AT_FAST_RAM_WITH_ALIGNMENT(TSW_SOC_DATA_BUS_WIDTH)
__RW uint8_t tsw_tx_buff[TSW_SEND_DESC_COUNT][TSW_SEND_BUFF_LEN];

// UDP�����������λ���������
#define UDP_BUFFER_SIZE (4096 * 8)
ATTR_PLACE_AT_FAST_RAM_INIT byte eTSWRecvTxbuf[UDP_BUFFER_SIZE];                  // �����ȼ�֡���ջ�����
ATTR_PLACE_AT_FAST_RAM_INIT byte pTSWRecvTxbuf[UDP_BUFFER_SIZE];                  // �����ȼ�֡���ջ�����
sByteRingBuffer TSWCurveFifo_e = {                     // �����ȼ�֡���λ�����
    .ByteCapacity = sizeof(eTSWRecvTxbuf),
    .pByteBuf = eTSWRecvTxbuf,
    .iFront = 0,
    .iRear = 0
};
sByteRingBuffer TSWCurveFifo_p = {                     // �����ȼ�֡���λ�����
    .ByteCapacity = sizeof(pTSWRecvTxbuf),
    .pByteBuf = pTSWRecvTxbuf,
    .iFront = 0,
    .iRear = 0
};
uint8_t data_buf_e[UDP_BUFFER_SIZE];                   // �����ȼ�֡���ݻ���
uint8_t data_buf_p[UDP_BUFFER_SIZE];                   // �����ȼ�֡���ݻ���

// MAC��ַ����
uint8_t mac2pc1[6] = {0x08, 0x92, 0x04, 0x1e, 0xb9, 0x62};  // �˿�1Ŀ��MAC��PC1��
uint8_t mac2pc2[6] = {0x7C, 0x4D, 0x8F, 0xAA, 0xCC, 0x67};  // �˿�2Ŀ��MAC��PC2��
uint8_t mac2pc3[6] = {0x98, 0x2C, 0xBC, 0xB1, 0x9F, 0x17};  // �˿�3Ŀ��MAC��PC3��
uint8_t mac2cpu[6] = {0x4e, 0x00, 0x00, 0x00, 0xf0, 0x64};   // CPU����MAC


volatile tsn_frame_t tsn_frame = {0};  // TSN֡�ṹ��volatileȷ���ڴ�ɼ��ԣ�

/* Private function prototypes ----------------------------------------------- */

/* Private functions --------------------------------------------------------- */

/**
 * @brief ����TSW���Ͷ˿�
 * @param port Ŀ��˿ں�
 * @note ��ʼ�����з����������Ķ˿���Ϣ
 */
void tsw_config_send_port(uint8_t port)
{
    for (uint8_t i = 0; i < TSW_SEND_DESC_COUNT; i++) {
        *tsw_tx_buff[i] = port + 1;  // �˿ں�ƫ�ƴ���
    }
}
/**
 * @brief TSW�˿���������Ӧ����
 * @note ���PHY����״̬�仯���Զ����ö˿����ʡ�˫��ģʽ�Ȳ���
 */
void tsw_self_adaptive_port_speed(void)
{
    // �˿�����ӳ���
    tsw_port_speed_t port_speed[] = {tsw_port_speed_10mbps, 
                                    tsw_port_speed_100mbps, 
                                    tsw_port_speed_1000mbps};
    char *speed_str[] = {"10Mbps", "100Mbps", "1000Mbps"};  // ���������ַ���
    char *duplex_str[] = {"Half duplex", "Full duplex"};     // ˫��ģʽ�����ַ���

    tsw_phy_status_t phy1_status = {0};  // PHY1��ǰ״̬
    tsw_phy_status_t phy2_status = {0};  // PHY2��ǰ״̬
    tsw_phy_status_t phy3_status = {0};  // PHY3��ǰ״̬

    // ����PHY1״̬�仯
    yt8531_get_phy_status(HPM_TSW, TSW_TSNPORT_PORT1, 0, &phy1_status);
    if (memcmp(&phy1_last_status, &phy1_status, sizeof(tsw_phy_status_t)) != 0) {
        memcpy(&phy1_last_status, &phy1_status, sizeof(tsw_phy_status_t));
        
        if (phy1_status.tsw_phy_link) {
            // ���ӽ�������ӡ״̬�����ö˿�
            printf("PHY1 Link Status: Up\n");
            printf("PHY1 Link Speed:  %s\n", speed_str[phy1_status.tsw_phy_speed]);
            printf("PHY1 Link Duplex: %s\n", duplex_str[phy1_status.tsw_phy_duplex]);

            tsw_set_port_speed(HPM_TSW, TSW_TSNPORT_PORT1, port_speed[phy1_status.tsw_phy_speed]);
            tsw_ep_disable_mac_ctrl(HPM_TSW, TSW_TSNPORT_PORT1, tsw_mac_type_emac);
            tsw_ep_disable_mac_ctrl(HPM_TSW, TSW_TSNPORT_PORT1, tsw_mac_type_pmac);
            tsw_ep_set_mac_mode(HPM_TSW, TSW_TSNPORT_PORT1, 
                (tsw_phy_port_speed_1000mbps == phy1_status.tsw_phy_speed) ? tsw_mac_mode_gmii : tsw_mac_mode_mii);
            tsw_ep_enable_mac_ctrl(HPM_TSW, TSW_TSNPORT_PORT1, tsw_mac_type_emac);
            tsw_ep_enable_mac_ctrl(HPM_TSW, TSW_TSNPORT_PORT1, tsw_mac_type_pmac);
            tsw_config_send_port(TSW_TSNPORT_PORT1);
            
            // ���˫��ģʽ��TSW MAC��֧��ȫ˫����
            if (!phy1_status.tsw_phy_duplex) {
                printf("Error: PHY1 is in half duplex now, but TSW MAC supports only full duplex mode!\n");
                return;
            }
            #if defined(NO_SYS) && !NO_SYS
            msg = tsw_phy_link_up;
            sys_mbox_trypost_fromisr(&netif_status_mbox, &msg);
            #else
            //netif_set_link_up(netif_get_by_index(LWIP_NETIF_IDX));
            #endif
        } else {
            // ���ӶϿ�
            printf("PHY1 Link Status: Down\n");
            #if defined(NO_SYS) && !NO_SYS
            msg = tsw_phy_link_down;
            sys_mbox_trypost_fromisr(&netif_status_mbox, &msg);
            #else
            //netif_set_link_down(netif_get_by_index(LWIP_NETIF_IDX));
            #endif
        }
    }

    // ����PHY2״̬�仯���߼�ͬPHY1��
    yt8531_get_phy_status(HPM_TSW, TSW_TSNPORT_PORT2, 0, &phy2_status);
    if (memcmp(&phy2_last_status, &phy2_status, sizeof(tsw_phy_status_t)) != 0) {
        memcpy(&phy2_last_status, &phy2_status, sizeof(tsw_phy_status_t));
        
        if (phy2_status.tsw_phy_link) {
            printf("PHY2 Link Status: Up\n");
            printf("PHY2 Link Speed:  %s\n", speed_str[phy2_status.tsw_phy_speed]);
            printf("PHY2 Link Duplex: %s\n", duplex_str[phy2_status.tsw_phy_duplex]);
            
            tsw_set_port_speed(HPM_TSW, TSW_TSNPORT_PORT2, port_speed[phy2_status.tsw_phy_speed]);
            tsw_ep_disable_mac_ctrl(HPM_TSW, TSW_TSNPORT_PORT2, tsw_mac_type_emac);
            tsw_ep_disable_mac_ctrl(HPM_TSW, TSW_TSNPORT_PORT2, tsw_mac_type_pmac);
            tsw_ep_set_mac_mode(HPM_TSW, TSW_TSNPORT_PORT2, 
                (tsw_phy_port_speed_1000mbps == phy2_status.tsw_phy_speed) ? tsw_mac_mode_gmii : tsw_mac_mode_mii);
            tsw_ep_enable_mac_ctrl(HPM_TSW, TSW_TSNPORT_PORT2, tsw_mac_type_emac);
            tsw_ep_enable_mac_ctrl(HPM_TSW, TSW_TSNPORT_PORT2, tsw_mac_type_pmac);
            tsw_config_send_port(TSW_TSNPORT_PORT2);

            if (!phy2_status.tsw_phy_duplex) {
                printf("Error: PHY2 is in half duplex now, but TSW MAC supports only full duplex mode!\n");
                return;
            }
            #if defined(NO_SYS) && !NO_SYS
            msg = tsw_phy_link_up;
            sys_mbox_trypost_fromisr(&netif_status_mbox, &msg);
            #else
            //netif_set_link_up(netif_get_by_index(LWIP_NETIF_IDX));
            #endif
        } else {
            printf("PHY2 Link Status: Down\n");
            #if defined(NO_SYS) && !NO_SYS
            msg = tsw_phy_link_down;
            sys_mbox_trypost_fromisr(&netif_status_mbox, &msg);
            #else
            //netif_set_link_down(netif_get_by_index(LWIP_NETIF_IDX));
            #endif
        }
    }
    
    // ����PHY3״̬�仯���߼�ͬPHY1��
    yt8531_get_phy_status(HPM_TSW, TSW_TSNPORT_PORT3, 0, &phy3_status);
    if (memcmp(&phy3_last_status, &phy3_status, sizeof(tsw_phy_status_t)) != 0) {
        memcpy(&phy3_last_status, &phy3_status, sizeof(tsw_phy_status_t));
        
        if (phy3_status.tsw_phy_link) {
            printf("PHY3 Link Status: Up\n");
            printf("PHY3 Link Speed:  %s\n", speed_str[phy3_status.tsw_phy_speed]);
            printf("PHY3 Link Duplex: %s\n", duplex_str[phy3_status.tsw_phy_duplex]);
            
            tsw_set_port_speed(HPM_TSW, TSW_TSNPORT_PORT3, port_speed[phy3_status.tsw_phy_speed]);
            tsw_ep_disable_mac_ctrl(HPM_TSW, TSW_TSNPORT_PORT3, tsw_mac_type_emac);
            tsw_ep_disable_mac_ctrl(HPM_TSW, TSW_TSNPORT_PORT3, tsw_mac_type_pmac);
            tsw_ep_set_mac_mode(HPM_TSW, TSW_TSNPORT_PORT3, 
                (tsw_phy_port_speed_1000mbps == phy3_status.tsw_phy_speed) ? tsw_mac_mode_gmii : tsw_mac_mode_mii);
            tsw_ep_enable_mac_ctrl(HPM_TSW, TSW_TSNPORT_PORT3, tsw_mac_type_emac);
            tsw_ep_enable_mac_ctrl(HPM_TSW, TSW_TSNPORT_PORT3, tsw_mac_type_pmac);
            tsw_config_send_port(TSW_TSNPORT_PORT3);

            if (!phy3_status.tsw_phy_duplex) {
                printf("Error: PHY3 is in half duplex now, but TSW MAC supports only full duplex mode!\n");
                return;
            }
            #if defined(NO_SYS) && !NO_SYS
            msg = tsw_phy_link_up;
            sys_mbox_trypost_fromisr(&netif_status_mbox, &msg);
            #else
            //netif_set_link_up(netif_get_by_index(LWIP_NETIF_IDX));
            #endif
        } else {
            printf("PHY3 Link Status: Down\n");
            #if defined(NO_SYS) && !NO_SYS
            msg = tsw_phy_link_down;
            sys_mbox_trypost_fromisr(&netif_status_mbox, &msg);
            #else
            //netif_set_link_down(netif_get_by_index(LWIP_NETIF_IDX));
            #endif
        }
    }
}

/**
 * @brief ����TSW PHY�˿�1
 * @return ״̬�룺status_success-�ɹ���status_fail-ʧ��
 * @note ��ʼ��PHY1Ӳ�������û�������ģʽ
 */
hpm_stat_t reset_tsw_phy_port1(void)
{
    yt8531_config_t phy_config;

    yt8531_reset(HPM_TSW, TSW_TSNPORT_PORT1, 0);
    board_delay_ms(10);  // �ȴ���λ���
    
    yt8531_basic_mode_default_config(HPM_TSW, &phy_config);
    if (yt8531_basic_mode_init(HPM_TSW, TSW_TSNPORT_PORT1, 0, &phy_config) == true) {
        printf("TSW phy1 init passed !\n");
    } else {
        printf("TSW phy1 init failed !\n");
        return status_fail;
    }
    return status_success;
}

/**
 * @brief ����TSW PHY�˿�2
 * @return ״̬�룺status_success-�ɹ���status_fail-ʧ��
 * @note ��ʼ��PHY2Ӳ�������û�������ģʽ
 */
hpm_stat_t reset_tsw_phy_port2(void)
{
    yt8531_config_t phy_config;

    yt8531_reset(HPM_TSW, TSW_TSNPORT_PORT2, 0);
    board_delay_ms(10);  // �ȴ���λ���
    
    yt8531_basic_mode_default_config(HPM_TSW, &phy_config);
    if (yt8531_basic_mode_init(HPM_TSW, TSW_TSNPORT_PORT2, 0, &phy_config) == true) {
        printf("TSW phy2 init passed !\n");
    } else {
        printf("TSW phy2 init failed !\n");
        return status_fail;
    }
    return status_success;
}

/**
 * @brief ����TSW PHY�˿�3
 * @return ״̬�룺status_success-�ɹ���status_fail-ʧ��
 * @note ��ʼ��PHY3Ӳ�������û�������ģʽ
 */
hpm_stat_t reset_tsw_phy_port3(void)
{
    yt8531_config_t phy_config;

    yt8531_reset(HPM_TSW, TSW_TSNPORT_PORT3, 0);
    board_delay_ms(10);  // �ȴ���λ���
    
    yt8531_basic_mode_default_config(HPM_TSW, &phy_config);
    if (yt8531_basic_mode_init(HPM_TSW, TSW_TSNPORT_PORT3, 0, &phy_config) == true) {
        printf("TSW phy3 init passed !\n");
    } else {
        printf("TSW phy3 init failed !\n");
        return status_fail;
    }
    return status_success;
}

/**
 * @brief ϵͳ��ʱ���ص�����
 * @note ������ִ�ж˿���������Ӧ���
 */
void sys_timer_callback(void)
{
    sys_tick++;
    // ÿ2��ִ��һ�ζ˿���������Ӧ���
    if (sys_tick % (2000 * 1) == 0) {
        tsw_self_adaptive_port_speed();
    }
}

/*---------------------------------------------------------------------*
 * TSW��������ʼ��
 *---------------------------------------------------------------------*/
/**
 * @brief ��ʼ��TSW������
 * @param ptr TSW����������ַ
 * @return ״̬�룺status_success-�ɹ���status_fail-ʧ��
 * @note ����MAC��ַ���˿�ģʽ��DMA��PHY��ʼ��
 */
hpm_stat_t tsw_init(TSW_Type *ptr)
{
    yt8531_config_t phy_config;
    tsw_dma_config_t config;
    
    /* ��������MAC��TX/RX���� */
    tsw_ep_disable_all_mac_ctrl(ptr, tsw_mac_type_emac);
    tsw_ep_disable_all_mac_ctrl(ptr, tsw_mac_type_pmac);
    
    /* ���ö˿�MAC��ַ */
    tsw_ep_set_mac_addr(ptr, TSW_TSNPORT_PORT1, mac2pc1, true);
    tsw_ep_set_mac_addr(ptr, TSW_TSNPORT_PORT2, mac2pc2, true);
    tsw_ep_set_mac_addr(ptr, TSW_TSNPORT_PORT3, mac2pc3, true);

    /* ����MAC����ģʽ */
    tsw_ep_set_mac_mode(ptr, TSW_TSNPORT_PORT1, tsw_mac_mode_gmii);
    tsw_ep_set_mac_mode(ptr, TSW_TSNPORT_PORT2, tsw_mac_mode_gmii);
    tsw_ep_set_mac_mode(ptr, TSW_TSNPORT_PORT3, tsw_mac_mode_gmii);

    /* ���ö˿����ʺ�PHY�ӿ� */
    tsw_port_gpr(ptr, TSW_TSNPORT_PORT1, tsw_port_speed_1000mbps, tsw_port_phy_itf_rgmii, 0x8, 0);
    tsw_port_gpr(ptr, TSW_TSNPORT_PORT2, tsw_port_speed_1000mbps, tsw_port_phy_itf_rgmii, 0x8, 0);
    tsw_port_gpr(ptr, TSW_TSNPORT_PORT3, tsw_port_speed_1000mbps, tsw_port_phy_itf_rgmii, 0x8, 0);

    /* ʹ������MAC��TX/RX���� */
    tsw_ep_enable_all_mac_ctrl(ptr, tsw_mac_type_emac);
    tsw_ep_enable_all_mac_ctrl(ptr, tsw_mac_type_pmac);
    
    /* ���CAM���ȴ���� */
    tsw_clear_hitmem_table();
    tsw_clear_cam(ptr);
    board_delay_ms(10);

    /* ����VLAN��֡ת������ */

    /* �������ж˿ڵ�Ĭ��VLAN ID(TSW_DEFAULT_VID) */
    tsw_set_cam_vid(ptr, tsw_dst_port_cpu | tsw_dst_port_1 | tsw_dst_port_2 | tsw_dst_port_3, TSW_DEFAULT_VID);
    /* ��CPU�˿�MAC��ַ��ӵ�MAC��(Ĭ��VLAN) */
    tsw_set_mac_table(HPM_TSW, 0, tsw_dst_port_cpu, mac2cpu, TSW_DEFAULT_VID);
    
    tsw_set_cam_vlan_port(ptr);
    tsw_set_unknown_frame_action(HPM_TSW, tsw_dst_port_cpu | tsw_dst_port_3 | tsw_dst_port_2 | tsw_dst_port_1);
    tsw_set_broadcast_frame_action(HPM_TSW, tsw_dst_port_cpu | tsw_dst_port_3 | tsw_dst_port_2 | tsw_dst_port_1);
    //tsw_set_internal_frame_action(HPM_TSW, tsw_dst_port_cpu | tsw_dst_port_3 | tsw_dst_port_2 | tsw_dst_port_1);

    // tsw_enable_store_forward_mode(HPM_TSW, TSW_TSNPORT_PORT1);
    // tsw_enable_store_forward_mode(HPM_TSW, TSW_TSNPORT_PORT2);
    // tsw_enable_store_forward_mode(HPM_TSW, TSW_TSNPORT_PORT3);

    /* ��ȡĬ��DMA���ò���ʼ������ͨ�� */
    tsw_get_default_dma_config(&config);
    tsw_init_send(ptr, &config);
    for (uint8_t i = 0; i < TSW_SEND_DESC_COUNT; i++) {
        *tsw_tx_buff[i] = TSW_TSNPORT_PORT3 + 1;
    }

    /* ��ʼ������ͨ�� */
    tsw_init_recv(ptr, &config);
    for (uint8_t i = 0; i < TSW_RECV_DESC_COUNT; i++) {
        tsw_commit_recv_desc(ptr, (void*)tsw_rx_buff[i], TSW_RECV_BUFF_LEN, i);
    }

    /* ʹ��TSW�ж� */
    intc_m_enable_irq(IRQn_TSW_0);

    /* ����MDIOʱ��Ƶ��Ϊ2.5MHz */
    tsw_ep_set_mdio_config(HPM_TSW, TSW_TSNPORT_PORT1, 19);
    tsw_ep_set_mdio_config(HPM_TSW, TSW_TSNPORT_PORT2, 19);
    tsw_ep_set_mdio_config(HPM_TSW, TSW_TSNPORT_PORT3, 19);

    /* ��ʼ��PHYӲ�� */
    reset_tsw_phy_port3();
    board_delay_ms(10);
    reset_tsw_phy_port1();
    board_delay_ms(10);
    reset_tsw_phy_port2();

    yt8531_reset(HPM_TSW, TSW_TSNPORT_PORT1, 0);
    yt8531_reset(HPM_TSW, TSW_TSNPORT_PORT2, 0);
    yt8531_reset(HPM_TSW, TSW_TSNPORT_PORT3, 0);

    board_delay_ms(1000);  // �ȴ�PHY�ȶ�

    /* ����PHY��������ģʽ */
    yt8531_basic_mode_default_config(HPM_TSW, &phy_config);

    if (yt8531_basic_mode_init(HPM_TSW, TSW_TSNPORT_PORT1, 0, &phy_config) == false) {
        printf("TSW phy1 init failed !\n");
        return status_fail;
    }

    if (yt8531_basic_mode_init(HPM_TSW, TSW_TSNPORT_PORT2, 0, &phy_config) == false) {
        printf("TSW phy2 init failed !\n");
        return status_fail;
    }

    if (yt8531_basic_mode_init(HPM_TSW, TSW_TSNPORT_PORT3, 0, &phy_config) == false) {
        printf("TSW phy3 init failed !\n");
        return status_fail;
    }

    printf("TSW phy all init passed !\n");
    return status_success;
}

/**
 * @brief ��ȡTSW�˿�����״̬
 * @param index �˿�������TSW_TSNPORT_PORT1/2/3��
 * @return ����״̬��true-�����ӣ�false-δ����
 */
bool tsw_get_link_status(uint8_t index)
{
    tsw_self_adaptive_port_speed();
    switch (index) {
        case TSW_TSNPORT_PORT1:
            return (phy1_last_status.tsw_phy_link == tsw_phy_link_up) ? true : false;
        case TSW_TSNPORT_PORT2:
            return (phy2_last_status.tsw_phy_link == tsw_phy_link_up) ? true : false;
        case TSW_TSNPORT_PORT3:
            return (phy3_last_status.tsw_phy_link == tsw_phy_link_up) ? true : false;
        default:
            return false;
    }
}

/**
 * @brief �弶TSW��ʼ����ں���
 * @return 0-�ɹ�������-ʧ��
 * @note ��ʼ��GPIO��PHY��λ����ʱ����TSW������
 */
int Bsp_InitTsw(void)
{
    tsw_fpe_config_t fpe_config;

    /* ��ʼ��TSW���GPIO */
    board_init_tsw();

    //tsw_get_mac_address(mac2cpu);
    tsw_mac_table_init(mac2cpu, TSW_DEFAULT_VID);
    /* ��λPHY�˿ڣ�Ӳ����λ�� */
    board_tsw_phy_set(TSW_TSNPORT_PORT1, 0);
    board_delay_ms(10);
    board_tsw_phy_set(TSW_TSNPORT_PORT1, 1);
    board_delay_ms(10);

    board_tsw_phy_set(TSW_TSNPORT_PORT2, 0);
    board_delay_ms(10);
    board_tsw_phy_set(TSW_TSNPORT_PORT2, 1);
    board_delay_ms(10);

    board_tsw_phy_set(TSW_TSNPORT_PORT3, 0);
    board_delay_ms(10);
    board_tsw_phy_set(TSW_TSNPORT_PORT3, 1);
    board_delay_ms(10);

    /* ����ϵͳ��ʱ�������������Լ�⣩ */
    board_timer_create(1, sys_timer_callback);

    /* ����TSWʱ���ӳ� */
    tsw_set_rtc_time_increment(HPM_TSW, (10 << 24));
    tsw_set_port_clock_delay(HPM_TSW, TSW_TSNPORT_PORT1, 0x8, 0);
    tsw_set_port_clock_delay(HPM_TSW, TSW_TSNPORT_PORT2, 0x8, 0);
    tsw_set_port_clock_delay(HPM_TSW, TSW_TSNPORT_PORT3, 0x8, 0);

    /* ��ʼ��TSW������ */
    if (tsw_init(HPM_TSW) == 0) {
        tsw_fpe_get_default_mms_ctrl_config(HPM_TSW, TSW_TSNPORT_PORT1, &fpe_config);
        tsw_fpe_set_mms_ctrl(HPM_TSW, TSW_TSNPORT_PORT1, &fpe_config);
        tsw_fpe_enable_mms(HPM_TSW, TSW_TSNPORT_PORT1);
        tsw_fpe_get_default_mms_ctrl_config(HPM_TSW, TSW_TSNPORT_PORT2, &fpe_config);
        tsw_fpe_set_mms_ctrl(HPM_TSW, TSW_TSNPORT_PORT2, &fpe_config);
        tsw_fpe_enable_mms(HPM_TSW, TSW_TSNPORT_PORT2);
        printf("TSW initialization success !!!\n");
    } else {
        printf("TSW initialization fails !!!\n");
        return -1;
    }
    return 0;
}

/**
 * @brief ϵͳ��TSW֡���ͺ������ײ�ʵ�֣�
 * @param payload ����������ָ��
 * @param len ���ݳ���
 * @return 0-�ɹ�
 * @note ϵͳ�ڲ�ʹ�õķ��ͽӿڣ����û����ͳ�ͻ���
 */
ATTR_RAMFUNC
int Bsp_TransmitTswFrameLowLevel(uint8_t *payload, int len)
{
    hpm_stat_t stat_s;
    uint32_t port;
    static uint32_t i = 0;
    uint8_t id;
    uint8_t send_port = 1;

    if (g_tsw_queue > 7) {
        g_tsw_queue = 7;
    }

    if (send_global_port == 255) {
        send_port = TSW_TSNPORT_PORT3 + 1;
    } else {
        send_port = send_global_port;
    }

    tx_hdr_desc_t hdr = {
        .tx_hdr0_bm.dest_port = send_port,
        .tx_hdr0_bm.queue = g_tsw_queue,
    };

    id = i++ % TSW_SOC_DMA_MAX_DESC_COUNT;

    
    memcpy((void*)tsw_tx_buff[id], &hdr, sizeof(tx_hdr_desc_t));
    memcpy((void*)tsw_tx_buff[id] + sizeof(tx_hdr_desc_t), payload, len);
    len += sizeof(tx_hdr_desc_t);
    
    // stat = tsw_send_frame_check_response(HPM_TSW, (void*)tsw_tx_buff[id], len, id);
    stat_s = tsw_send_frame(HPM_TSW, (void*)tsw_tx_buff[id], len, id);

    if (stat_s == status_success) {
        send_success_time++;
    } else {
        send_fail_time++;
        if (port == 8080) {
            /* Ԥ���˿�8080���ض������߼� */
        }
        return -1;
    }
    port = tsw_tx_buff[id][50] << 8 | tsw_tx_buff[id][51];
    return 0;
}

/**
 * @brief ��ȡTSW��MAC��ַ
 * @param pMac ������������ڴ洢MAC��ַ�Ļ�����������6�ֽڣ�
 */
void Bsp_GetTswMacAddr(uint8_t *pMac)
{
    memcpy(pMac, mac2cpu, 6);
}

/**
 * @brief TSW֡���պ������ײ�ʵ�֣�
 * @param payload �������ݻ�����ָ��
 * @param toReadLen ������ȡ�ĳ��ȣ�δʹ�ã�
 * @return ʵ�ʽ��յ�֡����
 * @note �ӻ��λ�������ȡ֡���ݣ����ָ�/�����ȼ�֡�����Ƴ�queue����
 */
ATTR_RAMFUNC
int Bsp_ReceiveTswFrameLowLevel(uint8_t *payload, int toReadLen)
{
    (void)toReadLen;  // δʹ�ò���
    uint32_t frameLen = 0, bodyLen = 0;
    rx_hdr_desc_t *rx_hdr = NULL;
    uint8_t recv_frame[1536];

    // ��������ȼ�֡���У�����ռ��
    if (!ByteRingBuf_IsEmpty(&TSWCurveFifo_e)) {
        frameLen = ByteRingBuf_ReadFrame(&TSWCurveFifo_e, data_buf_e, sizeof(data_buf_e));
        if (frameLen > TSW_RX_HDR_LEN) {
            rx_hdr = (rx_hdr_desc_t *)data_buf_e;
            bodyLen = frameLen - TSW_RX_HDR_LEN;

            memcpy(recv_frame, data_buf_e, frameLen);
            memcpy(payload, data_buf_e + TSW_RX_HDR_LEN, bodyLen);
            recv_global_port = rx_hdr->rx_hdr0_bm.src_port;
        }
    }
    if (!ByteRingBuf_IsEmpty(&TSWCurveFifo_p)) {
        frameLen = ByteRingBuf_ReadFrame(&TSWCurveFifo_p, data_buf_p, sizeof(data_buf_p));
        if (frameLen > TSW_RX_HDR_LEN) {
            rx_hdr = (rx_hdr_desc_t *)data_buf_p;
            bodyLen = frameLen - TSW_RX_HDR_LEN;
            
            memcpy(recv_frame, data_buf_p, frameLen);
            memcpy(payload, data_buf_p + TSW_RX_HDR_LEN, bodyLen);
            recv_global_port = rx_hdr->rx_hdr0_bm.src_port;
        }
    }

    rx_flag = false;
    return bodyLen;
}

/**
 * @brief TSW�˿�CPU�жϷ������
 * @note ����TSW�����жϣ���֡���������Ӧ���λ�����
 */
SDK_DECLARE_EXT_ISR_M(IRQn_TSW_0, isr_tsw_port_cpu)
void isr_tsw_port_cpu(void)
{
    intc_m_disable_irq(IRQn_TSW_0);
    hpm_stat_t stat_r;
    static int idx = 0;
    rx_flag = true;
    idx = idx % TSW_SOC_DMA_MAX_DESC_COUNT;
    stat_r = tsw_recv_frame(HPM_TSW, (void*)&frame[idx]);

    rx_hdr_desc_t *rx_hdr = (rx_hdr_desc_t *)&tsw_rx_buff[frame[idx].id][0];
    if (stat_r == status_success) {
        tsw_mac_table_learn((void*)tsw_rx_buff[frame[idx].id], frame[idx].length);
        bool is_preemptible = (rx_hdr->rx_hdr0_bm.fpe == 1);

        if (!is_preemptible) {
            ByteRingBuf_WriteFrame(&TSWCurveFifo_e, (void*)&tsw_rx_buff[frame[idx].id][0], frame[idx].length);
            FPE_0++;
        }else {
            ByteRingBuf_WriteFrame(&TSWCurveFifo_p, (void*)&tsw_rx_buff[frame[idx].id][0], frame[idx].length);
            FPE_1++;
        }
        memcpy(data_buf_e, (void*)&tsw_rx_buff[frame[idx].id][0], frame[idx].length);
        tsw_commit_recv_desc(HPM_TSW, (void*)tsw_rx_buff[frame[idx].id], TSW_RECV_BUFF_LEN, frame[idx].id);
    } else {
        tsw_commit_recv_desc(HPM_TSW, (void*)tsw_rx_buff[frame[idx].id], TSW_RECV_BUFF_LEN, frame[idx].id);
    }
    
    idx++;
    irq_frame_cnt = idx;
    intc_m_enable_irq_with_priority(IRQn_TSW_0, 7);
}
