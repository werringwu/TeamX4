#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/prctl.h>

#include "sample_comm.h"
#include "mpi_snap.h"

#define  vi_chn_0_bmp  "./res/vi_chn_0.bmp"
#define  mm_bmp        "./res/mm.bmp"
#define  huawei_bmp    "./res/huawei.bmp"
#define  mm2_bmp       "./res/mm2.bmp"

typedef struct HiCommVbPool{
    HI_U32   rawBuffNum;
    HI_U32   picBuffNum;
}CommVbPool;

CommVbPool  g_commVbPool = {
    .rawBuffNum = 0,
    .picBuffNum = 0,
};

SAMPLE_VI_CONFIG_S g_stViConfig          = {0};
SAMPLE_VO_CONFIG_S g_stVoConfig          = {0};
VPSS_GRP_ATTR_S    g_stVpssGrpAttr       = {0};
HI_BOOL            abChnEnable[4] = {1,1,0,0};
HI_U32             g_u32DisBufLen = 3;
extern HI_CHAR*    Path_BMP;
VI_VPSS_MODE_E     g_enMastPipeMode = VI_ONLINE_VPSS_ONLINE;

/******************************************************************************
* function : show usage
******************************************************************************/
void SAMPLE_REGION_Usage(char* sPrgNm)
{
    printf("Usage : %s <index> \n", sPrgNm);
    printf("index:\n");
    printf("\t 0)VI OSDEX.\n");
    printf("\t 1)VI COVEREX.\n");
    printf("\t 2)VPSS OSDEX.\n");
    printf("\t 3)VPSS COVEREX.\n");
    printf("\t 4)VPSS COVER.\n");
    printf("\t 5)VPSS MOSAIC.\n");
    printf("\t 6)VO OSDEX.\n");
    printf("\t 7)VO COEREX.\n");
    printf("\t 8)VENC OSD Mix.\n");
    //printf("\t 9)VENC OSD.\n");
    return;
}

/******************************************************************************
* function : to process abnormal case
******************************************************************************/
void SAMPLE_REGION_HandleSig(HI_S32 signo)
{
    if (SIGINT == signo || SIGTERM == signo)
    {
        SAMPLE_COMM_All_ISP_Stop();
        SAMPLE_COMM_SYS_Exit();
        printf("\033[0;31mprogram termination abnormally!\033[0;39m\n");
    }
    exit(-1);
}

HI_S32 SAMPLE_REGION_InitSys(HI_VOID)
{
    HI_S32             s32Ret;
    SIZE_S             stSize;
    HI_U32             u32BlkSize;
    VB_CONFIG_S        stVbConf;
    PIC_SIZE_E         enPicSize ;

    SAMPLE_COMM_VI_GetSensorInfo(&g_stViConfig);

    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(g_stViConfig.astViInfo[0].stSnsInfo.enSnsType, &enPicSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size by sensor failed!\n");
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enPicSize, &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size failed!\n");
        return s32Ret;
    }

    memset(&stVbConf, 0, sizeof(VB_CONFIG_S));
    stVbConf.u32MaxPoolCnt = 2;

    u32BlkSize = VI_GetRawBufferSize(stSize.u32Width, stSize.u32Height, PIXEL_FORMAT_RGB_BAYER_12BPP, COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[0].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt   = g_commVbPool.rawBuffNum;

    u32BlkSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8, COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[1].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt   = g_commVbPool.picBuffNum;

    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        return s32Ret;
    }

    return HI_SUCCESS;
}

HI_S32 SAMPLE_REGION_StartVi(HI_VOID)
{
    HI_S32             s32Ret;
    VI_DEV             ViDev               = 0;
    VI_PIPE            ViPipe              = 0;
    VI_CHN             ViChn               = 0;
    HI_S32             s32SnsId            = 0;
    DYNAMIC_RANGE_E    enDynamicRange      = DYNAMIC_RANGE_SDR8;

    /************************************************
    step1:  config vi
    *************************************************/
    SAMPLE_COMM_VI_GetSensorInfo(&g_stViConfig);

    g_stViConfig.s32WorkingViNum                                  = 1;

    g_stViConfig.as32WorkingViId[0]                               = s32SnsId;

    g_stViConfig.astViInfo[s32SnsId].stSnsInfo.MipiDev            = ViDev;;
    g_stViConfig.astViInfo[s32SnsId].stSnsInfo.s32BusId           = 0;

    g_stViConfig.astViInfo[s32SnsId].stDevInfo.ViDev              = ViDev;
    g_stViConfig.astViInfo[s32SnsId].stDevInfo.enWDRMode          = WDR_MODE_NONE;

    g_stViConfig.astViInfo[s32SnsId].stPipeInfo.enMastPipeMode    = g_enMastPipeMode;
    g_stViConfig.astViInfo[s32SnsId].stPipeInfo.aPipe[0]          = ViPipe;
    g_stViConfig.astViInfo[s32SnsId].stPipeInfo.aPipe[1]          = -1;

    g_stViConfig.astViInfo[s32SnsId].stChnInfo.ViChn              = ViChn;
    g_stViConfig.astViInfo[s32SnsId].stChnInfo.enPixFormat        = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    g_stViConfig.astViInfo[s32SnsId].stChnInfo.enDynamicRange     = enDynamicRange;
    g_stViConfig.astViInfo[s32SnsId].stChnInfo.enVideoFormat      = VIDEO_FORMAT_LINEAR;
    g_stViConfig.astViInfo[s32SnsId].stChnInfo.enCompressMode     = COMPRESS_MODE_NONE;

    /************************************************
    step 2: start VI
    *************************************************/
    s32Ret = SAMPLE_COMM_VI_StartVi(&g_stViConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_StartVi failed with %d!\n", s32Ret);
        goto EXIT1;
    }

    return s32Ret;

EXIT1:
    SAMPLE_COMM_VI_StopVi(&g_stViConfig);

    return s32Ret;
}

HI_S32 SAMPLE_REGION_StopVi(HI_VOID)
{
    SAMPLE_COMM_VI_StopVi(&g_stViConfig);
    return HI_SUCCESS;
}

HI_S32 SAMPLE_REGION_StartVpss(HI_VOID)
{
    HI_S32 i;
    HI_S32 s32Ret;
    VPSS_GRP_ATTR_S stVpssGrpAttr;
    VPSS_CHN_ATTR_S stVpssChnAttr[VPSS_MAX_PHY_CHN_NUM];
    PIC_SIZE_E      enPicSize;
    SIZE_S          stSize;
    VPSS_GRP        VpssGrp        = 0;
    HI_S32          s32SnsId       = 0;

    SAMPLE_COMM_VI_GetSizeBySensor(g_stViConfig.astViInfo[s32SnsId].stSnsInfo.enSnsType, &enPicSize);

    SAMPLE_COMM_SYS_GetPicSize(enPicSize, &stSize);

    hi_memset(&stVpssGrpAttr, sizeof(VPSS_GRP_ATTR_S), 0, sizeof(VPSS_GRP_ATTR_S));
    stVpssGrpAttr.enDynamicRange = DYNAMIC_RANGE_SDR8;
    stVpssGrpAttr.enPixelFormat  = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    stVpssGrpAttr.u32MaxW        = stSize.u32Width;
    stVpssGrpAttr.u32MaxH        = stSize.u32Height;
    stVpssGrpAttr.stFrameRate.s32SrcFrameRate = -1;
    stVpssGrpAttr.stFrameRate.s32DstFrameRate = -1;
    stVpssGrpAttr.bNrEn          = HI_FALSE;
    stVpssGrpAttr.stNrAttr.enCompressMode        = COMPRESS_MODE_FRAME;
    stVpssGrpAttr.stNrAttr.enNrMotionMode        = NR_MOTION_MODE_NORMAL;
    for(i=0; i<VPSS_MAX_PHY_CHN_NUM; i++)
    {
        if(HI_TRUE == abChnEnable[i])
        {
            stVpssChnAttr[i].u32Width                    = stSize.u32Width;
            stVpssChnAttr[i].u32Height                   = stSize.u32Height;
            stVpssChnAttr[i].enChnMode                   = VPSS_CHN_MODE_USER;
            stVpssChnAttr[i].enCompressMode              = COMPRESS_MODE_NONE;
            stVpssChnAttr[i].enDynamicRange              = DYNAMIC_RANGE_SDR8;
            stVpssChnAttr[i].enVideoFormat               = VIDEO_FORMAT_LINEAR;
            stVpssChnAttr[i].enPixelFormat               = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
            stVpssChnAttr[i].stFrameRate.s32SrcFrameRate = -1;
            stVpssChnAttr[i].stFrameRate.s32DstFrameRate = -1;
            stVpssChnAttr[i].u32Depth                    = 0;
            stVpssChnAttr[i].bMirror                     = HI_FALSE;
            stVpssChnAttr[i].bFlip                       = HI_FALSE;
            stVpssChnAttr[i].stAspectRatio.enMode        = ASPECT_RATIO_NONE;
        }
            stVpssChnAttr[1].u32Width                     = 1920;
            stVpssChnAttr[1].u32Height                    = 1080;
    }

    s32Ret = SAMPLE_COMM_VPSS_Start(VpssGrp, abChnEnable,&stVpssGrpAttr,stVpssChnAttr);
    if(HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VPSS_StartVPSS failed with %d!\n", s32Ret);
        goto EXIT;
    }
    return HI_SUCCESS;
EXIT:
    SAMPLE_COMM_VPSS_Stop(VpssGrp, abChnEnable);
    return s32Ret;
}

HI_S32 SAMPLE_REGION_StopVpss(HI_VOID)
{
    SAMPLE_COMM_VPSS_Stop(0, abChnEnable);
    return HI_SUCCESS;
}

HI_S32 SAMPLE_REGION_StartVenc(HI_VOID)
{
    HI_S32          s32Ret;
    VENC_GOP_MODE_E enGopMode;
    VENC_GOP_ATTR_S stGopAttr;
    SAMPLE_RC_E     enRcMode;
    HI_S32          s32SnsId            = 0;
    PIC_SIZE_E      enPicSize;
    HI_BOOL         bRcnRefShareBuf = HI_FALSE;
    enRcMode = SAMPLE_RC_CBR;

    enGopMode = VENC_GOPMODE_NORMALP;

    SAMPLE_COMM_VI_GetSizeBySensor(g_stViConfig.astViInfo[s32SnsId].stSnsInfo.enSnsType, &enPicSize);

    s32Ret = SAMPLE_COMM_VENC_GetGopAttr(enGopMode,&stGopAttr);

   /***encode h.265 **/
    s32Ret = SAMPLE_COMM_VENC_Start(0, PT_H265,enPicSize, enRcMode,0,bRcnRefShareBuf,&stGopAttr);
    if(HI_SUCCESS !=s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VENC_StartVenc failed with %d!\n", s32Ret);
        goto EXIT;
    }
    return s32Ret;
EXIT:
    SAMPLE_COMM_VENC_Stop(0);
    return s32Ret;
}
HI_S32 SAMPLE_REGION_StopVenc(HI_VOID)
{
    SAMPLE_COMM_VENC_Stop(0);
    return HI_SUCCESS;
}
HI_S32 SAMPLE_REGION_StartVo(HI_VOID)
{
    HI_S32 s32Ret;
    SAMPLE_COMM_VO_GetDefConfig(&g_stVoConfig);

    g_stVoConfig.u32DisBufLen = g_u32DisBufLen;
    s32Ret= SAMPLE_COMM_VO_StartVO(&g_stVoConfig);
    if(HI_SUCCESS!=s32Ret)
    {
       SAMPLE_PRT("SAMPLE_COMM_VO_Start failed! s32Ret:0x%x.\n", s32Ret);
       goto EXIT;
    }
    return HI_SUCCESS;
EXIT:
    SAMPLE_COMM_VO_StopVO(&g_stVoConfig);
    return s32Ret;
}

HI_S32 SAMPLE_REGION_StopVo(HI_VOID)
{
    SAMPLE_COMM_VO_StopVO(&g_stVoConfig);
    return HI_SUCCESS;
}

HI_VOID SAMPLE_REGION_StartGetVencStream(HI_VOID)
{
    HI_S32 VencChn[2]={0,1};

    SAMPLE_COMM_VENC_StartGetStream(VencChn,1);
    return ;
}

HI_VOID SAMPLE_REGION_StopGetVencStream(HI_VOID)
{
    SAMPLE_COMM_VENC_StopGetStream();
    return;
}
HI_S32 SAMPLE_REGION_MPP_VI_VPSS_VENC_START(HI_VOID)
{
    HI_S32 s32Ret;

    s32Ret = SAMPLE_REGION_InitSys();
    if(HI_SUCCESS != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = SAMPLE_REGION_StartVi();
    if(HI_SUCCESS != s32Ret)
    {
        goto START_VI_FAILED;
    }

    s32Ret = SAMPLE_REGION_StartVpss();
    if(HI_SUCCESS != s32Ret)
    {
        goto START_VPSS_FAILED;
    }

    s32Ret = SAMPLE_COMM_VI_Bind_VPSS(0, 0, 0);
    if(HI_SUCCESS != s32Ret)
    {
        goto BIND_VI_VPSS_FAILED;
    }

    s32Ret = SAMPLE_REGION_StartVenc();
    if(HI_SUCCESS != s32Ret)
    {
        goto START_VENC_FAILED;
    }

    s32Ret = SAMPLE_COMM_VPSS_Bind_VENC(0, 0, 0);
    if(HI_SUCCESS != s32Ret)
    {
        goto BIND_VPSS_VENC_FAILED;
    }

    SAMPLE_REGION_StartGetVencStream();
    return HI_SUCCESS;

BIND_VPSS_VENC_FAILED:
    SAMPLE_REGION_StopVenc();
START_VENC_FAILED:
    SAMPLE_COMM_VI_UnBind_VPSS(0,0,0);
BIND_VI_VPSS_FAILED:
    SAMPLE_REGION_StopVpss();
START_VPSS_FAILED:
    SAMPLE_REGION_StopVi();
START_VI_FAILED:
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}

HI_VOID SAMPLE_REGION_MPP_VI_VPSS_VENC_END(HI_VOID)
{
    SAMPLE_REGION_StopGetVencStream();
    SAMPLE_COMM_VPSS_UnBind_VENC(0,0,0);
    SAMPLE_REGION_StopVenc();
    SAMPLE_COMM_VI_UnBind_VPSS(0,0,0);
    SAMPLE_REGION_StopVpss();
    SAMPLE_REGION_StopVi();
    SAMPLE_COMM_SYS_Exit();
    return ;
}

HI_S32 SAMPLE_REGION_MPP_VI_VPSS_VO_START(HI_VOID)
{
    HI_S32 s32Ret;

    s32Ret = SAMPLE_REGION_InitSys();
    if(HI_SUCCESS != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = SAMPLE_REGION_StartVi();
    if(HI_SUCCESS != s32Ret)
    {
        goto START_VI_FAILED;
    }

    s32Ret = SAMPLE_REGION_StartVpss();
    if(HI_SUCCESS != s32Ret)
    {
        goto START_VPSS_FAILED;
    }

    s32Ret = SAMPLE_COMM_VI_Bind_VPSS(0, 0, 0);
    if(HI_SUCCESS != s32Ret)
    {
        goto VI_BIND_VPSS_FAILED;

    }

    s32Ret = SAMPLE_REGION_StartVo();
    if(HI_SUCCESS != s32Ret)
    {
        goto START_Vo_FAILED;
    }

    s32Ret = SAMPLE_COMM_VPSS_Bind_VO(0,0,0,0);
    if(HI_SUCCESS != s32Ret)
    {
        goto VPSS_BIND_VO_FAILED;
    }
    return HI_SUCCESS;

VPSS_BIND_VO_FAILED:
    SAMPLE_REGION_StopVo();
START_Vo_FAILED:
    SAMPLE_COMM_VI_UnBind_VPSS(0, 0, 0);
VI_BIND_VPSS_FAILED:
    SAMPLE_REGION_StopVpss();
START_VPSS_FAILED:
    SAMPLE_REGION_StopVi();
START_VI_FAILED:
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}

HI_VOID SAMPLE_REGION_MPP_VI_VPSS_VO_END(HI_VOID)
{
    SAMPLE_COMM_VPSS_UnBind_VO(0, 0, 0, 0);
    SAMPLE_REGION_StopVo();
    SAMPLE_COMM_VI_UnBind_VPSS(0, 0, 0);
    SAMPLE_REGION_StopVpss();
    SAMPLE_REGION_StopVi();
    SAMPLE_COMM_SYS_Exit();
    return ;
}

HI_S32 SAMPLE_REGION_VI_VPSS_VO(HI_S32 HandleNum,RGN_TYPE_E  enType,MPP_CHN_S *pstChn)
{
    HI_S32         i;
    HI_S32         s32Ret;
    HI_S32         MinHandle;

    s32Ret = SAMPLE_REGION_MPP_VI_VPSS_VO_START();
    if(HI_SUCCESS != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_REGION_Create(HandleNum,enType);
    if(HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_REGION_Create failed!\n");
        goto EXIT1;
    }
    s32Ret = SAMPLE_COMM_REGION_AttachToChn(HandleNum,enType,pstChn);
    if(HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_REGION_AttachToChn failed!\n");
        goto EXIT2;
    }
    MinHandle = SAMPLE_COMM_REGION_GetMinHandle(enType);
    if(MinHandle == -1)
    {
        SAMPLE_PRT("SAMPLE_COMM_REGION_GetMinHandle failed!\n");
        return HI_FAILURE;
    }
    if(OVERLAY_RGN == enType || OVERLAYEX_RGN == enType)
    {
        for(i= MinHandle; i<MinHandle + HandleNum;i++)
        {

            //s32Ret = SAMPLE_COMM_REGION_SetBitMap(i);
            s32Ret = SAMPLE_COMM_REGION_GetUpCanvas(i);
            if(HI_SUCCESS != s32Ret)
            {
                SAMPLE_PRT("SAMPLE_COMM_REGION_SetBitMap failed!\n");
                goto EXIT2;
            }
        }
    }

    PAUSE();
EXIT2:
    s32Ret = SAMPLE_COMM_REGION_DetachFrmChn(HandleNum,enType,pstChn);
    if(HI_SUCCESS!= s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_REGION_AttachToChn failed!\n");
    }
EXIT1:
    s32Ret = SAMPLE_COMM_REGION_Destroy(HandleNum,enType);
    if(HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_REGION_AttachToChn failed!\n");
    }

    SAMPLE_REGION_MPP_VI_VPSS_VO_END();
    return s32Ret;
}

HI_S32 SAMPLE_REGION_VI_OSDEX(HI_VOID)
{
    HI_S32             s32Ret;
    HI_S32             HandleNum;
    RGN_TYPE_E         enType;
    MPP_CHN_S          stChn;

    HandleNum =8;
    enType = OVERLAYEX_RGN;
    stChn.enModId = HI_ID_VI;
    stChn.s32DevId = 0;
    stChn.s32ChnId = 0;
    Path_BMP = vi_chn_0_bmp;
    g_u32DisBufLen = 0;
    g_enMastPipeMode = VI_ONLINE_VPSS_OFFLINE;
    g_commVbPool.picBuffNum = 3;
    g_commVbPool.rawBuffNum = 3;
    s32Ret = SAMPLE_REGION_VI_VPSS_VO(HandleNum,enType,&stChn);
    return s32Ret;
}

HI_S32 SAMPLE_REGION_VI_COVEREX(HI_VOID)
{
    HI_S32             s32Ret;
    HI_S32             HandleNum;
    RGN_TYPE_E         enType;
    MPP_CHN_S          stChn;

    HandleNum =8;
    enType = COVEREX_RGN;
    stChn.enModId = HI_ID_VI;
    stChn.s32DevId = 0;
    stChn.s32ChnId = 0;
    g_u32DisBufLen = 0;
    g_enMastPipeMode = VI_ONLINE_VPSS_OFFLINE;
    g_commVbPool.picBuffNum = 3;
    g_commVbPool.rawBuffNum = 3;

    s32Ret = SAMPLE_REGION_VI_VPSS_VO(HandleNum,enType,&stChn);
    return s32Ret;
}

HI_S32 SAMPLE_REGION_VPSS_OSDEX(HI_VOID)
{
    HI_S32             s32Ret;
    HI_S32             HandleNum;
    RGN_TYPE_E         enType;
    MPP_CHN_S          stChn;

    HandleNum =8;
    enType = OVERLAYEX_RGN;
    stChn.enModId = HI_ID_VPSS;
    stChn.s32DevId = 0;
    stChn.s32ChnId = 0;
    g_u32DisBufLen = 0;
    Path_BMP = vi_chn_0_bmp;
    g_enMastPipeMode = VI_ONLINE_VPSS_ONLINE;
    g_commVbPool.picBuffNum = 6;
    g_commVbPool.rawBuffNum = 0;

    s32Ret = SAMPLE_REGION_VI_VPSS_VO(HandleNum,enType,&stChn);
    return s32Ret;
}

HI_S32 SAMPLE_REGION_VPSS_COVEREX(HI_VOID)
{
    HI_S32             s32Ret;
    HI_S32             HandleNum;
    RGN_TYPE_E         enType;
    MPP_CHN_S          stChn;

    HandleNum =8;
    enType = COVEREX_RGN;
    stChn.enModId = HI_ID_VPSS;
    stChn.s32DevId = 0;
    stChn.s32ChnId = 0;
    g_u32DisBufLen = 0;
    g_enMastPipeMode = VI_ONLINE_VPSS_ONLINE;
    g_commVbPool.picBuffNum = 7;
    g_commVbPool.rawBuffNum = 0;

    s32Ret = SAMPLE_REGION_VI_VPSS_VO(HandleNum,enType,&stChn);
    return s32Ret;
}

HI_S32 SAMPLE_REGION_VPSS_COVER(HI_VOID)
{
    HI_S32             s32Ret;
    HI_S32             HandleNum;
    RGN_TYPE_E         enType;
    MPP_CHN_S          stChn;

    HandleNum =8;
    enType = COVER_RGN;
    stChn.enModId = HI_ID_VPSS;
    stChn.s32DevId = 0;
    stChn.s32ChnId = 0;
    g_u32DisBufLen = 0;
    g_enMastPipeMode = VI_ONLINE_VPSS_ONLINE;
    g_commVbPool.picBuffNum = 6;
    g_commVbPool.rawBuffNum = 0;

    s32Ret = SAMPLE_REGION_VI_VPSS_VO(HandleNum,enType,&stChn);
    return s32Ret;
}

HI_S32 SAMPLE_REGION_VPSS_MOSAIC(HI_VOID)
{
    HI_S32             s32Ret;
    HI_S32             HandleNum;
    RGN_TYPE_E         enType;
    MPP_CHN_S          stChn;

    HandleNum =4;
    enType = MOSAIC_RGN;
    stChn.enModId = HI_ID_VPSS;
    stChn.s32DevId = 0;
    stChn.s32ChnId = 0;
    g_u32DisBufLen = 0;
    g_enMastPipeMode = VI_ONLINE_VPSS_ONLINE;
    g_commVbPool.picBuffNum = 6;
    g_commVbPool.rawBuffNum = 0;

    s32Ret = SAMPLE_REGION_VI_VPSS_VO(HandleNum,enType,&stChn);
    return s32Ret;
}

HI_S32 SAMPLE_REGION_VO_OSDEX(HI_VOID)
{
    HI_S32             s32Ret;
    HI_S32             HandleNum;
    RGN_TYPE_E         enType;
    MPP_CHN_S          stChn;

    HandleNum =1;
    enType = OVERLAYEX_RGN;
    stChn.enModId = HI_ID_VO;
    stChn.s32DevId = 0;
    stChn.s32ChnId = 0;
    Path_BMP = vi_chn_0_bmp;
    g_u32DisBufLen = 3;
    g_enMastPipeMode = VI_ONLINE_VPSS_ONLINE;
    g_commVbPool.picBuffNum = 6;
    g_commVbPool.rawBuffNum = 0;

    s32Ret = SAMPLE_REGION_VI_VPSS_VO(HandleNum,enType,&stChn);
    return s32Ret;
}

HI_S32 SAMPLE_REGION_VO_COVEREX(HI_VOID)
{
    HI_S32             s32Ret;
    HI_S32             HandleNum;
    RGN_TYPE_E         enType;
    MPP_CHN_S          stChn;

    HandleNum =1;
    enType = COVEREX_RGN;
    stChn.enModId = HI_ID_VO;
    stChn.s32DevId = 0;
    stChn.s32ChnId = 0;
    g_u32DisBufLen = 3;
    g_enMastPipeMode = VI_ONLINE_VPSS_ONLINE;
    g_commVbPool.picBuffNum = 6;
    g_commVbPool.rawBuffNum = 0;

    s32Ret = SAMPLE_REGION_VI_VPSS_VO(HandleNum,enType,&stChn);
    return s32Ret;
}

HI_S32 SAMPLE_REGION_VENC_OSDEX(HI_VOID)
{
    HI_S32             s32Ret;
    HI_S32             HandleNum;
    RGN_TYPE_E         enType;
    MPP_CHN_S          stChn;

    HandleNum =8;
    enType = OVERLAYEX_RGN;
    stChn.enModId = HI_ID_VENC;
    stChn.s32DevId = 0;
    stChn.s32ChnId = 0;
    s32Ret = SAMPLE_REGION_VI_VPSS_VO(HandleNum,enType,&stChn);
    return s32Ret;
}

HI_S32 SAMPLE_REGION_VENC_OSD(HI_VOID)
{
    HI_S32             s32Ret;
    HI_S32             HandleNum;
    RGN_TYPE_E         enType;
    MPP_CHN_S          stChn;

    HandleNum =8;
    enType = OVERLAY_RGN;
    stChn.enModId = HI_ID_VENC;
    stChn.s32DevId = 0;
    stChn.s32ChnId = 0;
    s32Ret = SAMPLE_REGION_VI_VPSS_VO(HandleNum,enType,&stChn);
    return s32Ret;
}


HI_S32 SAMPLE_REGION_CreateOverLay_Mix(HI_S32 HandleNum)
{
    HI_S32 s32Ret;
    HI_S32 i;
    RGN_ATTR_S stRegion;
    PIXEL_FORMAT_E aenPixelFmt[3] = {PIXEL_FORMAT_ARGB_1555,PIXEL_FORMAT_ARGB_4444,PIXEL_FORMAT_ARGB_2BPP};
    HI_U32 au32BgColor[3] = {0x00ff00ff,0x00ff00ff,1};
    HI_S32 MinHandle = SAMPLE_COMM_REGION_GetMinHandle(OVERLAY_RGN);

    stRegion.enType = OVERLAY_RGN;
    stRegion.unAttr.stOverlay.stSize.u32Height = 160;
    stRegion.unAttr.stOverlay.stSize.u32Width  = 160;
    stRegion.unAttr.stOverlay.u32CanvasNum = 5;
    for(i = MinHandle;i < MinHandle + HandleNum;i++)
    {
        stRegion.unAttr.stOverlay.enPixelFmt = aenPixelFmt[i%3];
        stRegion.unAttr.stOverlay.u32BgColor = au32BgColor[i%3];
        s32Ret = HI_MPI_RGN_Create(i, &stRegion);
        if(HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("HI_MPI_RGN_Create failed with %#x!\n", s32Ret);
            return HI_FAILURE;
        }
    }

    return s32Ret;
}



HI_S32 SAMPLE_REGION_VI_VPSS_VENC(HI_S32 HandleNum,RGN_TYPE_E  enType,MPP_CHN_S *pstChn)
{
    HI_S32         i;
    HI_S32         s32Ret;
    HI_S32         MinHandle;
    PIXEL_FORMAT_E aenPixelFmt[3] = {PIXEL_FORMAT_ARGB_1555,PIXEL_FORMAT_ARGB_4444,PIXEL_FORMAT_ARGB_2BPP};

    SAMPLE_REGION_MPP_VI_VPSS_VENC_START();

    s32Ret = SAMPLE_REGION_CreateOverLay_Mix(HandleNum);
    if(HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_REGION_Create failed!\n");
        return HI_FAILURE;
    }
    s32Ret = SAMPLE_COMM_REGION_AttachToChn(HandleNum,enType,pstChn);
    if(HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_REGION_AttachToChn failed!\n");
        return HI_FAILURE;
    }
    MinHandle = SAMPLE_COMM_REGION_GetMinHandle(enType);
    if(MinHandle == -1)
    {
        SAMPLE_PRT("SAMPLE_COMM_REGION_GetMinHandle failed!\n");
        return HI_FAILURE;
    }
    if(OVERLAY_RGN == enType || OVERLAYEX_RGN == enType)
    {
        for(i= MinHandle; i<MinHandle + HandleNum;i++)
        {

            s32Ret = SAMPLE_COMM_REGION_SetBitMap(i,aenPixelFmt[(i - MinHandle)%3]);
            if(HI_SUCCESS != s32Ret)
            {
                SAMPLE_PRT("SAMPLE_COMM_REGION_SetBitMap failed!\n");
                return HI_FAILURE;
            }
        }

    }

    PAUSE();

    s32Ret = SAMPLE_COMM_REGION_DetachFrmChn(HandleNum,enType,pstChn);
    if(HI_SUCCESS!= s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_REGION_AttachToChn failed!\n");
        return HI_FAILURE;
    }

    s32Ret = SAMPLE_COMM_REGION_Destroy(HandleNum,enType);
    if(HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_REGION_AttachToChn failed!\n");
        return HI_FAILURE;
    }

    SAMPLE_REGION_MPP_VI_VPSS_VENC_END();
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}

HI_S32 SAMPLE_REGION_VENC_OSD_MIX(HI_VOID)
{

    HI_S32             s32Ret;
    HI_S32             HandleNum;
    RGN_TYPE_E         enType;
    MPP_CHN_S          stChn;

    HandleNum =8;
    enType = OVERLAY_RGN;
    stChn.enModId = HI_ID_VENC;
    stChn.s32DevId = 0;
    stChn.s32ChnId = 0;
    Path_BMP = mm_bmp;
    g_enMastPipeMode = VI_ONLINE_VPSS_ONLINE;
    g_commVbPool.picBuffNum = 5;
    g_commVbPool.rawBuffNum = 0;

    s32Ret = SAMPLE_REGION_VI_VPSS_VENC(HandleNum,enType,&stChn);
    return s32Ret;
}
/******************************************************************************
* function    : main()
* Description : main
******************************************************************************/
#ifdef __HuaweiLite__
int app_main(int argc, char *argv[])
#else
int main(int argc, char* argv[])
#endif
{
    HI_S32 s32Ret = HI_FAILURE;
    HI_S32 s32Index;

    if (argc < 2)
    {
        SAMPLE_REGION_Usage(argv[0]);
        return HI_FAILURE;
    }

#ifndef __HuaweiLite__
    signal(SIGINT, SAMPLE_REGION_HandleSig);
    signal(SIGTERM, SAMPLE_REGION_HandleSig);
#endif

    s32Index = atoi(argv[1]);
    switch (s32Index)
    {
        case 0:
            s32Ret = SAMPLE_REGION_VI_OSDEX();
            break;
        case 1:
            s32Ret = SAMPLE_REGION_VI_COVEREX();
            break;
        case 2:
            s32Ret = SAMPLE_REGION_VPSS_OSDEX();
            break;
        case 3:
            s32Ret = SAMPLE_REGION_VPSS_COVEREX();
            break;
        case 4:
            s32Ret = SAMPLE_REGION_VPSS_COVER();
            break;
        case 5:
            s32Ret = SAMPLE_REGION_VPSS_MOSAIC();
            break;
        case 6:
            s32Ret = SAMPLE_REGION_VO_OSDEX();
            break;
        case 7:
            s32Ret = SAMPLE_REGION_VO_COVEREX();
            break;
        case 8:
            s32Ret = SAMPLE_REGION_VENC_OSD_MIX();
            break;
        default:
            SAMPLE_PRT("the index %d is invaild!\n",s32Index);
            SAMPLE_REGION_Usage(argv[0]);
            s32Ret = HI_FAILURE;
            break;
    }

    if (HI_SUCCESS == s32Ret)
    {
        SAMPLE_PRT("program exit normally!\n");
    }
    else
    {
        SAMPLE_PRT("program exit abnormally!\n");
    }

    return (s32Ret);
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
