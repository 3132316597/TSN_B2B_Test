// data_u.h（主机/从机必须完全一致）
#ifndef __DATA_U_H
#define __DATA_U_H

#include <stdint.h>
#include <stddef.h>

// 核心宏定义（确保字段偏移正确）
#define DEVICE_ID_OFFSET_TARGET 42  // device_id从42字节开始
#define RESERVED_LEN (DEVICE_ID_OFFSET_TARGET - (6 + 6 + 2)) // reserved=28字节
#define VALID_DATA_LEN 64           // payload原始长度（不修改）
#define EXTRA_DATA_LEN 32           // 新增字段长度（存储新数据）
#define TOTAL_FRAME_LEN 1536        // TSW标准帧长（固定）

/**
 * 统一帧结构：满足“发送方带时间戳、接收方填设备ID、payload不修改”需求
 * 字段偏移计算（总长度1522字节）：
 * 0-5:  dest_mac        | 6-11: src_mac         | 12-13: eth_type
 * 14-41: reserved       | 42-47: device_id      | 48-51: num
 * 52-55: num_flag       | 56-63: sender_ts      | 64-69: src_device_id
 * 70-101: extra_data    | 102-1521: payload     | （总1522字节）
 * 
 * 关键规则：
 * 1. sender_ts: 发送方填充自己的时间戳（毫秒级）
 * 2. src_device_id: 接收方填充“原发送方的device_id”
 * 3. extra_data: 存储新数据（如接收方处理状态）
 * 4. payload: 完全不修改，接收/发送均原样传递
 */
typedef struct {
    // 基础网络字段（不变）
    uint8_t dest_mac[6];        // 0-5：目的MAC
    uint8_t src_mac[6];         // 6-11：发送方自身MAC
    uint8_t eth_type[2];        // 12-13：以太网类型（固定0x0806）
    uint8_t reserved[RESERVED_LEN]; // 14-41：预留（确保device_id偏移正确）

    // 发送方标识与时间戳（发送方填充）
    uint8_t device_id[6];       // 42-47：发送方自身设备ID（如主机：FF:FF:FF:FF:FF:CC）
    uint32_t num;               // 48-51：周期计数（0-19循环）
    uint32_t num_flag;          // 52-55：计数标志（19→1，其他→0）
    uint64_t sender_ts;         // 56-63：发送方的时间戳（新增！发送方填自己的时间）

    // 接收方填充字段（接收方处理时填充）
    uint8_t src_device_id[6];   // 64-69：原发送方的device_id（接收方填）
    uint8_t extra_data[EXTRA_DATA_LEN]; // 70-101：新增！存储新数据（不占用payload）

    // 核心：payload完全不修改
    uint8_t payload[TOTAL_FRAME_LEN - 102]; // 102-1521：payload（1522-102=1420字节，含原64字节有效数据）
} TswFrame_t;

// 设备配置（主机/从机区分）
#define HOST_MAC          {0x4E, 0x00, 0x00, 0x00, 0xF0, 0x32}
#define HOST_DEVICE_ID    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xCC}
#define SLAVE_MAC         {0x98, 0x2C, 0xBC, 0xB1, 0x9F, 0x10}
#define SLAVE_DEVICE_ID   {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEE}

// 全局变量声明（主机/从机均需声明）
extern TswFrame_t s_tsw_frame;
extern uint8_t data_buff[]; // 主机原始payload数据

#endif // __DATA_U_H