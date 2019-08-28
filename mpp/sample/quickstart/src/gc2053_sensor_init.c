
/******************************************************************************

  Copyright (C), 2016, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : gc2053_sensor_ctl.c

  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2018/07/23
  Description   :
  History       :
  1.Date        : 2018/07/23
    Author      :
    Modification: Created file

******************************************************************************/
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <hi_math.h>

#include "hi_comm_video.h"
#include "hi_sns_ctrl.h"

#ifdef HI_GPIO_I2C
#include "gpioi2c_ex.h"
#else
#include "hi_i2c.h"
#endif
#include "mpi_isp.h"

#ifndef __HuaweiLite__
#include <linux/fb.h>
#include "quick_i2c-dev.h"
#endif


const unsigned char sensor_gc2053_i2c_addr     =    0x6e;        /* I2C Address of GC2053 */
const unsigned int  sensor_gc2053_addr_byte    =    1;
const unsigned int  sensor_gc2053_data_byte    =    1;
static int g_i2cfd = -1;
#define I2C_DEV_SENSOR (0)

int sensor_gc2053_i2c_init(VI_PIPE ViPipe)
{
    char acDevFile[16] = {0};
    HI_U8 u8DevNum;

    if (g_i2cfd >= 0)
    {
        return HI_SUCCESS;
    }
#ifdef HI_GPIO_I2C
    int ret;

    g_i2cfd = open("/dev/gpioi2c_ex", O_RDONLY, S_IRUSR);
    if (g_i2cfd < 0)
    {
        ISP_TRACE(HI_DBG_ERR, "Open gpioi2c_ex error!\n");
        return HI_FAILURE;
    }
#else
    int ret;

    u8DevNum = I2C_DEV_SENSOR;
    snprintf(acDevFile, sizeof(acDevFile),  "/dev/i2c-%u", u8DevNum);

    g_i2cfd = open(acDevFile, O_RDWR, S_IRUSR | S_IWUSR);

    if (g_i2cfd < 0)
    {
        ISP_TRACE(HI_DBG_ERR, "Open /dev/hi_i2c_drv-%u error!\n", u8DevNum);
        return HI_FAILURE;
    }

    ret = ioctl(g_i2cfd, I2C_SLAVE_FORCE, (sensor_gc2053_i2c_addr >> 1));
    if (ret < 0)
    {
        ISP_TRACE(HI_DBG_ERR, "I2C_SLAVE_FORCE error!\n");
        close(g_i2cfd);
        g_i2cfd = -1;
        return ret;
    }
#endif

    return HI_SUCCESS;
}

int sensor_gc2053_i2c_exit(VI_PIPE ViPipe)
{
    if (g_i2cfd >= 0)
    {
        close(g_i2cfd);
        g_i2cfd = -1;
        return HI_SUCCESS;
    }
    return HI_FAILURE;
}

int sensor_gc2053_write_register(VI_PIPE ViPipe, HI_U32 addr, HI_U32 data)
{
    if (0 > g_i2cfd)
    {
        return HI_SUCCESS;
    }

#ifdef HI_GPIO_I2C
    i2c_data.dev_addr = sensor_gc2053_i2c_addr;
    i2c_data.reg_addr = addr;
    i2c_data.addr_byte_num = sensor_gc2053_addr_byte;
    i2c_data.data = data;
    i2c_data.data_byte_num = sensor_gc2053_data_byte;

    ret = ioctl(g_i2cfd, GPIO_I2C_WRITE, &i2c_data);

    if (ret)
    {
        ISP_TRACE(HI_DBG_ERR, "GPIO-I2C write faild!\n");
        return ret;
    }
#else
    int idx = 0;
    int ret;
    char buf[8];

    if (sensor_gc2053_addr_byte == 2)
    {
        buf[idx] = (addr >> 8) & 0xff;
        idx++;
        buf[idx] = addr & 0xff;
        idx++;
    }
    else
    {
        buf[idx] = addr & 0xff;
        idx++;
    }

    if (sensor_gc2053_data_byte == 2)
    {
        buf[idx] = (data >> 8) & 0xff;
        idx++;
        buf[idx] = data & 0xff;
        idx++;
    }
    else
    {
        buf[idx] = data & 0xff;
        idx++;
    }

    //printf("addr:0x%x;data:0x%x\n",buf[0],buf[1]);

    ret = write(g_i2cfd, buf, sensor_gc2053_addr_byte + sensor_gc2053_data_byte);
    if (ret < 0)
    {
        ISP_TRACE(HI_DBG_ERR, "I2C_WRITE error!\n");
        return HI_FAILURE;
    }

#endif
    return HI_SUCCESS;
}


void sensor_gc2053_linear_1080p30_init(VI_PIPE ViPipe);

static HI_VOID gc2053_writeSnsExp(HI_S32 s32SnsExp)
{
    VI_PIPE ViPipe = 0;
    HI_S32 s32RegValue = 0;

//bit [5:0]
#define PDT_GC2053_EXP_H_ADDR       (0x03)
//bit [7:0]
#define PDT_GC2053_EXP_L_ADDR       (0x04)

    s32RegValue = ((s32SnsExp>>8)&0x0000003F);
    sensor_gc2053_write_register(ViPipe, PDT_GC2053_EXP_H_ADDR, s32RegValue);

    s32RegValue = ((s32SnsExp)&0x000000FF);
    sensor_gc2053_write_register(ViPipe, PDT_GC2053_EXP_L_ADDR, s32RegValue);
}

static HI_VOID gc2053_writeSnsGain(HI_S32 s32SnsAgain, HI_S32 s32SnsDgain)
{
    VI_PIPE ViPipe = 0;
    HI_S32 s32RegValue = 0;

//bit [7:0]
#define PDT_GC2053_AGAIN_L_ADDR     (0xb3)
//bit [1:0]
#define PDT_GC2053_AGAIN_H_ADDR     (0xb4)
//bit[5:0]
#define PDT_GC2053_DGAIN_H_ADDR     (0xb8)
//bit[5:0]
#define PDT_GC2053_DGAIN_L_ADDR     (0xb9)

    s32RegValue = ((s32SnsAgain >> 8)&0x00000003);
    sensor_gc2053_write_register(ViPipe, PDT_GC2053_AGAIN_H_ADDR, s32RegValue);

    s32RegValue = ((s32SnsAgain)&0x000000FF);
    sensor_gc2053_write_register(ViPipe, PDT_GC2053_AGAIN_L_ADDR, s32RegValue);

    s32RegValue = ((s32SnsDgain>>8)&0x0000003F);
    sensor_gc2053_write_register(ViPipe, PDT_GC2053_DGAIN_H_ADDR, s32RegValue);

    s32RegValue = ((s32SnsDgain)&0x0000003F);
    sensor_gc2053_write_register(ViPipe, PDT_GC2053_DGAIN_L_ADDR, s32RegValue);
}

void gc2053_sensor_init(VI_PIPE ViPipe, HI_S32 s32SnsExp, HI_S32 s32SnsAgain, HI_S32 s32SnsDgain)
{
    /* 1. sensor i2c init */
    sensor_gc2053_i2c_init(ViPipe);

    sensor_gc2053_linear_1080p30_init(ViPipe);

    gc2053_writeSnsExp(s32SnsExp);
    gc2053_writeSnsGain(s32SnsAgain, s32SnsDgain);

    sensor_gc2053_write_register(ViPipe, 0xb1, 0x1);
    sensor_gc2053_write_register(ViPipe, 0xb2, 0x14);
    sensor_gc2053_write_register(ViPipe, 0x90, 0x0);
    sensor_gc2053_write_register(ViPipe, 0x42, 0x46);
    sensor_gc2053_write_register(ViPipe, 0x41, 0x5);

    return ;
}

void gc2053_sensor_deinit(VI_PIPE ViPipe)
{
    sensor_gc2053_i2c_exit(ViPipe);

    return;
}

void sensor_gc2053_linear_1080p30_init(VI_PIPE ViPipe)
{
        sensor_gc2053_write_register(ViPipe,0xfe, 0x80);
        sensor_gc2053_write_register(ViPipe,0xfe, 0x80);
        sensor_gc2053_write_register(ViPipe,0xfe, 0x80);
        sensor_gc2053_write_register(ViPipe,0xfe, 0x00);
        sensor_gc2053_write_register(ViPipe,0xf2, 0x00);
        sensor_gc2053_write_register(ViPipe,0xf3, 0x00);
        sensor_gc2053_write_register(ViPipe,0xf4, 0x36);
        sensor_gc2053_write_register(ViPipe,0xf5, 0xc0);
        sensor_gc2053_write_register(ViPipe,0xf6, 0x44);
        sensor_gc2053_write_register(ViPipe,0xf7, 0x01);
        sensor_gc2053_write_register(ViPipe,0xf8, 0x2c);
        sensor_gc2053_write_register(ViPipe,0xf9, 0x42);
        sensor_gc2053_write_register(ViPipe,0xfc, 0x8e);

        sensor_gc2053_write_register(ViPipe,0xfe, 0x00);
        sensor_gc2053_write_register(ViPipe,0x87, 0x18);
        sensor_gc2053_write_register(ViPipe,0xee, 0x30);
        sensor_gc2053_write_register(ViPipe,0xd0, 0xb7);
        sensor_gc2053_write_register(ViPipe,0x03, 0x04);
        sensor_gc2053_write_register(ViPipe,0x04, 0x60);
        sensor_gc2053_write_register(ViPipe,0x05, 0x04);
        sensor_gc2053_write_register(ViPipe,0x06, 0x4c);
        sensor_gc2053_write_register(ViPipe,0x07, 0x00);
        sensor_gc2053_write_register(ViPipe,0x08, 0x11);
        sensor_gc2053_write_register(ViPipe,0x0a, 0x02);
        sensor_gc2053_write_register(ViPipe,0x0c, 0x02);
        sensor_gc2053_write_register(ViPipe,0x0d, 0x04);
        sensor_gc2053_write_register(ViPipe,0x0e, 0x40);
        sensor_gc2053_write_register(ViPipe,0x12, 0xe2);
        sensor_gc2053_write_register(ViPipe,0x13, 0x16);
        sensor_gc2053_write_register(ViPipe,0x19, 0x0a);
        sensor_gc2053_write_register(ViPipe,0x28, 0x0a);
        sensor_gc2053_write_register(ViPipe,0x2b, 0x04);
        sensor_gc2053_write_register(ViPipe,0x37, 0x03);
        sensor_gc2053_write_register(ViPipe,0x43, 0x07);
        sensor_gc2053_write_register(ViPipe,0x44, 0x40);
        sensor_gc2053_write_register(ViPipe,0x46, 0x0b);
        sensor_gc2053_write_register(ViPipe,0x4b, 0x20);
        sensor_gc2053_write_register(ViPipe,0x4e, 0x08);
        sensor_gc2053_write_register(ViPipe,0x55, 0x20);
        sensor_gc2053_write_register(ViPipe,0x77, 0x01);
        sensor_gc2053_write_register(ViPipe,0x78, 0x00);
        sensor_gc2053_write_register(ViPipe,0x7c, 0x93);
        sensor_gc2053_write_register(ViPipe,0x8d, 0x92);
        sensor_gc2053_write_register(ViPipe,0x90, 0x00);
        sensor_gc2053_write_register(ViPipe,0x41, 0x04);
        sensor_gc2053_write_register(ViPipe,0x42, 0x65);
        sensor_gc2053_write_register(ViPipe,0xce, 0x7c);
        sensor_gc2053_write_register(ViPipe,0xd2, 0x41);
        sensor_gc2053_write_register(ViPipe,0xd3, 0xdc);
        sensor_gc2053_write_register(ViPipe,0xe6, 0x50);
        sensor_gc2053_write_register(ViPipe,0xb6, 0xc0);
        sensor_gc2053_write_register(ViPipe,0xb0, 0x70);
        sensor_gc2053_write_register(ViPipe,0x26, 0x30);
        sensor_gc2053_write_register(ViPipe,0xfe, 0x01);
        sensor_gc2053_write_register(ViPipe,0x55, 0x07);
        sensor_gc2053_write_register(ViPipe,0xfe, 0x04);
        sensor_gc2053_write_register(ViPipe,0x14, 0x78);
        sensor_gc2053_write_register(ViPipe,0x15, 0x78);
        sensor_gc2053_write_register(ViPipe,0x16, 0x78);
        sensor_gc2053_write_register(ViPipe,0x17, 0x78);
        sensor_gc2053_write_register(ViPipe,0xfe, 0x01);
        sensor_gc2053_write_register(ViPipe,0x04, 0x00);
        sensor_gc2053_write_register(ViPipe,0x94, 0x03);
        sensor_gc2053_write_register(ViPipe,0x97, 0x07);
        sensor_gc2053_write_register(ViPipe,0x98, 0x80);
        sensor_gc2053_write_register(ViPipe,0x9a, 0x06);
        sensor_gc2053_write_register(ViPipe,0xfe, 0x00);
        sensor_gc2053_write_register(ViPipe,0x7b, 0x2a);
        sensor_gc2053_write_register(ViPipe,0x23, 0x2d);
        sensor_gc2053_write_register(ViPipe,0xfe, 0x03);
        sensor_gc2053_write_register(ViPipe,0x01, 0x27);
        sensor_gc2053_write_register(ViPipe,0x02, 0x56);
        sensor_gc2053_write_register(ViPipe,0x03, 0xb6);
        sensor_gc2053_write_register(ViPipe,0x12, 0x80);
        sensor_gc2053_write_register(ViPipe,0x13, 0x07);
        sensor_gc2053_write_register(ViPipe,0x15, 0x12);
        sensor_gc2053_write_register(ViPipe,0xfe, 0x00);
        sensor_gc2053_write_register(ViPipe,0x3e, 0x91);

        printf("quick sensor init gc2053\n");

        return;
}

#ifndef __HuaweiLite__
int gc2053_read_registers( HI_U32 addr)
{
    HI_S32 s32RegVal = 0;
    if (g_i2cfd <  0)
    {
        printf("read register error\n");
        return 0;
    }
    HI_S32 s32Ret = 0;
    HI_U32 u32RegWidth = sensor_gc2053_addr_byte;
    HI_U32 u32DataWidth = sensor_gc2053_data_byte;
    HI_U8 aRecvbuf[4];

    HI_U32 u32SnsI2cAddr = (sensor_gc2053_i2c_addr >> 1);
    struct i2c_rdwr_ioctl_data stRdwr;
    struct i2c_msg astMsg[2];
    memset(&stRdwr, 0x0, sizeof(stRdwr));
    memset(astMsg, 0x0, sizeof(astMsg));
    memset(aRecvbuf, 0x0, sizeof(aRecvbuf));

    astMsg[0].addr = u32SnsI2cAddr;
    astMsg[0].flags = 0;
    astMsg[0].len = u32RegWidth;
    astMsg[0].buf = aRecvbuf;

    astMsg[1].addr = u32SnsI2cAddr;
    astMsg[1].flags = 0;
    astMsg[1].flags |= I2C_M_RD;
    astMsg[1].len = u32DataWidth;
    astMsg[1].buf = aRecvbuf;
    stRdwr.msgs = &astMsg[0];
    stRdwr.nmsgs = 2;

    if (u32RegWidth == 2) {
        aRecvbuf[0] = (addr >> 8) & 0xff;
        aRecvbuf[1] = addr & 0xff;
    } else {
        aRecvbuf[0] = addr & 0xff;
    }
    s32Ret = ioctl(g_i2cfd, I2C_RDWR, &stRdwr);

    if (s32Ret < 0) {
        printf("i2c read error\n");
        return HI_FAILURE;
    }
    if (u32DataWidth == 2) {
        s32RegVal = aRecvbuf[0] | (aRecvbuf[1] << 8);
    } else {
        s32RegVal = aRecvbuf[0];
    }
    return s32RegVal;
}
#endif

HI_U32 gc2053_ReadSnsExp(HI_VOID)
{
    HI_U32 u32SnsExp = 0;

    //bit [5:0]
#define PDT_GC2053_EXP_H_ADDR       (0x03)
    //bit [7:0]
#define PDT_GC2053_EXP_L_ADDR       (0x04)

#ifdef __HuaweiLite__
    VI_PIPE ViPipe = 0;
    extern ISP_SNS_OBJ_S stSnsGc2053Obj;
    const ISP_SNS_OBJ_S* pstIspObj = &stSnsGc2053Obj;
    u32SnsExp = ((pstIspObj->pfnReadReg(ViPipe, PDT_GC2053_EXP_H_ADDR) & 0x0000003F)<<8);
    u32SnsExp |= (pstIspObj->pfnReadReg(ViPipe, PDT_GC2053_EXP_L_ADDR) & 0x000000FF);
#else
    u32SnsExp = ((gc2053_read_registers( PDT_GC2053_EXP_H_ADDR) & 0x0000003F)<<8);
    u32SnsExp |= (gc2053_read_registers( PDT_GC2053_EXP_L_ADDR) & 0x000000FF);
#endif
    return u32SnsExp;
}

HI_VOID gc2053_ReadSnsGain(HI_U32* pu32SnsAgain, HI_U32* pu32SnsDgain)
{
    HI_U32 u32SnsAgain = 0;
    HI_U32 u32SnsDgain = 0;


//bit [7:0]
#define PDT_GC2053_AGAIN_L_ADDR     (0xb3)
//bit [1:0]
#define PDT_GC2053_AGAIN_H_ADDR     (0xb4)
//bit[5:0]
#define PDT_GC2053_DGAIN_H_ADDR     (0xb8)
//bit[5:0]
#define PDT_GC2053_DGAIN_L_ADDR     (0xb9)
#ifdef __HuaweiLite__
    VI_PIPE ViPipe = 0;
    extern ISP_SNS_OBJ_S stSnsGc2053Obj;
    const ISP_SNS_OBJ_S* pstIspObj = &stSnsGc2053Obj;
    u32SnsAgain = (pstIspObj->pfnReadReg(ViPipe, PDT_GC2053_AGAIN_H_ADDR) & 0x00000003) << 8;
    u32SnsAgain |= (pstIspObj->pfnReadReg(ViPipe, PDT_GC2053_AGAIN_L_ADDR) & 0x000000FF);
    u32SnsDgain = (pstIspObj->pfnReadReg(ViPipe, PDT_GC2053_DGAIN_H_ADDR) & 0x0000003F) << 8;
    u32SnsDgain |= (pstIspObj->pfnReadReg(ViPipe, PDT_GC2053_DGAIN_L_ADDR) & 0x0000003F);
#else
    u32SnsAgain = (gc2053_read_registers( PDT_GC2053_AGAIN_H_ADDR) & 0x00000003) << 8;
    u32SnsAgain |= (gc2053_read_registers( PDT_GC2053_AGAIN_L_ADDR) & 0x000000FF);
    u32SnsDgain = (gc2053_read_registers( PDT_GC2053_DGAIN_H_ADDR) & 0x0000003F) << 8;
    u32SnsDgain |= (gc2053_read_registers( PDT_GC2053_DGAIN_L_ADDR) & 0x0000003F);
#endif
    *pu32SnsAgain = u32SnsAgain;
    *pu32SnsDgain = u32SnsDgain;
}
