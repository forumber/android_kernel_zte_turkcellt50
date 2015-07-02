/* drivers/input/touchscreen/gsl_ts_qualcomm.c
 * 
 * 2010 - 2013 SLIEAD Technology.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be a reference 
 * to you, when you are integrating the SLIEAD's CTP IC into your system, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 * General Public License for more details.
 * 
 */
 
//#include <linux/module.h>
//#include <linux/slab.h>
//#include <linux/input.h>
//#include <linux/interrupt.h>
//#include <linux/i2c.h>
//#include <linux/delay.h>
//#include <mach/gpio.h>
//#include <mach/gpio_data.h>
#include <linux/jiffies.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/pm_runtime.h>

#include <linux/input/mt.h>

#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/async.h>
#include <linux/hrtimer.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <mach/gpio.h>/*kingshine niro*/
#include <linux/regulator/consumer.h>

#if defined(CONFIG_FB)
#include <linux/notifier.h>
#include <linux/fb.h>
#elif defined(CONFIG_HAS_EARLYSUSPEND)
#include <linux/earlysuspend.h>
#endif
#include <mach/irqs.h>
#include <mach/hardware.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/of_gpio.h>

#include "gsl_ts.h"

#ifdef REPORT_DATA_ANDROID_4_0
    #include <linux/input/mt.h>
#endif
#define GSL_COORDS_ARR_SIZE	4
#define MAX_BUTTONS		4

#define TPD_PROC_DEBUG
#ifdef TPD_PROC_DEBUG
static struct proc_dir_entry *gsl_config_proc = NULL;
#define GSL_CONFIG_PROC_FILE "gsl_config"
#define CONFIG_LEN 31
static char gsl_read[CONFIG_LEN];
static u8 gsl_data_proc[8] = {0};
static u8 gsl_proc_flag = 0;
#endif

#ifdef GSL_MONITOR
static struct delayed_work gsl_monitor_work;
static struct workqueue_struct *gsl_monitor_workqueue = NULL;
static char int_1st[4] = {0};
static char int_2nd[4] = {0};
#endif

#ifdef GSL_DEBUG 
#define print_info(fmt, args...)   \
        do{                              \
                printk("[tp-gsl][%s]"fmt,__func__, ##args);     \
        }while(0)
#else
#define print_info(fmt, args...)   //
#endif


static struct mutex *gsl_i2c_lock;
static struct i2c_client *gsl_client = NULL;



#ifdef TOUCH_VIRTUAL_KEYS
static ssize_t virtual_keys_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{

	return sprintf(buf,
         __stringify(EV_KEY) ":" __stringify(KEY_MENU)   ":80:900:80:80"
	 ":" __stringify(EV_KEY) ":" __stringify(KEY_HOME)   ":240:900:80:80"
	 ":" __stringify(EV_KEY) ":" __stringify(KEY_BACK) ":400:900:80:80"
	 "\n");
}

static struct kobj_attribute virtual_keys_attr = {
    .attr = {
        .name = "virtualkeys.gsl_tp",
        .mode = S_IRUGO,
    },
    .show = &virtual_keys_show,
};

static struct attribute *properties_attrs[] = {
    &virtual_keys_attr.attr,
    NULL
};

static struct attribute_group properties_attr_group = {
    .attrs = properties_attrs,
};

static void gsl_ts_virtual_keys_init(void)
{
    int ret;
    struct kobject *properties_kobj;

    //print_info("%s\n",__func__);

    properties_kobj = kobject_create_and_add("board_properties", NULL);
    if (properties_kobj)
        ret = sysfs_create_group(properties_kobj,
                     &properties_attr_group);
    if (!properties_kobj || ret)
        pr_err("failed to create board_properties\n");
}

#endif

static void gsl_hw_init(void)
{
	int ret;
	//add power
	GSL_POWER_ON();	
	//

	ret = gpio_request(GSL_IRQ_PORT, "GSL_IRQ");
	if (ret) {
		printk("gsl_hw_init,GSL_IRQ_PORT fail");
		//dev_err(&client->dev, "ret = %d : could not req gpio irq\n", ret);
	}

	ret = gpio_request(GSL_RST_PORT, "GSL_RST");
	if (ret) {
		//dev_err(&client->dev, "ret = %d : could not req gpio reset\n", ret);
		printk("gsl_hw_init,GSL_RST_PORT fail");
	}
	
	gpio_direction_input(GSL_IRQ_PORT);
	//add by wkh begin
	ret = gpio_tlmm_config(GPIO_CFG(GSL_RST_PORT, 0,
				GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,
				GPIO_CFG_6MA), GPIO_CFG_ENABLE);
	//end
	gpio_direction_output(GSL_RST_PORT, 1);
	mdelay(20);
	//gpio_direction_output(GSL_RST_PORT, 0);
	gpio_set_value(GSL_RST_PORT, 0);
	mdelay(20);
	//gpio_direction_output(GSL_RST_PORT, 1);
	gpio_set_value(GSL_RST_PORT, 1);
	mdelay(100);
	//	
}


static void gsl_ts_shutdown_low(void)
{
	//gpio_direction_output(GSL_RST_PORT, 0);
	gpio_set_value(GSL_RST_PORT, 0);
}

static void gsl_ts_shutdown_high(void)
{
	//gpio_direction_output(GSL_RST_PORT, 1);
	gpio_set_value(GSL_RST_PORT, 1);
}

static int gsl_read_interface(struct i2c_client *client, u8 reg, u8 *buf, u32 num)
{

	int err = 0;
	u8 temp = reg;
	err = gpio_get_value(GSL_RST_PORT);
	//printk("gsl_read_interface,rst value = %d\n",err);
	mutex_lock(gsl_i2c_lock);
	if(temp < 0x80)
	{
		temp = (temp+8)&0x5c;
		i2c_master_send(client,&temp,1);
		err = i2c_master_recv(client,&buf[0],4);

		temp = reg;
		i2c_master_send(client,&temp,1);
		err = i2c_master_recv(client,&buf[0],4);
	}
	i2c_master_send(client,&reg,1);
	err = i2c_master_recv(client,&buf[0],num);
	//printk("gsl_read_interface,ll = %d\n",err);
	mutex_unlock(gsl_i2c_lock);
	return (err==num)?1:-1;

}

static int gsl_write_interface(struct i2c_client *client, const u8 reg, u8 *buf, u32 num)
{
	int err;
	u8 tmp_buf[num+1];
	tmp_buf[0] = reg;
	memcpy(tmp_buf + 1, buf, num);

	mutex_lock(gsl_i2c_lock);
	err = i2c_master_send(client, tmp_buf, num+1);
	mutex_unlock(gsl_i2c_lock);
	
	return (err==(num+1))?1:-1;
}


static int gsl_ts_write(struct i2c_client *client, u8 addr, u8 *pdata, u32 datalen)
{
	int ret = 0;
	if (datalen > 125)
	{
		printk("%s too big datalen = %d!\n", __func__, datalen);
		return -1;
	}
	
	ret = gsl_write_interface(client,addr,pdata,datalen);
	return ret;
}
static int gsl_ts_read(struct i2c_client *client, u8 addr, u8 *pdata, unsigned int datalen)
{
	int ret = 0;

	if (datalen > 126)
	{
		printk("%s too big datalen = %d!\n", __func__, datalen);
		return -1;
	}
	ret = gsl_read_interface(client,addr,pdata,datalen);
	return ret;
}

#ifdef HIGH_SPEED_I2C
static void 
gsl_load_fw(struct i2c_client *client,const struct fw_data *GSL_DOWNLOAD_DATA,int data_len)
{
	u8 buf[DMA_TRANS_LEN*4] = {0};
	u8 send_flag = 1;
	u8 addr=0;
	u32 source_line = 0;
	u32 source_len = data_len;//ARRAY_SIZE(GSL_DOWNLOAD_DATA);

	//print_info("=============gsl_load_fw start==============\n");

	for (source_line = 0; source_line < source_len; source_line++) 
	{
		/* init page trans, set the page val */
		if (0xf0 == GSL_DOWNLOAD_DATA[source_line].offset)
		{
			memcpy(buf,&GSL_DOWNLOAD_DATA[source_line].val,4);	
			gsl_write_interface(client, 0xf0, buf, 4);
			send_flag = 1;
		}
		else 
		{
			if (1 == send_flag % (DMA_TRANS_LEN < 0x20 ? DMA_TRANS_LEN : 0x20))
	    			addr = (u8)GSL_DOWNLOAD_DATA[source_line].offset;

			memcpy((buf+send_flag*4 -4),&GSL_DOWNLOAD_DATA[source_line].val,4);	

			if (0 == send_flag % (DMA_TRANS_LEN < 0x20 ? DMA_TRANS_LEN : 0x20)) 
			{
	    		gsl_write_interface(client, addr, buf, DMA_TRANS_LEN * 4);
				send_flag = 0;
			}

			send_flag++;
		}
	}

	//print_info("=============gsl_load_fw end==============\n");

}
#else 
static void 
gsl_load_fw(struct i2c_client *client,const struct fw_data *GSL_DOWNLOAD_DATA,int data_len)
{
	u8 buf[4] = {0};
	//u8 send_flag = 1;
	u8 addr=0;
	u32 source_line = 0;
	u32 source_len = data_len;//ARRAY_SIZE(GSL_DOWNLOAD_DATA);

	//print_info("=============gsl_load_fw start==============\n");

	for (source_line = 0; source_line < source_len; source_line++) 
	{
		/* init page trans, set the page val */
    		addr = (u8)GSL_DOWNLOAD_DATA[source_line].offset;
			memcpy(buf,&GSL_DOWNLOAD_DATA[source_line].val,4);
    		gsl_write_interface(client, addr, buf, 4);	
	}
	//print_info("=============gsl_load_fw end==============\n");
}
#endif
static int gsl_test_i2c(struct i2c_client *client)
{
	u8 read_buf[4]= {0};
	int ret;
	int i;
	for(i=0;i<5;i++){
		ret = gsl_read_interface(client,0xfc,read_buf,4);
		if(ret > 0){
			break;
		}
		msleep(20);
	}
	return (ret<0)?-1:1;
}
static void gsl_start_core(struct i2c_client *client)
{
	u8 tmp = 0x00;
	gsl_ts_write(client, 0xe0, &tmp, 1);
	msleep(10);	
#ifdef GSL_NOID_VERSION
	gsl_DataInit(gsl_cfg_table[gsl_cfg_index].data_id);
#endif	
}

static void gsl_reset_core(struct i2c_client *client)
{
	u8 tmp = 0x88;
	u8 buf[4] = {0x00};
	
	gsl_ts_write(client, 0xe0, &tmp, sizeof(tmp));
	msleep(20);
	tmp = 0x04;
	gsl_ts_write(client, 0xe4, &tmp, sizeof(tmp));
	msleep(10);
	gsl_ts_write(client, 0xbc, buf, sizeof(buf));
	msleep(10);
}

static void gsl_clear_reg(struct i2c_client *client)
{
	u8 write_buf[4]	= {0};

	write_buf[0] = 0x88;
	gsl_ts_write(client, 0xe0, &write_buf[0], 1); 	
	msleep(20);
	write_buf[0] = 0x03;
	gsl_ts_write(client, 0x80, &write_buf[0], 1); 	
	msleep(5);
	write_buf[0] = 0x04;
	gsl_ts_write(client, 0xe4, &write_buf[0], 1); 	
	msleep(5);
	write_buf[0] = 0x00;
	gsl_ts_write(client, 0xe0, &write_buf[0], 1); 	
	msleep(20);

}
static void gsl_sw_init(struct i2c_client *client)
{
	struct gsl_ts_data *ts = i2c_get_clientdata(client);
	int rc;
	if(ts->sw_init_flag == 1)
		return;
	ts->sw_init_flag = 1;
	disable_irq(client->irq);
	gsl_ts_shutdown_low();	
	msleep(20); 	
	gsl_ts_shutdown_high();	
	msleep(20); 		
	rc = gsl_test_i2c(client);
	if(rc < 0)
	{
		ts->sw_init_flag = 0;
		printk("------gsl_ts test_i2c error------\n");
		enable_irq(client->irq);
		return;
	}

	gsl_clear_reg(client);
	gsl_reset_core(client);
	gsl_load_fw(client,gsl_cfg_table[gsl_cfg_index].fw,
		gsl_cfg_table[gsl_cfg_index].fw_size);
	gsl_start_core(client);
	gsl_reset_core(client);
	gsl_start_core(client);
	ts->sw_init_flag = 0;
	enable_irq(client->irq);
}

static void check_mem_data(struct i2c_client *client)
{

	u8 read_buf[4]  = {0};
	
	msleep(30);
	gsl_ts_read(client,0xb0, read_buf, sizeof(read_buf));
	if (read_buf[3] != 0x5a || read_buf[2] != 0x5a 
		|| read_buf[1] != 0x5a || read_buf[0] != 0x5a)
	{
		//printk("#########check mem read 0xb0 = %x %x %x %x #########\n", 
			//read_buf[3], read_buf[2], read_buf[1], read_buf[0]);
		gsl_sw_init(client);
	}
}
#ifdef TPD_PROC_DEBUG
#ifdef GSL_APPLICATION
static int gsl_read_MorePage(struct i2c_client *client,u32 addr,u8 *buf,u32 num)
{
	u8 tmp_buf[4];
	int i;
	int err;
	u32 temp1=addr;
	for(i=0;i<num/128;i++){
		temp1 = addr+i*0x80;
		tmp_buf[3]=(u8)((temp1/0x80)>>24);
		tmp_buf[2]=(u8)((temp1/0x80)>>16);
		tmp_buf[1]=(u8)((temp1/0x80)>>8);
		tmp_buf[0]=(u8)((temp1/0x80));
		gsl_write_interface(client,0xf0,tmp_buf,4);
		err = gsl_read_interface(client,temp1%0x80,&buf[i*128],128);
	}
	if(i*128 < num){
		temp1=addr+i*0x80;
		tmp_buf[3]=(u8)((temp1/0x80)>>24);
		tmp_buf[2]=(u8)((temp1/0x80)>>16);
		tmp_buf[1]=(u8)((temp1/0x80)>>8);
		tmp_buf[0]=(u8)((temp1/0x80));
		gsl_write_interface(client,0xf0,tmp_buf,4);
		err=gsl_read_interface(client,temp1%0x80,&buf[i*128],(num-i*0x80));
	}
	return err;
}
#endif
static int char_to_int(char ch)
{
	if(ch>='0' && ch<='9')
		return (ch-'0');
	else
		return (ch-'a'+10);
}
//
static int gsl_config_read_proc(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	char *ptr = page;
	char temp_data[5] = {0};
	unsigned int tmp=0;
#ifdef GSL_APPLICATION
	unsigned int temp1=0;
	char *buf;
#endif
	if('v'==gsl_read[0]&&'s'==gsl_read[1])
	{
#ifdef GSL_NOID_VERSION 
		tmp=gsl_version_id();
#else 
		tmp=0x20121215;
#endif
		ptr += sprintf(ptr,"version:%x\n",tmp);
	}
	else if('r'==gsl_read[0]&&'e'==gsl_read[1])
	{
		if('i'==gsl_read[3])
		{
#ifdef GSL_NOID_VERSION 
			tmp=(gsl_data_proc[5]<<8) | gsl_data_proc[4];
			ptr +=sprintf(ptr,"gsl_config_data_id[%d] = ",tmp);
			if(tmp>=0&&tmp<gsl_cfg_table[gsl_cfg_index].data_size)
				ptr +=sprintf(ptr,"%d\n",gsl_cfg_table[gsl_cfg_index].data_id[tmp]); 
#endif
		}
		else 
		{
			gsl_write_interface(gsl_client,0xf0,&gsl_data_proc[4],4);
			gsl_read_interface(gsl_client,gsl_data_proc[0],temp_data,4);
			ptr +=sprintf(ptr,"offset : {0x%02x,0x",gsl_data_proc[0]);
			ptr +=sprintf(ptr,"%02x",temp_data[3]);
			ptr +=sprintf(ptr,"%02x",temp_data[2]);
			ptr +=sprintf(ptr,"%02x",temp_data[1]);
			ptr +=sprintf(ptr,"%02x};\n",temp_data[0]);
		}
	}
#ifdef GSL_APPLICATION
	else if('a'==gsl_read[0]&&'p'==gsl_read[1]){
		tmp = (unsigned int)(((gsl_data_proc[2]<<8)|gsl_data_proc[1])&0xffff);
		buf=kzalloc(tmp,GFP_KERNEL);
		if(buf==NULL)
			return -1;
		if(3==gsl_data_proc[0]){
			gsl_read_interface(gsl_client,gsl_data_proc[3],buf,tmp);
			memcpy(page,buf,tmp);
		}else if(4==gsl_data_proc[0]){
			temp1=((gsl_data_proc[6]<<24)|(gsl_data_proc[5]<<16)|
				(gsl_data_proc[4]<<8)|gsl_data_proc[3]);
			gsl_read_MorePage(gsl_client,temp1,buf,tmp);
			memcpy(page,buf,tmp);
		}
		kfree(buf);
		*eof=1;
		return tmp;
	}
#endif
	*eof = 1;
	return (ptr - page);
}
static int gsl_config_write_proc(struct file *file, const char *buffer, unsigned long count, void *data)
{
	u8 buf[8] = {0};
	char temp_buf[CONFIG_LEN];
	char *path_buf;
	int tmp = 0;
	int tmp1 = 0;
	//print_info("[tp-gsl][%s] \n",__func__);
	if(count > 512)
	{
		print_info("size not match [%d:%ld]\n", CONFIG_LEN, count);
        	return -EFAULT;
	}
	path_buf=kzalloc(count,GFP_KERNEL);
	if(!path_buf)
	{
		printk("alloc path_buf memory error \n");
		return -1;
	}	
	if(copy_from_user(path_buf, buffer, count))
	{
		print_info("copy from user fail\n");
		goto exit_write_proc_out;
	}
	memcpy(temp_buf,path_buf,(count<CONFIG_LEN?count:CONFIG_LEN));
	//print_info("[tp-gsl][%s][%s]\n",__func__,temp_buf);
#ifdef GSL_APPLICATION
	if('a'!=temp_buf[0]||'p'!=temp_buf[1]){
#endif
	buf[3]=char_to_int(temp_buf[14])<<4 | char_to_int(temp_buf[15]);	
	buf[2]=char_to_int(temp_buf[16])<<4 | char_to_int(temp_buf[17]);
	buf[1]=char_to_int(temp_buf[18])<<4 | char_to_int(temp_buf[19]);
	buf[0]=char_to_int(temp_buf[20])<<4 | char_to_int(temp_buf[21]);
	
	buf[7]=char_to_int(temp_buf[5])<<4 | char_to_int(temp_buf[6]);
	buf[6]=char_to_int(temp_buf[7])<<4 | char_to_int(temp_buf[8]);
	buf[5]=char_to_int(temp_buf[9])<<4 | char_to_int(temp_buf[10]);
	buf[4]=char_to_int(temp_buf[11])<<4 | char_to_int(temp_buf[12]);
#ifdef GSL_APPLICATION
	}
#endif
	if('v'==temp_buf[0]&& 's'==temp_buf[1])//version //vs
	{
		memcpy(gsl_read,temp_buf,4);
		//printk("gsl version\n");
	}
	else if('s'==temp_buf[0]&& 't'==temp_buf[1])//start //st
	{
	#ifdef GSL_MONITOR
		cancel_delayed_work_sync(&gsl_monitor_work);
	#endif
		gsl_proc_flag = 1;
		gsl_reset_core(gsl_client);
	}
	else if('e'==temp_buf[0]&&'n'==temp_buf[1])//end //en
	{
		msleep(20);
		gsl_reset_core(gsl_client);
		gsl_start_core(gsl_client);
		gsl_proc_flag = 0;
	}
	else if('r'==temp_buf[0]&&'e'==temp_buf[1])//read buf //
	{
		memcpy(gsl_read,temp_buf,4);
		memcpy(gsl_data_proc,buf,8);
	}
	else if('w'==temp_buf[0]&&'r'==temp_buf[1])//write buf
	{
		gsl_write_interface(gsl_client,buf[4],buf,4);
	}
	
#ifdef GSL_NOID_VERSION
	else if('i'==temp_buf[0]&&'d'==temp_buf[1])//write id config //
	{
		tmp1=(buf[7]<<24)|(buf[6]<<16)|(buf[5]<<8)|buf[4];
		tmp=(buf[3]<<24)|(buf[2]<<16)|(buf[1]<<8)|buf[0];
		if(tmp1>=0 && tmp1<gsl_cfg_table[gsl_cfg_index].data_size)
		{
			gsl_cfg_table[gsl_cfg_index].data_id[tmp1] = tmp;
		}
	}
#endif
#ifdef GSL_APPLICATION
	else if('a'==temp_buf[0]&&'p'==temp_buf[1]){
		if(1==path_buf[3]){
			tmp=((path_buf[5]<<8)|path_buf[4]);
			gsl_write_interface(gsl_client,path_buf[6],&path_buf[10],tmp);
		}else if(2==path_buf[3]){
			tmp = ((path_buf[5]<<8)|path_buf[4]);
			tmp1=((path_buf[9]<<24)|(path_buf[8]<<16)|(path_buf[7]<<8)
				|path_buf[6]);
			buf[0]=(char)((tmp1/0x80)&0xff);
			buf[1]=(char)(((tmp1/0x80)>>8)&0xff);
			buf[2]=(char)(((tmp1/0x80)>>16)&0xff);
			buf[3]=(char)(((tmp1/0x80)>>24)&0xff);
			buf[4]=(char)(tmp1%0x80);
			gsl_write_interface(gsl_client,0xf0,buf,4);
			gsl_write_interface(gsl_client,buf[4],&path_buf[10],tmp);
		}else if(3==path_buf[3]||4==path_buf[3]){
			memcpy(gsl_read,temp_buf,4);
			memcpy(gsl_data_proc,&path_buf[3],7);
		}
	}
#endif
	else if('s'==temp_buf[0]&&'s'==temp_buf[1]){
		check_mem_data(gsl_client);
	}

exit_write_proc_out:
	kfree(path_buf);
	return count;
}
#endif

//
#ifdef GSL_HAVE_TOUCH_KEY
static int gsl_report_key(struct gsl_ts_data *ts,int x,int y)
{
	int i;
	for(i=0;i<GSL_KEY_NUM;i++)
	{
		if(x > gsl_key_data[i].x_min &&
			x < gsl_key_data[i].x_max &&
			y > gsl_key_data[i].y_min &&
			y < gsl_key_data[i].y_max)
		{
			ts->key_state = i+1;
			input_report_key(ts->input,gsl_key_data[i].key,1);
			input_sync(ts->input);
			return 1;
		}
	}
	return 0;
}
#endif
static void gsl_report_point_up(struct gsl_ts_data *ts, u32 x, u32 y, u32 id)
{
#ifdef REPORT_DATA_ANDROID_4_0
	input_mt_slot(ts->input, id);
//	input_report_abs(ts->input, ABS_MT_TRACKING_ID, -1);
	input_mt_report_slot_state(ts->input, MT_TOOL_FINGER, false);
#else
	input_report_key(ts->input, BTN_TOUCH, 0);
//	input_report_abs(ts->input, ABS_MT_TOUCH_MAJOR, 0);
//	input_report_abs(ts->input, ABS_MT_WIDTH_MAJOR, 0);
	input_mt_sync(ts->input);
	
#endif
	
}
static void gsl_report_point_down(struct gsl_ts_data *ts, u32 x, u32 y, u32 id)
{
	//print_info("[report point]id=%d,x=%d,y=%d;\n",id,x,y);

	#ifdef GSL_HAVE_TOUCH_KEY
	if(x > SCREEN_MAX_X || y > SCREEN_MAX_Y)
	{
		gsl_report_key(ts,x,y);
		return;
	}
	#endif
#ifdef REPORT_DATA_ANDROID_4_0
	input_mt_slot(ts->input, id);		
	input_report_abs(ts->input, ABS_MT_POSITION_X, x);//x
	input_report_abs(ts->input, ABS_MT_POSITION_Y, y);	//y
	input_report_abs(ts->input, ABS_MT_WIDTH_MAJOR, 1);
	input_report_abs(ts->input, ABS_MT_TOUCH_MAJOR, 10);
	input_report_abs(ts->input, ABS_MT_TRACKING_ID, id);
#else
	input_report_abs(ts->input, ABS_MT_POSITION_X,x);
	input_report_abs(ts->input, ABS_MT_POSITION_Y, y);
	input_report_abs(ts->input, ABS_MT_WIDTH_MAJOR, 1);
	input_report_abs(ts->input, ABS_MT_TOUCH_MAJOR, 10);
	input_report_abs(ts->input, ABS_MT_TRACKING_ID, id);
	input_mt_sync(ts->input);
#endif
}

static void gsl_report_work(struct work_struct *work)
{
	static int up_flag=0;
	int rc,i;
//	u16 x,y;
	u8 touches;
	//u8 id,touches;
	u8 buf[44] = {0};
	u32 cur_point_state = 0;
	u32 cur_point_up = 0;
	int tmp1=0;
	struct gsl_ts_data *ts = container_of(work, struct gsl_ts_data,work);
	struct gsl_touch_info *cinfo = ts->cinfo;
	
	if(ts->sw_init_flag == 1){
		goto schedule;
	}
#ifdef TPD_PROC_DEBUG
	if(gsl_proc_flag == 1){
		goto schedule;
	}
#endif

	/* read data from DATA_REG */
	rc = gsl_ts_read(ts->client, 0x80, buf, 44);
	if (rc < 0){
		dev_err(&ts->client->dev, "[gsl] I2C read failed\n");
		goto schedule;
	}

	if (buf[0] == 0xff) {
		goto schedule;
	}
	memset(cinfo,0,sizeof(struct gsl_touch_info));
	cinfo->finger_num = buf[0];
	//print_info("pre finger_num= %d\n",cinfo->finger_num);
	for(i=0;i<(cinfo->finger_num>10 ? 10:cinfo->finger_num);i++)
	{
		cinfo->y[i] = (buf[i*4+4] | ((buf[i*4+5])<<8));
		cinfo->x[i] = (buf[i*4+6] | ((buf[i*4+7] & 0x0f)<<8));
		cinfo->id[i] = buf[i*4+7] >> 4;
		//print_info("[kernel point]  x = %d y = %d \n",cinfo->x[i],cinfo->y[i]);
	}
	
#ifdef GSL_NOID_VERSION 
	cinfo->finger_num = (buf[3]<<24)|(buf[2]<<16)|(buf[1]<<8)|(buf[0]);
	gsl_alg_id_main(cinfo);
	tmp1=gsl_mask_tiaoping();
	//print_info("[tp-gsl] tmp1=%x\n",tmp1);
	if(tmp1>0&&tmp1<0xffffffff)
	{
		buf[0]=0xa;
		buf[1]=0;
		buf[2]=0;
		buf[3]=0;
		gsl_write_interface(ts->client,0xf0,buf,4);
		buf[0]=(u8)(tmp1 & 0xff);
		buf[1]=(u8)((tmp1>>8) & 0xff);
		buf[2]=(u8)((tmp1>>16) & 0xff);
		buf[3]=(u8)((tmp1>>24) & 0xff);
		print_info("tmp1=%08x,buf[0]=%02x,buf[1]=%02x,buf[2]=%02x,buf[3]=%02x\n",
			tmp1,buf[0],buf[1],buf[2],buf[3]);
		gsl_write_interface(ts->client,0x8,buf,4);
	}
#endif
	touches = cinfo->finger_num;
	for(i = 0; i < (touches > MAX_FINGERS ? MAX_FINGERS : touches); i ++)
	{
		up_flag = 0;
		if(1 <= cinfo->id[i] && cinfo->id[i] <= MAX_CONTACTS)
		{
			cur_point_state |= 0x1<<(cinfo->id[i]-1);
			gsl_report_point_down(ts, cinfo->x[i], cinfo->y[i], cinfo->id[i]-1);		
		}
	}
	cur_point_up = (cur_point_state & ts->point_state) ^ ts->point_state;
	ts->point_state = cur_point_state;
#ifdef REPORT_DATA_ANDROID_4_0
	for(i = 0; i < MAX_CONTACTS; i ++)
	{
		if(cur_point_up & (0x1<<i)){
			gsl_report_point_up(ts,0,0,i);
		}
	}
#endif
	if(0 == touches)
	{
		if(up_flag == 1)
			goto schedule;
		up_flag = 1;
	#ifdef GSL_HAVE_TOUCH_KEY
		if(ts->key_state!=0){
			input_report_key(ts->input, BTN_TOUCH, 0);
			input_report_key(ts->input, gsl_key_data[ts->key_state-1].key, 0);
			input_sync(ts->input);
			ts->key_state = 0;
		}
	#endif			
	#ifndef REPORT_DATA_ANDROID_4_0
		gsl_report_point_up(ts,0,0,0);	
	#endif
	}
	input_sync(ts->input);
schedule:
	enable_irq(ts->irq);

}

#ifdef GSL_MONITOR
static void gsl_monitor_worker(struct work_struct *work)
{
	struct gsl_ts_data *ts = i2c_get_clientdata(gsl_client);
	static int i2c_lock_flag = 0;
	char read_buf[4]  = {0};
	char init_chip_flag = 0;
	int i,flag;
	//print_info("----------------gsl_monitor_worker-----------------\n");	

	if(i2c_lock_flag != 0)
		return;
	else
		i2c_lock_flag = 1;

	gsl_read_interface(gsl_client, 0xb4, read_buf, 4);	
	int_2nd[3] = int_1st[3];
	int_2nd[2] = int_1st[2];
	int_2nd[1] = int_1st[1];
	int_2nd[0] = int_1st[0];
	int_1st[3] = read_buf[3];
	int_1st[2] = read_buf[2];
	int_1st[1] = read_buf[1];
	int_1st[0] = read_buf[0];

	if(int_1st[3] == int_2nd[3] && int_1st[2] == int_2nd[2] &&
		int_1st[1] == int_2nd[1] && int_1st[0] == int_2nd[0])
	{
		//printk("======int_1st: %x %x %x %x , int_2nd: %x %x %x %x ======\n",
			//int_1st[3], int_1st[2], int_1st[1], int_1st[0], 
			//int_2nd[3], int_2nd[2],int_2nd[1],int_2nd[0]);
		init_chip_flag = 1;
		goto queue_monitor_work;
	}
	/*check 0xb0 register,check firmware if ok*/
	for(i=0;i<5;i++){
		gsl_read_interface(gsl_client, 0xb0, read_buf, 4);
		if(read_buf[3] != 0x5a || read_buf[2] != 0x5a || 
			read_buf[1] != 0x5a || read_buf[0] != 0x5a){
			//printk("gsl_monitor_worker 0xb0 = {0x%02x%02x%02x%02x};\n",
				//read_buf[3],read_buf[2],read_buf[1],read_buf[0]);
			flag = 1;
		}else{
			flag = 0;
			break;
		}

	}
	if(flag == 1){
		init_chip_flag = 1;
		goto queue_monitor_work;
	}
	
	/*check 0xbc register,check dac if normal*/
	for(i=0;i<5;i++){
		gsl_read_interface(gsl_client, 0xbc, read_buf, 4);
		if(read_buf[3] != 0 || read_buf[2] != 0 || 
			read_buf[1] != 0 || read_buf[0] != 0){
			flag = 1;
		}else{
			flag = 0;
			break;
		}
	}
	if(flag == 1){
		gsl_reset_core(gsl_client);
		gsl_start_core(gsl_client);
		init_chip_flag = 0;
	}
queue_monitor_work:
	if(init_chip_flag){
		gsl_sw_init(gsl_client);
		memset(int_1st,0xff,sizeof(int_1st));
	}
	
	if(ts->is_suspend==0){
		queue_delayed_work(gsl_monitor_workqueue, &gsl_monitor_work, 200);
	}
	i2c_lock_flag = 0;
}
#endif

static irqreturn_t gsl_ts_irq(int irq, void *dev_id)
{
	struct gsl_ts_data *ts = dev_id;
	
	//print_info("========gsl_ts Interrupt=========\n");				 
	
	disable_irq_nosync(ts->irq);
	if (!work_pending(&ts->work)) {
		queue_work(ts->wq, &ts->work);
	}
	
	return IRQ_HANDLED;
}

static int gsl_request_input_dev(struct gsl_ts_data *ts)
{
	struct input_dev *input_device;
	int rc;
#ifdef GSL_HAVE_TOUCH_KEY
	int i;
#endif
	/*allocate input device*/
	//print_info("==input_allocate_device=\n");
	input_device = input_allocate_device();
	if (!input_device) {
		rc = -ENOMEM;
		return rc;
	}
	ts->input = input_device;
	input_device->name = GSL_TS_NAME;
	input_device->id.bustype = BUS_I2C;
	input_device->dev.parent = &ts->client->dev;
	input_set_drvdata(input_device, ts);
	
#ifdef REPORT_DATA_ANDROID_4_0
	__set_bit(EV_ABS, input_device->evbit);
	__set_bit(EV_KEY, input_device->evbit);
	__set_bit(EV_REP, input_device->evbit);
	__set_bit(INPUT_PROP_DIRECT, input_device->propbit);
	input_mt_init_slots(input_device, 255);
	//input_mt_init_slots(input_device, (MAX_CONTACTS+1));
#else
//	input_set_abs_params(input_device,ABS_MT_TRACKING_ID, 0, (MAX_CONTACTS+1), 0, 0);
	set_bit(EV_ABS, input_device->evbit);
	set_bit(EV_KEY, input_device->evbit);
	__set_bit(INPUT_PROP_DIRECT, input_device->propbit);
	__set_bit(BTN_TOUCH, input_device->keybit);
//	input_device->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);
#endif
	__set_bit(EV_SYN, input_device->evbit);
	
#ifdef GSL_HAVE_TOUCH_KEY
	for(i=0;i<GSL_KEY_NUM;i++)
	{
		set_bit(gsl_key_data[i].key, input_device->keybit);
	}
#endif
	set_bit(ABS_MT_TOUCH_MAJOR, input_device->absbit);
	set_bit(ABS_MT_POSITION_X, input_device->absbit);
	set_bit(ABS_MT_POSITION_Y, input_device->absbit);
	set_bit(ABS_MT_WIDTH_MAJOR, input_device->absbit);
	
	input_set_abs_params(input_device,ABS_MT_POSITION_X, 0, SCREEN_MAX_X, 0, 0);
	input_set_abs_params(input_device,ABS_MT_POSITION_Y, 0, SCREEN_MAX_Y, 0, 0);
	input_set_abs_params(input_device,ABS_MT_TRACKING_ID, 0, 255, 0, 0);
	input_set_abs_params(input_device,ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(input_device,ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);

	/*register input device*/
	rc = input_register_device(input_device);
	if (rc) {
		goto err_register_input_device_fail;
	}
	return 0;
err_register_input_device_fail:
	input_free_device(input_device);			
	return rc;
}


static int gsl_ts_suspend(struct device *dev)
{
	u32 tmp;
	struct gsl_ts_data *ts = dev_get_drvdata(dev);

  	//printk("I'am in gsl_ts_suspend() start\n");
	ts->is_suspend = 1;
#ifdef GSL_MONITOR	
	//printk( "gsl_ts_suspend () : cancel gsl_monitor_work\n");
	cancel_delayed_work_sync(&gsl_monitor_work);
#endif

#ifdef GSL_NOID_VERSION
	tmp = gsl_version_id();	
	//printk("[tp-gsl]the version of alg_id:%08x\n",tmp);
#endif
	disable_irq_nosync(ts->irq);
	gsl_ts_shutdown_low();

	return 0;
}

static int gsl_ts_resume(struct device *dev)
{	
	struct gsl_ts_data *ts = dev_get_drvdata(dev);
  	
	//printk("I'am in gsl_ts_resume() start\n");

	gsl_ts_shutdown_high();
	msleep(20);	
	gsl_reset_core(gsl_client);
	gsl_start_core(gsl_client);
	check_mem_data(gsl_client);	

#ifdef GSL_MONITOR
	//printk( "gsl_ts_resume () : queue gsl_monitor_work\n");
	queue_delayed_work(gsl_monitor_workqueue, &gsl_monitor_work, 300);
#endif
	enable_irq(ts->irq);
	ts->is_suspend = 0;
	return 0;

}
#if defined(CONFIG_FB)
static int fb_notifier_callback(struct notifier_block *self,
				 unsigned long event, void *data)
{
	struct fb_event *evdata = data;
	int *blank;
	//struct gsl_ts_data *ts = i2c_get_clientdata(gsl_client);
	struct gsl_ts_data *ts =
		container_of(self, struct gsl_ts_data, fb_notif);
	//printk("tp-gsl %s 11\n",__func__);
	if (evdata && evdata->data && event == FB_EVENT_BLANK &&
			ts && ts->client) {
		blank = evdata->data;
		if (*blank == FB_BLANK_UNBLANK)
			gsl_ts_resume(&ts->client->dev);
		else if (*blank == FB_BLANK_POWERDOWN)
			gsl_ts_suspend(&ts->client->dev);
		else
			printk("tp-gsl %s 22\n",__func__);
	}
	return 0;
}

#elif defined(CONFIG_HAS_EARLYSUSPEND)
static void gsl_ts_early_suspend(struct early_suspend *h)
{
	struct gsl_ts_data *ts = container_of(h, struct gsl_ts_data, early_suspend);
	//printk("[GSL_TS] Enter %s\n", __func__);
	gsl_ts_suspend(&ts->client->dev);
}

static void gsl_ts_late_resume(struct early_suspend *h)
{
	struct gsl_ts_data *ts = container_of(h, struct gsl_ts_data, early_suspend);
	//printk("[GSL_TS] Enter %s\n", __func__);
	gsl_ts_resume(&ts->client->dev);
}
#endif
#if (!defined(CONFIG_FB) && !defined(CONFIG_HAS_EARLYSUSPEND))
static const struct dev_pm_ops gsl_ts_dev_pm_ops = {
	.suspend = gsl_ts_suspend,
	.resume = gsl_ts_resume,
};
#else
static const struct dev_pm_ops gsl_ts_dev_pm_ops = {
};
#endif


#if 0
static int gsl_ts_get_dt_coords(struct device *dev, char *name,
				struct gsl_ts_platform_data *pdata)
{
	u32 coords[GSL_COORDS_ARR_SIZE];
	struct property *prop;
	struct device_node *np = dev->of_node;
	int coords_size, rc;

	prop = of_find_property(np, name, NULL);
	if (!prop)
		return -EINVAL;
	if (!prop->value)
		return -ENODATA;

	coords_size = prop->length / sizeof(u32);
	if (coords_size != GSL_COORDS_ARR_SIZE) {
		dev_err(dev, "invalid %s\n", name);
		return -EINVAL;
	}

	rc = of_property_read_u32_array(np, name, coords, coords_size);
	if (rc && (rc != -EINVAL)) {
		dev_err(dev, "Unable to read %s\n", name);
		return rc;
	}

	if (!strcmp(name, "gslX,panel-coords")) {
		pdata->panel_minx = coords[0];
		pdata->panel_miny = coords[1];
		pdata->panel_maxx = coords[2];
		pdata->panel_maxy = coords[3];
	} else if (!strcmp(name, "gslX,display-coords")) {
		pdata->disp_minx = coords[0];
		pdata->disp_miny = coords[1];
		pdata->disp_maxx= coords[2];
		pdata->disp_maxy= coords[3];
	} else {
		dev_err(dev, "unsupported property %s\n", name);
		return -EINVAL;
	}

	return 0;
}
#endif
static int gsl_ts_parse_dt(struct device *dev,
			struct gsl_ts_platform_data *pdata)
{
	int rc;
	struct device_node *np = dev->of_node;
	struct property *prop;
	u32 temp_val, num_buttons;
	u32 button_map[MAX_BUTTONS];
    //print_info("gsl_ts_parse_dt,==kzalloc1=\n");
	#if 0
	rc = gsl_ts_get_dt_coords(dev, "gslX,panel-coords", pdata);
	if (rc && (rc != -EINVAL))
		return rc;
    print_info("gsl_ts_parse_dt,==kzalloc2=\n");
	rc = gsl_ts_get_dt_coords(dev, "gslX,display-coords", pdata);
	if (rc)
		return rc;
	print_info("gsl_ts_parse_dt,==kzalloc3=\n");
	pdata->i2c_pull_up = of_property_read_bool(np,
						"gslX,i2c-pull-up");
	#endif

	//pdata->no_force_update = of_property_read_bool(np,
	//					"gslX,no-force-update");
	/* reset, irq gpio info */


	pdata->reset_gpio = of_get_named_gpio_flags(np, "gslX,reset-gpio",
				0, &pdata->reset_flags);
	if (pdata->reset_gpio < 0)
		return pdata->reset_gpio;
	//print_info("gsl_ts_parse_dt,==kzalloc2=\n");

	pdata->irq_gpio = of_get_named_gpio_flags(np, "gslX,irq-gpio",
				0, &pdata->irq_flags);
	if (pdata->irq_gpio < 0)
		return pdata->irq_gpio;
	//print_info("gsl_ts_parse_dt,==kzalloc3=\n");

	//rc = of_property_read_u32(np, "gslX,family-id", &temp_val);
	//if (!rc)
		//pdata->family_id = temp_val;
	//else
		//return rc;




	prop = of_find_property(np, "gslX,button-map", NULL);
	if (prop) {
		num_buttons = prop->length / sizeof(temp_val);
		if (num_buttons > MAX_BUTTONS)
			return -EINVAL;

		rc = of_property_read_u32_array(np,
			"gslX,button-map", button_map,
			num_buttons);
		if (rc) {
			dev_err(dev, "Unable to read key codes\n");
			return rc;
		}
	}

	return 0;
}
#if 0
static void _HalTscrHWReset(void){	
	gpio_direction_output(GSL_RST_PORT, 1);	    
	gpio_set_value(GSL_RST_PORT, 1);	    
	gpio_set_value(GSL_RST_PORT, 0);	    
	mdelay(10);  /* Note that the RST must be in LOW 10ms at least */	    
	gpio_set_value(GSL_RST_PORT, 1);	    /* Enable the interrupt service thread/routine for INT after 50ms */	    
	mdelay(50);
}
#endif
#if 1
static int gsl_power_deinit(struct gsl_ts_data *ts)
{
	//regulator_put(ts->vdd);
	regulator_put(ts->vcc_i2c);
	regulator_put(ts->avdd);

	return 0;
}

static int gsl_power_init(struct gsl_ts_data *ts)
{
	int ret;

	ts->avdd = regulator_get(&ts->client->dev, "8226_l19");
	if (IS_ERR(ts->avdd)) {
		ret = PTR_ERR(ts->avdd);
		dev_info(&ts->client->dev,
			"Regulator get failed avdd ret=%d\n", ret);
	}
#if 0
	ts->vdd = regulator_get(&ts->client->dev, "vdd");
	if (IS_ERR(ts->vdd)) {
		ret = PTR_ERR(ts->vdd);
		dev_info(&ts->client->dev,
			"Regulator get failed vdd ret=%d\n", ret);
	}
#endif
	ts->vcc_i2c = regulator_get(&ts->client->dev, "8226_lvs1");/*[BUGFIX]-MOD  by TCTNB.XQJ,09/18/2013,FR-514195,GT915 tp MIATA development,i2c,only for compil*/
	if (IS_ERR(ts->vcc_i2c)) {
		ret = PTR_ERR(ts->vcc_i2c);
		dev_info(&ts->client->dev,
			"Regulator get failed vcc_i2c ret=%d\n", ret);
	}

	return 0;
}
#define GSL_VDD_LOAD_MAX_UA 10000
#define GSL_VTG_MIN_UV		2600000
#define GSL_VTG_MAX_UV		3300000
#define GSL_VDD_LOAD_MAX_UA 10000
#define GSL_I2C_VTG_MIN_UV  1800000
#define GSL_I2C_VTG_MAX_UV	1800000
#define GSL_VIO_LOAD_MAX_UA	10000
static int reg_set_optimum_mode_check(struct regulator *reg, int load_uA)
{
	return (regulator_count_voltages(reg) > 0) ?
		regulator_set_optimum_mode(reg, load_uA) : 0;
}
static int gsl_power_off(struct gsl_ts_data *ts)
{
	int ret;

	if (!IS_ERR(ts->vcc_i2c)) {
		ret = regulator_set_voltage(ts->vcc_i2c, 0,
			GSL_I2C_VTG_MAX_UV);
		if (ret < 0)
			dev_err(&ts->client->dev,
				"Regulator vcc_i2c set_vtg failed ret=%d\n",
				ret);
		ret = regulator_disable(ts->vcc_i2c);
		if (ret)
			dev_err(&ts->client->dev,
				"Regulator vcc_i2c disable failed ret=%d\n",
				ret);
	}
#if 0
	if (!IS_ERR(ts->vdd)) {
		ret = regulator_set_voltage(ts->vdd, 0, GOODIX_VTG_MAX_UV);
		if (ret < 0)
			dev_err(&ts->client->dev,
				"Regulator vdd set_vtg failed ret=%d\n", ret);
		ret = regulator_disable(ts->vdd);
		if (ret)
			dev_err(&ts->client->dev,
				"Regulator vdd disable failed ret=%d\n", ret);
	}
#endif
	if (!IS_ERR(ts->avdd)) {
		ret = regulator_disable(ts->avdd);
		if (ret)
			dev_err(&ts->client->dev,
				"Regulator avdd disable failed ret=%d\n", ret);
	}

	return 0;
}

static int gsl_power_on(struct gsl_ts_data *ts)
{
	int ret;

	if (!IS_ERR(ts->avdd)) {
		ret = reg_set_optimum_mode_check(ts->avdd,
			GSL_VDD_LOAD_MAX_UA);
		if (ret < 0) {
			dev_err(&ts->client->dev,
				"Regulator avdd set_opt failed rc=%d\n", ret);
			goto err_set_opt_avdd;
		}
		ret = regulator_enable(ts->avdd);
		if (ret) {
			dev_err(&ts->client->dev,
				"Regulator avdd enable failed ret=%d\n", ret);
			goto err_enable_avdd;
		}
	}
#if 0
	if (!IS_ERR(ts->vdd)) {
		ret = regulator_set_voltage(ts->vdd, GSL_VTG_MIN_UV,
					   GSL_VTG_MAX_UV);
		if (ret) {
			dev_err(&ts->client->dev,
				"Regulator set_vtg failed vdd ret=%d\n", ret);
			goto err_set_vtg_vdd;
		}
		ret = reg_set_optimum_mode_check(ts->vdd,
			GSL_VDD_LOAD_MAX_UA);
		if (ret < 0) {
			dev_err(&ts->client->dev,
				"Regulator vdd set_opt failed rc=%d\n", ret);
			goto err_set_opt_vdd;
		}
		ret = regulator_enable(ts->vdd);
		if (ret) {
			dev_err(&ts->client->dev,
				"Regulator vdd enable failed ret=%d\n", ret);
			goto err_enable_vdd;
		}
	}
#endif
#if 1
	if (!IS_ERR(ts->vcc_i2c)) {
	 if (regulator_count_voltages(ts->vcc_i2c) > 0) {/*[BUGFIX]-ADD  by TCTNB.XQJ,09/18/2013,FR-514195,GT915 tp MIATA development,i2c,l*/
		ret = regulator_set_voltage(ts->vcc_i2c, GSL_I2C_VTG_MIN_UV,
					   GSL_I2C_VTG_MAX_UV);
		if (ret) {
			dev_err(&ts->client->dev,
				"Regulator set_vtg failed vcc_i2c ret=%d\n",
				ret);
			goto err_set_vtg_vcc_i2c;
		}
	 }
		ret = reg_set_optimum_mode_check(ts->vcc_i2c,
			GSL_VIO_LOAD_MAX_UA);
		if (ret < 0) {
			dev_err(&ts->client->dev,
				"Regulator vcc_i2c set_opt failed rc=%d\n",
				ret);
			goto err_set_opt_vcc_i2c;
		}
		ret = regulator_enable(ts->vcc_i2c);
		if (ret) {
			dev_err(&ts->client->dev,
				"Regulator vcc_i2c enable failed ret=%d\n",
				ret);
			regulator_disable(ts->vdd);
			goto err_enable_vcc_i2c;
			}

		}
	
#endif
	return 0;
#if 1
err_enable_vcc_i2c:
err_set_opt_vcc_i2c:
	if (!IS_ERR(ts->vcc_i2c))
		regulator_set_voltage(ts->vcc_i2c, 0, GSL_I2C_VTG_MAX_UV);
err_set_vtg_vcc_i2c:
	if (!IS_ERR(ts->vdd))
		regulator_disable(ts->vdd);
#endif
//err_enable_vdd:
//err_set_opt_vdd:
	if (!IS_ERR(ts->vdd))
		regulator_set_voltage(ts->vdd, 0, GSL_VTG_MAX_UV);
//err_set_vtg_vdd:
	if (!IS_ERR(ts->avdd))
		regulator_disable(ts->avdd);
err_enable_avdd:
err_set_opt_avdd:
	return ret;
}
#endif
static int __devinit gsl_ts_probe(struct i2c_client *client, 
			const struct i2c_device_id *id)
{
	struct gsl_ts_platform_data *pdata;
	struct gsl_ts_data *ts;
	static int tp_load_ok = 0;
	
	int rc;
	//_HalTscrHWReset();
	if(1==tp_load_ok)
		return -ENODEV;
	printk("GSL_TS Enter %s\n", __func__);
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "I2C functionality not supported\n");
		return -ENODEV;
	}
    //print_info("==kzalloc1=");
	//add by wkh begin
    if (client->dev.of_node) {
	pdata = devm_kzalloc(&client->dev,
		sizeof(struct gsl_ts_platform_data), GFP_KERNEL);
	//print_info("==kzalloc11=");
	if (!pdata) {
		dev_err(&client->dev, "Failed to allocate memory\n");
		return -ENOMEM;
	}
		//print_info("==kzalloc12=");

	rc = gsl_ts_parse_dt(&client->dev, pdata);
	if (rc)
		return rc;
	}
    else{
		pdata = client->dev.platform_data;
    	}

	if (!pdata) {
		dev_err(&client->dev, "Invalid !ts.pdata\n");
		return -EINVAL;
	}
	//add by wkh end
	
	//print_info("==kzalloc2=");
	#if 1
	ts = kzalloc(sizeof(struct gsl_ts_data), GFP_KERNEL);
	if (ts==NULL){
		rc = -ENOMEM;
		return rc;
	}
	#endif

	//print_info("==kzalloc3=");
	ts->cinfo = kzalloc(sizeof(struct gsl_touch_info),GFP_KERNEL);
	if(ts->cinfo == NULL){
		rc = -ENOMEM;
		goto exit_alloc_cinfo_fail;
	}

	ts->is_suspend = 0;
	ts->sw_init_flag = 0;
	ts->point_state = 0;
#ifdef GSL_HAVE_TOUCH_KEY
	ts->key_state = 0;
#endif
	gsl_i2c_lock = &ts->i2c_lock;
	mutex_init(gsl_i2c_lock);
	ts->client = client;
	i2c_set_clientdata(client, ts);
	gsl_client = client;
	print_info("I2C addr=%x\n", client->addr);
	rc = gsl_power_init(ts);
	rc = gsl_power_on(ts);

	gsl_hw_init();

	rc = gsl_test_i2c(client);
	if(rc < 0){
		printk("------gsl_ts test_i2c error, not silead chip------\n");	
		rc = -ENODEV;
		goto exit_i2c_transfer_fail; 
	}	
	
	if(client->addr == 0x40)
	{
		gsl_cfg_index = 0;
	}
	else if(client->addr == 0x41)
	{
	    gsl_cfg_index = 1;
	}
	//printk("------gsl_ts gsl_cfg_index = %d------\n",gsl_cfg_index);
	//printk("------gsl_ts gsl_= %08x------\n",gsl_cfg_table[gsl_cfg_index].fw[33].val);
	/*request input system*/	
	rc = gsl_request_input_dev(ts);
	if(rc < 0){
		rc = -ENODEV;
		goto exit_request_input_dev_fail;	
	}

	gsl_sw_init(ts->client);
	check_mem_data(ts->client);
	
	/*init work queue*/
	INIT_WORK(&ts->work, gsl_report_work);
	ts->wq = create_singlethread_workqueue(dev_name(&client->dev));
	if (!ts->wq) {
		rc = -ESRCH;
		goto exit_create_singlethread_fail;
	}
	/*request irq */
	client->irq = __gpio_to_irq(GSL_IRQ_PORT);
	ts->irq = client->irq;
	//print_info("%s IRQ number is %d\n", client->name, client->irq);
	rc=  request_irq(client->irq, gsl_ts_irq, IRQF_TRIGGER_RISING, client->name, ts);
	if (rc < 0) {
		printk( "gsl_probe: request irq failed\n");
		goto exit_irq_request_failed;
	}
#if defined(CONFIG_FB)
		ts->fb_notif.notifier_call = fb_notifier_callback;
		rc = fb_register_client(&ts->fb_notif);
		if (rc)
			dev_err(&ts->client->dev,
				"Unable to register fb_notifier: %d\n",
				rc);
#elif defined(CONFIG_HAS_EARLYSUSPEND)
	ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	//ts->early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB + 1;
	ts->early_suspend.suspend = gsl_ts_early_suspend;
	ts->early_suspend.resume = gsl_ts_late_resume;
	register_early_suspend(&ts->early_suspend);
#endif
	/*gsl of software init*/
#ifdef TPD_PROC_DEBUG
	gsl_config_proc = create_proc_entry(GSL_CONFIG_PROC_FILE, 0666, NULL);
	if (gsl_config_proc == NULL)
	{
		print_info("create_proc_entry %s failed\n", GSL_CONFIG_PROC_FILE);
	}
	else
	{
		gsl_config_proc->read_proc = gsl_config_read_proc;
		gsl_config_proc->write_proc = gsl_config_write_proc;
	}
	gsl_proc_flag = 0;
#endif

#ifdef GSL_MONITOR
	//printk( "tpd_i2c_probe () : queue gsl_monitor_workqueue\n");
	INIT_DELAYED_WORK(&gsl_monitor_work, gsl_monitor_worker);
	gsl_monitor_workqueue = create_singlethread_workqueue("gsl_monitor_workqueue");
	queue_delayed_work(gsl_monitor_workqueue, &gsl_monitor_work, 100);
#endif
	tp_load_ok = 1;
#ifdef TOUCH_VIRTUAL_KEYS
	gsl_ts_virtual_keys_init();
#endif
	//print_info("%s: ==probe over =\n",__func__);
	return 0;
exit_irq_request_failed:
	cancel_work_sync(&ts->work);
	destroy_workqueue(ts->wq);
exit_create_singlethread_fail:
	input_unregister_device(ts->input);
	input_free_device(ts->input);
exit_request_input_dev_fail:
exit_i2c_transfer_fail:
	gsl_power_off(ts);
	gsl_power_deinit(ts);
	mutex_destroy(gsl_i2c_lock);
	gsl_i2c_lock = NULL;
	gpio_free(GSL_IRQ_PORT);
	gpio_free(GSL_RST_PORT);
	i2c_set_clientdata(client, NULL);
	kfree(ts->cinfo);
exit_alloc_cinfo_fail:
	kfree(ts);
	return rc;
}

static int __devexit gsl_ts_remove(struct i2c_client *client)
{
	struct gsl_ts_data *ts = i2c_get_clientdata(client);
	//print_info("==gslX68X_ts_remove=\n");
	
#ifdef TPD_PROC_DEBUG
	if(gsl_config_proc!=NULL)
		remove_proc_entry(GSL_CONFIG_PROC_FILE, NULL);
#endif

#ifdef GSL_MONITOR
	cancel_delayed_work_sync(&gsl_monitor_work);
	destroy_workqueue(gsl_monitor_workqueue);
#endif
#if defined(CONFIG_FB)
	if (fb_unregister_client(&ts->fb_notif))
		dev_err(&client->dev,
			"Error occurred while unregistering fb_notifier.\n");
#elif defined(CONFIG_HAS_EARLYSUSPEND)
	unregister_early_suspend(&ts->early_suspend);
#endif
	free_irq(ts->irq,ts);
	input_unregister_device(ts->input);
	input_free_device(ts->input);
//	gpio_free(GSL_RST_PORT);
	gpio_free(GSL_IRQ_PORT);
	
	cancel_work_sync(&ts->work);
	destroy_workqueue(ts->wq);
	
	gsl_power_off(ts);
	gsl_power_deinit(ts);
	
	i2c_set_clientdata(client, NULL);

	mutex_destroy(gsl_i2c_lock);
	kfree(ts->cinfo);
	kfree(ts);

	return 0;
}

static const struct i2c_device_id gsl_ts_id[] = {
	{ GSL_TS_NAME, 0 },{ }
};


MODULE_DEVICE_TABLE(i2c, gsl_ts_id);
#ifdef CONFIG_OF
static struct of_device_id gsl_match_table[] = {
	{ .compatible = "gslX,gslX680",},
	{ },
};
#else
#define gsl_match_table NULL
#endif

static struct i2c_driver gsl_ts_driver = {
	.driver	= {
		.name	= GSL_TS_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = gsl_match_table,
#if CONFIG_PM
		.pm = &gsl_ts_dev_pm_ops,
#endif
	},
#ifndef CONFIG_HAS_EARLYSUSPEND
	//.suspend	= gsl_ts_suspend,
	//.resume		= gsl_ts_resume,
#endif
	.probe		= gsl_ts_probe,
	.remove		= __devexit_p(gsl_ts_remove),
	.id_table	= gsl_ts_id,
};

#if I2C_BOARD_INFO_METHOD
static int __init gsl_ts_init(void)
{
	print_info();	
	i2c_add_driver(&gsl_ts_driver);
	return 0;
}

static void __exit gsl_ts_exit(void)
{
	print_info();
	i2c_del_driver(&gsl_ts_driver);	
}
#else
static struct i2c_board_info gsl_i2c_info = {
	.type = GSL_TS_NAME,
	.addr = GSL_TS_ADDR,
}; 
static int __init gsl_ts_init(void)
{
	int ret;
	struct i2c_client *client;
	struct i2c_adapter *adapter;
	adapter = i2c_get_adapter(GSL_I2C_ADAPTER_INDEX);
	if(!adapter)
	{
		printk("Can'n get %d adapter failed", GSL_I2C_ADAPTER_INDEX);
		return -ENODEV;
	}
	client = i2c_new_device(adapter,&gsl_i2c_info);
	if(!client)
	{
		ret = -1;
		goto err_put_adapter;
	}
	i2c_put_adapter(adapter);

	ret = i2c_add_driver(&gsl_ts_driver);
	if(ret < 0 )
	{
		goto err_add_driver;
	}
	return 0;

err_add_driver:
	kfree(client);
err_put_adapter:
	kfree(adapter);
	return ret;
}
static void __exit gsl_ts_exit(void)
{
	print_info();
	i2c_unregister_device(gsl_client);
	i2c_del_driver(&gsl_ts_driver);	
}
#endif
module_init(gsl_ts_init);
module_exit(gsl_ts_exit);

MODULE_AUTHOR("sileadinc");
MODULE_DESCRIPTION("GSL Series Driver");
MODULE_LICENSE("GPL");

