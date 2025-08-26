#ifndef __DATA_U_H
#define __DATA_U_H

#include <stdint.h>

typedef struct {
    uint8_t dest_mac[6];
    uint8_t src_mac[6];
    uint8_t eth_type[2];  
    uint8_t reserved[14]; 
    uint8_t device_id[6]; 
    uint32_t num;         
    uint32_t num_flag;    
    uint8_t device_id2[6];
    uint8_t payload[1474];
} TswFrame_t;

extern uint8_t data_buff[];
extern uint8_t data_buff1[];
#endif // __DATA_U_H