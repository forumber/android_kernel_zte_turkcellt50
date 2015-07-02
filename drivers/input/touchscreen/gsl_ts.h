/*
 *
 * gsl_ts.h header file for the driver for sileadinc gsl touchscreen
 *
 * Author: sileadinc 
 * Date:   2013 -10 -15
 *
 */

#ifndef __GSL_TS_H__
#define __GSL_TS_H__


#define GSL_DEBUG
#define TPD_PROC_DEBUG
#define GSL_APPLICATION
#define GSL_MONITOR
#define GSL_NOID_VERSION
//#define GSL_HAVE_TOUCH_KEY
#define HIGH_SPEED_I2C
#define REPORT_DATA_ANDROID_4_0
//#define TOUCH_VIRTUAL_KEYS

/*define screen of resolution ratio*/
#define SCREEN_MAX_X				1280//320
#define SCREEN_MAX_Y				800//480

#define MAX_FINGERS	  			5
#define MAX_CONTACTS				10

#define DMA_TRANS_LEN				0x08

#define GSL_I2C_ADAPTER_INDEX			0
/*i2c of adapter number*/
#define I2C_BOARD_INFO_METHOD			1
/*define i2c addr and device name*/
#define GSL_TS_ADDR 				0x40
#define GSL_TS_NAME				"gsl_tp"

/*define irq and reset gpio num*/
#define GSL_IRQ_PORT				17//61
#define GSL_RST_PORT				16//140
#define GSL_IRQ_NAME				"gsl_irq"
#define GSL_RST_NAME				"gsl_reset"

/*vdd power*/
#define GSL_POWER_ON()	do{\
			}while(0)




/*button of key*/
#ifdef GSL_HAVE_TOUCH_KEY
	struct key_data{
		u16 key;
		u32 x_min;
		u32 x_max;
		u32 y_min;
		u32 y_max;
	};
	#define GSL_KEY_NUM	3 
	static struct key_data gsl_key_data[GSL_KEY_NUM] = {
		{KEY_MENU,0,50,810,920},
		{KEY_HOME,380,420,810,920},
		{KEY_BACK,430,480,810,920},
	};
#endif

struct fw_data
{
    u32 offset : 8;
    u32 : 0;
    u32 val;
};
struct gsl_touch_info{
	int x[10];
	int y[10];
	int id[10];
	int finger_num;
};

struct gsl_ts_platform_data {
	//int irq_gpio_number;
	//int reset_gpio_number;	
	bool x_flip;
	bool y_flip;
	bool i2c_pull_up;
	bool power_down_enable;
	bool disable_gpios;
	bool do_lockdown;
	unsigned irq_gpio;
	u32 irq_flags;
	u32 reset_flags;
	unsigned reset_gpio;
	unsigned panel_minx;
	unsigned panel_miny;
	unsigned panel_maxx;
	unsigned panel_maxy;
	unsigned disp_minx;
	unsigned disp_miny;
	unsigned disp_maxx;
	unsigned disp_maxy;
	unsigned reset_delay;
	//const char *fw_image_name;
};

struct gsl_ts_data {
	struct i2c_client *client;
	struct input_dev *input;
	struct work_struct work;
	struct workqueue_struct *wq;
	struct gsl_touch_info *cinfo;
	struct mutex i2c_lock;
	struct gsl_ts_platform_data *pdata;
	int irq;
	u32 is_suspend;			//0 normal;1 the machine is suspend;
	u32 sw_init_flag;			//1:sw init 0:no 
	u32 point_state;
#if defined(CONFIG_FB)
	struct notifier_block fb_notif;
#elif defined(CONFIG_HAS_EARLYSUSPEND)
	struct early_suspend early_suspend;
#endif
#ifdef GSL_HAVE_TOUCH_KEY
	int key_state;
#endif
#if 0
	struct gsl_ts_platform_data *pdata;
#endif
	struct regulator *avdd;
	struct regulator *vdd;
	struct regulator *vcc_i2c;

};

#ifdef GSL_NOID_VERSION
extern unsigned int gsl_version_id(void);
extern void gsl_alg_id_main(struct gsl_touch_info *cinfo);
extern void gsl_DataInit(unsigned int *ret);
extern unsigned int gsl_mask_tiaoping(void);
#endif

#include "gsl_ts_fw_success.h"
#include "gsl_ts_fw_jianbang.h"

static u8 gsl_cfg_index = 0;
struct fw_config_type
{
	const struct fw_data *fw;
	unsigned int fw_size;
#ifdef GSL_NOID_VERSION
	unsigned int *data_id;
	unsigned int data_size;
#endif
};
static struct fw_config_type gsl_cfg_table[9] = {
/*0*/{GSLX68X_FW_SUCCESS,(sizeof(GSLX68X_FW_SUCCESS)/sizeof(struct fw_data)),
	gsl_config_data_id_success,(sizeof(gsl_config_data_id_success)/4)},
/*1*/{GSLX68X_FW_JIANBANG,(sizeof(GSLX68X_FW_JIANBANG)/sizeof(struct fw_data)),
	gsl_config_data_id_jianbang,(sizeof(gsl_config_data_id_jianbang)/4)}, //GSL_TS_CFG_CHAOSHENG_4_2_OGS_1680,
/*2*/{NULL,0,NULL,0}, //GSL_TS_CFG_BAOMING_4_5_G_G_2682,
/*3*/{NULL,0,NULL,0}, //GSL_TS_CFG_LIYOU_4_5_P_G_1688,
/*4*/{NULL,0,NULL,0},
/*5*/{NULL,0,NULL,0},
/*6*/{NULL,0,NULL,0},
/*7*/{NULL,0,NULL,0},
/*8*/{NULL,0,NULL,0},
};
#endif  //#ifndef __GSL_TS_H__
