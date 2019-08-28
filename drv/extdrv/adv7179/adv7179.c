/*   extdrv/peripheral/vda/adv7179.c
 *
 *
 * Copyright (c) 2006 Hisilicon Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.
 *
 *
 * History:
 *     17-Apr-2006 create this file
 *
 */

#include <linux/string.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/miscdevice.h>
#include <linux/fcntl.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/workqueue.h>

#include <linux/proc_fs.h>
#include <linux/poll.h>
#include <asm/bitops.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <linux/moduleparam.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include "adv7179.h"
#include <asm/io.h>
#include <linux/miscdevice.h>

#include <linux/i2c.h>

#ifdef __HuaweiLite__
#include "hi_adv7179.h"
#include "stdio.h"
#undef DEBUG_LEVEL
#endif

#define I2C_ADV7179       0x56
static struct i2c_client* adv7179_client;
static struct i2c_board_info adv7179_i2c_info =
{
    I2C_BOARD_INFO("adv7179", (I2C_ADV7179>>1)),
};

/***************I2C device num***********************/
static unsigned int i2c_num = 1;
static unsigned int ADV7179_I2C_NUM = 1;

#ifdef __HuaweiLite__
struct i2c_client adv7179_client_obj;
struct i2c_client * hi_adv7179_i2c_client_init(void)
{
    int ret = 0;
    struct i2c_client * i2c_client0 = &adv7179_client_obj;
    i2c_client0->addr = I2C_ADV7179 >> 1;
    ret = client_attach(i2c_client0, ADV7179_I2C_NUM);
    if(ret) {
        dprintf("Fail to attach client!\n");
        return -1;
    }
    return &adv7179_client_obj;
}
#endif

#ifndef __HuaweiLite__
static unsigned char hi_i2c_read(unsigned char dev_addr, unsigned int reg_addr,
    unsigned int reg_addr_num, unsigned int data_byte_num)
{
    unsigned char tmp_buf0[4];
    unsigned char tmp_buf1[4];
    int ret = 0;
    int ret_data = 0xFF;
    int idx = 0;
    struct i2c_client client;
    struct i2c_msg msg[2];
    memcpy(&client, adv7179_client, sizeof(struct i2c_client));
    msg[0].addr = (dev_addr>>1);
    msg[0].flags = client.flags & I2C_M_TEN;
    msg[0].len = reg_addr_num;
    msg[0].buf = tmp_buf0;
    if(reg_addr_num == 1)
    {
        tmp_buf0[idx++] = reg_addr&0xff;
    }
    else
    {
        tmp_buf0[idx++] = (reg_addr >> 8)&0xff;
        tmp_buf0[idx++] = reg_addr&0xff;
    }
    msg[1].addr = (dev_addr>>1);
    msg[1].flags = client.flags & I2C_M_TEN;
    msg[1].flags |= I2C_M_RD;
    msg[1].len = data_byte_num;
    msg[1].buf = tmp_buf1;
    ret = i2c_transfer(client.adapter, msg, 2);
    if (ret == 2)
    {
        if (data_byte_num == 2)
        {
            ret_data = tmp_buf1[1] | (tmp_buf1[0] << 8);
        }
        else
        {
            ret_data = tmp_buf1[0];
        }
    }
    else
    {
        ret_data = -EIO;
    }
    return ret_data;
}

static int hi_i2c_write(unsigned char dev_addr, unsigned int reg_addr,
    unsigned int reg_addr_num, unsigned int data, unsigned int data_byte_num)
{
    unsigned char tmp_buf[8];
    int ret = 0;
    int idx = 0;
    struct i2c_client client;
    memcpy(&client, adv7179_client, sizeof(struct i2c_client));
    client.addr = (dev_addr>>1);
    if(reg_addr_num == 1)
    {
        tmp_buf[idx++] = reg_addr&0xff;
    }
    else
    {
        tmp_buf[idx++] = (reg_addr >> 8)&0xff;
        tmp_buf[idx++] = reg_addr&0xff;
    }
    if(data_byte_num == 1)
    {
        tmp_buf[idx++] = data;
    }
    else
    {
        tmp_buf[idx++] = (data >> 8)&0xff;
        tmp_buf[idx++] = data&0xff;
    }
    ret = i2c_master_send(&client, tmp_buf, idx);
    return ret;
}

unsigned char adv7179_i2c_read_byte(unsigned char dev_addr, unsigned char reg_addr)
{
    return hi_i2c_read(dev_addr, reg_addr, 1, 1);
}

int adv7179_i2c_write_byte(unsigned char dev_addr, unsigned char reg_addr, unsigned char data)
{
    return hi_i2c_write(dev_addr, reg_addr, 1, data, 1);
}

#else
unsigned char adv7179_i2c_read_byte(unsigned char devaddress, unsigned char address)
{
    unsigned char ret_data = 0xFF;
    int ret;
    struct i2c_client* client = adv7179_client;
    unsigned char buf[2];

    buf[0] = address;
    ret = i2c_master_recv(client, buf, 2);
    if (ret >= 0)
    {
        ret_data = buf[0];
    }
    return ret_data;
}

int adv7179_i2c_write_byte(unsigned char devaddress, unsigned char address, unsigned char data)
{
    int ret;
    unsigned char buf[2];
    struct i2c_client* client = adv7179_client;

    buf[0] = address;
    buf[1] = data;

    ret = i2c_master_send(client, buf, 2);
    return ret;
}
#endif

static int i2c_client_init(void)
{
#ifdef __HuaweiLite__
    adv7179_client = hi_adv7179_i2c_client_init();
#else
    struct i2c_adapter* i2c_adap;
    i2c_adap   = i2c_get_adapter(ADV7179_I2C_NUM);
    adv7179_client = i2c_new_device(i2c_adap, &adv7179_i2c_info);

    i2c_put_adapter(i2c_adap);
#endif
    return 0;
}

static void i2c_client_exit(void)
{
#ifndef __HuaweiLite__
    i2c_unregister_device(adv7179_client);
#endif
}

unsigned char _hi_i2c_read_byte(unsigned char devaddress, unsigned char address)
{
#ifdef DRV_I2C
    return drv_i2c_read(ADV7179_I2C_NUM, devaddress, address, 1, 1);
#else
    unsigned char         ret_data = 0xFF;
    int                   ret;
    struct i2c_client*    client = adv7179_client;
    static struct i2c_msg msg[2];
    unsigned char *       buffer;
    unsigned int          data_width = 1;
    buffer = kzalloc(data_width,GFP_ATOMIC);
    if(!buffer)
    {
        printk("NO memory.\n");
        return 0;
    }

    buffer[0] = address;
    msg[0].addr = client->addr;
    msg[0].flags = 0;
    msg[0].len = 1;
    msg[0].buf = buffer;

    msg[1].addr  = client->addr;
    msg[1].flags = client->flags | I2C_M_RD;
    msg[1].len   = data_width;
    msg[1].buf   = buffer;

    ret = hi_i2c_transfer(client->adapter, msg, 2);  // success : ret=2
    if (ret < 0)
    {
        printk("i2c read failed.\n");
    }
    else
    {
        //printk("i2c read success buffer[0] = 0x%x.\n",buffer[0]);
        memcpy(&ret_data,buffer,data_width);
    }
    kfree(buffer);

    return ret_data;
#endif
}

int _hi_i2c_write_byte(unsigned char devaddress, unsigned char address, unsigned char data)
{
#ifdef DRV_I2C
    return drv_i2c_write(ADV7179_I2C_NUM, devaddress, address, 1, data, 1);
#else
    int                   ret;
    unsigned char         buf[2];
    struct i2c_client*    client = adv7179_client;
    unsigned int u32Tries = 0;

#ifdef __HuaweiLite__
    client->flags &= ~I2C_M_16BIT_REG;
    buf[0] = address & 0xff;
    client->flags &= ~I2C_M_16BIT_DATA;
    buf[1] = data & 0xff;
#else
    buf[0] = address & 0xff;
    buf[1] = data & 0xff;
#endif

    while (1)
    {
        ret = hi_i2c_master_send(client, buf, 2);
        if (ret == 2)
        {
            break;
        }
#ifdef __HuaweiLite__
        else if ((ret == -EAGAIN))
#else
        else if ((ret == -EAGAIN) && (in_atomic() || irqs_disabled()))
#endif
        {
            u32Tries++;
            if (u32Tries > 5)
            {
                return -1;
            }
        }
        else
        {
            printk("[%s %d] hi_i2c_master_send error, ret=%d. \n", __func__, __LINE__, ret);
            return ret;
        }
    }

    return ret;
#endif
}

#define  I2CReadByte   _hi_i2c_read_byte
#define  I2CWriteByte  _hi_i2c_write_byte

/* adv7179 i2c slaver address micro-definition. */
#define ADV7179_NUM       1
#define ADV7179_REG_WIDTH 1
#define ADV7179_DAT_WIDTH 1
#define adv7179_NUM_REGISTERS   128

/* Output filter:  S-Video  Composite */
#define MR050       0x11
#define MR060       0x14
#define TR0MODE_656  0x08
#define TR0MODE_601  0x0c
#define TR0RST      0x80
#define TR1CAPT     0x00
#define TR1PLAY     0x00

static int norm_mode =  VIDEO_NORM_PAL;

static const unsigned char init_656_NTSC[] =
{
    0x00, 0x00,     /* MR0 */
    0x01, 0x00,     /* MR1 */
    0x02, 0x00,     /* MR2 RTC control: bits 2 and 1 */
    0x03, 0x00,     /* MR3 */
    0x04, 0x15,     /* MR4 */
    0x05, 0x00,     /* Reserved */
    0x06, 0x00,     /* Reserved */
    0x07, TR0MODE_656,  /* TM0 */
    0x08, TR1CAPT,      /* TM1 */
    0x09, 0x16,     /* Fsc0 */
    0x0a, 0x7c,     /* Fsc1 */
    0x0b, 0xf0,     /* Fsc2 */
    0x0c, 0x21,     /* Fsc3 */
    0x0d, 0x00,     /* Subcarrier Phase */
    0x0e, 0x00,     /* Closed Capt. Ext 0 */
    0x0f, 0x00,     /* Closed Capt. Ext 1 */
    0x10, 0x00,     /* Closed Capt. 0 */
    0x11, 0x00,     /* Closed Capt. 1 */
    0x12, 0x00,     /* Pedestal Ctl 0 */
    0x13, 0x00,     /* Pedestal Ctl 1 */
    0x14, 0x00,     /* Pedestal Ctl 2 */
    0x15, 0x00,     /* Pedestal Ctl 3 */
    0x16, 0x00,     /* CGMS_WSS_0 */
    0x17, 0x00,     /* CGMS_WSS_1 */
    0x18, 0x00,     /* CGMS_WSS_2 */
    0x19, 0x00,     /* Teletext Ctl */
};

static const unsigned char init_656_PAL[] =
{
    0x00, 0x05,     /* MR0, PAL BDGHI */
    0x01, 0x00,     /* MR1 */
    0x02, 0x00,     /* MR2 RTC control: bits 2 and 1 */
    0x03, 0x00,     /* MR3 */
    0x04, 0x15,     /* MR4, RGB OUTPUT */
    0x05, 0x00,     /* Reserved */
    0x06, 0x00,     /* Reserved */
    0x07, TR0MODE_656,  /* TM0 */
    0x08, TR1CAPT,      /* TM1 */
    0x09, 0xcb,     /* Fsc0 */
    0x0a, 0x8a,     /* Fsc1 */
    0x0b, 0x09,     /* Fsc2 */
    0x0c, 0x2a,     /* Fsc3 */
    0x0d, 0x00,     /* Subcarrier Phase */
    0x0e, 0x00,     /* Closed Capt. Ext 0 */
    0x0f, 0x00,     /* Closed Capt. Ext 1 */
    0x10, 0x00,     /* Closed Capt. 0 */
    0x11, 0x00,     /* Closed Capt. 1 */
    0x12, 0x00,     /* Pedestal Ctl 0 */
    0x13, 0x00,     /* Pedestal Ctl 1 */
    0x14, 0x00,     /* Pedestal Ctl 2 */
    0x15, 0x00,     /* Pedestal Ctl 3 */
    0x16, 0x00,     /* CGMS_WSS_0 */
    0x17, 0x00,     /* CGMS_WSS_1 */
    0x18, 0x00,     /* CGMS_WSS_2 */
    0x19, 0x00,     /* Teletext Ctl */
};

static  unsigned char init_601_NTSC[] =
{
    0x00, 0x00,     /* MR0 */
    0x01, 0x00,     /* MR1 */
    0x02, 0x08,     /* MR2 RTC control: bits 2 and 1 */
    0x03, 0x04,     /* MR3 */
    0x04, 0x0C,     /* MR4 */
    0x05, 0x00,     /* Reserved */
    0x06, 0x00,     /* Reserved */
    0x07, TR0MODE_601,      /* TM0 */
    0x08, TR1CAPT,      /* TM1 */
    0x09, 0x16,     /* Fsc0 */
    0x0a, 0x7c,     /* Fsc1 */
    0x0b, 0xf0,     /* Fsc2 */
    0x0c, 0x21,     /* Fsc3 */
    0x0d, 0x00,     /* Subcarrier Phase */
    0x0e, 0x00,     /* Closed Capt. Ext 0 */
    0x0f, 0x00,     /* Closed Capt. Ext 1 */
    0x10, 0x00,     /* Closed Capt. 0 */
    0x11, 0x00,     /* Closed Capt. 1 */
    0x12, 0x00,     /* Pedestal Ctl 0 */
    0x13, 0x00,     /* Pedestal Ctl 1 */
    0x14, 0x00,     /* Pedestal Ctl 2 */
    0x15, 0x00,     /* Pedestal Ctl 3 */
    0x16, 0x00,     /* CGMS_WSS_0 */
    0x17, 0x00,     /* CGMS_WSS_1 */
    0x18, 0x00,     /* CGMS_WSS_2 */
    0x19, 0x00,     /* Teletext Ctl  */
};

static  unsigned char init_601_PAL[] =
{
    0x00, 0x05,     /* MR0, PAL BDGHI */
    0x01, 0x00,     /* MR1 */
    0x02, 0x08,     /* MR2 RTC control: bits 2 and 1 */
    0x03, 0x04,     /* MR3 */
    0x04, 0x1C,     /* MR4, RGB OUTPUT */
    0x05, 0x00,     /* Reserved */
    0x06, 0x00,     /* Reserved */
    0x07, TR0MODE_601,  /* TM0 */
    0x08, TR1CAPT,      /* TM1 */
    0x09, 0xcb,     /* Fsc0 */
    0x0a, 0x8a,     /* Fsc1 */
    0x0b, 0x09,     /* Fsc2 */
    0x0c, 0x2a,     /* Fsc3 */
    0x0d, 0x00,     /* Subcarrier Phase */
    0x0e, 0x00,     /* Closed Capt. Ext 0 */
    0x0f, 0x00,     /* Closed Capt. Ext 1 */
    0x10, 0x00,     /* Closed Capt. 0 */
    0x11, 0x00,     /* Closed Capt. 1 */
    0x12, 0x00,     /* Pedestal Ctl 0 */
    0x13, 0x00,     /* Pedestal Ctl 1 */
    0x14, 0x00,     /* Pedestal Ctl 2 */
    0x15, 0x00,     /* Pedestal Ctl 3 */
    0x16, 0x00,     /* CGMS_WSS_0 */
    0x17, 0x00,     /* CGMS_WSS_1 */
    0x18, 0x00,     /* CGMS_WSS_2 */
    0x19, 0x00,     /* Teletext Ctl */
};

static int write_regs(unsigned char* pdevdata,   unsigned long datalen)
{
    int i = 0;

    while (i < datalen)
    {
        I2CWriteByte(I2C_ADV7179, pdevdata[i], pdevdata[i + 1]);
        i += 2;
    }
    return 0;
}

/*
 * adv7179 initialise routine.
 *
 * @param vdaccir: adv7179's working mode:0--VIDEO_MODE_CCIR656; 1--VIDEO_MODE_CCIR601
 * @param vdanorm: adv7179's norm mode;
 * @param vdamaster: adv7179's slave or master mode;
 *
 * @return value:0--success; -1--error.
 *
 */
int init_vda(int vdaccir, int vdanorm, int vdamaster)
{
    if ((vdaccir == VIDEO_MODE_CCIR656) && (vdanorm == VIDEO_NORM_PAL))
    {
        if (write_regs((unsigned char*)&init_656_PAL, sizeof(init_656_PAL)) != 0)
        {
            goto err_out;
        }
    }
    else if ((vdaccir == VIDEO_MODE_CCIR656) && (vdanorm == VIDEO_NORM_NTSC))
    {
        if (write_regs((unsigned char*)&init_656_NTSC, sizeof(init_656_NTSC)) != 0)
        {
            goto err_out;
        }
    }
    else if ((vdaccir == VIDEO_MODE_CCIR601) && (vdanorm == VIDEO_NORM_PAL))
    {
        if (vdamaster == VIDEO_MODE_MASTER)
        {
            init_601_PAL[15] |= VIDEO_MODE_MASTER;
        }
        else if (vdamaster == VIDEO_MODE_SLAVER)
        {
            init_601_PAL[15] &= ~VIDEO_MODE_MASTER;
        }

        if (write_regs((unsigned char*)&init_601_PAL, sizeof(init_601_PAL))  != 0)
        {
            goto err_out;
        }
    }
    else if ((vdaccir == VIDEO_MODE_CCIR601) && (vdanorm == VIDEO_NORM_NTSC))
    {
        if (vdamaster == VIDEO_MODE_MASTER)
        {
            init_601_NTSC[15] |= VIDEO_MODE_MASTER;
        }
        if (vdamaster == VIDEO_MODE_SLAVER)
        {
            init_601_NTSC[15] &= ~VIDEO_MODE_MASTER;
        }

        if (write_regs((unsigned char*)&init_601_NTSC, sizeof(init_601_NTSC))  != 0)
        {
            goto err_out;
        }
    }

    if (vdaccir == VIDEO_MODE_CCIR656)
    {
        I2CWriteByte(I2C_ADV7179, 0x07, (TR0MODE_656 | TR0RST));
        I2CWriteByte(I2C_ADV7179, 0x07, TR0MODE_656);
    }
    else if (vdaccir == VIDEO_MODE_CCIR601)
    {
        if (vdamaster == VIDEO_MODE_MASTER)
        {
            I2CWriteByte(I2C_ADV7179, 0x07, (TR0MODE_601 | TR0RST | VIDEO_MODE_MASTER));
            I2CWriteByte(I2C_ADV7179, 0x07, (TR0MODE_601 | VIDEO_MODE_MASTER));
        }
        else
        {
            I2CWriteByte(I2C_ADV7179, 0x07, (TR0MODE_601 | TR0RST));
            I2CWriteByte(I2C_ADV7179, 0x07, TR0MODE_601);
        }
    }

    return (0);

err_out:
    printk("err_out\n");
    return -1;
}

/*
 * adv7179 open routine.
 * do nothing.
 *
 */
int adv7179_open(struct inode* inode, struct file* file)
{
    return 0;
}

/*
 * adv7179 close routine.
 * do nothing.
 *
 */
int adv7179_close(struct inode* inode, struct file* file)
{
    return 0;
}

/*
 * adv7179 ioctl routine.
 * @param inode: pointer of the node;
 * @param file: pointer of the file;
 *
 * @param cmd: command from the app:
 * ENCODER_SET_NORM(2):set adv7179's work mode.
 *
 * @param arg:arg from app layer.
 *
 * @return value:0-- set success; -1-- set error.
 *
 */
static long adv7179_ioctl(struct file* file, unsigned int cmd, unsigned long arg)
{
    switch (cmd)
    {
        case ENCODER_SET_NORM:
        {
            int iarg = (int) arg;

            switch (iarg)
            {
                case VIDEO_MODE_656_PAL:
                    write_regs((unsigned char*)&init_656_PAL, sizeof(init_656_PAL));

                    I2CWriteByte(I2C_ADV7179, 0x07, (TR0MODE_656 | TR0RST));
                    I2CWriteByte(I2C_ADV7179, 0x07, TR0MODE_656);
                    break;

                case VIDEO_MODE_656_NTSC:
                    write_regs((unsigned char*)&init_656_NTSC, sizeof(init_656_NTSC));

                    I2CWriteByte(I2C_ADV7179, 0x07, (TR0MODE_656 | TR0RST));
                    I2CWriteByte(I2C_ADV7179, 0x07, TR0MODE_656);
                    break;

                case VIDEO_MODE_601_PAL_MASTER:
                    init_601_PAL[15] |= VIDEO_MODE_MASTER;
                    write_regs((unsigned char*)&init_601_PAL, sizeof(init_601_PAL));

                    I2CWriteByte(I2C_ADV7179, 0x07, (TR0MODE_601 | TR0RST | VIDEO_MODE_MASTER));
                    I2CWriteByte(I2C_ADV7179, 0x07, (TR0MODE_601 | VIDEO_MODE_MASTER));
                    break;


                case VIDEO_MODE_601_NTSC_MASTER:
                    init_601_NTSC[15] |= VIDEO_MODE_MASTER;
                    write_regs((unsigned char*)&init_601_NTSC, sizeof(init_601_NTSC));

                    I2CWriteByte(I2C_ADV7179, 0x07, (TR0MODE_601 | TR0RST | VIDEO_MODE_MASTER));
                    I2CWriteByte(I2C_ADV7179, 0x07, (TR0MODE_601 | VIDEO_MODE_MASTER));
                    break;

                case VIDEO_MODE_601_PAL_SLAVER:
                    init_601_PAL[15] &= ~VIDEO_MODE_MASTER;
                    write_regs((unsigned char*)&init_601_PAL, sizeof(init_601_PAL));

                    I2CWriteByte(I2C_ADV7179, 0x07, (TR0MODE_601 | TR0RST));
                    I2CWriteByte(I2C_ADV7179, 0x07, TR0MODE_601);
                    break;

                case VIDEO_MODE_601_NTSC_SLAVER:
                    init_601_NTSC[15] &= ~VIDEO_MODE_MASTER;
                    write_regs((unsigned char*)&init_601_NTSC, sizeof(init_601_NTSC));

                    I2CWriteByte(I2C_ADV7179, 0x07, (TR0MODE_601 | TR0RST));
                    I2CWriteByte(I2C_ADV7179, 0x07, TR0MODE_601);
                    break;

                default:
                    return -1;
            }
        }
        break;

        default:
            return -1;
    }

    return 0;
}

/*
 *  The various file operations we support.
 */
static struct file_operations adv7179_fops =
{
    .owner      = THIS_MODULE,
    .unlocked_ioctl     = adv7179_ioctl,
    .open         = adv7179_open ,
    .release    = adv7179_close
};

#ifndef __HuaweiLite__
static struct miscdevice adv7179_dev =
{
    MISC_DYNAMIC_MINOR,
    "adv7179",
    &adv7179_fops,
};
#endif

static int adv7179_device_init(void)
{
    unsigned char regvalue1, regvalue2;
    regvalue1 = I2CReadByte(I2C_ADV7179, 0x07);
    I2CWriteByte(I2C_ADV7179, 0x07, 0xa5);
    regvalue2 = I2CReadByte(I2C_ADV7179, 0x07);
    if (regvalue2 != 0xa5)
    {
        printk("read adv7179 register is %x\n", regvalue2);
        printk("check adv7179 error.\n");
        return -EFAULT;
    }
    I2CWriteByte(I2C_ADV7179, 0x07, regvalue1);
    if (norm_mode == VIDEO_NORM_NTSC)
    {
        if (init_vda(VIDEO_MODE_CCIR656, VIDEO_NORM_NTSC, VIDEO_MODE_MASTER) == 0)
        {
            return 0;
        }
        else
        {
            return -EFAULT;
        }
    }
    else
    {
        if (init_vda(VIDEO_MODE_CCIR656, VIDEO_NORM_PAL, VIDEO_MODE_MASTER) == 0)
        {
            return 0;
        }
        else
        {
            return -EFAULT;
        }
    }

}

#ifdef __HuaweiLite__
int adv7179_init(void* pArgs)
{
    int ret = 0;

    switch (i2c_num)
    {
        case 0:
            ADV7179_I2C_NUM = 0;
            break;
        case 1:
            ADV7179_I2C_NUM = 1;
            break;
        case 2:
            ADV7179_I2C_NUM = 2;
            break;
        default:
            ADV7179_I2C_NUM = 1;
            break;
    }

    if (NULL != pArgs)
    {
        norm_mode = *(int*)pArgs;
    }

    i2c_client_init();

    sleep(3);

    if (adv7179_device_init() < 0)
    {
        printk("adv7179 driver init fail for device init error!\n");
        return -1;
    }

    printk("adv7179 driver init successful!\n");
    return ret;
}

void __exit adv7179_exit(void)
{
}

#else
static int __init adv7179_init(void)
{
    int ret = 0;

    switch (i2c_num)
    {
        case 0:
            ADV7179_I2C_NUM = 0;
            break;
        case 1:
            ADV7179_I2C_NUM = 1;
            break;
        case 2:
            ADV7179_I2C_NUM = 2;
            break;
        default:
            ADV7179_I2C_NUM = 1;
            break;
    }

    i2c_client_init();

    ret = misc_register(&adv7179_dev);
    if (ret)
    {
        printk("could not register adv7179 devices. \n");
        return -1;
    }

    if (adv7179_device_init() < 0)
    {
        i2c_client_exit();

        misc_deregister(&adv7179_dev);

        printk("adv7179 driver init fail for device init error!\n");
        return -1;
    }
    printk("adv7179 driver init successful!\n");
    return ret;
}

static void __exit adv7179_exit(void)
{
    i2c_client_exit();
    misc_deregister(&adv7179_dev);
}
#endif

module_init(adv7179_init);
module_exit(adv7179_exit);

module_param(norm_mode, int, S_IRUGO);
MODULE_PARM_DESC(norm_mode, "0:PAL, 1:NTSC");

module_param(i2c_num, uint, S_IRUGO);
MODULE_PARM_DESC(i2c_num, "0:I2c0, 1:I2C1, 2:I2C2");

MODULE_LICENSE("GPL");
MODULE_AUTHOR("hisilicon");

