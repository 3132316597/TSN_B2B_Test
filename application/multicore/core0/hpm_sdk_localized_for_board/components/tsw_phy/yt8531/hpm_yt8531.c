/*
 * Copyright (c) 2024 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

/*---------------------------------------------------------------------
 * Includes
 *---------------------------------------------------------------------
 */
#include "hpm_tsw_drv.h"
#include "hpm_yt8531_regs.h"
#include "hpm_yt8531.h"

/*---------------------------------------------------------------------
 * Internal API
 *---------------------------------------------------------------------
 */
static bool yt8531_check_id(TSW_Type *ptr, uint8_t port, uint8_t phy_addr)
{
    uint16_t id1, id2;

    tsw_ep_mdio_read(ptr, port, phy_addr, YT8531_PHYID1, &id1);
    tsw_ep_mdio_read(ptr, port, phy_addr, YT8531_PHYID2, &id2);

    if (YT8531_PHYID1_OUI_MSB_GET(id1) == YT8531_ID1 && YT8531_PHYID2_OUI_LSB_GET(id2) == YT8531_ID2) {
        return true;
    } else {
        return false;
    }
}

/*---------------------------------------------------------------------
 * API
 *---------------------------------------------------------------------
 */
void yt8531_reset(TSW_Type *ptr, uint8_t port, uint8_t phy_addr)
{
    uint16_t data;

    /* PHY reset */
    tsw_ep_mdio_write(ptr, port, phy_addr, YT8531_BMCR, YT8531_BMCR_RESET_SET(1));

    /* wait until the reset is completed */
    do {
        tsw_ep_mdio_read(ptr, port, phy_addr, YT8531_BMCR, &data);
    } while (YT8531_BMCR_RESET_GET(data));
}

void yt8531_basic_mode_default_config(TSW_Type *ptr, yt8531_config_t *config)
{
    (void)ptr;

    config->loopback         = false;                        /* Disable PCS loopback mode */
    #if defined(__DISABLE_AUTO_NEGO) && (__DISABLE_AUTO_NEGO)
    config->auto_negotiation = false;                        /* Disable Auto-Negotiation */
    config->speed            = tsw_phy_port_speed_100mbps;
    config->duplex           = tsw_phy_duplex_full;
    #else
    config->auto_negotiation = true;                         /* Enable Auto-Negotiation */
    #endif
}

bool yt8531_basic_mode_init(TSW_Type *ptr, uint8_t port, uint8_t phy_addr, yt8531_config_t *config)
{
    uint16_t data = 0;

    data |= YT8531_BMCR_RESET_SET(0)                        /* Normal operation */
         |  YT8531_BMCR_LOOPBACK_SET(config->loopback)      /* configure PCS loopback mode */
         |  YT8531_BMCR_ANE_SET(config->auto_negotiation)   /* configure Auto-Negotiation */
         |  YT8531_BMCR_PWD_SET(0)                          /* Normal operation */
         |  YT8531_BMCR_ISOLATE_SET(0)                      /* Normal operation */
         |  YT8531_BMCR_RESTART_AN_SET(0)                   /* Normal operation (ignored when Auto-Negotiation is disabled) */
         |  YT8531_BMCR_COLLISION_TEST_SET(0);              /* Normal operation */

    if (config->auto_negotiation == 0) {
        data |= YT8531_BMCR_SPEED0_SET(config->speed) | YT8531_BMCR_SPEED1_SET(config->speed >> 1);   /* Set port speed */
        data |= YT8531_BMCR_DUPLEX_SET(config->duplex);                                                /* Set duplex mode */
    }

    /* check the id of rtl8211 */
    if (yt8531_check_id(ptr, port, phy_addr) == false) {
        return false;
    }

    tsw_ep_mdio_write(ptr, port, phy_addr, YT8531_BMCR, data);

    return true;
}

void yt8531_get_phy_status(TSW_Type *ptr, uint8_t port, uint8_t phy_addr, tsw_phy_status_t *status)
{
    uint16_t data;

    tsw_ep_mdio_read(ptr, port, phy_addr, YT8531_PHYSR, &data);
    status->tsw_phy_link = YT8531_PHYSR_LINK_REAL_TIME_GET(data);
    status->tsw_phy_speed = YT8531_PHYSR_SPEED_GET(data) == 0 ? tsw_phy_port_speed_10mbps : YT8531_PHYSR_SPEED_GET(data) == 1 ? tsw_phy_port_speed_100mbps : tsw_phy_port_speed_1000mbps;
    status->tsw_phy_duplex = YT8531_PHYSR_DUPLEX_GET(data);
}
