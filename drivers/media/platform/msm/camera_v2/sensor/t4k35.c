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
#define T4K35_SENSOR_NAME "t4k35"
DEFINE_MSM_MUTEX(t4k35_mut);
#undef CDBG
#ifdef CONFIG_MSMB_CAMERA_DEBUG
#define CDBG(fmt, args...) pr_err(fmt, ##args)
#else
#define CDBG(fmt, args...) do { } while (0)
#endif
static struct msm_sensor_ctrl_t t4k35_s_ctrl;
extern uint8_t eeprom_buffer[12];
extern uint16_t module_integrator_id;
static struct msm_sensor_power_setting t4k35_power_setting[] = {
	{
		.seq_type = SENSOR_VREG,
		.seq_val = CAM_VIO,
		.config_val = 0,
		.delay = 1,
	},
       {
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_VANA,
		.config_val = GPIO_OUT_HIGH,
		.delay = 0,
	},	
	{
		.seq_type = SENSOR_VREG,
		.seq_val = CAM_VDIG,
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
		.seq_val = SENSOR_GPIO_STANDBY,
		.config_val = GPIO_OUT_LOW,
		.delay = 10,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_STANDBY,
		.config_val = GPIO_OUT_HIGH,
		.delay = 10,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_RESET,
		.config_val = GPIO_OUT_LOW,
		.delay = 10,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_RESET,
		.config_val = GPIO_OUT_HIGH,
		.delay = 10,
	},
	{
		.seq_type = SENSOR_CLK,
		.seq_val = SENSOR_CAM_MCLK,
		.config_val = 24000000,
		.delay = 5,
	},
	{
		.seq_type = SENSOR_I2C_MUX,
		.seq_val = 0,
		.config_val = 0,
		.delay = 0,
	},
};

static struct v4l2_subdev_info t4k35_subdev_info[] = {
	{
		.code   = V4L2_MBUS_FMT_SGRBG10_1X10,
		.colorspace = V4L2_COLORSPACE_JPEG,
		.fmt    = 1,
		.order    = 0,
	},
};

static const struct i2c_device_id t4k35_i2c_id[] = {
	{T4K35_SENSOR_NAME, (kernel_ulong_t)&t4k35_s_ctrl},
	{ }
};

static int32_t msm_t4k35_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	return msm_sensor_i2c_probe(client, id, &t4k35_s_ctrl);
}

static struct i2c_driver t4k35_i2c_driver = {
	.id_table = t4k35_i2c_id,
	.probe  = msm_t4k35_i2c_probe,
	.driver = {
		.name = T4K35_SENSOR_NAME,
	},
};

static struct msm_camera_i2c_client t4k35_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};

static const struct of_device_id t4k35_dt_match[] = {
	{.compatible = "qcom,t4k35", .data = &t4k35_s_ctrl},
	{}
};

MODULE_DEVICE_TABLE(of, t4k35_dt_match);

static struct platform_driver t4k35_platform_driver = {
	.driver = {
		.name = "qcom,t4k35",
		.owner = THIS_MODULE,
		.of_match_table = t4k35_dt_match,
	},
};

static int32_t t4k35_platform_probe(struct platform_device *pdev)
{
	int32_t rc = 0;
	const struct of_device_id *match;
	match = of_match_device(t4k35_dt_match, &pdev->dev);
	rc = msm_sensor_platform_probe(pdev, match->data);
	return rc;
}

static int __init t4k35_init_module(void)
{
	int32_t rc = 0;
	CDBG("%s:%d\n", __func__, __LINE__);
	rc = platform_driver_probe(&t4k35_platform_driver,
		t4k35_platform_probe);
	if (!rc)
		return rc;
	CDBG("%s:%d rc %d\n", __func__, __LINE__, rc);
	return i2c_add_driver(&t4k35_i2c_driver);
}

static void __exit t4k35_exit_module(void)
{
	CDBG("%s:%d\n", __func__, __LINE__);
	if (t4k35_s_ctrl.pdev) {
		msm_sensor_free_sensor_data(&t4k35_s_ctrl);
		platform_driver_unregister(&t4k35_platform_driver);
	} else
		i2c_del_driver(&t4k35_i2c_driver);
	return;
}
#if 1

static int32_t t4k35_sensor_i2c_read(struct msm_sensor_ctrl_t *s_ctrl,
	uint16_t addr, uint16_t *data,
	enum msm_camera_i2c_data_type data_type)
{
    int32_t rc = 0;
    rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_read(
            s_ctrl->sensor_i2c_client,
            addr,
            data, data_type);
    return rc;
}

static int32_t t4k35_sensor_i2c_write(struct msm_sensor_ctrl_t *s_ctrl,
	uint16_t addr, uint16_t data,
	enum msm_camera_i2c_data_type data_type)
{
    int32_t rc = 0;
    rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
        i2c_write(s_ctrl->sensor_i2c_client, addr, data, data_type);
    if (rc < 0) {
        msleep(100);
        rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
               i2c_write(s_ctrl->sensor_i2c_client, addr, data, data_type);
    }
	return rc;
}

#define SET_T4k35_REG(reg, val) t4k35_sensor_i2c_write(s_ctrl,reg,val,MSM_CAMERA_I2C_BYTE_DATA)
#define GET_T4k35_REG(reg, val) t4k35_sensor_i2c_read(s_ctrl,reg,&val,MSM_CAMERA_I2C_BYTE_DATA)

typedef struct t4k35_otp_struct 
{
  uint8_t LSC[53];              /* LSC */
  uint8_t AWB[8];               /* AWB */
  uint8_t Module_Info[9];
  uint16_t AF_Macro;
  uint16_t AF_Inifity;
} st_t4k35_otp;
st_t4k35_otp t4k35_data;
#define T4k35_OTP_PSEL 0x3502
#define T4k35_OTP_CTRL 0x3500
#define T4k35_OTP_DATA_BEGIN_ADDR 0x3504
#define T4k35_OTP_DATA_END_ADDR 0x3543

static uint16_t t4k35_otp_data[T4k35_OTP_DATA_END_ADDR - T4k35_OTP_DATA_BEGIN_ADDR + 1] = {0x00};
static uint16_t t4k35_otp_data_backup[T4k35_OTP_DATA_END_ADDR - T4k35_OTP_DATA_BEGIN_ADDR + 1] = {0x00};

static uint16_t t4k35_r_golden_value=0x50;
static uint16_t t4k35_g_golden_value=0x90;
static uint16_t t4k35_b_golden_value=0x5D;
static uint16_t t4k35_af_macro_pos=679;
static uint16_t t4k35_af_inifity_pos=221;

void t4k35_otp_set_page(struct msm_sensor_ctrl_t *s_ctrl, uint16_t page)
{
   	//set page
    SET_T4k35_REG(T4k35_OTP_PSEL, page);
}
void t4k35_otp_access(struct msm_sensor_ctrl_t *s_ctrl)
{
	uint16_t reg_val;
	//OTP access
	GET_T4k35_REG(T4k35_OTP_CTRL, reg_val);
	SET_T4k35_REG(T4k35_OTP_CTRL, reg_val | 0x80);
	usleep(30);
}
void t4k35_otp_read_enble(struct msm_sensor_ctrl_t *s_ctrl, uint8_t enble)
{
    if (enble)
        SET_T4k35_REG(T4k35_OTP_CTRL, 0x01);
    else
        SET_T4k35_REG(T4k35_OTP_CTRL, 0x00);
}



int32_t t4k35_otp_read_data(struct msm_sensor_ctrl_t *s_ctrl, uint16_t* otp_data)
{
    uint16_t i = 0;
    //uint16_t data = 0;
    for (i = 0; i <= (T4k35_OTP_DATA_END_ADDR - T4k35_OTP_DATA_BEGIN_ADDR); i++)
	{
        GET_T4k35_REG(T4k35_OTP_DATA_BEGIN_ADDR+i,otp_data[i]);
    }

    return 0;
}

void t4k35_update_awb(struct msm_sensor_ctrl_t *s_ctrl, struct t4k35_otp_struct *p_otp)
{
  uint16_t rg,bg,r_otp,g_otp,b_otp;

  r_otp=p_otp->AWB[1]+(p_otp->AWB[0]<<8);
  g_otp=(p_otp->AWB[3]+(p_otp->AWB[2]<<8)+p_otp->AWB[5]+(p_otp->AWB[4]<<8))/2;
  b_otp=p_otp->AWB[7]+(p_otp->AWB[6]<<8);
  
  rg = 256*(t4k35_r_golden_value *g_otp)/(r_otp*t4k35_g_golden_value);
  bg = 256*(t4k35_b_golden_value*g_otp)/(b_otp*t4k35_g_golden_value);

  CDBG("r_golden=0x%x,g_golden=0x%x, b_golden=0x%0x\n", t4k35_r_golden_value,t4k35_g_golden_value,t4k35_b_golden_value);
  CDBG("r_otp=0x%x,g_opt=0x%x, b_otp=0x%0x\n", r_otp,g_otp,b_otp);
  CDBG("rg=0x%x, bg=0x%0x\n", rg,bg);

  //R
  SET_T4k35_REG(0x0212, rg >> 8);
  SET_T4k35_REG(0x0213, rg & 0xff);

  //B
  SET_T4k35_REG(0x0214, bg >> 8);
  SET_T4k35_REG(0x0215, bg & 0xff);

}

void t4k35_update_lsc(struct msm_sensor_ctrl_t *s_ctrl, struct t4k35_otp_struct *p_otp)
{
  uint16_t addr;
  int i;

  //set lsc parameters
  addr = 0x323A;
  SET_T4k35_REG(addr, p_otp->LSC[0]);
  addr = 0x323E;
  for(i = 1; i < 53; i++) 
  {
    CDBG("SET LSC[%d], addr:0x%0x, val:0x%0x \n", i, addr, p_otp->LSC[i]);
    SET_T4k35_REG(addr++, p_otp->LSC[i]);
  }

  //turn on lsc
  SET_T4k35_REG(0x3237,0x80);
}

int32_t t4k35_otp_init_lsc_awb(struct msm_sensor_ctrl_t *s_ctrl, struct t4k35_otp_struct *p_otp)
{
  int i,j;
  //uint16_t reg_val;
  uint16_t check_sum=0x00;

  //read OTP LSC and AWB data
  for(i = 3; i >= 0; i--) 
  {
  	//otp enable
  	t4k35_otp_read_enble(s_ctrl, 1);
  	//read data area
  	//set page
	t4k35_otp_set_page(s_ctrl, i);
	//OTP access
    t4k35_otp_access(s_ctrl);

	CDBG("otp lsc data area data: %d \n", i);
    t4k35_otp_read_data(s_ctrl, t4k35_otp_data);


	//read data backup area
	CDBG("otp lsc backup area data: %d\n", i + 6);
	//set page
	t4k35_otp_set_page(s_ctrl, i+6);
	//OTP access
    t4k35_otp_access(s_ctrl);
	
	t4k35_otp_read_data(s_ctrl, t4k35_otp_data_backup);
	//otp disable
  	t4k35_otp_read_enble(s_ctrl, 0);

	//get final OTP data;
    for(j = 0; j < 64; j++) 
	{
        t4k35_otp_data[j]=t4k35_otp_data[j]|t4k35_otp_data_backup[j];
    }

	//check program flag
    if (0 == t4k35_otp_data[0]) 
	{
      continue;
    } 
	else 
	{
	  //checking check sum
	  for(j = 2; j < 64; j++) 
	  {
        check_sum=check_sum+t4k35_otp_data[j];
      }
	  
	  if((check_sum & 0xFF) == t4k35_otp_data[1])
	  {
	  	CDBG("otp lsc checksum ok!\n");
		for(j=3;j<=55;j++)
		{
			p_otp->LSC[j-3]=t4k35_otp_data[j];
		}
		for(j=56;j<=63;j++)
		{
			p_otp->AWB[j-56]=t4k35_otp_data[j];
		}
		return 0;
	  }
	  else
	  {
		CDBG("otp lsc checksum error!\n");
		return -1;
	  }
    }
  }

  if (i < 0) 
  {
    return -1;
    CDBG("No otp lsc data on sensor t4k35\n");
  }
  else 
  {
    return 0;
  }
}

int32_t t4k35_otp_init_module_info(struct msm_sensor_ctrl_t *s_ctrl, struct t4k35_otp_struct *p_otp)
{
  int i,pos;
  uint16_t check_sum=0x00;

  //otp enable
  t4k35_otp_read_enble(s_ctrl, 1);
  //set page
  t4k35_otp_set_page(s_ctrl, 4);
  //OTP access
  t4k35_otp_access(s_ctrl);
  CDBG("data area data:\n");
  t4k35_otp_read_data(s_ctrl, t4k35_otp_data);


  //set page
  t4k35_otp_set_page(s_ctrl, 10);
  //OTP access
  t4k35_otp_access(s_ctrl);
  t4k35_otp_read_data(s_ctrl, t4k35_otp_data_backup);
  //otp disable
  t4k35_otp_read_enble(s_ctrl, 0);		
  
  //get final OTP data;
  for(i = 0; i < 64; i++) 
  {
	  t4k35_otp_data[i]=t4k35_otp_data[i]|t4k35_otp_data_backup[i];
  }

  //check flag
  if(t4k35_otp_data[32])
  {
	  pos=32;
  }
  else if(t4k35_otp_data[0])
  {
  	  pos=0;
  }
  else
  {
  	  CDBG("otp no module information!\n");
  	  return -1;
  }
  

  //checking check sum
  for(i = pos+2; i <pos+32; i++) 
  {
     check_sum=check_sum+t4k35_otp_data[i];
  }
	  
  if((check_sum&0xFF)==t4k35_otp_data[pos+1])
  {
	  	CDBG("otp module info checksum ok!\n");
	  	eeprom_buffer[0]=t4k35_otp_data[pos+6];//vendor
	  	eeprom_buffer[2]=t4k35_otp_data[pos+8];//vcm
	  	eeprom_buffer[3]=t4k35_otp_data[pos+9];//vcm driver
	  	eeprom_buffer[4]=t4k35_otp_data[pos+10];//lens
	  	module_integrator_id=t4k35_otp_data[pos+6];
		if((t4k35_otp_data[pos+15]==0x00)&&(t4k35_otp_data[pos+16]==0x00)
			&&(t4k35_otp_data[pos+17]==0x00)&&(t4k35_otp_data[pos+18]==0x00)
			&&(t4k35_otp_data[pos+19]==0x00)&&(t4k35_otp_data[pos+20]==0x00)
			&&(t4k35_otp_data[pos+21]==0x00)&&(t4k35_otp_data[pos+22]==0x00))
			return 0;
		
			
		t4k35_r_golden_value=t4k35_otp_data[pos+16]+(t4k35_otp_data[pos+15]<<8);
		t4k35_g_golden_value=(t4k35_otp_data[pos+18]+(t4k35_otp_data[pos+17]<<8)+t4k35_otp_data[pos+20]+(t4k35_otp_data[pos+19]<<8))/2;
		t4k35_b_golden_value=t4k35_otp_data[pos+22]+(t4k35_otp_data[pos+21]<<8);
		return 0;
  }
  else
  {
	CDBG("otp module info checksum error!\n");
	return -1;
  }

}

int32_t t4k35_otp_init_af(struct msm_sensor_ctrl_t *s_ctrl)
{
  int i,pos;
  uint16_t check_sum=0x00;

  //otp enable
  t4k35_otp_read_enble(s_ctrl, 1);
  //set page
  t4k35_otp_set_page(s_ctrl, 5);
  //OTP access
  t4k35_otp_access(s_ctrl);
  CDBG("data area data:\n");
  t4k35_otp_read_data(s_ctrl, t4k35_otp_data);


  //set page
  t4k35_otp_set_page(s_ctrl, 11);
  //OTP access
  t4k35_otp_access(s_ctrl);
  t4k35_otp_read_data(s_ctrl, t4k35_otp_data_backup);
  //otp disable
  t4k35_otp_read_enble(s_ctrl, 0);		
  
  //get final OTP data;
  for(i = 0; i < 64; i++) 
  {
	  t4k35_otp_data[i]=t4k35_otp_data[i]|t4k35_otp_data_backup[i];
  }

  //macro AF
  //check flag
  if(t4k35_otp_data[24])
  {
	  pos=24;
  }
  else if(t4k35_otp_data[16])
  {
  	  pos=16;
  }
  else if(t4k35_otp_data[8])
  {
  	  pos=8;
  }
  else if(t4k35_otp_data[0])
  {
  	  pos=0;
  }
  else
  {
  	  pr_err("no otp macro AF information!\n");
	  s_ctrl->af_otp_macro = t4k35_af_macro_pos;
	  s_ctrl->af_otp_inifity = t4k35_af_inifity_pos;
	  eeprom_buffer[8]=s_ctrl->af_otp_macro>>8;
	  eeprom_buffer[9]=s_ctrl->af_otp_macro;
	  eeprom_buffer[10]= s_ctrl->af_otp_inifity>>8;
      eeprom_buffer[11]= s_ctrl->af_otp_inifity;
  	  return -1;
  }
  
  //checking check sum
  check_sum=0x00;
  for(i = pos+2; i <pos+8; i++) 
  {
     check_sum=check_sum+t4k35_otp_data[i];
  }
	  
  if((check_sum&0xFF)==t4k35_otp_data[pos+1])
  {
	  	CDBG("otp macro AF checksum ok!\n");
        s_ctrl->af_otp_macro=(t4k35_otp_data[pos+3]<<8)+t4k35_otp_data[pos+4];
        if(s_ctrl->af_otp_macro==0x00)
        {
           s_ctrl->af_otp_macro=t4k35_af_macro_pos;
        }
		    eeprom_buffer[8]=s_ctrl->af_otp_macro>>8;
		    eeprom_buffer[9]=s_ctrl->af_otp_macro;

  }
  else
  {
	pr_err("otp macro AF checksum error!\n");
    s_ctrl->af_otp_macro=t4k35_af_macro_pos;
	eeprom_buffer[8]=s_ctrl->af_otp_macro>>8;
	eeprom_buffer[9]=s_ctrl->af_otp_macro;
	//return -1;
  }

  //inifity AF
  //check flag
  if(t4k35_otp_data[56])
  {
	  pos=56;
  }
  else if(t4k35_otp_data[48])
  {
  	  pos=48;
  }
  else if(t4k35_otp_data[40])
  {
  	  pos=40;
  }
  else if(t4k35_otp_data[32])
  {
  	  pos=32;
  }
  else
  {
  	  pr_err("no otp inifity AF information!\n");
	  s_ctrl->af_otp_macro = t4k35_af_macro_pos;
	  s_ctrl->af_otp_inifity = t4k35_af_inifity_pos;
	  eeprom_buffer[8]=s_ctrl->af_otp_macro>>8;
	  eeprom_buffer[9]=s_ctrl->af_otp_macro;
	  eeprom_buffer[10]= s_ctrl->af_otp_inifity>>8;
      eeprom_buffer[11]= s_ctrl->af_otp_inifity;
  	  return -1;
  }
  
  //checking check sum
  check_sum=0x00;
  for(i = pos+2; i <pos+8; i++) 
  {
     check_sum=check_sum+t4k35_otp_data[i];
  }
	  
  if((check_sum&0xFF)==t4k35_otp_data[pos+1])
  {
	  	CDBG("otp inifity AF checksum ok!\n");
        s_ctrl->af_otp_inifity=(t4k35_otp_data[pos+4]<<8)+t4k35_otp_data[pos+5];
        if(s_ctrl->af_otp_inifity==0x00)
        {
           s_ctrl->af_otp_inifity=t4k35_af_inifity_pos;
        }
           eeprom_buffer[10]= s_ctrl->af_otp_inifity>>8;
           eeprom_buffer[11]= s_ctrl->af_otp_inifity;

  }
  else
  {
	pr_err("otp inifity AF checksum error!\n");
    s_ctrl->af_otp_inifity=t4k35_af_inifity_pos;
	eeprom_buffer[10]= s_ctrl->af_otp_inifity>>8;
    eeprom_buffer[11]= s_ctrl->af_otp_inifity;
  }
#if 0
  if(s_ctrl->af_otp_inifity < 200)
  {
    pr_err("af_otp_inifity = %d\n",s_ctrl->af_otp_inifity);
	s_ctrl->af_otp_inifity = t4k35_af_inifity_pos;
    eeprom_buffer[10]= s_ctrl->af_otp_inifity>>8;
    eeprom_buffer[11]= s_ctrl->af_otp_inifity;
  }
#endif
  return 0;

}

int32_t t4k35_otp_init_setting(struct msm_sensor_ctrl_t *s_ctrl)
{
    int32_t rc = 0;

	rc=t4k35_otp_init_module_info(s_ctrl, &t4k35_data);
	if(rc==0x00)
	{
		//check module information
	}
	
    rc=t4k35_otp_init_lsc_awb(s_ctrl, &t4k35_data);

	t4k35_otp_init_af(s_ctrl);
    
    return rc;
}
int32_t t4k35_otp_write_setting(struct msm_sensor_ctrl_t *s_ctrl)
{
	t4k35_update_lsc(s_ctrl, &t4k35_data);
	t4k35_update_awb(s_ctrl, &t4k35_data);
		return 0;
}

#if 0
static int32_t t4k35_init_otp_setting(struct msm_sensor_ctrl_t * s_ctrl,
                                                        uint16_t page_num)
{
   	int32_t rc = -EFAULT;
    
	rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x3500,
	                                            0x01, MSM_CAMERA_I2C_BYTE_DATA);
	//open which page
	rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x3502,
	                                            page_num, MSM_CAMERA_I2C_BYTE_DATA);
	
	//OTP_RWT/OTP_RNUM[1:0]/OTP_VERIFY/OTP_VMOD/OTP_PCLK[2:0]   
	rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x3547,0x04,
                                                   MSM_CAMERA_I2C_BYTE_DATA);

    rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x3500,
                                                0x81, MSM_CAMERA_I2C_BYTE_DATA);
	//delay 30us at least
	msleep(1);
	
    if(rc < 0)
    {
       CDBG("%s:Page %x open FAILED\n",__func__,page_num);
       return rc;
    }
	 return rc;
}

/*After the whole OTP operation is over,close the OTP*/
static int32_t t4k35_close_otp(struct msm_sensor_ctrl_t * s_ctrl)
{
   	int32_t rc = -EFAULT;
	//close otp
	rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x3500,0x00,                                                  
	                                            MSM_CAMERA_I2C_BYTE_DATA);
    if(rc<0)
    {
       CDBG("%s:close OTP FAILED",__func__);
       return rc;
    }
    
    return rc;
}

static int32_t t4k35_update_otp_data(struct msm_sensor_ctrl_t * s_ctrl)
{
    uint16_t Program_Flag = 0; //0-this page is not programed 
    uint16_t check_sum = 0;    //check sum
    uint32_t sum = 0;
    int i = 0;
    uint16_t otpError = 0;
	int page_index = 0; //record the programmed page num
	uint16_t otp_data[64] = {0};  //data region
	uint16_t otp_data_backup[64] = {0};  //backup region
	//从PAGE 3开始读，如果0x3014数据读出来为0，则读前一页
	//直至读到PAGE 0
	// 1. find the programmed page 
    for(i = 3; i >= 0; i--)
    {
        t4k04_init_otp_setting(s_ctrl,i);
        msleep(5);
        msm_camera_i2c_read(s_ctrl->sensor_i2c_client, 0x3546, &otpError,
     		                            MSM_CAMERA_I2C_BYTE_DATA);
        if ((otpError & 0x08) != 0)
        {
            CDBG("%s: t4k35 read 0x3546 error!!! ", __func__);
            t4k35_close_otp(s_ctrl);
            return -1;
        }
        
        msm_camera_i2c_read(s_ctrl->sensor_i2c_client, 0x3504, &Program_Flag,
     		                            MSM_CAMERA_I2C_BYTE_DATA);
   	    CDBG("%s:t4k35 0x3504 Program_Flag is %d(PAGE %d)\n",__func__,Program_Flag,i);
     	if(Program_Flag != 0)
     	{
             CDBG("%s:t4k35 OTP Page %d is programmed\n",__func__,i);
   	         page_index = i;
             break;
   	    }

        if(i == 0)
        {
           CDBG("%s:t4k35 update OTP data FAILED,return\n",__func__);
           return -1;
        }
    }

	// 2. check the check_sum whether right
    msm_camera_i2c_read(s_ctrl->sensor_i2c_client, 0x3505, &check_sum,
 	                            MSM_CAMERA_I2C_BYTE_DATA);
    CDBG("%s:0x3505 check_sum is 0x%x\n",__func__,check_sum);

	for(i = 0; i < 62; i++)
	{
	      //read from 0x3106 to 0x3143
		msm_camera_i2c_read(s_ctrl->sensor_i2c_client,(0x3506 + i),&otp_data[i],
		                            MSM_CAMERA_I2C_BYTE_DATA);
		sum += otp_data[i];
	}

	 CDBG("%s:0x3507  is 0x%x\n",__func__,otp_data[1]);
	 CDBG("%s:0x3508  is 0x%x\n",__func__,otp_data[2]);
	 CDBG("%s:0x3509  is 0x%x\n",__func__,otp_data[3]);
	if((sum%256) != check_sum)
	{
		CDBG("%s:the check sum ERROR,return\n",__func__);   
		return -1;
	}
	
     // 3. check the backup data is right
     check_sum=0;
	 sum=0;
     t4k04_init_otp_setting(s_ctrl,(page_index + 6));
	  
     msm_camera_i2c_read(s_ctrl->sensor_i2c_client,0x3505,&check_sum,
		                            MSM_CAMERA_I2C_BYTE_DATA);
     CDBG("%s:0x3505 backup check_sum is 0x%x\n",__func__,check_sum);

	for(i=0;i<62;i++)
	{
	      //read from 0x3106 to 0x3143
		msm_camera_i2c_read(s_ctrl->sensor_i2c_client,(0x3506+i),&otp_data_backup[i],
		                            MSM_CAMERA_I2C_BYTE_DATA);
		sum  += otp_data_backup[i];
	}

	 CDBG("%s:0x3507 backup is 0x%x\n",__func__,otp_data_backup[1]);
	 CDBG("%s:0x3508 backup is 0x%x\n",__func__,otp_data_backup[2]);
	 CDBG("%s:0x3509 backup is 0x%x\n",__func__,otp_data_backup[3]);
	if((sum%256) != check_sum)
	{
		CDBG("%s:the backup check sum ERROR,return\n",__func__);   
		return -1;
	}

	// 4. otp_data_backup |otp_data

	for(i = 0; i < 62; i++)
	{
		otp_data[i] = otp_data[i] | otp_data_backup[i];
	}

	// 5. write the OTP data to the Registers
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x323A, otp_data[1],                                                  
	                                            MSM_CAMERA_I2C_BYTE_DATA);
	for(i = 0; i < 52; i++)
	{
	   msm_camera_i2c_write(s_ctrl->sensor_i2c_client,(0x323E + i), otp_data[1 + i],                                                  
	                                            MSM_CAMERA_I2C_BYTE_DATA);
	}

    msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x0212, otp_data[53],                                                  
	                                            MSM_CAMERA_I2C_BYTE_DATA);  

    msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x0213, otp_data[54],                                                  
	                                            MSM_CAMERA_I2C_BYTE_DATA);  

    msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x0210, otp_data[55],                                                  
	                                            MSM_CAMERA_I2C_BYTE_DATA);  

    msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x0211, otp_data[56],                                                  
	                                            MSM_CAMERA_I2C_BYTE_DATA);      

    msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x0216, otp_data[57],                                                  
	                                            MSM_CAMERA_I2C_BYTE_DATA);  

    msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x0217, otp_data[58],                                                  
	                                            MSM_CAMERA_I2C_BYTE_DATA);  
        
    msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x0214, otp_data[59],                                                  
	                                            MSM_CAMERA_I2C_BYTE_DATA);   

    msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x0215, otp_data[60],                                                  
	                                            MSM_CAMERA_I2C_BYTE_DATA);      
	return 0;
}

#endif

static struct msm_sensor_fn_t t4k35_sensor_fn_t = {
	.sensor_config = msm_sensor_config,
	.sensor_power_up = msm_sensor_power_up,
	.sensor_power_down = msm_sensor_power_down,
	.sensor_match_id = msm_sensor_match_id,
	.sensor_set_opt_setting = t4k35_otp_write_setting,
};


#endif
static struct msm_sensor_ctrl_t t4k35_s_ctrl = {
	.sensor_i2c_client = &t4k35_sensor_i2c_client,
	.power_setting_array.power_setting = t4k35_power_setting,
	.power_setting_array.size = ARRAY_SIZE(t4k35_power_setting),
	.msm_sensor_mutex = &t4k35_mut,
	.sensor_v4l2_subdev_info = t4k35_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(t4k35_subdev_info),
	.func_tbl = &t4k35_sensor_fn_t,
};

module_init(t4k35_init_module);
module_exit(t4k35_exit_module);
MODULE_DESCRIPTION("t4k35");
MODULE_LICENSE("GPL v2");
