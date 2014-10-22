/* Copyright (c) 2009-2013, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <asm/gpio.h>
#include <linux/if.h>
#include <linux/skbuff.h>
#include <linux/wifi_tiwlan.h>

#include "board-u8860.h"


#define PREALLOC_WLAN_NUMBER_OF_SECTIONS	4
#define PREALLOC_WLAN_NUMBER_OF_BUFFERS		160
#define PREALLOC_WLAN_SECTION_HEADER		24

#define WLAN_SECTION_SIZE_0	(PREALLOC_WLAN_NUMBER_OF_BUFFERS * 128)
#define WLAN_SECTION_SIZE_1	(PREALLOC_WLAN_NUMBER_OF_BUFFERS * 128)
#define WLAN_SECTION_SIZE_2	(PREALLOC_WLAN_NUMBER_OF_BUFFERS * 512)
#define WLAN_SECTION_SIZE_3	(PREALLOC_WLAN_NUMBER_OF_BUFFERS * 1024)

#define WLAN_SKB_BUF_NUM	16

static struct sk_buff *wlan_static_skb[WLAN_SKB_BUF_NUM];

typedef struct wifi_mem_prealloc_struct {
	void *mem_ptr;
	unsigned long size;
} wifi_mem_prealloc_t;

static wifi_mem_prealloc_t wifi_mem_array[PREALLOC_WLAN_NUMBER_OF_SECTIONS] = {
	{ NULL, (WLAN_SECTION_SIZE_0 + PREALLOC_WLAN_SECTION_HEADER) },
	{ NULL, (WLAN_SECTION_SIZE_1 + PREALLOC_WLAN_SECTION_HEADER) },
	{ NULL, (WLAN_SECTION_SIZE_2 + PREALLOC_WLAN_SECTION_HEADER) },
	{ NULL, (WLAN_SECTION_SIZE_3 + PREALLOC_WLAN_SECTION_HEADER) }
};

#define WL_HOST_WAKE	18
#define WL_REG_ON	162
#define WL_PWR_ON	164

static struct msm_gpio wifi_gpios[] = {
	{ GPIO_CFG(WL_HOST_WAKE, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
		"WL_HOST_WAKE" },
	{ GPIO_CFG(WL_REG_ON, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
		"WL_REG_ON" },
	{ GPIO_CFG(WL_PWR_ON, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
		"WL_PWR_ON" },
};

static void *u8860_wifi_mem_prealloc(int section, unsigned long size)
{
	if (section == PREALLOC_WLAN_NUMBER_OF_SECTIONS)
		return wlan_static_skb;
	if ((section < 0) || (section > PREALLOC_WLAN_NUMBER_OF_SECTIONS))
		return NULL;
	if (wifi_mem_array[section].size < size)
		return NULL;
	return wifi_mem_array[section].mem_ptr;
}

static int u8860_wifi_power(int val)
{
	pr_debug("%s: val %d\n", __func__, val);
	if (val) {
		gpio_direction_output(WL_REG_ON, 1);
		msleep(1);
		gpio_direction_output(WL_PWR_ON, 1);
		msleep(150);
	} else {
		gpio_direction_output(WL_PWR_ON, 0);
		msleep(1);
		gpio_direction_output(WL_REG_ON, 0);
		msleep(1);
	}
	return 0;
}

static int u8860_wifi_reset(int val)
{
	pr_debug("%s: val %d\n", __func__, val);
	return 0;
}

static int u8860_wifi_set_carddetect(int val)
{
	u8860_wifi_cd = val;
	if (wifi_status_cb) {
		wifi_status_cb(val, wifi_status_cb_devid);
	} else
		pr_warning("%s: Nobody to notify\n", __func__);
	return 0;
}

extern int hwprops_get_wlanmac(uint8_t *wlanmac);
static int u8860_wifi_get_mac_addr(unsigned char *buf)
{
	int ret;
	uint8_t mac_address[IFHWADDRLEN];

	ret = hwprops_get_wlanmac(mac_address);
	if (ret) {
		pr_err("Failed to get hwprops wlan mac ret=%d\n", ret);
		return ret;
	}

	memcpy(buf, mac_address, IFHWADDRLEN);

	return 0;
}

static struct resource u8860_wifi_resources[] = {
	[0] = {
		.name		= "bcmdhd_wlan_irq",
		.start		= MSM_GPIO_TO_INT(WL_HOST_WAKE),
		.end		= MSM_GPIO_TO_INT(WL_HOST_WAKE),
		.flags		= IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHLEVEL
					| IORESOURCE_IRQ_SHAREABLE,
	},
};

static struct wifi_platform_data u8860_wifi_control = {
	.set_power	= u8860_wifi_power,
	.set_reset	= u8860_wifi_reset,
	.set_carddetect	= u8860_wifi_set_carddetect,
	.mem_prealloc	= u8860_wifi_mem_prealloc,
	.get_mac_addr	= u8860_wifi_get_mac_addr,
};

static struct platform_device u8860_wifi_device = {
	.name		= "bcmdhd_wlan",
	.id		= 1,
	.num_resources	= ARRAY_SIZE(u8860_wifi_resources),
	.resource	= u8860_wifi_resources,
	.dev		= {
		.platform_data = &u8860_wifi_control,
	},
};

static int __init u8860_init_wifi_mem(void)
{
	int i;

	for (i = 0; (i < WLAN_SKB_BUF_NUM); i++) {
		if (i < (WLAN_SKB_BUF_NUM/2))
			wlan_static_skb[i] = dev_alloc_skb(4096);
		else
			wlan_static_skb[i] = dev_alloc_skb(8192);
	}
	for (i = 0; (i < PREALLOC_WLAN_NUMBER_OF_SECTIONS); i++) {
		wifi_mem_array[i].mem_ptr = kmalloc(wifi_mem_array[i].size,
			GFP_KERNEL);
		if (wifi_mem_array[i].mem_ptr == NULL)
			return -ENOMEM;
	}
	return 0;
}

static int __init u8860_init_wifi_gpio(void)
{
	int ret;

	ret = msm_gpios_request_enable(wifi_gpios, ARRAY_SIZE(wifi_gpios));
	if (ret) {
		pr_err("Failed to request enable wifi gpios ret=%d\n", ret);
		return ret;
	}

	/* Turn power off. */
	gpio_direction_output(WL_REG_ON, 0);
	msleep(1);
	gpio_direction_output(WL_PWR_ON, 0);
	msleep(1);

	return 0;
}

static int __init u8860_wifi_init(void)
{
	u8860_init_wifi_gpio();
	u8860_init_wifi_mem();
	return platform_device_register(&u8860_wifi_device);;
}

module_init(u8860_wifi_init);
