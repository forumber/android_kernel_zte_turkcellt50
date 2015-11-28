/* Copyright (c) 2011-2013, The Linux Foundation. All rights reserved.
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

#include <linux/delay.h>
#include <linux/module.h>
#include <linux/of.h>
#include "msm_camera_io_util.h"
#include <mach/gpiomux.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>

#define ZTE_CSP_DRV_NAME                    "zte_csp"

#define DBG_CSID 0

#define TRUE   1
#define FALSE  0

#undef CDBG
//#define CONFIG_MSMB_CAMERA_DEBUG
#ifdef CONFIG_MSMB_CAMERA_DEBUG
#define CDBG(fmt, args...) pr_err(fmt, ##args)
#else
#define CDBG(fmt, args...) do { } while (0)
#endif


static int32_t zte_sensor_get_dt_gpio_req_tbl(struct device_node *of_node,
	struct msm_camera_gpio_conf *gconf, uint16_t *gpio_array,
	uint16_t gpio_array_size)
{
	int32_t rc = 0, i = 0;
	uint32_t count = 0;
	uint32_t *val_array = NULL;
	CDBG("%s:%d enter\n", __func__, __LINE__);
	if (!of_get_property(of_node, "zte,gpio-req-tbl-num", &count))
		return 0;

	count /= sizeof(uint32_t);
	if (!count) {
		pr_err("%s zte,gpio-req-tbl-num 0\n", __func__);
		return 0;
	}

	val_array = kzalloc(sizeof(uint32_t) * count, GFP_KERNEL);
	if (!val_array) {
		pr_err("%s failed %d\n", __func__, __LINE__);
		return -ENOMEM;
	}

	gconf->cam_gpio_req_tbl = kzalloc(sizeof(struct gpio) * count,
		GFP_KERNEL);
	if (!gconf->cam_gpio_req_tbl) {
		pr_err("%s failed %d\n", __func__, __LINE__);
		rc = -ENOMEM;
		goto ERROR1;
	}
	gconf->cam_gpio_req_tbl_size = count;

	rc = of_property_read_u32_array(of_node, "zte,gpio-req-tbl-num",
		val_array, count);
	if (rc < 0) {
		pr_err("%s failed %d\n", __func__, __LINE__);
		goto ERROR2;
	}
	for (i = 0; i < count; i++) {
		if (val_array[i] >= gpio_array_size) {
			pr_err("%s gpio req tbl index %d invalid\n",
				__func__, val_array[i]);
			return -EINVAL;
		}
		gconf->cam_gpio_req_tbl[i].gpio = gpio_array[val_array[i]];
		CDBG("%s cam_gpio_req_tbl[%d].gpio = %d\n", __func__, i,
			gconf->cam_gpio_req_tbl[i].gpio);
	}

	rc = of_property_read_u32_array(of_node, "zte,gpio-req-tbl-flags",
		val_array, count);
	if (rc < 0) {
		pr_err("%s failed %d\n", __func__, __LINE__);
		goto ERROR2;
	}
	for (i = 0; i < count; i++) {
		gconf->cam_gpio_req_tbl[i].flags = val_array[i];
		CDBG("%s cam_gpio_req_tbl[%d].flags = %ld\n", __func__, i,
			gconf->cam_gpio_req_tbl[i].flags);
	}

	for (i = 0; i < count; i++) {
		rc = of_property_read_string_index(of_node,
			"zte,gpio-req-tbl-label", i,
			&gconf->cam_gpio_req_tbl[i].label);
		CDBG("%s cam_gpio_req_tbl[%d].label = %s\n", __func__, i,
			gconf->cam_gpio_req_tbl[i].label);
		if (rc < 0) {
			pr_err("%s failed %d\n", __func__, __LINE__);
			goto ERROR2;
		}
	}
	CDBG("%s exit\n", __func__);
	kfree(val_array);
	return rc;

ERROR2:
	kfree(gconf->cam_gpio_req_tbl);
ERROR1:
	kfree(val_array);
	gconf->cam_gpio_req_tbl_size = 0;
	return rc;
}
static int32_t zte_sensor_get_dt_gpio_set_tbl(struct device_node *of_node,
	struct msm_camera_gpio_conf *gconf, uint16_t *gpio_array,
	uint16_t gpio_array_size)
{
	int32_t rc = 0, i = 0;
	uint32_t count = 0;
	uint32_t *val_array = NULL;
	CDBG("%s:%d enter\n", __func__, __LINE__);
	if (!of_get_property(of_node, "zte,gpio-set-tbl-sequence", &count))
		return 0;

	count /= sizeof(uint32_t);
	if (!count) {
		pr_err("%s zte,gpio-set-tbl-num 0\n", __func__);
		return 0;
	}

	val_array = kzalloc(sizeof(uint32_t) * count, GFP_KERNEL);
	if (!val_array) {
		pr_err("%s failed %d\n", __func__, __LINE__);
		return -ENOMEM;
	}

	gconf->cam_gpio_set_tbl = kzalloc(sizeof(struct msm_gpio_set_tbl) *
		count, GFP_KERNEL);
	if (!gconf->cam_gpio_set_tbl) {
		pr_err("%s failed %d\n", __func__, __LINE__);
		rc = -ENOMEM;
		goto ERROR1;
	}
	gconf->cam_gpio_set_tbl_size = count;

	rc = of_property_read_u32_array(of_node, "zte,gpio-set-tbl-sequence",
		val_array, count);
	if (rc < 0) {
		pr_err("%s failed %d\n", __func__, __LINE__);
		goto ERROR2;
	}
	for (i = 0; i < count; i++) {
		if (val_array[i] >= gpio_array_size) {
			pr_err("%s gpio set tbl index %d invalid\n",
				__func__, val_array[i]);
			return -EINVAL;
		}
		gconf->cam_gpio_set_tbl[i].gpio = gpio_array[val_array[i]];
		CDBG("%s cam_gpio_set_tbl[%d].gpio = %d\n", __func__, i,
			gconf->cam_gpio_set_tbl[i].gpio);
	}

	rc = of_property_read_u32_array(of_node, "zte,gpio-set-tbl-value",
		val_array, count);
	if (rc < 0) {
		pr_err("%s failed %d\n", __func__, __LINE__);
		goto ERROR2;
	}
	for (i = 0; i < count; i++) {
		gconf->cam_gpio_set_tbl[i].flags = val_array[i];
		CDBG("%s cam_gpio_set_tbl[%d].flags = %ld\n", __func__, i,
			gconf->cam_gpio_set_tbl[i].flags);
	}

	rc = of_property_read_u32_array(of_node, "zte,gpio-set-tbl-delay",
		val_array, count);
	if (rc < 0) {
		pr_err("%s failed %d\n", __func__, __LINE__);
		goto ERROR2;
	}
	for (i = 0; i < count; i++) {
		gconf->cam_gpio_set_tbl[i].delay = val_array[i];
		CDBG("%s cam_gpio_set_tbl[%d].delay = %d\n", __func__, i,
			gconf->cam_gpio_set_tbl[i].delay);
	}
	CDBG("%s exit\n", __func__);
	kfree(val_array);
	return rc;

ERROR2:
	kfree(gconf->cam_gpio_set_tbl);
ERROR1:
	kfree(val_array);
	gconf->cam_gpio_set_tbl_size = 0;
	return rc;
}


static int32_t zte_camera_sensor_power_up(struct msm_camera_gpio_conf *gpio_conf)
{
	int32_t rc = 0, index = 0;
	
	CDBG("%s:%d enter\n", __func__, __LINE__);

	rc = msm_camera_request_gpio_table(
		        gpio_conf->cam_gpio_req_tbl,gpio_conf->cam_gpio_req_tbl_size, 1);
	if (rc < 0) {
		pr_err("%s: request gpio failed\n", __func__);
		return rc;
	}
	
	for (index = 0; index < gpio_conf->cam_gpio_set_tbl_size; index++) {
		pr_err("%s:%d gpio %d: value = %ld delay = %d\n",
					__func__, __LINE__,
					gpio_conf->cam_gpio_set_tbl[index].gpio, gpio_conf->cam_gpio_set_tbl[index].flags,gpio_conf->cam_gpio_set_tbl[index].delay);
		gpio_set_value_cansleep(
				gpio_conf->cam_gpio_set_tbl[index].gpio,
				(int)(gpio_conf->cam_gpio_set_tbl[index].flags & 0x1));

		if (gpio_conf->cam_gpio_set_tbl[index].delay > 20) {
			msleep(gpio_conf->cam_gpio_set_tbl[index].delay);
		} else if (gpio_conf->cam_gpio_set_tbl[index].delay) {
			usleep_range(gpio_conf->cam_gpio_set_tbl[index].delay * 1000,
				(gpio_conf->cam_gpio_set_tbl[index].delay * 1000) + 1000);
		}
	}

	CDBG("%s exit\n", __func__);


	return rc;
}

static int __devinit csp_probe(struct platform_device *pdev)
{
	struct device_node *of_node = pdev->dev.of_node;
	uint16_t *gpio_array = NULL;
	uint16_t gpio_array_size = 0;
	struct msm_camera_gpio_conf *gconf = NULL;
	int rc = 0,i = 0;

	CDBG("%s:%d enter\n", __func__, __LINE__);
	
	gconf = kzalloc(sizeof(struct msm_camera_gpio_conf),
		GFP_KERNEL);
	if (!gconf) {
		pr_err("%s failed %d\n", __func__, __LINE__);
		rc = -ENOMEM;
		goto ERROR0;
	}

	gpio_array_size = of_gpio_count(of_node);
	CDBG("%s gpio count %d\n", __func__, gpio_array_size);

	if (gpio_array_size) {
		gpio_array = kzalloc(sizeof(uint16_t) * gpio_array_size,
			GFP_KERNEL);
		if (!gpio_array) {
			pr_err("%s failed %d\n", __func__, __LINE__);
			goto ERROR1;
		}
		for (i = 0; i < gpio_array_size; i++) {
			gpio_array[i] = of_get_gpio(of_node, i);
			CDBG("%s gpio_array[%d] = %d\n", __func__, i,
				gpio_array[i]);
		}

		rc = zte_sensor_get_dt_gpio_req_tbl(of_node, gconf,
			gpio_array, gpio_array_size);
		if (rc < 0) {
			pr_err("%s failed %d\n", __func__, __LINE__);
			goto ERROR2;
		}

		rc = zte_sensor_get_dt_gpio_set_tbl(of_node, gconf,
			gpio_array, gpio_array_size);
		if (rc < 0) {
			pr_err("%s failed %d\n", __func__, __LINE__);
			goto ERROR3;
		}

		rc = zte_camera_sensor_power_up(gconf);
		if (rc < 0) {
			pr_err("%s failed %d\n", __func__, __LINE__);
			goto ERROR4;
		}
	}
	
ERROR4:
	kfree(gconf->cam_gpio_set_tbl);
ERROR3:
	kfree(gconf->cam_gpio_req_tbl);
ERROR2:
	kfree(gpio_array);
ERROR1:
	kfree(gconf);
ERROR0:
	CDBG("%s exit\n", __func__);
	return rc;
		
}

static const struct of_device_id zte_csp_dt_match[] = {
	{.compatible = "zte,csp_front_rear"},
	{.compatible = "zte,csp_rear"},
	{}
};

MODULE_DEVICE_TABLE(of, msm_csid_dt_match);

static struct platform_driver csp_driver = {
	.probe = csp_probe,
	.driver = {
		.name = ZTE_CSP_DRV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = zte_csp_dt_match,
	},
};

static int __init zte_camera_sensor_power_init_module(void)
{
	CDBG("%s:%d enter\n", __func__, __LINE__);
	return platform_driver_register(&csp_driver);
}

static void __exit zte_camera_sensor_power_exit_module(void)
{
	CDBG("%s:%d enter\n", __func__, __LINE__);
	platform_driver_unregister(&csp_driver);
}

module_init(zte_camera_sensor_power_init_module);
module_exit(zte_camera_sensor_power_exit_module);
MODULE_DESCRIPTION("ZTE CAMERA SENSOR POWER driver");
MODULE_LICENSE("GPL v2");

