#include "board.h" 
#include "tswLowLevel.h"
#include <stdio.h>
#include <string.h>
#include "bsp_gptmr.h" 
#include "data_u.h"

// 分隔线宏
#define PRINT_MAIN_SEP() printf("=========================================================\n")
#define PRINT_SUB_SEP()  printf("---------------------------------------------------------\n")

// 全局变量
uint64_t idx = 0, num_id = 0;
uint8_t recv_data[TSW_RECV_DESC_COUNT][1536];
uint16_t recv_len;                  // 实际接收长度
rx_hdr_desc_t local_rx_hdr;         // 接收帧头缓存
uint8_t self_mac[6] = SLAVE_MAC;    // 从机自身MAC
uint8_t self_device_id[6] = SLAVE_DEVICE_ID; // 从机自身设备ID
uint8_t host_mac[6] = HOST_MAC;     // 主机MAC（过滤用）
uint32_t last_fpe_cnt = 0;          // FPE计数器去重

// 从机发送帧（非缓存区，初始化为0）
ATTR_PLACE_AT_NONCACHEABLE TswFrame_t s_tsw_frame = {0};

// -------------------------- 辅助函数：打印MAC地址 --------------------------
static void print_mac(const char *label, const uint8_t *mac) {
    printf("%s: %02X:%02X:%02X:%02X:%02X:%02X\n", 
           label, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

// -------------------------- 辅助函数：打印关键字段（调试用） --------------------------
static void print_frame_key_info(const char *label, const TswFrame_t *frame) {
    printf("[%s] Key Info:\n", label);
    print_mac("  Device ID (Sender's own ID)", frame->device_id);
    print_mac("  Src Device ID (Original Sender's ID)", frame->src_device_id);
    
    uint32_t sec = (uint32_t)(frame->sender_ts / 1000);
    uint32_t nsec = (uint32_t)((frame->sender_ts % 1000) * 1000000); // 毫秒转纳秒
    printf("  Sender Timestamp: %d.%09d\n", sec, nsec);
    
    printf("  Extra Data (first 8 bytes): ");
    for (int i=0; i<8; i++) printf("%02X ", frame->extra_data[i]);
    printf("\n  Payload (first 8 bytes, unmodified): ");
    for (int i=0; i<8; i++) printf("%02X ", frame->payload[i]);
    printf("\n");
}

// -------------------------- 1. 从机构造发送帧：带自身时间戳 --------------------------
int DATA_MAKE() {
    memset(&s_tsw_frame, 0, sizeof(TswFrame_t)); // 清空帧结构（除payload外）
    num_id++;
    uint32_t num = num_id % 20;

    // 1. 基础字段：发送方自身标识（从机）
    memcpy(s_tsw_frame.dest_mac, host_mac, 6);    // 目的MAC：主机（可改为广播）
    memcpy(s_tsw_frame.src_mac, self_mac, 6);     // 源MAC：从机自身
    memcpy(s_tsw_frame.device_id, self_device_id, 6); // 设备ID：从机自身

    // 2. 核心：发送方时间戳（从机自己的时间戳，毫秒级）
    s_tsw_frame.sender_ts = Bsp_GetMchtmrTimestampMs();

    // 3. 周期计数（原有逻辑保留）
    s_tsw_frame.num = num;
    s_tsw_frame.num_flag = (num == 19) ? 1 : 0;

    // 4. 以太网类型（固定）
    s_tsw_frame.eth_type[0] = 0x08;
    s_tsw_frame.eth_type[1] = 0x06;

    // 5. 新增字段：extra_data填新数据
    s_tsw_frame.extra_data[0] = (uint8_t)(num & 0xFF);          // 低8位周期计数
    s_tsw_frame.extra_data[1] = 0x01;                          // 状态：正常（自定义）
    memcpy(&s_tsw_frame.extra_data[4], &s_tsw_frame.sender_ts, 4); // 时间戳低4字节

    return 0;
}

// -------------------------- 2. 从机发送回调：携带自身时间戳发送 --------------------------
static void TswSendCallback_500ms(void) {
    static uint64_t last_send_ts = 0;
    uint32_t curr_fpe = 0;
    uint64_t curr_ts = Bsp_GetMchtmrTimestampMs();

    // 周期偏差警告（使用新的时间格式）
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

    // 检查端口链接
    if (!(tsw_get_link_status(TSW_TSNPORT_PORT1) || tsw_get_link_status(TSW_TSNPORT_PORT2))) {
        printf("[Slave Send] No port linked, skip\n");
        PRINT_SUB_SEP();
        return;
    }

    // 构造发送帧（带自身时间戳）
    DATA_MAKE();

    // 打印发送前关键信息（使用新的时间格式）
    PRINT_SUB_SEP();
    printf("[Slave Send] Prepare to send (port: 2, len: %d bytes)\n", TOTAL_FRAME_LEN);
    print_frame_key_info("Slave Send Frame", &s_tsw_frame);

    // 发送帧
    g_tsw_queue = 1;
    send_global_port = 2;
    hpm_stat_t send_stat = Bsp_TransmitTswFrameLowLevel((uint8_t*)&s_tsw_frame, 1024);
    g_tsw_queue = 0;
    Bsp_TransmitTswFrameLowLevel((uint8_t*)&s_tsw_frame, 101);

    // 发送结果日志
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

// -------------------------- 3. 从机接收解析：填原发送方设备ID --------------------------
static bool parse_host_frame(const uint8_t *recv_data, uint16_t len, 
                            TswFrame_t *recv_frame, uint8_t *original_sender_id) {
    // 1. 检查帧长度
    if (len < 101) {
        printf("[Slave Parse Err] Frame too short (need ≥%zu bytes, got %d)\n", 
               offsetof(TswFrame_t, payload), len);
        return false;
    }

    // 2. 转换为统一帧结构
    *recv_frame = *(TswFrame_t *)recv_data;

    // 3. 提取原发送方的device_id
    memcpy(original_sender_id, recv_frame->device_id, 6);

    // 4. 打印接收关键信息（使用新的时间格式）
    printf("[Slave Recv] Got host frame (len: %d bytes)\n", len);
    print_frame_key_info("Host Send Frame (Received)", recv_frame);

    // 关键：payload不修改
    memcpy(s_tsw_frame.payload, recv_frame->payload, sizeof(s_tsw_frame.payload));

    return true;
}

// -------------------------- 4. 从机重组帧：填原发送方设备ID --------------------------
static void repack_slave_frame(const uint8_t *original_sender_id, const TswFrame_t *recv_host_frame) {
    // 1. 填入原发送方设备ID
    memcpy(s_tsw_frame.src_device_id, original_sender_id, 6);

    // 2. 新增数据：记录接收时间（转换为纳秒存储）
    uint64_t recv_ts = Bsp_GetMchtmrTimestampMs();
    uint32_t recv_sec = (uint32_t)(recv_ts / 1000);
    uint32_t recv_nsec = (uint32_t)((recv_ts % 1000) * 1000000);
    memcpy(&s_tsw_frame.extra_data[8], &recv_sec, 4);    // 接收秒数
    memcpy(&s_tsw_frame.extra_data[12], &recv_nsec, 4);  // 接收纳秒

    printf("[Slave Repack] Frame updated:\n");
    print_mac("  Filled Src Device ID (Host's ID)", s_tsw_frame.src_device_id);
    printf("  Filled Receive Time: %d.%09d\n", recv_sec, recv_nsec);
}

// -------------------------- 主函数 --------------------------
int main(void) {
    board_init();
    printf("Slave TSW Frame Demo Start!\n");
    print_mac("Self MAC ", self_mac);
    print_mac("Dest MAC ", host_mac);
    printf("Frame Total Len: %d bytes | Payload Len: %zu bytes | Extra Data Len: %d bytes\n",
           TOTAL_FRAME_LEN, sizeof(s_tsw_frame.payload), EXTRA_DATA_LEN);
    PRINT_MAIN_SEP();

    // 初始化TSW
    if (Bsp_InitTsw() != status_success) {
        printf("[Slave Init Err] TSW init failed!\n");
        while (1) {}
    }

    // 启用500ms周期发送
    Bsp_InitTswSendTmr(TswSendCallback_500ms);

    // 核心循环：接收主机帧
    while (1) {
        uint32_t id = idx % TSW_RECV_DESC_COUNT;
        if (tsw_get_link_status(TSW_TSNPORT_PORT1) || tsw_get_link_status(TSW_TSNPORT_PORT2)) {
            // 更新帧头缓存
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
                    // 过滤非主机帧
                    uint8_t recv_src_mac[6];
                    memcpy(recv_src_mac, recv_data[id] + 6, 6);
                    if (memcmp(recv_src_mac, host_mac, 6) != 0) {
                        printf("[Slave Recv] Skip non-host frame (src MAC: ");
                        for (int i=0; i<6; i++) printf("%02X:", recv_src_mac[i]);
                        printf("\b)\n");
                        PRINT_SUB_SEP();
                        continue;
                    }

                    // 解析主机帧
                    TswFrame_t recv_host_frame;
                    uint8_t host_device_id[6] = {0};
                    bool parse_ok = parse_host_frame(recv_data[id], recv_len, &recv_host_frame, host_device_id);

                    // 重组从机发送帧
                    if (parse_ok) {
                        repack_slave_frame(host_device_id, &recv_host_frame);
                    }

                    PRINT_MAIN_SEP();
                }
            }
        }
    }
}
