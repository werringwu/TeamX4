
/******************************************************************************

  Copyright (C), 2016, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : sc2231_sensor_ctl.c

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
#include "hi_comm_video.h"
#include "hi_sns_ctrl.h"

#ifdef HI_GPIO_I2C
#include "gpioi2c_ex.h"
#else

#ifdef __HuaweiLite__
#include "i2c.h"
#else
#include <linux/fb.h>
#include "quick_i2c-dev.h"
#endif

#include "hi_i2c.h"
#endif

#include "mpi_isp.h"
const unsigned char sensor_sc2231_i2c_addr     =    0x60;        /* I2C Address of sc2231 */
const unsigned int  sensor_sc2231_addr_byte    =    2;
const unsigned int  sensor_sc2231_data_byte    =    1;
static int g_i2cfd = -1;

#define I2C_DEV_SENSOR (0)

int sensor_sc2231_i2c_init(VI_PIPE ViPipe)
{
    char acDevFile[16] = {0};
    HI_U8 u8DevNum;

    if (g_i2cfd >= 0)
    {
        printf("sensor i2c fd already be created\n");
        return HI_SUCCESS;
    }
#ifdef HI_GPIO_I2C
    int ret;

    g_i2cfd = open("/dev/gpioi2c_ex", O_RDONLY, S_IRUSR);
    if (g_i2cfd < 0)
    {
        printf( "Open gpioi2c_ex error!\n");
        return HI_FAILURE;
    }
#else
    int ret;

    u8DevNum = I2C_DEV_SENSOR;
    snprintf(acDevFile, sizeof(acDevFile),  "/dev/i2c-%u", u8DevNum);

    g_i2cfd = open(acDevFile, O_RDWR, S_IRUSR | S_IWUSR);

    if (g_i2cfd < 0)
    {
        printf( "Open /dev/hi_i2c_drv-%u error!\n", u8DevNum);
        return HI_FAILURE;
    }

    ret = ioctl(g_i2cfd, I2C_SLAVE_FORCE, (sensor_sc2231_i2c_addr >> 1));
    if (ret < 0)
    {
        printf( "I2C_SLAVE_FORCE error!\n");
        close(g_i2cfd);
        g_i2cfd = -1;
        return ret;
    }
#endif

    return HI_SUCCESS;
}

int sensor_sc2231_i2c_exit(VI_PIPE ViPipe)
{
    if (g_i2cfd >= 0)
    {
        close(g_i2cfd);
        g_i2cfd = -1;
        return HI_SUCCESS;
    }
    return HI_FAILURE;
}

int sensor_sc2231_write_register(VI_PIPE ViPipe, HI_U32 addr, HI_U32 data)
{
    if (0 > g_i2cfd)
    {
        printf( "invalid i2c fd\n");
        return HI_FAILURE;
    }

#ifdef HI_GPIO_I2C
    i2c_data.dev_addr = sensor_sc2231_i2c_addr;
    i2c_data.reg_addr = addr;
    i2c_data.addr_byte_num = sensor_sc2231_addr_byte;
    i2c_data.data = data;
    i2c_data.data_byte_num = sensor_sc2231_data_byte;

    ret = ioctl(g_i2cfd, GPIO_I2C_WRITE, &i2c_data);

    if (ret)
    {
        printf( "GPIO-I2C write faild!\n");
        return ret;
    }
#else
    int idx = 0;
    int ret;
    char buf[8];

    if (sensor_sc2231_addr_byte == 2)
    {
        buf[idx] = (addr >> 8) & 0xff;
        idx++;
        buf[idx] = addr & 0xff;
        idx++;
    }
    else
    {
        //buf[idx] = addr & 0xff;
        //idx++;
    }

    if (sensor_sc2231_data_byte == 2)
    {
        //buf[idx] = (data >> 8) & 0xff;
        //idx++;
        //buf[idx] = data & 0xff;
        //idx++;
    }
    else
    {
        buf[idx] = data & 0xff;
        idx++;
    }

    ret = write(g_i2cfd, buf, sensor_sc2231_addr_byte + sensor_sc2231_data_byte);
    if (ret < 0)
    {
        printf( "I2C_WRITE error!\n");
        return HI_FAILURE;
    }

#endif
    return HI_SUCCESS;
}

static void sensor_sc2231_linear_1080P30_init(VI_PIPE ViPipe);

static HI_VOID sc2231_writeSnsExp(HI_S32 s32SnsExp)
{
    VI_PIPE ViPipe = 0;
    HI_S32 s32RegValue = 0;

    //bit [3:0]
#define PDT_SC2231_EXP_H_ADDR       (0x3e00)
    //bit [7:0]
#define PDT_SC2231_EXP_M_ADDR       (0x3e01)
    //bit [7:4]
#define PDT_SC2231_EXP_L_ADDR       (0x3e02)

    s32RegValue = ((s32SnsExp>>12)&0x0000000F);
    sensor_sc2231_write_register(ViPipe, PDT_SC2231_EXP_H_ADDR, s32RegValue);

    s32RegValue = ((s32SnsExp>>4)&0x000000FF);
    sensor_sc2231_write_register(ViPipe, PDT_SC2231_EXP_M_ADDR, s32RegValue);

    s32RegValue = ((s32SnsExp&0x0000000F) << 4);
    sensor_sc2231_write_register(ViPipe, PDT_SC2231_EXP_L_ADDR, s32RegValue);
}

static HI_VOID sc2231_writeSnsGain(HI_S32 s32SnsAgain, HI_S32 s32SnsDgain)
{
    VI_PIPE ViPipe = 0;
    HI_S32 s32RegValue = 0;

#define PDT_SC2231_AGAIN_H_ADDR     (0x3e08)
#define PDT_SC2231_AGAIN_L_ADDR     (0x3e09)
#define PDT_SC2231_DGAIN_H_ADDR     (0x3e06)
#define PDT_SC2231_DGAIN_L_ADDR     (0x3e07)

    s32RegValue = ((s32SnsAgain>>8)&0x000000FF);
    sensor_sc2231_write_register(ViPipe, PDT_SC2231_AGAIN_H_ADDR, s32RegValue);

    s32RegValue = ((s32SnsAgain)&0x000000FF);
    sensor_sc2231_write_register(ViPipe, PDT_SC2231_AGAIN_L_ADDR, s32RegValue);

    s32RegValue = ((s32SnsDgain>>8)&0x000000FF);
    sensor_sc2231_write_register(ViPipe, PDT_SC2231_DGAIN_H_ADDR, s32RegValue);

    s32RegValue = ((s32SnsDgain)&0x000000FF);
    sensor_sc2231_write_register(ViPipe, PDT_SC2231_DGAIN_L_ADDR, s32RegValue);
}

void sc2231_sensor_init(VI_PIPE ViPipe, HI_S32 s32SnsExp, HI_S32 s32SnsAgain, HI_S32 s32SnsDgain)
{
    sensor_sc2231_i2c_init(ViPipe);

    sensor_sc2231_linear_1080P30_init(ViPipe);
    usleep(10*1000);

    sc2231_writeSnsExp(s32SnsExp);
    sc2231_writeSnsGain(s32SnsAgain, s32SnsDgain);

    sensor_sc2231_write_register (ViPipe, 0x320e, 0x5);
    sensor_sc2231_write_register (ViPipe, 0x320f, 0x46);
    sensor_sc2231_write_register (ViPipe, 0x3812, 0x00);
    sensor_sc2231_write_register (ViPipe, 0x3630, 0x40);
    sensor_sc2231_write_register (ViPipe, 0x3632, 0x88);
    sensor_sc2231_write_register (ViPipe, 0x3812, 0x30);
    return;
}

void sc2231_sensor_deinit(VI_PIPE ViPipe)
{
    sensor_sc2231_i2c_exit(ViPipe);

    return;
}

/***************** 1080P30 (1920X1080@30fps) ***************/
static void sensor_sc2231_linear_1080P30_init(VI_PIPE ViPipe)
{
    sensor_sc2231_write_register (ViPipe, 0x0103, 0x01);
    sensor_sc2231_write_register (ViPipe, 0x0100, 0x00);
    sensor_sc2231_write_register (ViPipe, 0x3000, 0x00);
    sensor_sc2231_write_register (ViPipe, 0x3001, 0x00);
    sensor_sc2231_write_register (ViPipe, 0x3002, 0x80);
    sensor_sc2231_write_register (ViPipe, 0x3018, 0x33);
    sensor_sc2231_write_register (ViPipe, 0x301a, 0xf0);
    sensor_sc2231_write_register (ViPipe, 0x301c, 0x78);
    sensor_sc2231_write_register (ViPipe, 0x3461, 0x0d);
    sensor_sc2231_write_register (ViPipe, 0x3037, 0x20);
    sensor_sc2231_write_register (ViPipe, 0x3038, 0x6e);
    sensor_sc2231_write_register (ViPipe, 0x3039, 0x52);
    sensor_sc2231_write_register (ViPipe, 0x303b, 0x14);
    sensor_sc2231_write_register (ViPipe, 0x303d, 0x10);
    sensor_sc2231_write_register (ViPipe, 0x303f, 0x01);
    sensor_sc2231_write_register (ViPipe, 0x3461, 0x01);
    sensor_sc2231_write_register (ViPipe, 0x3221, 0x80);
    sensor_sc2231_write_register (ViPipe, 0x3301, 0x08);
    sensor_sc2231_write_register (ViPipe, 0x3303, 0x38);
    sensor_sc2231_write_register (ViPipe, 0x3306, 0x50);
    sensor_sc2231_write_register (ViPipe, 0x3309, 0x70);
    sensor_sc2231_write_register (ViPipe, 0x330a, 0x00);
    sensor_sc2231_write_register (ViPipe, 0x330b, 0xd0);
    sensor_sc2231_write_register (ViPipe, 0x330d, 0x36);
    sensor_sc2231_write_register (ViPipe, 0x330e, 0x18);
    sensor_sc2231_write_register (ViPipe, 0x330f, 0x01);
    sensor_sc2231_write_register (ViPipe, 0x3310, 0x23);
    sensor_sc2231_write_register (ViPipe, 0x3314, 0x14);
    sensor_sc2231_write_register (ViPipe, 0x331e, 0x31);
    sensor_sc2231_write_register (ViPipe, 0x331f, 0x69);
    sensor_sc2231_write_register (ViPipe, 0x3338, 0x37);
    sensor_sc2231_write_register (ViPipe, 0x3339, 0x37);
    sensor_sc2231_write_register (ViPipe, 0x333a, 0x33);
    sensor_sc2231_write_register (ViPipe, 0x335d, 0x20);
    sensor_sc2231_write_register (ViPipe, 0x3364, 0x1d);
    sensor_sc2231_write_register (ViPipe, 0x3367, 0x08);
    sensor_sc2231_write_register (ViPipe, 0x33aa, 0x00);
    sensor_sc2231_write_register (ViPipe, 0x33ae, 0x32);
    sensor_sc2231_write_register (ViPipe, 0x33b3, 0x32);
    sensor_sc2231_write_register (ViPipe, 0x33b4, 0x32);
    sensor_sc2231_write_register (ViPipe, 0x33b6, 0x0f);
    sensor_sc2231_write_register (ViPipe, 0x33b7, 0x3e);
    sensor_sc2231_write_register (ViPipe, 0x33b8, 0x08);
    sensor_sc2231_write_register (ViPipe, 0x33b9, 0x80);
    sensor_sc2231_write_register (ViPipe, 0x33ba, 0xc0);
    sensor_sc2231_write_register (ViPipe, 0x360f, 0x05);
    sensor_sc2231_write_register (ViPipe, 0x3614, 0x80);
    sensor_sc2231_write_register (ViPipe, 0x3622, 0xf6);
    sensor_sc2231_write_register (ViPipe, 0x3630, 0x40);
    sensor_sc2231_write_register (ViPipe, 0x3631, 0x88);
    sensor_sc2231_write_register (ViPipe, 0x3632, 0x88);
    sensor_sc2231_write_register (ViPipe, 0x3633, 0x24);
    sensor_sc2231_write_register (ViPipe, 0x3635, 0x1c);
    sensor_sc2231_write_register (ViPipe, 0x3637, 0x2c);
    sensor_sc2231_write_register (ViPipe, 0x3638, 0x24);
    sensor_sc2231_write_register (ViPipe, 0x363a, 0x80);
    sensor_sc2231_write_register (ViPipe, 0x363b, 0x16);
    sensor_sc2231_write_register (ViPipe, 0x363c, 0x06);
    sensor_sc2231_write_register (ViPipe, 0x366e, 0x04);
    sensor_sc2231_write_register (ViPipe, 0x3670, 0x48);
    sensor_sc2231_write_register (ViPipe, 0x3671, 0xf6);
    sensor_sc2231_write_register (ViPipe, 0x3672, 0x16);
    sensor_sc2231_write_register (ViPipe, 0x3673, 0x16);
    sensor_sc2231_write_register (ViPipe, 0x367a, 0x38);
    sensor_sc2231_write_register (ViPipe, 0x367b, 0x38);
    sensor_sc2231_write_register (ViPipe, 0x3690, 0x24);
    sensor_sc2231_write_register (ViPipe, 0x3691, 0x44);
    sensor_sc2231_write_register (ViPipe, 0x3692, 0x44);
    sensor_sc2231_write_register (ViPipe, 0x3699, 0x80);
    sensor_sc2231_write_register (ViPipe, 0x369a, 0x80);
    sensor_sc2231_write_register (ViPipe, 0x369b, 0x9f);
    sensor_sc2231_write_register (ViPipe, 0x369c, 0x38);
    sensor_sc2231_write_register (ViPipe, 0x369d, 0x38);
    sensor_sc2231_write_register (ViPipe, 0x36a2, 0x00);
    sensor_sc2231_write_register (ViPipe, 0x36a3, 0x3f);
    sensor_sc2231_write_register (ViPipe, 0x3902, 0xc5);
    sensor_sc2231_write_register (ViPipe, 0x391e, 0x00);
    sensor_sc2231_write_register (ViPipe, 0x3933, 0x0a);
    sensor_sc2231_write_register (ViPipe, 0x3934, 0x00);
    sensor_sc2231_write_register (ViPipe, 0x3940, 0x69);
    sensor_sc2231_write_register (ViPipe, 0x3942, 0x02);
    sensor_sc2231_write_register (ViPipe, 0x3943, 0x18);
    sensor_sc2231_write_register (ViPipe, 0x3e09, 0x20);
    sensor_sc2231_write_register (ViPipe, 0x3e26, 0x20);
    sensor_sc2231_write_register (ViPipe, 0x3f00, 0x0f);
    sensor_sc2231_write_register (ViPipe, 0x3f04, 0x04);
    sensor_sc2231_write_register (ViPipe, 0x3f05, 0x28);
    sensor_sc2231_write_register (ViPipe, 0x4603, 0x00);
    sensor_sc2231_write_register (ViPipe, 0x4800, 0x64);
    sensor_sc2231_write_register (ViPipe, 0x4837, 0x35);
    sensor_sc2231_write_register (ViPipe, 0x5000, 0x00);
    sensor_sc2231_write_register (ViPipe, 0x5002, 0x00);
    sensor_sc2231_write_register (ViPipe, 0x5988, 0x02);
    sensor_sc2231_write_register (ViPipe, 0x598e, 0x08);
    sensor_sc2231_write_register (ViPipe, 0x598f, 0x50);

    sensor_sc2231_write_register (ViPipe, 0x3221, 0x66); // mirror & filp

    sensor_sc2231_write_register (ViPipe, 0x0100, 0x01);

    printf("quick init sensor 2231\n");
    return;
}

int sc2231_read_registers( HI_U32 addr)
{
    HI_S32 s32RegVal = 0;
    if (g_i2cfd <  0)
    {
        printf("read register error\n");
        return 0;
    }
    HI_S32 s32Ret = 0;
    HI_U32 u32RegWidth = sensor_sc2231_addr_byte;
    HI_U32 u32DataWidth = sensor_sc2231_data_byte;
    HI_U8 aRecvbuf[4];

    HI_U32 u32SnsI2cAddr = (sensor_sc2231_i2c_addr >> 1);
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

HI_U32 sc2231_ReadSnsExp(HI_VOID)
{

    //bit [3:0]
#define PDT_SC2230_EXP_H_ADDR       (0x3e00)
    //bit [7:0]
#define PDT_SC2230_EXP_M_ADDR       (0x3e01)
    //bit [7:4]
#define PDT_SC2230_EXP_L_ADDR       (0x3e02)
    HI_U32 u32SnsExp = 0;
#ifdef __HuaweiLite__
    VI_PIPE ViPipe = 0;

    extern ISP_SNS_OBJ_S stSnsSc2231Obj;
    const ISP_SNS_OBJ_S* pstIspObj = &stSnsSc2231Obj;

    u32SnsExp = (pstIspObj->pfnReadReg(ViPipe, PDT_SC2230_EXP_H_ADDR) & 0x0000000F) << 12;

    u32SnsExp |= ((pstIspObj->pfnReadReg(ViPipe, PDT_SC2230_EXP_M_ADDR) & 0x000000FF)<<4);

    u32SnsExp |= ((pstIspObj->pfnReadReg(ViPipe, PDT_SC2230_EXP_L_ADDR) & 0x000000F0)>>4);
#else
    u32SnsExp = (sc2231_read_registers(PDT_SC2230_EXP_H_ADDR) & 0x0000000F) << 12;
    u32SnsExp |= ((sc2231_read_registers( PDT_SC2230_EXP_M_ADDR) & 0x000000FF)<<4);
    u32SnsExp |= ((sc2231_read_registers( PDT_SC2230_EXP_L_ADDR) & 0x000000F0)>>4);
#endif
    return u32SnsExp;
}


HI_VOID sc2231_ReadSnsGain(HI_U32* pu32SnsAgain, HI_U32* pu32SnsDgain)
{
    HI_U32 u32SnsAgain = 0;
    HI_U32 u32SnsDgain = 0;


#define PDT_SC2230_AGAIN_H_ADDR     (0x3e08)
#define PDT_SC2230_AGAIN_L_ADDR     (0x3e09)
#define PDT_SC2230_DGAIN_H_ADDR     (0x3e06)
#define PDT_SC2230_DGAIN_L_ADDR     (0x3e07)

#ifdef __HuaweiLite__
    VI_PIPE ViPipe = 0;
    extern ISP_SNS_OBJ_S stSnsSc2231Obj;
    const ISP_SNS_OBJ_S* pstIspObj = &stSnsSc2231Obj;
    u32SnsAgain = (pstIspObj->pfnReadReg(ViPipe, PDT_SC2230_AGAIN_H_ADDR) & 0x000000FF) << 8;
    u32SnsAgain |= (pstIspObj->pfnReadReg(ViPipe, PDT_SC2230_AGAIN_L_ADDR) & 0x000000FF);
    u32SnsDgain = (pstIspObj->pfnReadReg(ViPipe, PDT_SC2230_DGAIN_H_ADDR) & 0x000000FF) << 8;
    u32SnsDgain |= (pstIspObj->pfnReadReg(ViPipe, PDT_SC2230_DGAIN_L_ADDR) & 0x000000FF);
#else

    u32SnsAgain = (sc2231_read_registers( PDT_SC2230_AGAIN_H_ADDR) & 0x000000FF) << 8;
    u32SnsAgain |= (sc2231_read_registers( PDT_SC2230_AGAIN_L_ADDR) & 0x000000FF);
    u32SnsDgain = (sc2231_read_registers( PDT_SC2230_DGAIN_H_ADDR) & 0x000000FF) << 8;
    u32SnsDgain |= (sc2231_read_registers( PDT_SC2230_DGAIN_L_ADDR) & 0x000000FF);
#endif
    *pu32SnsAgain = u32SnsAgain;
    *pu32SnsDgain = u32SnsDgain;
}
