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

#ifndef __ARCH_ARM_MACH_MSM_BOARD_HW7X30_H__
#define __ARCH_ARM_MACH_MSM_BOARD_HW7X30_H__

/* board-hw7x30-regulator.c */
extern struct proccomm_regulator_platform_data msm7x30_proccomm_regulator_data;

/* Common GPIOs */
#define GPIO_MDP_IRQ	30

#define GPIO_AMP_EN	82

#define GPIO_TS_RST	85
#define GPIO_TS_IRQ	148

#define GPIO_DCDC_SCL	149
#define GPIO_DCDC_SDA	150

/* U8860 GPIOs */
#ifdef CONFIG_MACH_HUAWEI_U8860
#define GPIO_APDS_IRQ	89
#define GPIO_AKM_IRQ	132
#endif

#endif /* __ARCH_ARM_MACH_MSM_BOARD_HW7X30_H__ */
