# Copyright (c) 2024 HPMicro
# SPDX-License-Identifier: BSD-3-Clause

sdk_inc(.)

if($ENV{CONFIG_TSW_PHY_RTL8211})
    add_subdirectory(rtl8211)
    message(STATUS "TSW PHY Enabled: RTL8211")
endif()

if($ENV{CONFIG_TSW_PHY_JL1111})
    add_subdirectory(jl1111)
    message(STATUS "TSW PHY Enabled: JL1111")
endif()

if($ENV{CONFIG_TSW_PHY_YT8531})
    add_subdirectory(yt8531)
    message(STATUS "TSW PHY Enabled: YT8531")
endif()

