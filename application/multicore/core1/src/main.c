/*
 * @Author: Joe jie.huang@leetx.com
 * @Date: 2025-03-25 19:18:59
 * @LastEditors: Joe jie.huang@leetx.com
 * @LastEditTime: 2025-03-26 09:30:35
 * @FilePath: \Controller-ToolV2-HPM\application\multicore\core1\src\main.c
 * @Description:
 *
 * Copyright (c) 2025 by Leetx, All Rights Reserved.
 */

#include <stdio.h>

#include "board.h"
#include "hpm_clock_drv.h"

#define LED_FLASH_PERIOD_IN_MS 300

int main(void)
{
    board_init_core1();
    board_init_led_pins();
    
    printf("Core1 process ...... ！！！");

    board_timer_create(LED_FLASH_PERIOD_IN_MS, board_led_toggle);

    return 0;
}
