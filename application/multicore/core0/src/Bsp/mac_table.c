
//#include "lwip/prot/ieee.h"
#include "hpm_mchtmr_drv.h"

#include "mac_table.h"

#define ETHTYPE_VLAN 0x8100

ATTR_PLACE_AT_FAST_RAM_BSS uint8_t mac_null[TSW_MAC_ADDR_LEN] = {0x0};
ATTR_PLACE_AT_FAST_RAM_BSS tsn_mac_t mac_table[TSW_MAC_TABLE_MAX] = {0};

static int tsw_mac_table_find(uint8_t mac[]);
static int tsw_mac_table_null(void);
static int tsw_mac_table_time_min(void);
static int tsw_mac_table_add(uint8_t mac[], uint16_t vid, uint8_t prot);
static int tsw_mac_table_del(uint8_t index);;

ATTR_RAMFUNC void tsw_set_mac_table(TSW_Type *ptr, uint16_t entry_num, uint8_t dest_port, uint8_t* dest_mac, uint8_t vid)
{
    /* Create a new ALMEM entry. This will specify what will be done with those detected frames */
    if (TSW_APB2AXIS_ALMEM_STS_RDY_GET(ptr->APB2AXIS_ALMEM_STS)) {

        ptr->APB2AXIS_ALMEM_REQDATA_1 = TSW_APB2AXIS_ALMEM_REQDATA_1_WR_NRD_SET(1) | TSW_APB2AXIS_ALMEM_REQDATA_1_ENTRY_NUM_SET(entry_num);

        /* set forward to destination port, use PCP field, UTAG 1 and trigger the interface for sending the data */
        ptr->APB2AXIS_ALMEM_REQDATA_0 = TSW_APB2AXIS_ALMEM_REQDATA_0_UTAG_SET(UTAG_TB_FRAME) |
                                        TSW_APB2AXIS_ALMEM_REQDATA_0_QSEL_SET(0) |
                                        TSW_APB2AXIS_ALMEM_REQDATA_0_DROP_SET(0) |
                                        TSW_APB2AXIS_ALMEM_REQDATA_0_QUEUE_SET(0) |
                                        TSW_APB2AXIS_ALMEM_REQDATA_0_DEST_SET(dest_port);
    }

    /* Create a new CAM entry */
    uint16_t dest_mac_47_32 = ((uint32_t)(dest_mac[5]) << 8) | ((uint32_t)(dest_mac[4]));
    uint32_t dest_mac_32_0 = ((uint32_t)(dest_mac[3]) << 24) | ((uint32_t)(dest_mac[2]) << 16) | ((uint32_t)(dest_mac[1]) << 8) | ((uint32_t)(dest_mac[0]));

    ptr->APB2AXI_CAM_REQDATA_2 = TSW_APB2AXI_CAM_REQDATA_2_VID_SET(vid) | TSW_APB2AXI_CAM_REQDATA_2_DESTMAC_HI_SET(dest_mac_47_32);
    ptr->APB2AXI_CAM_REQDATA_1 = TSW_APB2AXI_CAM_REQDATA_1_DESTMAC_LO_PORT_VEC_SET(dest_mac_32_0);
    ptr->APB2AXI_CAM_REQDATA_0 = TSW_APB2AXI_CAM_REQDATA_0_ENTRY_NUM_SET(entry_num) |
                                 TSW_APB2AXI_CAM_REQDATA_0_TYPE_SET(1) |   /* Set one DEST_MAC/VLAN_ID entry */
                                 TSW_APB2AXI_CAM_REQDATA_0_CH_SET(0);      /* CAM APB2AXIS channel selection. Always 0 for writing to DEST_MAC/VLAN_ID */

    /* Add a new VLAN_PORT entry (VID 1) */
    ptr->APB2AXI_CAM_REQDATA_1 = 0x0f;
    ptr->APB2AXI_CAM_REQDATA_0 = (vid << 16)  /* VID = 1 */
                               |TSW_APB2AXI_CAM_REQDATA_0_TYPE_SET(1)  /* 1: Set one VLAN_PORT entry */
                               |TSW_APB2AXI_CAM_REQDATA_0_CH_SET(1); /* CAM APB2AXIS channel selection. Always 1 for writing to VLAN_PORT table. */
}

ATTR_RAMFUNC void tsw_clr_mac_table(TSW_Type *ptr, uint16_t entry_num, uint8_t dest_port, uint8_t* dest_mac, uint8_t vid)
{

    /* Create a new ALMEM entry. This will specify what will be done with those detected frames */
    if (TSW_APB2AXIS_ALMEM_STS_RDY_GET(ptr->APB2AXIS_ALMEM_STS)) {

        ptr->APB2AXIS_ALMEM_REQDATA_1 = TSW_APB2AXIS_ALMEM_REQDATA_1_WR_NRD_SET(1) | TSW_APB2AXIS_ALMEM_REQDATA_1_ENTRY_NUM_SET(entry_num);

        /* set forward to destination port, use PCP field, UTAG 1 and trigger the interface for sending the data */
        ptr->APB2AXIS_ALMEM_REQDATA_0 = TSW_APB2AXIS_ALMEM_REQDATA_0_UTAG_SET(1) |
                                        TSW_APB2AXIS_ALMEM_REQDATA_0_QSEL_SET(0) |
                                        TSW_APB2AXIS_ALMEM_REQDATA_0_DROP_SET(0) |
                                        TSW_APB2AXIS_ALMEM_REQDATA_0_QUEUE_SET(0) |
                                        TSW_APB2AXIS_ALMEM_REQDATA_0_DEST_SET(dest_port);
    }

    uint16_t dest_mac_47_32 = ((uint32_t)(dest_mac[5]) << 8) | ((uint32_t)(dest_mac[4]));
    uint32_t dest_mac_32_0 = ((uint32_t)(dest_mac[3]) << 24) | ((uint32_t)(dest_mac[2]) << 16) | ((uint32_t)(dest_mac[1]) << 8) | ((uint32_t)(dest_mac[0]));

    ptr->APB2AXI_CAM_REQDATA_2 = TSW_APB2AXI_CAM_REQDATA_2_VID_SET(vid) | TSW_APB2AXI_CAM_REQDATA_2_DESTMAC_HI_SET(dest_mac_47_32);
    ptr->APB2AXI_CAM_REQDATA_1 = TSW_APB2AXI_CAM_REQDATA_1_DESTMAC_LO_PORT_VEC_SET(dest_mac_32_0);
    ptr->APB2AXI_CAM_REQDATA_0 = TSW_APB2AXI_CAM_REQDATA_0_ENTRY_NUM_SET(entry_num) |
                                 TSW_APB2AXI_CAM_REQDATA_0_TYPE_SET(2) |   /* Set one DEST_MAC/VLAN_ID entry */
                                 TSW_APB2AXI_CAM_REQDATA_0_CH_SET(0);      /* CAM APB2AXIS channel selection. Always 0 for writing to DEST_MAC/VLAN_ID */

    /* Add a new VLAN_PORT entry (VID 1) */
    ptr->APB2AXI_CAM_REQDATA_1 = 0x0f;
    ptr->APB2AXI_CAM_REQDATA_0 = (vid << 16)  /* VID = 1 */
                               |TSW_APB2AXI_CAM_REQDATA_0_TYPE_SET(2)  /* 1: Set one VLAN_PORT entry */
                               |TSW_APB2AXI_CAM_REQDATA_0_CH_SET(1); /* CAM APB2AXIS channel selection. Always 1 for writing to VLAN_PORT table. */      
}

ATTR_RAMFUNC void tsw_set_cam_vid(TSW_Type *ptr, uint8_t dest_port, uint16_t vid)
{
    ptr->APB2AXI_CAM_REQDATA_1 = dest_port;
    ptr->APB2AXI_CAM_REQDATA_0 = (vid << 16)
                                |TSW_APB2AXI_CAM_REQDATA_0_TYPE_SET(1)  /* 1: Set one VLAN_PORT entry */
                                |TSW_APB2AXI_CAM_REQDATA_0_CH_SET(1); /* CAM APB2AXIS channel selection. Always 1 for writing to VLAN_PORT table. */
}

ATTR_RAMFUNC void tsw_clr_cam_vid(TSW_Type *ptr, uint8_t dest_port, uint16_t vid)
{
    ptr->APB2AXI_CAM_REQDATA_1 = dest_port;
    ptr->APB2AXI_CAM_REQDATA_0 = (vid << 16)
                                |TSW_APB2AXI_CAM_REQDATA_0_TYPE_SET(2)  /* 2: Clear one VLAN_PORT entry */
                                |TSW_APB2AXI_CAM_REQDATA_0_CH_SET(1); /* CAM APB2AXIS channel selection. Always 1 for writing to VLAN_PORT table. */
}


ATTR_RAMFUNC uint32_t tsw_timestamp_s(void)
{
    return mchtmr_get_count(HPM_MCHTMR)/HPM_MCHTMR_FREQ;
}

ATTR_RAMFUNC static int tsw_mac_table_find(uint8_t mac[])
{
    for(int i = 0; i < TSW_MAC_TABLE_MAX; i++){
        fencerw ();
        //printf("%2x:%2x:%2x:%2x:%2x:%2x\n", mac_table[i].mac[0], mac_table[i].mac[1],mac_table[i].mac[2],mac_table[i].mac[3],mac_table[i].mac[4],mac_table[i].mac[5]);
        //printf("%2x:%2x:%2x:%2x:%2x:%2x\n", mac[0],  mac[1], mac[2], mac[3], mac[4], mac[5]);
        if(memcmp(mac_table[i].mac, mac, TSW_MAC_ADDR_LEN) == 0){
            return i;
        }
    }
    return TSW_FAIL;
}

ATTR_RAMFUNC static int tsw_mac_table_null(void)
{
    for(int i = 0; i < TSW_MAC_TABLE_MAX; i++){
        fencerw ();
        if(memcmp(mac_table[i].mac, mac_null, TSW_MAC_ADDR_LEN) == 0){
            return i;
        }
    }
    return TSW_FAIL;
}

ATTR_RAMFUNC static int tsw_mac_table_time_min(void)
{
    int index = -1; // 初始化为无效索引
    // 注意：hitmem是无符号的，时间差计算可能为负，用有符号类型
    int32_t time_min = TSW_MAC_AGING_S; // 初始化为最大的有符号整数

    for(int i = 0; i < TSW_MAC_TABLE_MAX; i++){
        fencerw ();
        // 跳过空条目 和 永不过期的静态条目 (比如你自己的MAC)
        if( (memcmp(mac_table[i].mac, mac_null, TSW_MAC_ADDR_LEN) != 0) &&
            (mac_table[i].hitmem != 0xffffffff) ) {

            int32_t test_time = (int32_t)(mac_table[i].hitmem - tsw_timestamp_s());
            if(test_time < time_min){ // 找到一个更老的
                time_min = test_time;
                index = i;
            }
        }
    }
    // 如果没有找到可替换的条目 (例如表中全是静态条目)，返回失败
    return (index == -1) ? TSW_FAIL : index;
}

ATTR_RAMFUNC static int tsw_mac_table_add(uint8_t mac[], uint16_t vid, uint8_t prot)
{
    int index = tsw_mac_table_null();
    if(TSW_FAIL == index){
        index = tsw_mac_table_time_min();
        if (TSW_FAIL == index) { // 检查是否真的找到了可替换的条目
            // 此时表满了，且无法替换，可以选择丢弃或打印错误
            return TSW_FAIL; 
        }
        tsw_mac_table_del(index);
    }
    //printf("index:%d !\n", index);
    memcpy(mac_table[index].mac, mac, TSW_MAC_ADDR_LEN);
    mac_table[index].vid = vid;
    mac_table[index].port = prot;
    mac_table[index].hitmem = tsw_timestamp_s() + TSW_MAC_AGING_S;
    //printf("mac_table[index].hitmem:%u !\n", mac_table[index].hitmem);
    tsw_set_mac_table(HPM_TSW, index, mac_table[index].port, mac_table[index].mac, mac_table[index].vid);
    return index;
}

ATTR_RAMFUNC static int tsw_mac_table_del(uint8_t index)
{
    tsw_clr_mac_table(HPM_TSW, index, mac_table[index].port, mac_table[index].mac, mac_table[index].vid);
    mac_table[index].vid = 0;
    mac_table[index].port = 0;
    mac_table[index].hitmem = 0;
    memset(mac_table[index].mac, 0, TSW_MAC_ADDR_LEN);
    return TSW_OK;
}

ATTR_RAMFUNC int tsw_mac_table_init(uint8_t* mac, uint16_t vid)
{
    memset(&mac_table, 0, sizeof(tsn_mac_t) * TSW_MAC_TABLE_MAX);
    memcpy(mac_table[0].mac, mac, TSW_MAC_ADDR_LEN);
    mac_table[0].vid = vid;
    mac_table[0].port= tsw_dst_port_cpu;
    mac_table[0].hitmem = 0xffffffff;
    return TSW_OK;
}

ATTR_RAMFUNC void tsw_clear_hitmem_table(void)
{
    HPM_TSW->HITMEM[0] = 0;
    HPM_TSW->HITMEM[1] = 0;
    HPM_TSW->HITMEM[2] = 0;
    HPM_TSW->HITMEM[3] = 0;
}

ATTR_RAMFUNC void tsw_mac_table_hitmem(void)
{
    uint8_t  g_hitmem_hit = 0;
    uint32_t hitmem0 = HPM_TSW->HITMEM[0];
    uint32_t hitmem1 = HPM_TSW->HITMEM[1];
    uint32_t hitmem2 = HPM_TSW->HITMEM[2];
    uint32_t hitmem3 = HPM_TSW->HITMEM[3];

    for(int i = 0; i < TSW_MAC_TABLE_MAX; i++){
        fencerw ();
        if((memcmp(mac_table[i].mac, mac_null, TSW_MAC_ADDR_LEN) != 0) &&
            (mac_table[i].hitmem != 0xffffffff)){
            uint8_t  s_hitmem_hit = 0;
            if(i < 32){
                s_hitmem_hit = (hitmem0 >> i) & 0x1;
            }else if(i < 64){
                s_hitmem_hit = (hitmem1 >> (i-32)) & 0x1;
            }else if(i < 96){
                s_hitmem_hit = (hitmem2 >> (i-64)) & 0x1;
            }else{
                s_hitmem_hit = (hitmem3 >> (i-96)) & 0x1;
            }
            if(s_hitmem_hit){
                // 老化时间
                mac_table[i].hitmem += TSW_MAC_AGING_S;
                g_hitmem_hit = s_hitmem_hit;
            }
            if(mac_table[i].hitmem < tsw_timestamp_s()){
               tsw_mac_table_del(i);
               //printf("del:mac index %d !\n", i);
            }
        }
    }
    if(g_hitmem_hit){
        tsw_clear_hitmem_table();
    }
}

#define TSW_FRAME_UTAG_MASK (0x38U)
#define TSW_FRAME_UTAG_SHIFT (3U)
#define TSW_FRAME_UTAG_SET(x) (((uint32_t)(x) << TSW_FRAME_UTAG_SHIFT) & TSW_FRAME_UTAG_MASK)
#define TSW_FRAME_UTAG_GET(x) (((uint32_t)(x) & TSW_FRAME_UTAG_MASK) >> TSW_FRAME_UTAG_SHIFT)

ATTR_RAMFUNC void tsw_mac_table_learn(uint8_t* frame_buff, uint32_t frame_len)
{
    uint8_t* src_mac =  frame_buff + TSW_HEAD_LEN + TSW_MAC_LEN;
    uint16_t u16_type = GET_TSW_TYPE(frame_buff, TSW_HEAD_LEN + TSW_MAC_LEN + TSW_MAC_LEN);
    uint16_t u16_vid  = (ETHTYPE_VLAN == u16_type) ? GET_TSW_TYPE(frame_buff, TSW_HEAD_LEN +  TSW_MAC_LEN + TSW_MAC_LEN + TSW_TYPE_LEN) : TSW_DEFAULT_VID;
    uint8_t  tsw_port  = (1 << frame_buff[0]);
    uint8_t  tsw_utag  = TSW_FRAME_UTAG_GET(frame_buff[2]);
    if(UTAG_NN_FRAME == tsw_utag || UTAG_BC_FRAME == tsw_utag){
        tsw_mac_table_hitmem();
        if(TSW_FAIL ==  tsw_mac_table_find(src_mac)){
            tsw_mac_table_add(src_mac, u16_vid, tsw_port);
        }
    }
}

