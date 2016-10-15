/* Copyright (c) 2013, The Linux Foundation. All rights reserved.
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
#include "msm_sensor.h"
#include "msm_cci.h"
#define IMX135_SENSOR_NAME "imx135"
DEFINE_MSM_MUTEX(imx135_mut);
#undef CDBG
#ifdef CONFIG_MSMB_CAMERA_DEBUG
#define CDBG(fmt, args...) pr_err(fmt, ##args)
#else
#define CDBG(fmt, args...) do { } while (0)
#endif
static struct msm_sensor_ctrl_t imx135_s_ctrl;

extern uint16_t module_integrator_id;
static struct msm_sensor_power_setting imx135_power_setting[] = {
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_VDIG,
		.config_val = GPIO_OUT_HIGH,
		.delay = 0,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_VANA,
		.config_val = GPIO_OUT_HIGH,
		.delay = 0,
	},
	{
		.seq_type = SENSOR_VREG,
		.seq_val = CAM_VIO,
		.config_val = 0,
		.delay = 0,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_VAF,
		.config_val = GPIO_OUT_HIGH,
		.delay = 0,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_RESET,
		.config_val = GPIO_OUT_LOW,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_RESET,
		.config_val = GPIO_OUT_HIGH,
		.delay = 30,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_STANDBY,
		.config_val = GPIO_OUT_LOW,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_STANDBY,
		.config_val = GPIO_OUT_HIGH,
		.delay = 30,
	},
	{
		.seq_type = SENSOR_CLK,
		.seq_val = SENSOR_CAM_MCLK,
		.config_val = 24000000,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_I2C_MUX,
		.seq_val = 0,
		.config_val = 0,
		.delay = 0,
	},
};

static struct v4l2_subdev_info imx135_subdev_info[] = {
	{
		.code = V4L2_MBUS_FMT_SBGGR10_1X10,
		.colorspace = V4L2_COLORSPACE_JPEG,
		.fmt = 1,
		.order = 0,
	},
};

static const struct i2c_device_id imx135_i2c_id[] = {
	{IMX135_SENSOR_NAME, (kernel_ulong_t)&imx135_s_ctrl},
	{ }
};

static int32_t msm_imx135_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	return msm_sensor_i2c_probe(client, id, &imx135_s_ctrl);
}

static struct i2c_driver imx135_i2c_driver = {
	.id_table = imx135_i2c_id,
	.probe  = msm_imx135_i2c_probe,
	.driver = {
		.name = IMX135_SENSOR_NAME,
	},
};

static struct msm_camera_i2c_client imx135_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};

static const struct of_device_id imx135_dt_match[] = {
	{.compatible = "qcom,imx135", .data = &imx135_s_ctrl},
	{}
};

MODULE_DEVICE_TABLE(of, imx135_dt_match);

static struct platform_driver imx135_platform_driver = {
	.driver = {
		.name = "qcom,imx135",
		.owner = THIS_MODULE,
		.of_match_table = imx135_dt_match,
	},
};

static int32_t imx135_platform_probe(struct platform_device *pdev)
{
	int32_t rc = 0;
	const struct of_device_id *match;
	match = of_match_device(imx135_dt_match, &pdev->dev);
	rc = msm_sensor_platform_probe(pdev, match->data);
	return rc;
}

#define imx135_otp_blocks 6
struct imx135_otp_block_info {
	uint16_t slave_id;
	uint32_t start_addr;
	int block_size;
	uint8_t *data;
};
static uint8_t imx135_model_info[8];
static uint8_t imx135_model_awb[8];
static uint8_t imx135_model_af[4];
static uint8_t imx135_model_lsc[512];
struct imx135_otp_block_info imx135_block_info[imx135_otp_blocks] = {
	{0x52, 0x00,   5, &imx135_model_info[0]},
	{0x52, 0x15,   8, &imx135_model_awb[0]},
	{0x52, 0x21,   4, &imx135_model_af[0]},
	{0x52, 0x25, 219, &imx135_model_lsc[0]},
	{0x53, 0x00, 256, &imx135_model_lsc[0]+219},
	{0x54, 0x00,  37, &imx135_model_lsc[0]+219+256}
};
#ifdef DUMP_OTP_INFO
static void dump_otp_info(void){
	int i = 0;
	CDBG("%s: dump E\n", __func__);
	for(i = 0; i < 5; i++){
		CDBG("%s: imx135_model_info[%d]:%d\n", __func__, i, imx135_model_info[i]);
	}
	for(i = 0; i < 8; i++){
		CDBG("%s: imx135_model_awb[%d]:%d\n", __func__, i, imx135_model_awb[i]);
	}
	for(i = 0; i < 4; i++){
		CDBG("%s: imx135_model_af[%d]:%d\n", __func__, i, imx135_model_af[i]);
	}
	for(i = 0; i < 512; i++){
		CDBG("%s: imx135_model_lsc[%d]:%d\n", __func__, i, imx135_model_lsc[i]);
	}
	CDBG("%s: dump X\n", __func__);
}
#endif
static int32_t imx135_read_otp_info(struct msm_sensor_ctrl_t *s_ctrl)
{
	int rc = 0;
	int i = 0;
	uint16_t sid_bk = 0;
	enum msm_camera_i2c_reg_addr_type addr_type_bk = 0;
	sid_bk = s_ctrl->sensor_i2c_client->cci_client->sid;
	addr_type_bk = s_ctrl->sensor_i2c_client->addr_type;
	for(i = 0; i < imx135_otp_blocks; i++) {
		s_ctrl->sensor_i2c_client->cci_client->sid = imx135_block_info[i].slave_id;
		s_ctrl->sensor_i2c_client->addr_type = MSM_CAMERA_I2C_BYTE_ADDR;
  	rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_read_seq(
          s_ctrl->sensor_i2c_client,
          imx135_block_info[i].start_addr,
          imx135_block_info[i].data, imx135_block_info[i].block_size);
		if (rc < 0) {
			CDBG("%s: read failed\n", __func__);
			goto otp_read_error;
		}	
	}
#ifdef DUMP_OTP_INFO
  dump_otp_info();
#endif
otp_read_error:
	s_ctrl->sensor_i2c_client->cci_client->sid = sid_bk;
	s_ctrl->sensor_i2c_client->addr_type = addr_type_bk;
	return rc;
}
int32_t imx135_msm_sensor_otp_probe(struct msm_sensor_ctrl_t *s_ctrl)
{	
	CDBG("%s: %d\n", __func__, __LINE__);
	if(0==imx135_read_otp_info(s_ctrl)){
		module_integrator_id = imx135_model_info[0];
  }
	return 0;
}

static int __init imx135_init_module(void)
{
	int32_t rc = 0;
	pr_info("%s:%d\n", __func__, __LINE__);
	rc = platform_driver_probe(&imx135_platform_driver,
		imx135_platform_probe);
	if (!rc)
		return rc;
	CDBG("%s:%d rc %d\n", __func__, __LINE__, rc);
	return i2c_add_driver(&imx135_i2c_driver);
}

static void __exit imx135_exit_module(void)
{
	pr_info("%s:%d\n", __func__, __LINE__);
	if (imx135_s_ctrl.pdev) {
		msm_sensor_free_sensor_data(&imx135_s_ctrl);
		platform_driver_unregister(&imx135_platform_driver);
	} else
		i2c_del_driver(&imx135_i2c_driver);
	return;
}
int32_t imx135_msm_sensor_otp_write(struct msm_sensor_ctrl_t *s_ctrl)
{	
	int i = 0, rc = 0;
	uint16_t imx135_rg, imx135_bg, lsc_count, imx135_golden_rg, imx135_golden_bg;
	if(1 == module_integrator_id){
		imx135_golden_rg = 526;
		imx135_golden_bg = 598;
		imx135_rg = imx135_model_awb[0]*256+imx135_model_awb[1];
		imx135_bg = imx135_model_awb[2]*256+imx135_model_awb[3];
		CDBG("%s: imx135_rg %d\n", __func__, imx135_rg);
		imx135_rg = imx135_rg*256/imx135_golden_rg;
		CDBG("%s: imx135_rg %d\n", __func__, imx135_rg);
		CDBG("%s: imx135_bg %d\n", __func__, imx135_bg);
		imx135_bg = imx135_bg*256/imx135_golden_bg;
		CDBG("%s: imx135_bg %d\n", __func__, imx135_bg);
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_write(
		      s_ctrl->sensor_i2c_client,
		      0x0210, (imx135_rg&0x0700)>>8, MSM_CAMERA_I2C_BYTE_DATA);
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_write(
		      s_ctrl->sensor_i2c_client,
		      0x0211, imx135_rg&0x00FF, MSM_CAMERA_I2C_BYTE_DATA);
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_write(
		      s_ctrl->sensor_i2c_client,
		      0x0212, (imx135_bg&0x0700)>>8, MSM_CAMERA_I2C_BYTE_DATA);
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_write(
		      s_ctrl->sensor_i2c_client,
		      0x0213, imx135_bg&0x00FF, MSM_CAMERA_I2C_BYTE_DATA);
		lsc_count = imx135_model_lsc[0]*256+imx135_model_lsc[1];
		for(i = 0; i < lsc_count; i++){
			rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_write(
			      s_ctrl->sensor_i2c_client,
			      0x4800+i,
			      imx135_model_lsc[i+2], MSM_CAMERA_I2C_BYTE_DATA);
		}
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_write(
		      s_ctrl->sensor_i2c_client,
		      0x4500, 0x1f, MSM_CAMERA_I2C_BYTE_DATA);
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_write(
		      s_ctrl->sensor_i2c_client,
		      0x0700, 0x01, MSM_CAMERA_I2C_BYTE_DATA);
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_write(
		      s_ctrl->sensor_i2c_client,
		      0x3a63, 0x01, MSM_CAMERA_I2C_BYTE_DATA);
  }
	return 0;
}
static struct msm_sensor_fn_t imx135_sensor_fn_t = {
  .sensor_config = msm_sensor_config,
  .sensor_power_up = msm_sensor_power_up,
  .sensor_power_down = msm_sensor_power_down,
  .sensor_match_id = msm_sensor_match_id,
  .sensor_set_opt_setting = imx135_msm_sensor_otp_write,
};

static struct msm_sensor_ctrl_t imx135_s_ctrl = {
  .sensor_i2c_client = &imx135_sensor_i2c_client,
  .power_setting_array.power_setting = imx135_power_setting,
  .power_setting_array.size = ARRAY_SIZE(imx135_power_setting),
  .msm_sensor_mutex = &imx135_mut,
  .sensor_v4l2_subdev_info = imx135_subdev_info,
  .sensor_v4l2_subdev_info_size = ARRAY_SIZE(imx135_subdev_info),
  .func_tbl = &imx135_sensor_fn_t,
};

module_init(imx135_init_module);
module_exit(imx135_exit_module);
MODULE_DESCRIPTION("imx135");
MODULE_LICENSE("GPL v2");
