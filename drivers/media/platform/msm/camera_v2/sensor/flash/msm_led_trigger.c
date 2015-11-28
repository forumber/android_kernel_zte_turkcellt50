/* Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
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

#define pr_fmt(fmt) "%s:%d " fmt, __func__, __LINE__

#include <linux/module.h>
#include "msm_led_flash.h"
#if defined(CONFIG_MSM_CAMERA_EMODE)
#include <linux/sysdev.h>
#endif /* CONFIG_MSM_CAMERA_EMODE */

#define FLASH_NAME "camera-led-flash"

/*#define CONFIG_MSMB_CAMERA_DEBUG*/
#undef CDBG
#ifdef CONFIG_MSMB_CAMERA_DEBUG
#define CDBG(fmt, args...) pr_err(fmt, ##args)
#else
#define CDBG(fmt, args...) do { } while (0)
#endif

extern int32_t msm_led_torch_create_classdev(
				struct platform_device *pdev, void *data);

static enum flash_type flashtype;
static struct msm_led_flash_ctrl_t fctrl;

#if defined(CONFIG_MSM_CAMERA_EMODE)

static struct sysdev_class msm_led_trigger_sysdev_class = {
	.name = "led-flash"
};

static struct sys_device msm_led_trigger_sysdev;

static ssize_t store_torch(struct sys_device *dev, struct sysdev_attribute *attr, const char *buf, size_t buf_sz);
static ssize_t store_flash(struct sys_device *dev, struct sysdev_attribute *attr, const char *buf, size_t buf_sz);
int32_t msm_led_trigger_register_sysdev(struct msm_led_flash_ctrl_t *fctrl);

static ssize_t store_torch(struct sys_device *dev, struct sysdev_attribute *attr, const char *buf, size_t buf_sz)
{
	int32_t enable = 0,i;

	CDBG("%s: E\n", __func__);

	sscanf(buf, "%d", &enable);

	if (enable) {
		if (fctrl.torch_trigger) {
		led_trigger_event(fctrl.torch_trigger,fctrl.torch_op_current);	
		}	
	} else {
		for (i = 0; i < fctrl.num_sources; i++)
			if (fctrl.flash_trigger[i])
				led_trigger_event(fctrl.flash_trigger[i], 0);
		if (fctrl.torch_trigger)
			led_trigger_event(fctrl.torch_trigger, 0);		
	}

	CDBG("%s: X\n", __func__);

	return buf_sz;
}
static SYSDEV_ATTR(torch, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH, NULL, store_torch);

static ssize_t store_flash(struct sys_device *dev, struct sysdev_attribute *attr, const char *buf, size_t buf_sz)
{
	int32_t enable = 0,i;

	CDBG("%s: E\n", __func__);

	sscanf(buf, "%d", &enable);

	if (enable) {
		if (fctrl.torch_trigger)
			led_trigger_event(fctrl.torch_trigger, 0);
		for (i = 0; i < fctrl.num_sources; i++)
			if (fctrl.flash_trigger[i]) {
				led_trigger_event(fctrl.flash_trigger[i],fctrl.flash_op_current[i]);
			}
	} else {
		for (i = 0; i < fctrl.num_sources; i++)
			if (fctrl.flash_trigger[i])
				led_trigger_event(fctrl.flash_trigger[i], 0);
		if (fctrl.torch_trigger)
			led_trigger_event(fctrl.torch_trigger, 0);	
	}

	CDBG("%s: X\n", __func__);

	return buf_sz;
}
static SYSDEV_ATTR(flash, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH, NULL, store_flash);

static ssize_t name_read(struct sys_device *dev, struct sysdev_attribute *attr, char *buf)
{
	ssize_t ret = 0;

	CDBG("%s: E\n", __func__);

	ret = snprintf(buf,PAGE_SIZE,"%s\n",fctrl.board_info.flash_name);

	CDBG("%s: X\n", __func__);

	return ret;
}
static SYSDEV_ATTR(name, S_IRUGO , name_read, NULL);

static struct sysdev_attribute *msm_led_trigger_sysdev_attrs[] = {
	&attr_name,
	&attr_torch,
	&attr_flash,
};

/*
 * MSM LED Trigger Sys Device Register
 *
 * 1. Torch Mode
 *     enable: $ echo "1" > /sys/devices/system/led-flash/led-flash0/torch
 *    disable: $ echo "0" > /sys/devices/system/led-flash/led-flash0/torch
 *
 * 2. Flash Mode
 *     enable: $ echo "1" > /sys/devices/system/led-flash/led-flash0/flash
 *    disable: $ echo "0" > /sys/devices/system/led-flash/led-flash0/flash
 */
int32_t msm_led_trigger_register_sysdev(struct msm_led_flash_ctrl_t *fctrl)
{
	int32_t i, rc;

	rc = sysdev_class_register(&msm_led_trigger_sysdev_class);
	if (rc) {
			return rc;
	}

	msm_led_trigger_sysdev.id = 0;
	msm_led_trigger_sysdev.cls = &msm_led_trigger_sysdev_class;
	rc = sysdev_register(&msm_led_trigger_sysdev);
	if (rc) {
		sysdev_class_unregister(&msm_led_trigger_sysdev_class);
		return rc;
	}

	for (i = 0; i < ARRAY_SIZE(msm_led_trigger_sysdev_attrs); ++i) {
		rc = sysdev_create_file(&msm_led_trigger_sysdev, msm_led_trigger_sysdev_attrs[i]);
		if (rc) {
			goto msm_led_trigger_register_sysdev_failed;
		}
	}

	return 0;

msm_led_trigger_register_sysdev_failed:

	while (--i >= 0) sysdev_remove_file(&msm_led_trigger_sysdev, msm_led_trigger_sysdev_attrs[i]);

	sysdev_unregister(&msm_led_trigger_sysdev);
	sysdev_class_unregister(&msm_led_trigger_sysdev_class);

	return rc;
}
#endif /* CONFIG_MSM_CAMERA_EMODE */




static int32_t msm_led_trigger_get_subdev_id(struct msm_led_flash_ctrl_t *fctrl,
	void *arg)
{
	uint32_t *subdev_id = (uint32_t *)arg;
	if (!subdev_id) {
		pr_err("%s:%d failed\n", __func__, __LINE__);
		return -EINVAL;
	}
	*subdev_id = fctrl->pdev->id;
	CDBG("%s:%d subdev_id %d\n", __func__, __LINE__, *subdev_id);
	return 0;
}

static int32_t msm_led_trigger_config(struct msm_led_flash_ctrl_t *fctrl,
	void *data)
{
	int rc = 0;
	struct msm_camera_led_cfg_t *cfg = (struct msm_camera_led_cfg_t *)data;
	uint32_t i;
	uint32_t curr_l, max_curr_l;
	CDBG("called led_state %d\n", cfg->cfgtype);

	if (!fctrl) {
		pr_err("failed\n");
		return -EINVAL;
	}

	switch (cfg->cfgtype) {
	case MSM_CAMERA_LED_OFF:
		for (i = 0; i < fctrl->num_sources; i++)
			if (fctrl->flash_trigger[i])
				led_trigger_event(fctrl->flash_trigger[i], 0);
		if (fctrl->torch_trigger)
			led_trigger_event(fctrl->torch_trigger, 0);
		break;

	case MSM_CAMERA_LED_LOW:
		if (fctrl->torch_trigger) {
			max_curr_l = fctrl->torch_max_current;
			if (cfg->torch_current > 0 &&
					cfg->torch_current < max_curr_l) {
				curr_l = cfg->torch_current;
			} else {
				curr_l = fctrl->torch_op_current;
				pr_err("LED current clamped to %d\n",
					curr_l);
			}
			led_trigger_event(fctrl->torch_trigger,
				curr_l);
		}
		break;

	case MSM_CAMERA_LED_HIGH:
		if (fctrl->torch_trigger)
			led_trigger_event(fctrl->torch_trigger, 0);
		for (i = 0; i < fctrl->num_sources; i++)
			if (fctrl->flash_trigger[i]) {
				max_curr_l = fctrl->flash_max_current[i];
				if (cfg->flash_current[i] > 0 &&
						cfg->flash_current[i] < max_curr_l) {
					curr_l = cfg->flash_current[i];
				} else {
					curr_l = fctrl->flash_op_current[i];
					pr_err("LED current clamped to %d\n",
						curr_l);
				}
				led_trigger_event(fctrl->flash_trigger[i],
					curr_l);
			}
		break;

	case MSM_CAMERA_LED_INIT:
	case MSM_CAMERA_LED_RELEASE:
		for (i = 0; i < fctrl->num_sources; i++)
			if (fctrl->flash_trigger[i])
				led_trigger_event(fctrl->flash_trigger[i], 0);
		if (fctrl->torch_trigger)
			led_trigger_event(fctrl->torch_trigger, 0);
		break;

	default:
		rc = -EFAULT;
		break;
	}
	CDBG("flash_set_led_state: return %d\n", rc);
	return rc;
}

static const struct of_device_id msm_led_trigger_dt_match[] = {
	{.compatible = "qcom,camera-led-flash"},
	{}
};

/*added by caidezun to close the led-flash when powerdown begin 20140319*/
static void qcom_flash_shutdown(struct platform_device *pdev)
{
	int32_t i=0;

	pr_err("%s: E\n", __func__);

	for (i = 0; i < fctrl.num_sources; i++)
		if (fctrl.flash_trigger[i])
			led_trigger_event(fctrl.flash_trigger[i], 0);
	if (fctrl.torch_trigger)
		led_trigger_event(fctrl.torch_trigger, 0);
}
/*added by caidezun to close the led-flash when powerdown end 20140319*/

MODULE_DEVICE_TABLE(of, msm_led_trigger_dt_match);

static struct platform_driver msm_led_trigger_driver = {
	.driver = {
		.name = FLASH_NAME,
		.owner = THIS_MODULE,
		.of_match_table = msm_led_trigger_dt_match,
	},
	/*added by caidezun to close the led-flash when powerdown 20140319*/
	.shutdown = qcom_flash_shutdown,
};

static int32_t msm_led_trigger_probe(struct platform_device *pdev)
{
	int32_t rc = 0, rc_1 = 0, i = 0;
	struct device_node *of_node = pdev->dev.of_node;
	struct device_node *flash_src_node = NULL;
	uint32_t count = 0;
	struct led_trigger *temp = NULL;

	CDBG("called\n");

	if (!of_node) {
		pr_err("of_node NULL\n");
		return -EINVAL;
	}

	fctrl.pdev = pdev;
	fctrl.num_sources = 0;

	rc = of_property_read_u32(of_node, "cell-index", &pdev->id);
	if (rc < 0) {
		pr_err("failed\n");
		return -EINVAL;
	}
	CDBG("pdev id %d\n", pdev->id);

	rc = of_property_read_u32(of_node,
			"qcom,flash-type", &flashtype);
	if (rc < 0) {
		pr_err("flash-type: read failed\n");
		return -EINVAL;
	}

	if (of_get_property(of_node, "qcom,flash-source", &count)) {
		count /= sizeof(uint32_t);
		CDBG("count %d\n", count);
		if (count > MAX_LED_TRIGGERS) {
			pr_err("invalid count\n");
			return -EINVAL;
		}
		fctrl.num_sources = count;
		for (i = 0; i < count; i++) {
			flash_src_node = of_parse_phandle(of_node,
				"qcom,flash-source", i);
			if (!flash_src_node) {
				pr_err("flash_src_node NULL\n");
				continue;
			}

			rc = of_property_read_string(flash_src_node,
				"linux,default-trigger",
				&fctrl.flash_trigger_name[i]);
			if (rc < 0) {
				pr_err("default-trigger: read failed\n");
				of_node_put(flash_src_node);
				continue;
			}

			CDBG("default trigger %s\n",
				fctrl.flash_trigger_name[i]);

			if (flashtype == GPIO_FLASH) {
				/* use fake current */
				fctrl.flash_op_current[i] = LED_FULL;
			} else {
				rc = of_property_read_u32(flash_src_node,
					"qcom,current",
					&fctrl.flash_op_current[i]);
				rc_1 = of_property_read_u32(flash_src_node,
					"qcom,max-current",
					&fctrl.flash_max_current[i]);
				if ((rc < 0) || (rc_1 < 0)) {
					pr_err("current: read failed\n");
					of_node_put(flash_src_node);
					continue;
				}
			}

			of_node_put(flash_src_node);

			CDBG("max_current[%d] %d\n",
				i, fctrl.flash_op_current[i]);

			led_trigger_register_simple(fctrl.flash_trigger_name[i],
				&fctrl.flash_trigger[i]);

			if (flashtype == GPIO_FLASH)
				if (fctrl.flash_trigger[i])
					temp = fctrl.flash_trigger[i];
		}

		/* Torch source */
		flash_src_node = of_parse_phandle(of_node, "qcom,torch-source",
			0);
		if (flash_src_node) {
			rc = of_property_read_string(flash_src_node,
				"linux,default-trigger",
				&fctrl.torch_trigger_name);
			if (rc < 0) {
				pr_err("default-trigger: read failed\n");
				goto torch_failed;
			}

			CDBG("default trigger %s\n",
				fctrl.torch_trigger_name);
			
#if defined(CONFIG_MSM_CAMERA_EMODE)			
			rc = of_property_read_string(of_node, "qcom,flash-name", &fctrl.board_info.flash_name);
			if (rc < 0) {
				pr_err("%s: failed %d\n", __func__, __LINE__);
			}
#endif	
			if (flashtype == GPIO_FLASH) {
				/* use fake current */
				fctrl.torch_op_current = LED_FULL;
				if (temp)
					fctrl.torch_trigger = temp;
				else
					led_trigger_register_simple(
						fctrl.torch_trigger_name,
						&fctrl.torch_trigger);
			} else {
				rc = of_property_read_u32(flash_src_node,
					"qcom,current",
					&fctrl.torch_op_current);
				rc_1 = of_property_read_u32(flash_src_node,
					"qcom,max-current",
					&fctrl.torch_max_current);

				if ((rc < 0) || (rc_1 < 0)) {
					pr_err("current: read failed\n");
					goto torch_failed;
				}

				CDBG("torch max_current %d\n",
					fctrl.torch_op_current);

				led_trigger_register_simple(
					fctrl.torch_trigger_name,
					&fctrl.torch_trigger);
			}
torch_failed:
			of_node_put(flash_src_node);
		}
	}

	rc = msm_led_flash_create_v4lsubdev(pdev, &fctrl);
	if (!rc)
		msm_led_torch_create_classdev(pdev, &fctrl);

#if defined(CONFIG_MSM_CAMERA_EMODE)
	if (!rc)
		(void)msm_led_trigger_register_sysdev(&fctrl);
	CDBG("exit\n");
#endif /* CONFIG_MSM_CAMERA_EMODE */
	
	return rc;
}

static int __init msm_led_trigger_add_driver(void)
{
	CDBG("called\n");
	return platform_driver_probe(&msm_led_trigger_driver,
		msm_led_trigger_probe);
}

static struct msm_flash_fn_t msm_led_trigger_func_tbl = {
	.flash_get_subdev_id = msm_led_trigger_get_subdev_id,
	.flash_led_config = msm_led_trigger_config,
};

static struct msm_led_flash_ctrl_t fctrl = {
	.func_tbl = &msm_led_trigger_func_tbl,
};

module_init(msm_led_trigger_add_driver);
MODULE_DESCRIPTION("LED TRIGGER FLASH");
MODULE_LICENSE("GPL v2");
