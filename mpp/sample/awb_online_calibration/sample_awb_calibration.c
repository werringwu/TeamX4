/******************************************************************************

  Copyright (C), 2016-2016, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : Sample_awb_correction.c
  Version       : Initial Draft
  Author        : Hisilicon BVT PQ group
  Created       : 2016/12/15
  Description   :
  History       :
  1.Date        : 2016/12/15
    Author      : h00372898
    Modification: Created file

******************************************************************************/

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
#include <sys/time.h>
#include <math.h>


#include "mpi_isp.h"
#include "hi_comm_isp.h"
#include "mpi_ae.h"

#include "sample_comm.h"
#include "awb_calb_prev.h"

HI_S32 SAMPLE_AWB_CALI_START_PREV(AWB_CALI_PREV_S *pstAwb_CaliPrev)
{

    HI_S32             s32Ret = HI_SUCCESS;
    HI_S32             s32ViCnt       = 1;
    VI_DEV             ViDev          = 0;
    VI_PIPE            ViPipe         = 0;
    VI_CHN             ViChn          = 0;
    HI_S32             s32WorkSnsId   = 0;

    SIZE_S             stSize;
    VB_CONFIG_S        stVbConf;
    PIC_SIZE_E         enPicSize;
    HI_U32             u32BlkSize;

    WDR_MODE_E         enWDRMode      = WDR_MODE_NONE;
    //WDR_MODE_E         enWDRMode      = WDR_MODE_2To1_LINE;
    DYNAMIC_RANGE_E    enDynamicRange = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E     enPixFormat    = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    VIDEO_FORMAT_E     enVideoFormat  = VIDEO_FORMAT_LINEAR;
    COMPRESS_MODE_E    enCompressMode = COMPRESS_MODE_NONE;
    VI_VPSS_MODE_E     enMastPipeMode = VI_ONLINE_VPSS_ONLINE;
    VPSS_GRP_ATTR_S    stVpssGrpAttr;

    VPSS_CHN_ATTR_S    astVpssChnAttr[VPSS_MAX_PHY_CHN_NUM];
    HI_U8 i = 0;

    PAYLOAD_TYPE_E     enType      = PT_H265;
    SAMPLE_RC_E        enRcMode    = SAMPLE_RC_CBR;
    HI_U32             u32Profile  = 0;
    HI_BOOL            bRcnRefShareBuf = HI_TRUE;
    VENC_GOP_ATTR_S    stGopAttr;

    pstAwb_CaliPrev->VencChn[0] = 0;
    pstAwb_CaliPrev->VpssGrp    = 0;
    pstAwb_CaliPrev->VpssChn    = VPSS_CHN0;
    pstAwb_CaliPrev->VoChn      = 0;

    for(i=0;i<VPSS_MAX_PHY_CHN_NUM;i++)
    {
        pstAwb_CaliPrev->abChnEnable[i] = 0;
    }

    /*config vi*/
    SAMPLE_COMM_VI_GetSensorInfo(&pstAwb_CaliPrev->stViConfig);

    pstAwb_CaliPrev->stViConfig.s32WorkingViNum                                   = s32ViCnt;
    pstAwb_CaliPrev->stViConfig.as32WorkingViId[0]                                = 0;
    pstAwb_CaliPrev->stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.MipiDev         = ViDev;
    pstAwb_CaliPrev->stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.s32BusId        = 0;
    pstAwb_CaliPrev->stViConfig.astViInfo[s32WorkSnsId].stDevInfo.ViDev           = ViDev;
    pstAwb_CaliPrev->stViConfig.astViInfo[s32WorkSnsId].stDevInfo.enWDRMode       = enWDRMode;
    pstAwb_CaliPrev->stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.enMastPipeMode = enMastPipeMode;
    pstAwb_CaliPrev->stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[0]       = ViPipe;
    pstAwb_CaliPrev->stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[1]       = -1;
    pstAwb_CaliPrev->stViConfig.astViInfo[s32WorkSnsId].stChnInfo.ViChn           = ViChn;
    pstAwb_CaliPrev->stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enPixFormat     = enPixFormat;
    pstAwb_CaliPrev->stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enDynamicRange  = enDynamicRange;
    pstAwb_CaliPrev->stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enVideoFormat   = enVideoFormat;
    pstAwb_CaliPrev->stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enCompressMode  = enCompressMode;

    /*get picture size*/
    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(pstAwb_CaliPrev->stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType, &enPicSize);
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

    /*config vb*/
    hi_memset(&stVbConf, sizeof(VB_CONFIG_S), 0, sizeof(VB_CONFIG_S));
    stVbConf.u32MaxPoolCnt              = 1;

    u32BlkSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8, COMPRESS_MODE_SEG, DEFAULT_ALIGN);
    stVbConf.astCommPool[0].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt   = 3;

    u32BlkSize = COMMON_GetPicBufferSize(720, 576, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8, COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[1].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt   = 5;


    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        return s32Ret;
    }

    /*start vi*/
    s32Ret = SAMPLE_COMM_VI_StartVi(&pstAwb_CaliPrev->stViConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed.s32Ret:0x%x !\n", s32Ret);
        goto EXIT;
    }

    /*config vpss*/
    hi_memset(&stVpssGrpAttr, sizeof(VPSS_GRP_ATTR_S), 0, sizeof(VPSS_GRP_ATTR_S));
    stVpssGrpAttr.stFrameRate.s32SrcFrameRate    = -1;
    stVpssGrpAttr.stFrameRate.s32DstFrameRate    = -1;
    stVpssGrpAttr.enDynamicRange                 = DYNAMIC_RANGE_SDR8;
    stVpssGrpAttr.enPixelFormat                  = enPixFormat;
    stVpssGrpAttr.u32MaxW                        = stSize.u32Width;
    stVpssGrpAttr.u32MaxH                        = stSize.u32Height;
    stVpssGrpAttr.bNrEn                          = HI_TRUE;
    stVpssGrpAttr.stNrAttr.enCompressMode        = COMPRESS_MODE_FRAME;
    stVpssGrpAttr.stNrAttr.enNrMotionMode        = NR_MOTION_MODE_NORMAL;

    astVpssChnAttr[pstAwb_CaliPrev->VpssChn].u32Width                    = stSize.u32Width;
    astVpssChnAttr[pstAwb_CaliPrev->VpssChn].u32Height                   = stSize.u32Height;
    astVpssChnAttr[pstAwb_CaliPrev->VpssChn].enChnMode                   = VPSS_CHN_MODE_USER;
    astVpssChnAttr[pstAwb_CaliPrev->VpssChn].enCompressMode              = COMPRESS_MODE_SEG;
    astVpssChnAttr[pstAwb_CaliPrev->VpssChn].enDynamicRange              = enDynamicRange;
    astVpssChnAttr[pstAwb_CaliPrev->VpssChn].enVideoFormat               = enVideoFormat;
    astVpssChnAttr[pstAwb_CaliPrev->VpssChn].enPixelFormat               = enPixFormat;
    astVpssChnAttr[pstAwb_CaliPrev->VpssChn].stFrameRate.s32SrcFrameRate = -1;
    astVpssChnAttr[pstAwb_CaliPrev->VpssChn].stFrameRate.s32DstFrameRate = -1;
    astVpssChnAttr[pstAwb_CaliPrev->VpssChn].u32Depth                    = 0;
    astVpssChnAttr[pstAwb_CaliPrev->VpssChn].bMirror                     = HI_FALSE;
    astVpssChnAttr[pstAwb_CaliPrev->VpssChn].bFlip                       = HI_FALSE;
    astVpssChnAttr[pstAwb_CaliPrev->VpssChn].stAspectRatio.enMode        = ASPECT_RATIO_NONE;

    astVpssChnAttr[VPSS_CHN1].u32Width                    = 720;
    astVpssChnAttr[VPSS_CHN1].u32Height                   = 576;
    astVpssChnAttr[VPSS_CHN1].enChnMode                   = VPSS_CHN_MODE_USER;
    astVpssChnAttr[VPSS_CHN1].enCompressMode              = enCompressMode;
    astVpssChnAttr[VPSS_CHN1].enDynamicRange              = enDynamicRange;
    astVpssChnAttr[VPSS_CHN1].enVideoFormat               = enVideoFormat;
    astVpssChnAttr[VPSS_CHN1].enPixelFormat               = enPixFormat;
    astVpssChnAttr[VPSS_CHN1].stFrameRate.s32SrcFrameRate = -1;
    astVpssChnAttr[VPSS_CHN1].stFrameRate.s32DstFrameRate = -1;
    astVpssChnAttr[VPSS_CHN1].u32Depth                    = 0;
    astVpssChnAttr[VPSS_CHN1].bMirror                     = HI_FALSE;
    astVpssChnAttr[VPSS_CHN1].bFlip                       = HI_FALSE;
    astVpssChnAttr[VPSS_CHN1].stAspectRatio.enMode        = ASPECT_RATIO_NONE;
    /*start vpss*/
    pstAwb_CaliPrev->abChnEnable[0] = HI_TRUE;
    pstAwb_CaliPrev->abChnEnable[1] = HI_TRUE;
    s32Ret = SAMPLE_COMM_VPSS_Start(pstAwb_CaliPrev->VpssGrp, pstAwb_CaliPrev->abChnEnable, &stVpssGrpAttr, astVpssChnAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vpss group failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT1;
    }

    /*config venc */
    stGopAttr.enGopMode  = VENC_GOPMODE_SMARTP;
    stGopAttr.stSmartP.s32BgQpDelta  = 7;
    stGopAttr.stSmartP.s32ViQpDelta  = 2;
    stGopAttr.stSmartP.u32BgInterval = 1200;
    s32Ret = SAMPLE_COMM_VENC_Start(pstAwb_CaliPrev->VencChn[0], enType, enPicSize, enRcMode, u32Profile,bRcnRefShareBuf, &stGopAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start venc failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT2;
    }

    s32Ret = SAMPLE_COMM_VPSS_Bind_VENC(pstAwb_CaliPrev->VpssGrp, pstAwb_CaliPrev->VpssChn, pstAwb_CaliPrev->VencChn[0]);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Venc bind Vpss failed. s32Ret: 0x%x !n", s32Ret);
        goto EXIT3;
    }

    /*config vo*/
    SAMPLE_COMM_VO_GetDefConfig(&pstAwb_CaliPrev->stVoConfig);
    pstAwb_CaliPrev->stVoConfig.enDstDynamicRange = enDynamicRange;
    pstAwb_CaliPrev->stVoConfig.enVoIntfType = VO_INTF_BT1120;
    pstAwb_CaliPrev->stVoConfig.enIntfSync   = VO_OUTPUT_576P50;
    pstAwb_CaliPrev->stVoConfig.u32DisBufLen = 3;
    pstAwb_CaliPrev->stVoConfig.enPicSize = enPicSize;

    /*start vo*/
    s32Ret = SAMPLE_COMM_VO_StartVO(&pstAwb_CaliPrev->stVoConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vo failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT4;
    }

    /*vpss bind vo*/
    s32Ret = SAMPLE_COMM_VPSS_Bind_VO(pstAwb_CaliPrev->VpssGrp, VPSS_CHN1, pstAwb_CaliPrev->stVoConfig.VoDev, pstAwb_CaliPrev->VoChn);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vo bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT5;
    }

    s32Ret = SAMPLE_COMM_VENC_StartGetStream(pstAwb_CaliPrev->VencChn, sizeof(pstAwb_CaliPrev->VencChn)/sizeof(VENC_CHN));
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Get venc stream failed!\n");
        goto EXIT6;
    }

    return HI_SUCCESS;

EXIT6:
    SAMPLE_COMM_VPSS_UnBind_VO(pstAwb_CaliPrev->VpssGrp, VPSS_CHN1, pstAwb_CaliPrev->stVoConfig.VoDev, pstAwb_CaliPrev->VoChn);
EXIT5:
    SAMPLE_COMM_VO_StopVO(&pstAwb_CaliPrev->stVoConfig);
EXIT4:
    SAMPLE_COMM_VPSS_UnBind_VENC(pstAwb_CaliPrev->VpssGrp, pstAwb_CaliPrev->VpssChn, pstAwb_CaliPrev->VencChn[0]);
EXIT3:
    SAMPLE_COMM_VENC_Stop(pstAwb_CaliPrev->VencChn[0]);
EXIT2:
    SAMPLE_COMM_VPSS_Stop(pstAwb_CaliPrev->VpssGrp, pstAwb_CaliPrev->abChnEnable);
EXIT1:
    SAMPLE_COMM_VI_StopVi(&pstAwb_CaliPrev->stViConfig);
EXIT:
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;

}

HI_S32 SAMPLE_AWB_CALI_STOP_PREV(AWB_CALI_PREV_S *pstAwb_CaliPrev)
{
    //printf("%s,%d\n",__FUNCTION__,__LINE__);
    if(pstAwb_CaliPrev == HI_NULL)
    {
        SAMPLE_PRT("Err: pstAwb_CaliPrev is NULL \n");
        return HI_FAILURE;
    }

    SAMPLE_COMM_VENC_StopGetStream();
    SAMPLE_COMM_VPSS_UnBind_VO(pstAwb_CaliPrev->VpssGrp, VPSS_CHN1, pstAwb_CaliPrev->stVoConfig.VoDev, pstAwb_CaliPrev->VoChn);
    SAMPLE_COMM_VO_StopVO(&pstAwb_CaliPrev->stVoConfig);
    SAMPLE_COMM_VPSS_UnBind_VENC(pstAwb_CaliPrev->VpssGrp, pstAwb_CaliPrev->VpssChn, pstAwb_CaliPrev->VencChn[0]);
    SAMPLE_COMM_VENC_Stop(pstAwb_CaliPrev->VencChn[0]);
    SAMPLE_COMM_VPSS_Stop(pstAwb_CaliPrev->VpssGrp, pstAwb_CaliPrev->abChnEnable);
    SAMPLE_COMM_VI_StopVi(&pstAwb_CaliPrev->stViConfig);
    SAMPLE_COMM_SYS_Exit();
    return HI_SUCCESS;
}

void SAMPLE_AWB_CORRECTION_Usage(char* sPrgNm)
{
    printf("Usage : %s <mode> <intf1> <intf2> <intf3>\n", sPrgNm);
    printf("mode:\n");
    printf("\t 0) Calculate Sample gain.\n");
    printf("\t 1) Adjust Sample gain according to Golden Sample.\n");

    printf("intf1:\n");
    printf("\t The value of Rgain of Golden Sample.\n");

    printf("intf2:\n");
    printf("\t The value of Bgain of Golden Sample.\n");

    printf("intf3:\n");
    printf("\t The value of Alpha ranging from 0 to 1024 (The strength of adusting Sampe Gain will increase with the value of Alpha) .\n");

    return;
}
#ifdef __HuaweiLite__
int app_main(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
    HI_S16 total_count = 0;
    HI_S16 stable_count = 0;
    VI_PIPE ViPipe = 0;
    HI_U16 u16GoldenRgain = 0;
    HI_U16 u16GoldenBgain = 0;
    HI_S16 s16Alpha = 0;
    HI_U32 u32Mode = 0;
    HI_S32 s32ret;
    ISP_EXP_INFO_S stExpInfo;
    ISP_EXPOSURE_ATTR_S stExpAttr;


    if ( (argc < 2) || (1 != strlen(argv[1])))
    {
        SAMPLE_AWB_CORRECTION_Usage(argv[0]);
        return HI_FAILURE;
    }
    if ( (argc < 5) && ('1' == *argv[1]))
    {
        SAMPLE_AWB_CORRECTION_Usage(argv[0]);
        return HI_FAILURE;
    }
#ifdef __HuaweiLite__

    AWB_CALI_PREV_S stAwb_CaliPrev;
    s32ret = SAMPLE_AWB_CALI_START_PREV(&stAwb_CaliPrev);

    if (HI_SUCCESS == s32ret)
    {
        SAMPLE_PRT("ISP is now running normally\n");
    }
    else
    {
        SAMPLE_PRT("ISP is not running normally!Please check it\n");
        return -1;
    }
    printf("input anything to continue....\n");
    getchar();
#endif
    HI_MPI_ISP_GetExposureAttr(ViPipe, &stExpAttr);

    printf("set antiflicker enable and the value of frequency to 50Hz\n");
    stExpAttr.stAuto.stAntiflicker.bEnable = HI_TRUE;
    stExpAttr.stAuto.stAntiflicker.u8Frequency = 50;
    HI_MPI_ISP_SetExposureAttr(ViPipe, &stExpAttr);

    switch (*argv[1])
    {
        case '0':
            u32Mode = 0;
            u16GoldenRgain = 0;
            u16GoldenBgain = 0;
            break;

        case '1':
            u32Mode = 1;
            u16GoldenRgain = atoi(argv[2]);
            u16GoldenBgain = atoi(argv[3]);
            s16Alpha = atoi(argv[4]);
            break;

        default:
            SAMPLE_PRT("the mode is invaild!\n");
            SAMPLE_AWB_CORRECTION_Usage(argv[0]);
            s32ret = HI_FAILURE;
            goto EXIT;
    }

    do
    {


   	    HI_MPI_ISP_QueryExposureInfo(ViPipe, &stExpInfo);
        usleep(100000000 / DIV_0_TO_1(stExpInfo.u32Fps));

        /*judge whether AE is stable*/
        if (stExpInfo.s16HistError > stExpAttr.stAuto.u8Tolerance)
        {
            stable_count = 0;
        }
        else
        {
            stable_count ++;
        }
        total_count ++;
    }
    while ((stable_count < 5) && (total_count < 20));


    if (stable_count >= 5)
    {
        ISP_AWB_Calibration_Gain_S stAWBCalibGain;
        HI_MPI_ISP_GetLightboxGain(ViPipe, &stAWBCalibGain);
        /*Adjust the value of Rgain and Bgain of Sample according to Golden Sample*/
        if (1 == u32Mode)
        {
            stAWBCalibGain.u16AvgRgain =  (HI_U16)((HI_S16)(stAWBCalibGain.u16AvgRgain)  + ((((HI_S16)u16GoldenRgain - (HI_S16)(stAWBCalibGain.u16AvgRgain))* s16Alpha) >> 10));
            stAWBCalibGain.u16AvgBgain = (HI_U16)((HI_S16)(stAWBCalibGain.u16AvgBgain) + ((((HI_S16)u16GoldenBgain  - (HI_S16)(stAWBCalibGain.u16AvgBgain))* s16Alpha) >> 10 ));
        }

#if 1
    if (0 == u32Mode)
    {
        printf("Calculate Sample gain:\n");
    }
    else if (1 == u32Mode)
    {
        printf("Adjust Sample gain:\n");
    }
    printf("\tu16AvgRgain =%8d, u16AvgBgain = %8d\n", stAWBCalibGain.u16AvgRgain, stAWBCalibGain.u16AvgBgain);
#endif
        s32ret = HI_SUCCESS;
        goto EXIT;

    }
    else
    {
        printf("AE IS NOT STABLE,PLEASE WAIT");
        s32ret = HI_FAILURE;
        goto EXIT;
    }
EXIT:
#ifdef __HuaweiLite__
    printf("input anything to continue....\n");
    getchar();
    SAMPLE_AWB_CALI_STOP_PREV(&stAwb_CaliPrev);
#endif
    return s32ret;
}
