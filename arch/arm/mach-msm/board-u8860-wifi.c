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


#define WLAN_STATIC_SCAN_BUF0		5
#define WLAN_STATIC_SCAN_BUF1		6
#define PREALLOC_WLAN_SEC_NUM		4
#define PREALLOC_WLAN_BUF_NUM		160
#define PREALLOC_WLAN_SECTION_HEADER	24

#define WLAN_SECTION_SIZE_0	(PREALLOC_WLAN_BUF_NUM * 128)
#define WLAN_SECTION_SIZE_1	(PREALLOC_WLAN_BUF_NUM * 128)
#define WLAN_SECTION_SIZE_2	(PREALLOC_WLAN_BUF_NUM * 512)
#define WLAN_SECTION_SIZE_3	(PREALLOC_WLAN_BUF_NUM * 1024)

#define DHD_SKB_HDRSIZE		336
#define DHD_SKB_1PAGE_BUFSIZE	((PAGE_SIZE*1)-DHD_SKB_HDRSIZE)
#define DHD_SKB_2PAGE_BUFSIZE	((PAGE_SIZE*2)-DHD_SKB_HDRSIZE)
#define DHD_SKB_4PAGE_BUFSIZE	((PAGE_SIZE*4)-DHD_SKB_HDRSIZE)

#define WLAN_SKB_BUF_NUM	17

static struct sk_buff *wlan_static_skb[WLAN_SKB_BUF_NUM];
static void *wlan_static_scan_buf0;
static void *wlan_static_scan_buf1;

struct wlan_mem_prealloc {
	void *mem_ptr;
	unsigned long size;
};

static struct wlan_mem_prealloc wlan_mem_array[PREALLOC_WLAN_SEC_NUM] = {
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
	if (section == PREALLOC_WLAN_SEC_NUM)
		return wlan_static_skb;
	if (section == WLAN_STATIC_SCAN_BUF0)
		return wlan_static_scan_buf0;
	if (section == WLAN_STATIC_SCAN_BUF1)
		return wlan_static_scan_buf1;
	if ((section < 0) || (section > PREALLOC_WLAN_SEC_NUM))
		return NULL;

	if (wlan_mem_array[section].size < size)
		return NULL;

	return wlan_mem_array[section].mem_ptr;
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
	int j;

	for (i = 0; i < 8; i++) {
		wlan_static_skb[i] = dev_alloc_skb(DHD_SKB_1PAGE_BUFSIZE);
		if (!wlan_static_skb[i])
			goto err_skb_alloc;
	}

	for (; i < 16; i++) {
		wlan_static_skb[i] = dev_alloc_skb(DHD_SKB_2PAGE_BUFSIZE);
		if (!wlan_static_skb[i])
			goto err_skb_alloc;
	}

	wlan_static_skb[i] = dev_alloc_skb(DHD_SKB_4PAGE_BUFSIZE);
	if (!wlan_static_skb[i])
		goto err_skb_alloc;

	for (i = 0 ; i < PREALLOC_WLAN_SEC_NUM ; i++) {
		wlan_mem_array[i].mem_ptr =
			kmalloc(wlan_mem_array[i].size, GFP_KERNEL);

		if (!wlan_mem_array[i].mem_ptr)
			goto err_mem_alloc;
	}
	wlan_static_scan_buf0 = kmalloc(65536, GFP_KERNEL);
	if (!wlan_static_scan_buf0)
		goto err_mem_alloc;
	wlan_static_scan_buf1 = kmalloc(65536, GFP_KERNEL);
	if (!wlan_static_scan_buf1)
		goto err_mem_alloc;

	return 0;

 err_mem_alloc:
	pr_err("Failed to mem_alloc for wifi\n");
	for (j = 0 ; j < i ; j++)
		kfree(wlan_mem_array[j].mem_ptr);

	i = WLAN_SKB_BUF_NUM;

 err_skb_alloc:
	pr_err("Failed to skb_alloc for wifi\n");
	for (j = 0 ; j < i ; j++)
		dev_kfree_skb(wlan_static_skb[j]);

	return -ENOMEM;
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
