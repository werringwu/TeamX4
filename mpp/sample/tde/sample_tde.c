#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#ifdef __HuaweiLite__
#include <fb.h>
#else
#include <linux/fb.h>
#endif
#include <sys/mman.h>
#include <fcntl.h>
#include <signal.h>

#include "hi_tde_api.h"
#include "hi_tde_type.h"
#include "hi_tde_errcode.h"
#include "hifb.h"
#include "hi_type.h"
#include "hi_comm_vo.h"
#include "mpi_sys.h"
#include "mpi_vo.h"
#include "sample_comm.h"


#define TDE_PRINT printf
#define VoDev 0
#define VoChn 0
#define MIN(x,y) ((x) > (y) ? (y) : (x))
#define MAX(x,y) ((x) > (y) ? (x) : (y))

#ifdef __HuaweiLite__
static const HI_CHAR *pszImageNames[] =
{
    "/nfs/res/apple.bits",
    "/nfs/res/applets.bits",
    "/nfs/res/calendar.bits",
    "/nfs/res/foot.bits",
    "/nfs/res/gmush.bits",
    "/nfs/res/gimp.bits",
    "/nfs/res/gsame.bits",
    "/nfs/res/keys.bits"
};
#define BACKGROUND_NAME  "/nfs/res/background.bits"
#else
static const HI_CHAR *pszImageNames[] =
{
    "res/apple.bits",
    "res/applets.bits",
    "res/calendar.bits",
    "res/foot.bits",
    "res/gmush.bits",
    "res/gimp.bits",
    "res/gsame.bits",
    "res/keys.bits"
};
#define BACKGROUND_NAME  "res/background.bits"
#endif

#define N_IMAGES (HI_S32)((sizeof (pszImageNames) / sizeof (pszImageNames[0])))



#define PIXFMT  TDE2_COLOR_FMT_ARGB1555
#define BPP     2
#define SCREEN_WIDTH    640
#define SCREEN_HEIGHT   480
#define CYCLE_LEN       60

static HI_S32   g_s32FrameNum;
static TDE2_SURFACE_S g_stScreen[2];
static TDE2_SURFACE_S g_stBackGround;
static TDE2_SURFACE_S g_stImgSur[N_IMAGES];
HI_S32 g_s32Fd = -1;
HI_U32 g_u32Size;
HI_U8* g_pu8Screen = NULL;
HI_U8* g_pu8BackGroundVir = NULL;


int IntType = 0;

/******************************************************************************
* function : to process abnormal case
******************************************************************************/
void SAMPLE_TDE_HandleSig(HI_S32 signo)
{
    if (SIGINT == signo || SIGTERM == signo)
    {
        if (NULL != g_pu8Screen)
        {
            (HI_VOID)munmap(g_pu8Screen, g_u32Size);
            g_pu8Screen = NULL;
        }

        if (NULL != g_pu8BackGroundVir)
        {
            HI_MPI_SYS_MmzFree(g_stBackGround.PhyAddr, g_pu8BackGroundVir);
            g_pu8BackGroundVir = NULL;
        }

        if (g_s32Fd != -1)
        {
            close(g_s32Fd);
            g_s32Fd = -1;
        }

        HI_TDE2_Close();

        HI_MPI_VO_Disable(VoDev);
        SAMPLE_COMM_SYS_Exit();
        printf("\033[0;31mprogram termination abnormally!\033[0;39m\n");
    }
    exit(-1);
}

static HI_S32 TDE_CreateSurfaceByFile(const HI_CHAR *pszFileName, TDE2_SURFACE_S *pstSurface, HI_U8 *pu8Virt)
{
    FILE *fp;
    HI_U32 colorfmt, w, h, stride;
    HI_U64 packagelen;

    if((NULL == pszFileName) || (NULL == pstSurface))
    {
        TDE_PRINT("%s, LINE %d, NULL ptr!\n", __FUNCTION__, __LINE__);
        return -1;
    }

    fp = fopen(pszFileName, "rb");
    if(NULL == fp)
    {
        TDE_PRINT("error when open pszFileName %s, line:%d\n", pszFileName, __LINE__);
        return -1;
    }

    if (4 != fread(&colorfmt, 1, 4, fp))
    {
        TDE_PRINT("error when read pszFileName %s, line:%d\n", pszFileName, __LINE__);
        fclose(fp);
        return -1;
    }
    if (4 != fread(&w, 1, 4, fp))
    {
        TDE_PRINT("error when read pszFileName %s, line:%d\n", pszFileName, __LINE__);
        fclose(fp);
        return -1;
    }
    if (4 != fread(&h, 1, 4, fp))
    {
        TDE_PRINT("error when read pszFileName %s, line:%d\n", pszFileName, __LINE__);
        fclose(fp);
        return -1;
    }
    if (4 != fread(&stride, 1, 4, fp))
    {
        TDE_PRINT("error when read pszFileName %s, line:%d\n", pszFileName, __LINE__);
        fclose(fp);
        return -1;
    }

    pstSurface->enColorFmt = colorfmt;
    pstSurface->u32Width = w;
    pstSurface->u32Height = h;
    pstSurface->u32Stride = stride;
    pstSurface->u8Alpha0 = 0xff;
    pstSurface->u8Alpha1 = 0xff;
    pstSurface->bAlphaMax255 = HI_TRUE;
    pstSurface->bAlphaExt1555 = HI_TRUE;

    packagelen = (HI_U64)stride * (HI_U64)h;
    if ( packagelen > 0x7FFFFFFF)
    {
        TDE_PRINT("stride * h not valid: %d %d, line:%d\n", stride, h, __LINE__);
        fclose(fp);
        return -1;
    }

    fread(pu8Virt, 1, stride*h, fp);

    fclose(fp);

    return 0;
}

static HI_VOID circumrotate (HI_U32 u32CurOnShow)
{
    TDE_HANDLE s32Handle;
    TDE2_OPT_S stOpt = {0};
    HI_FLOAT eXMid, eYMid;
    HI_FLOAT eRadius;
    HI_U32 i;
    HI_FLOAT f;
    HI_U32 u32NextOnShow;
    TDE2_RECT_S stSrcRect;
    TDE2_RECT_S stDstRect;
    HI_S32 s32Ret = HI_SUCCESS;

    u32NextOnShow = !u32CurOnShow;

    stOpt.enOutAlphaFrom = TDE2_COLORKEY_MODE_FOREGROUND;
    stOpt.unColorKeyValue.struCkARGB.stRed.u8CompMask = 0xff;
    stOpt.unColorKeyValue.struCkARGB.stGreen.u8CompMask = 0xff;
    stOpt.unColorKeyValue.struCkARGB.stBlue.u8CompMask = 0xff;
    stOpt.enColorKeyMode = TDE2_COLORKEY_MODE_FOREGROUND;
    stOpt.unColorKeyValue.struCkARGB.stAlpha.bCompIgnore = HI_TRUE;

    f = (float) (g_s32FrameNum % CYCLE_LEN) / CYCLE_LEN;

    stSrcRect.s32Xpos = 0;
    stSrcRect.s32Ypos = 0;
    stSrcRect.u32Width = 640;
    stSrcRect.u32Height = 480;

    eXMid = g_stBackGround.u32Width/2.16f;
    eYMid = g_stBackGround.u32Height/2.304f;

    eRadius = MIN (eXMid, eYMid) / 2.0f;

    /* 1. start job */
    s32Handle = HI_TDE2_BeginJob();
    if(HI_ERR_TDE_INVALID_HANDLE == s32Handle)
    {
        TDE_PRINT("start job failed!\n");
        return ;
    }

    /* 2. bitblt background to screen */
    s32Ret = HI_TDE2_QuickCopy(s32Handle, &g_stBackGround, &stSrcRect,
        &g_stScreen[u32NextOnShow], &stSrcRect);
    if(s32Ret < 0)
    {
        TDE_PRINT("Line:%d failed,ret=0x%x!\n", __LINE__, s32Ret);
        HI_TDE2_CancelJob(s32Handle);
        return ;
    }

    for(i = 0; i < N_IMAGES; i++)
    {
        HI_FLOAT ang;
        HI_FLOAT r;

        stSrcRect.s32Xpos = 0;
        stSrcRect.s32Ypos = 0;
        stSrcRect.u32Width = g_stImgSur[i].u32Width;
        stSrcRect.u32Height = g_stImgSur[i].u32Height;

        /* 3. calculate new pisition */
        ang = 2.0f * (HI_FLOAT) M_PI * (HI_FLOAT) i / N_IMAGES - f * 2.0f * (HI_FLOAT) M_PI;
        r = eRadius + (eRadius / 3.0f) * sinf (f * 2.0 * M_PI);

        stDstRect.s32Xpos = eXMid + r * cosf (ang) - g_stImgSur[i].u32Width / 2.0f;;
        stDstRect.s32Ypos = eYMid + r * sinf (ang) - g_stImgSur[i].u32Height / 2.0f;
        stDstRect.u32Width = g_stImgSur[i].u32Width;
        stDstRect.u32Height = g_stImgSur[i].u32Height;

        /* 4. bitblt image to screen */
        s32Ret = HI_TDE2_Bitblit(s32Handle, &g_stScreen[u32NextOnShow], &stDstRect,
            &g_stImgSur[i], &stSrcRect, &g_stScreen[u32NextOnShow], &stDstRect, &stOpt);
        if(s32Ret < 0)
        {
        	TDE_PRINT("Line:%d,HI_TDE2_Bitblit failed,ret=0x%x!\n", __LINE__, s32Ret);
        	HI_TDE2_CancelJob(s32Handle);
        	return ;
        }
    }

    /* 5. submit job */
    s32Ret = HI_TDE2_EndJob(s32Handle, HI_FALSE, HI_TRUE, 1000);
    if(s32Ret < 0)
    {
        TDE_PRINT("Line:%d,HI_TDE2_EndJob failed,ret=0x%x!\n", __LINE__, s32Ret);
        HI_TDE2_CancelJob(s32Handle);
        return ;
    }

    g_s32FrameNum++;
    return;
}

HI_S32 TDE_DrawGraphicSample(HI_VOID)
{
    HI_U32 u32Times;
    HI_U32 u32PhyAddr;
    HI_S32 s32Ret = -1;
    HI_U32 i = 0;
    HI_BOOL bShow;
    HIFB_ALPHA_S stAlpha = {0};
#ifndef __HuaweiLite__
    struct fb_fix_screeninfo stFixInfo;
    struct fb_var_screeninfo stVarInfo;
    struct fb_bitfield stR32 = {10, 5, 0};
    struct fb_bitfield stG32 = {5, 5, 0};
    struct fb_bitfield stB32 = {0, 5, 0};
    struct fb_bitfield stA32 = {15, 1, 0};
#endif
    /* 1. open tde device */
    s32Ret = HI_TDE2_Open();
    if (HI_SUCCESS != s32Ret)
    {
        TDE_PRINT("HI_TDE2_Open failed:0x%x\n", s32Ret);
        return s32Ret;
    }

    /* 2. framebuffer operation */
    g_s32Fd = open("/dev/fb0", O_RDWR);
    if (g_s32Fd < 0)
    {
        TDE_PRINT("open frame buffer device error\n");
        goto FB_OPEN_ERROR;
    }

    stAlpha.bAlphaChannel = HI_FALSE;
    stAlpha.bAlphaEnable = HI_FALSE;
    if (ioctl(g_s32Fd, FBIOPUT_ALPHA_HIFB, &stAlpha) < 0)
    {
        TDE_PRINT("Put alpha info failed!\n");
        goto FB_PROCESS_ERROR0;
    }
#ifndef __HuaweiLite__
    /* get the variable screen info */
    if (ioctl(g_s32Fd, FBIOGET_VSCREENINFO, &stVarInfo) < 0)
    {
        TDE_PRINT("Get variable screen info failed!\n");
        goto FB_PROCESS_ERROR0;
    }

    stVarInfo.xres_virtual	 	= SCREEN_WIDTH;
    stVarInfo.yres_virtual		= SCREEN_HEIGHT*2;
    stVarInfo.xres      		= SCREEN_WIDTH;
    stVarInfo.yres      		= SCREEN_HEIGHT;
    stVarInfo.activate  		= FB_ACTIVATE_NOW;
    stVarInfo.bits_per_pixel	= 16;
    stVarInfo.xoffset = 0;
    stVarInfo.yoffset = 0;
    stVarInfo.red   = stR32;
    stVarInfo.green = stG32;
    stVarInfo.blue  = stB32;
    stVarInfo.transp = stA32;

    if (ioctl(g_s32Fd, FBIOPUT_VSCREENINFO, &stVarInfo) < 0)
    {
        TDE_PRINT("FBIOPUT_VSCREENINFO error\n");
        goto FB_PROCESS_ERROR0;
    }

    if (ioctl(g_s32Fd, FBIOGET_FSCREENINFO, &stFixInfo) < 0)
    {
        TDE_PRINT("FBIOGET_FSCREENINFO error\n");
        goto FB_PROCESS_ERROR0;
    }

    g_u32Size   = stFixInfo.smem_len;
    u32PhyAddr  = stFixInfo.smem_start;
    g_pu8Screen   = mmap(NULL, g_u32Size, PROT_READ|PROT_WRITE, MAP_SHARED, g_s32Fd, 0);
    if (NULL == g_pu8Screen)
    {
        TDE_PRINT("mmap fb0 failed!\n");
        goto FB_PROCESS_ERROR0;
    }

    memset_s(g_pu8Screen, stFixInfo.smem_len, 0x00, stFixInfo.smem_len);

    g_stScreen[0].u32Stride = stFixInfo.line_length;
#else
        struct hifb_info info;
    if (ioctl(g_s32Fd, FBIOGET_SCREENINFO_HIFB, &info) < 0)
    {
        SAMPLE_PRT("FBIOGET_SCREENINFO_HIFB failed!\n");
        close(g_s32Fd);
        return HI_NULL;
    }
    info.vinfo.xres = SCREEN_WIDTH;
    info.vinfo.yres = SCREEN_HEIGHT;
    info.oinfo.sarea.w = SCREEN_WIDTH;
    info.oinfo.sarea.h = SCREEN_HEIGHT;
    info.oinfo.bpp = 16;
    info.activate = 0;
    info.vinfo.fmt = HIFB_FMT_ARGB1555;

    if (ioctl(g_s32Fd, FBIOPUT_SCREENINFO_HIFB, &info) < 0)
    {
        TDE_PRINT("Put variable screen info failed!\n");
        goto FB_PROCESS_ERROR0;
    }

    if (ioctl(g_s32Fd, FBIOGET_SCREENINFO_HIFB, &info) < 0)
    {
        TDE_PRINT("Get fix screen info failed!\n");
        goto FB_PROCESS_ERROR0;
    }

    g_u32Size   = info.oinfo.fblen;
    u32PhyAddr  = (HI_U32)info.oinfo.fbmem;
    g_pu8Screen = (HI_U8*)(info.oinfo.fbmem);;

    memset_s(g_pu8Screen, g_u32Size, 0x00, g_u32Size);
    g_stScreen[0].u32Stride = info.oinfo.stride;
#endif
    /* 3. create surface */
    g_stScreen[0].enColorFmt = PIXFMT;
    g_stScreen[0].PhyAddr = u32PhyAddr;
    g_stScreen[0].u32Width = SCREEN_WIDTH;
    g_stScreen[0].u32Height = SCREEN_HEIGHT;

    g_stScreen[0].bAlphaMax255 = HI_TRUE;

    g_stScreen[1] = g_stScreen[0];
    g_stScreen[1].PhyAddr = g_stScreen[0].PhyAddr + (HI_U64)g_stScreen[0].u32Stride * (HI_U64)g_stScreen[0].u32Height;

    /* allocate memory (720*576*2*N_IMAGES bytes) to save Images' infornation */
    if (HI_FAILURE == HI_MPI_SYS_MmzAlloc(&(g_stBackGround.PhyAddr), ((void**)&g_pu8BackGroundVir),
            NULL, NULL, 720*576*2*N_IMAGES))
    {
        TDE_PRINT("allocate memory (720*576*2*N_IMAGES bytes) failed\n");
        goto FB_PROCESS_ERROR1;
    }
    TDE_CreateSurfaceByFile(BACKGROUND_NAME, &g_stBackGround, g_pu8BackGroundVir);

    g_stImgSur[0].PhyAddr = g_stBackGround.PhyAddr + (HI_U64)g_stBackGround.u32Stride * (HI_U64)g_stBackGround.u32Height;
    for(i = 0; i < N_IMAGES - 1; i++)
    {
        TDE_CreateSurfaceByFile(pszImageNames[i], &g_stImgSur[i],
            g_pu8BackGroundVir + ((HI_U32)g_stImgSur[i].PhyAddr - g_stBackGround.PhyAddr));
        g_stImgSur[i+1].PhyAddr = g_stImgSur[i].PhyAddr + (HI_U64)g_stImgSur[i].u32Stride * (HI_U64)g_stImgSur[i].u32Height;
    }
    TDE_CreateSurfaceByFile(pszImageNames[i], &g_stImgSur[i],
            g_pu8BackGroundVir + ((HI_U32)g_stImgSur[i].PhyAddr - g_stBackGround.PhyAddr));

    bShow = HI_TRUE;
    if (ioctl(g_s32Fd, FBIOPUT_SHOW_HIFB, &bShow) < 0)
    {
        fprintf (stderr, "Couldn't show fb\n");
        goto FB_PROCESS_ERROR2;
    }

    g_s32FrameNum = 0;

    /* 3. use tde and framebuffer to realize rotational effect */
    for (u32Times = 0; u32Times < 20; u32Times++)
    {
        circumrotate(u32Times%2);
#ifndef __HuaweiLite__
        stVarInfo.yoffset = (u32Times%2)?0:480;

        /*set frame buffer start position*/
        if (ioctl(g_s32Fd, FBIOPAN_DISPLAY, &stVarInfo) < 0)
        {
            TDE_PRINT("FBIOPAN_DISPLAY error\n");
            goto FB_PROCESS_ERROR2;
        }
#else
        info.oinfo.sarea.y = (u32Times%2)?0:480;

        /*set frame buffer start position*/
        if (ioctl(g_s32Fd, FBIOPAN_DISPLAY_HIFB, &info.oinfo) < 0)
        {
            TDE_PRINT("FBIOPAN_DISPLAY error\n");
            goto FB_PROCESS_ERROR2;
        }

#endif
        sleep(1);
    }
    TDE_PRINT("exit\n");
    s32Ret = 0;

FB_PROCESS_ERROR2:
    HI_MPI_SYS_MmzFree(g_stBackGround.PhyAddr, g_pu8BackGroundVir);
    g_pu8BackGroundVir = NULL;
FB_PROCESS_ERROR1:
#ifndef __HuaweiLite__
    (HI_VOID)munmap(g_pu8Screen, g_u32Size);
#endif

    g_pu8Screen = NULL;
FB_PROCESS_ERROR0:
    close(g_s32Fd);
    g_s32Fd = -1;
FB_OPEN_ERROR:
    HI_TDE2_Close();

    return s32Ret;
}

/*****************************************************************************
description: 	this sample shows how to use TDE interface to draw graphic.

note	   :    for showing graphic layer, VO device should be enabled first.
				This sample draws graphic on layer G0 which belongs to HD VO device.
				(here we insmod HiFB.ko like 'insmod hifb.ko video="hifb:vram0_size=XXX" '
				 so opening hifb sub-device '/dev/fb0' means to opening G0)
*****************************************************************************/
#ifdef __HuaweiLite__
#define SAMPLE_HIFB_NAME "sample"
int app_main(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
    HI_S32 s32Ret = 0;
    SAMPLE_VO_CONFIG_S      stVoConfig;

    VB_CONFIG_S stVbConf = {0};

#ifdef __HuaweiLite__
#else
    signal(SIGINT, SAMPLE_TDE_HandleSig);
    signal(SIGTERM, SAMPLE_TDE_HandleSig);
#endif



    stVbConf.u32MaxPoolCnt             = 16;
    /*no need common video buffer!*/

    /*1 enable Vo device HD first*/
    if(HI_SUCCESS != SAMPLE_COMM_SYS_Init(&stVbConf))
    {
        return -1;
    }
    s32Ret = SAMPLE_COMM_VO_GetDefConfig(&stVoConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VO_GetDefConfig failed with %d!\n", s32Ret);
        return -1;
    }
    stVoConfig.enDstDynamicRange = DYNAMIC_RANGE_SDR8;
    stVoConfig.enVoIntfType = VO_INTF_BT1120;
    stVoConfig.enIntfSync   = VO_OUTPUT_1080P25;
    s32Ret = SAMPLE_COMM_VO_StartVO(&stVoConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VO_StartVO failed with %d!\n", s32Ret);
        goto err0;
    }

    /*2 run tde sample which draw grahpic on HiFB memory*/
    s32Ret = TDE_DrawGraphicSample();
    if(s32Ret != HI_SUCCESS)
    {
        goto err1;
    }

err1:
    SAMPLE_COMM_VO_StopVO(&stVoConfig);
err0:
    SAMPLE_COMM_SYS_Exit();
    return 0;
}


