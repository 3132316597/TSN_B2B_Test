#include "board.h" 
#include "tswLowLevel.h"
#include <stdio.h>
#include <string.h>
#include "bsp_gptmr.h" 
#include "data_u.h"

// �ָ��ߺ�
#define PRINT_MAIN_SEP() printf("=========================================================\n")
#define PRINT_SUB_SEP()  printf("---------------------------------------------------------\n")

// ȫ�ֱ���
uint64_t idx = 0, num_id = 0;
uint8_t recv_data[TSW_RECV_DESC_COUNT][1536];
uint16_t recv_len;                  // ʵ�ʽ��ճ���
rx_hdr_desc_t local_rx_hdr;         // ����֡ͷ����
uint8_t self_mac[6] = SLAVE_MAC;    // �ӻ�����MAC
uint8_t self_device_id[6] = SLAVE_DEVICE_ID; // �ӻ������豸ID
uint8_t host_mac[6] = HOST_MAC;     // ����MAC�������ã�
uint32_t last_fpe_cnt = 0;          // FPE������ȥ��

// �ӻ�����֡���ǻ���������ʼ��Ϊ0��
ATTR_PLACE_AT_NONCACHEABLE TswFrame_t s_tsw_frame = {0};

// -------------------------- ������������ӡMAC��ַ --------------------------
static void print_mac(const char *label, const uint8_t *mac) {
    printf("%s: %02X:%02X:%02X:%02X:%02X:%02X\n", 
           label, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

// -------------------------- ������������ӡ�ؼ��ֶΣ������ã� --------------------------
static void print_frame_key_info(const char *label, const TswFrame_t *frame) {
    printf("[%s] Key Info:\n", label);
    print_mac("  Device ID (Sender's own ID)", frame->device_id);
    print_mac("  Src Device ID (Original Sender's ID)", frame->src_device_id);
    
    uint32_t sec = (uint32_t)(frame->sender_ts / 1000);
    uint32_t nsec = (uint32_t)((frame->sender_ts % 1000) * 1000000); // ����ת����
    printf("  Sender Timestamp: %d.%09d\n", sec, nsec);
    
    printf("  Extra Data (first 8 bytes): ");
    for (int i=0; i<8; i++) printf("%02X ", frame->extra_data[i]);
    printf("\n  Payload (first 8 bytes, unmodified): ");
    for (int i=0; i<8; i++) printf("%02X ", frame->payload[i]);
    printf("\n");
}

// -------------------------- 1. �ӻ����췢��֡��������ʱ��� --------------------------
int DATA_MAKE() {
    memset(&s_tsw_frame, 0, sizeof(TswFrame_t)); // ���֡�ṹ����payload�⣩
    num_id++;
    uint32_t num = num_id % 20;

    // 1. �����ֶΣ����ͷ������ʶ���ӻ���
    memcpy(s_tsw_frame.dest_mac, host_mac, 6);    // Ŀ��MAC���������ɸ�Ϊ�㲥��
    memcpy(s_tsw_frame.src_mac, self_mac, 6);     // ԴMAC���ӻ�����
    memcpy(s_tsw_frame.device_id, self_device_id, 6); // �豸ID���ӻ�����

    // 2. ���ģ����ͷ�ʱ������ӻ��Լ���ʱ��������뼶��
    s_tsw_frame.sender_ts = Bsp_GetMchtmrTimestampMs();

    // 3. ���ڼ�����ԭ���߼�������
    s_tsw_frame.num = num;
    s_tsw_frame.num_flag = (num == 19) ? 1 : 0;

    // 4. ��̫�����ͣ��̶���
    s_tsw_frame.eth_type[0] = 0x08;
    s_tsw_frame.eth_type[1] = 0x06;

    // 5. �����ֶΣ�extra_data��������
    s_tsw_frame.extra_data[0] = (uint8_t)(num & 0xFF);          // ��8λ���ڼ���
    s_tsw_frame.extra_data[1] = 0x01;                          // ״̬���������Զ��壩
    memcpy(&s_tsw_frame.extra_data[4], &s_tsw_frame.sender_ts, 4); // ʱ�����4�ֽ�

    return 0;
}

// -------------------------- 2. �ӻ����ͻص���Я������ʱ������� --------------------------
static void TswSendCallback_500ms(void) {
    static uint64_t last_send_ts = 0;
    uint32_t curr_fpe = 0;
    uint64_t curr_ts = Bsp_GetMchtmrTimestampMs();

    // ����ƫ��棨ʹ���µ�ʱ���ʽ��
    if (last_send_ts != 0) {
        uint32_t period = curr_ts - last_send_ts;
        if (abs((int)(period - 500)) > 10) {
            uint32_t sec = (uint32_t)(curr_ts / 1000);
            uint32_t nsec = (uint32_t)((curr_ts % 1000) * 1000000);
            printf("[Slave Warn] At %d.%09d: Send period deviate: %dms (expect 500ms)\n",
                   sec, nsec, (int)period);
        }
    }
    last_send_ts = curr_ts;

    // ���˿�����
    if (!(tsw_get_link_status(TSW_TSNPORT_PORT1) || tsw_get_link_status(TSW_TSNPORT_PORT2))) {
        printf("[Slave Send] No port linked, skip\n");
        PRINT_SUB_SEP();
        return;
    }

    // ���췢��֡��������ʱ�����
    DATA_MAKE();

    // ��ӡ����ǰ�ؼ���Ϣ��ʹ���µ�ʱ���ʽ��
    PRINT_SUB_SEP();
    printf("[Slave Send] Prepare to send (port: 2, len: %d bytes)\n", TOTAL_FRAME_LEN);
    print_frame_key_info("Slave Send Frame", &s_tsw_frame);

    // ����֡
    g_tsw_queue = 1;
    send_global_port = 2;
    hpm_stat_t send_stat = Bsp_TransmitTswFrameLowLevel((uint8_t*)&s_tsw_frame, 1024);
    g_tsw_queue = 0;
    Bsp_TransmitTswFrameLowLevel((uint8_t*)&s_tsw_frame, 101);

    // ���ͽ����־
    if (send_stat != status_success) {
        printf("[Slave Send] Failed! Status: %d\n", send_stat);
    } else {
        tsw_fpe_get_mms_statistics_counter(HPM_TSW, TSW_TSNPORT_PORT2, 
                                          tsw_fpe_mms_fragment_tx_counter, &curr_fpe);
        if (curr_fpe != last_fpe_cnt) {
            printf("[Slave Send] Success! FPE Counter: %d\n", curr_fpe);
            last_fpe_cnt = curr_fpe;
        }
    }
    PRINT_MAIN_SEP();
}

// -------------------------- 3. �ӻ����ս�������ԭ���ͷ��豸ID --------------------------
static bool parse_host_frame(const uint8_t *recv_data, uint16_t len, 
                            TswFrame_t *recv_frame, uint8_t *original_sender_id) {
    // 1. ���֡����
    if (len < 101) {
        printf("[Slave Parse Err] Frame too short (need ��%zu bytes, got %d)\n", 
               offsetof(TswFrame_t, payload), len);
        return false;
    }

    // 2. ת��Ϊͳһ֡�ṹ
    *recv_frame = *(TswFrame_t *)recv_data;

    // 3. ��ȡԭ���ͷ���device_id
    memcpy(original_sender_id, recv_frame->device_id, 6);

    // 4. ��ӡ���չؼ���Ϣ��ʹ���µ�ʱ���ʽ��
    printf("[Slave Recv] Got host frame (len: %d bytes)\n", len);
    print_frame_key_info("Host Send Frame (Received)", recv_frame);

    // �ؼ���payload���޸�
    memcpy(s_tsw_frame.payload, recv_frame->payload, sizeof(s_tsw_frame.payload));

    return true;
}

// -------------------------- 4. �ӻ�����֡����ԭ���ͷ��豸ID --------------------------
static void repack_slave_frame(const uint8_t *original_sender_id, const TswFrame_t *recv_host_frame) {
    // 1. ����ԭ���ͷ��豸ID
    memcpy(s_tsw_frame.src_device_id, original_sender_id, 6);

    // 2. �������ݣ���¼����ʱ�䣨ת��Ϊ����洢��
    uint64_t recv_ts = Bsp_GetMchtmrTimestampMs();
    uint32_t recv_sec = (uint32_t)(recv_ts / 1000);
    uint32_t recv_nsec = (uint32_t)((recv_ts % 1000) * 1000000);
    memcpy(&s_tsw_frame.extra_data[8], &recv_sec, 4);    // ��������
    memcpy(&s_tsw_frame.extra_data[12], &recv_nsec, 4);  // ��������

    printf("[Slave Repack] Frame updated:\n");
    print_mac("  Filled Src Device ID (Host's ID)", s_tsw_frame.src_device_id);
    printf("  Filled Receive Time: %d.%09d\n", recv_sec, recv_nsec);
}

// -------------------------- ������ --------------------------
int main(void) {
    board_init();
    printf("Slave TSW Frame Demo Start!\n");
    print_mac("Self MAC ", self_mac);
    print_mac("Dest MAC ", host_mac);
    printf("Frame Total Len: %d bytes | Payload Len: %zu bytes | Extra Data Len: %d bytes\n",
           TOTAL_FRAME_LEN, sizeof(s_tsw_frame.payload), EXTRA_DATA_LEN);
    PRINT_MAIN_SEP();

    // ��ʼ��TSW
    if (Bsp_InitTsw() != status_success) {
        printf("[Slave Init Err] TSW init failed!\n");
        while (1) {}
    }

    // ����500ms���ڷ���
    Bsp_InitTswSendTmr(TswSendCallback_500ms);

    // ����ѭ������������֡
    while (1) {
        uint32_t id = idx % TSW_RECV_DESC_COUNT;
        if (tsw_get_link_status(TSW_TSNPORT_PORT1) || tsw_get_link_status(TSW_TSNPORT_PORT2)) {
            // ����֡ͷ����
            if (g_rx_hdr_updated) {
                local_rx_hdr = g_rx_hdr_cache;
                g_rx_hdr_updated = false;
            }

            if (rx_flag) {
                bool local_rx_flag = rx_flag;
                if (local_rx_flag) {
                    rx_flag = false;
                    idx++;
                    recv_len = Bsp_ReceiveTswFrameLowLevel(recv_data[id], sizeof(recv_data[id]));
                }

                if (local_rx_flag && recv_len > 0) {
                    // ���˷�����֡
                    uint8_t recv_src_mac[6];
                    memcpy(recv_src_mac, recv_data[id] + 6, 6);
                    if (memcmp(recv_src_mac, host_mac, 6) != 0) {
                        printf("[Slave Recv] Skip non-host frame (src MAC: ");
                        for (int i=0; i<6; i++) printf("%02X:", recv_src_mac[i]);
                        printf("\b)\n");
                        PRINT_SUB_SEP();
                        continue;
                    }

                    // ��������֡
                    TswFrame_t recv_host_frame;
                    uint8_t host_device_id[6] = {0};
                    bool parse_ok = parse_host_frame(recv_data[id], recv_len, &recv_host_frame, host_device_id);

                    // ����ӻ�����֡
                    if (parse_ok) {
                        repack_slave_frame(host_device_id, &recv_host_frame);
                    }

                    PRINT_MAIN_SEP();
                }
            }
        }
    }
}
