// data_u.h������/�ӻ�������ȫһ�£�
#ifndef __DATA_U_H
#define __DATA_U_H

#include <stdint.h>
#include <stddef.h>

// ���ĺ궨�壨ȷ���ֶ�ƫ����ȷ��
#define DEVICE_ID_OFFSET_TARGET 42  // device_id��42�ֽڿ�ʼ
#define RESERVED_LEN (DEVICE_ID_OFFSET_TARGET - (6 + 6 + 2)) // reserved=28�ֽ�
#define VALID_DATA_LEN 64           // payloadԭʼ���ȣ����޸ģ�
#define EXTRA_DATA_LEN 32           // �����ֶγ��ȣ��洢�����ݣ�
#define TOTAL_FRAME_LEN 1536        // TSW��׼֡�����̶���

/**
 * ͳһ֡�ṹ�����㡰���ͷ���ʱ��������շ����豸ID��payload���޸ġ�����
 * �ֶ�ƫ�Ƽ��㣨�ܳ���1522�ֽڣ���
 * 0-5:  dest_mac        | 6-11: src_mac         | 12-13: eth_type
 * 14-41: reserved       | 42-47: device_id      | 48-51: num
 * 52-55: num_flag       | 56-63: sender_ts      | 64-69: src_device_id
 * 70-101: extra_data    | 102-1521: payload     | ����1522�ֽڣ�
 * 
 * �ؼ�����
 * 1. sender_ts: ���ͷ�����Լ���ʱ��������뼶��
 * 2. src_device_id: ���շ���䡰ԭ���ͷ���device_id��
 * 3. extra_data: �洢�����ݣ�����շ�����״̬��
 * 4. payload: ��ȫ���޸ģ�����/���;�ԭ������
 */
typedef struct {
    // ���������ֶΣ����䣩
    uint8_t dest_mac[6];        // 0-5��Ŀ��MAC
    uint8_t src_mac[6];         // 6-11�����ͷ�����MAC
    uint8_t eth_type[2];        // 12-13����̫�����ͣ��̶�0x0806��
    uint8_t reserved[RESERVED_LEN]; // 14-41��Ԥ����ȷ��device_idƫ����ȷ��

    // ���ͷ���ʶ��ʱ��������ͷ���䣩
    uint8_t device_id[6];       // 42-47�����ͷ������豸ID����������FF:FF:FF:FF:FF:CC��
    uint32_t num;               // 48-51�����ڼ�����0-19ѭ����
    uint32_t num_flag;          // 52-55��������־��19��1��������0��
    uint64_t sender_ts;         // 56-63�����ͷ���ʱ��������������ͷ����Լ���ʱ�䣩

    // ���շ�����ֶΣ����շ�����ʱ��䣩
    uint8_t src_device_id[6];   // 64-69��ԭ���ͷ���device_id�����շ��
    uint8_t extra_data[EXTRA_DATA_LEN]; // 70-101���������洢�����ݣ���ռ��payload��

    // ���ģ�payload��ȫ���޸�
    uint8_t payload[TOTAL_FRAME_LEN - 102]; // 102-1521��payload��1522-102=1420�ֽڣ���ԭ64�ֽ���Ч���ݣ�
} TswFrame_t;

// �豸���ã�����/�ӻ����֣�
#define HOST_MAC          {0x4E, 0x00, 0x00, 0x00, 0xF0, 0x32}
#define HOST_DEVICE_ID    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xCC}
#define SLAVE_MAC         {0x98, 0x2C, 0xBC, 0xB1, 0x9F, 0x10}
#define SLAVE_DEVICE_ID   {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEE}

// ȫ�ֱ�������������/�ӻ�����������
extern TswFrame_t s_tsw_frame;
extern uint8_t data_buff[]; // ����ԭʼpayload����

#endif // __DATA_U_H