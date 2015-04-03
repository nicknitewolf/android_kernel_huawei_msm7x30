/*
 * Copyright (c) 2011, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __ARCH_ARM_MACH_MSM_BOARD_U8860_H__
#define __ARCH_ARM_MACH_MSM_BOARD_U8860_H__

/* board-u8860.c */
extern void (*wifi_status_cb)(int card_present, void *dev_id);
extern void *wifi_status_cb_devid;
extern int u8860_wifi_cd; /* WIFI virtual 'card detect' status */

#endif
