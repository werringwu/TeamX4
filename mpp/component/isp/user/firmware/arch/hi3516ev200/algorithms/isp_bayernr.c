/******************************************************************************

  Copyright (C), 2016, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : isp_bayernr.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2015/07/21
  Description   :
  History       :
  1.Date        : 2015/07/21
    Author      :
    Modification: Created file

******************************************************************************/
#include <math.h>
#include "isp_alg.h"
#include "isp_sensor.h"
#include "isp_config.h"
#include "isp_ext_config.h"
#include "isp_proc.h"
#include "isp_math_utils.h"


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define HI_ISP_BAYERNR_BITDEP (16)
#define HI_ISP_NR_LUT_LEN     (16)
#define HI_WDR_EINIT_BLCNR    (64)

static const HI_U16 g_au16LutCoringRatio[HI_ISP_BAYERNR_LUT_LENGTH] = {60, 60, 60, 60, 65, 65, 65, 65, 70, 70, 70, 70, 70, 70, 70, 70, 80, 80, 80, 85, 85, 85, 90, 90, 90, 95, 95, 95, 100, 100, 100, 100, 100};
static const HI_U8  g_au8LutFineStr[ISP_AUTO_ISO_STRENGTH_NUM] = {70, 70, 70, 50, 48, 37, 28, 24, 20, 20, 20, 16, 16, 16, 16, 16};
static const HI_U8  g_au8ChromaStr[ISP_BAYER_CHN_NUM][ISP_AUTO_ISO_STRENGTH_NUM] = {
    {1, 1, 1, 1, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3},
    {0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2},
    {0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2},
    {1, 1, 1, 1, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3}
};
static const HI_U16 g_au16LutCoringWgt[ISP_AUTO_ISO_STRENGTH_NUM] = {30, 35, 40, 80, 100, 140, 200, 240, 280, 280, 300, 400, 400, 400, 400, 400};
static const HI_U16 g_au16CoarseStr[ISP_BAYER_CHN_NUM][ISP_AUTO_ISO_STRENGTH_NUM] = {
    {120, 120, 120, 120, 120, 120, 120, 140, 160, 160, 180, 200, 200, 200, 200, 200},
    {110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110},
    {110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110},
    {120, 120, 120, 120, 120, 120, 120, 140, 160, 160, 180, 200, 200, 200, 200, 200}
};
//static const HI_U8  g_au8WDRFrameStr[WDR_MAX_FRAME_NUM]    = {10, 16, 28};
static const HI_U8  g_au8WDRFrameStr[WDR_MAX_FRAME_NUM]    = {10, 16, 28};

static const HI_U8  g_au8FusionFrameStr[WDR_MAX_FRAME_NUM] = {26, 16};

typedef struct hiISP_NR_AUTO_S {
    HI_U8   au8ChromaStr[ISP_BAYER_CHN_NUM][ISP_AUTO_ISO_STRENGTH_NUM]; /*RW;Range:[0x0,0x3];Format:2.0; Strength of chrmoa noise reduction for R/Gr/Gb/B channel*/
    HI_U8   au8FineStr[ISP_AUTO_ISO_STRENGTH_NUM];                      /*RW;Range:[0x0,0x80];Format:8.0; Strength of luma noise reduction*/
    HI_U16  au16CoarseStr[ISP_BAYER_CHN_NUM][ISP_AUTO_ISO_STRENGTH_NUM];
    HI_U16  au16CoringWgt[ISP_AUTO_ISO_STRENGTH_NUM];                   /*RW;Range:[0x0,0xC80];Format:12.0; Strength of reserving the random noise*/
} ISP_NR_AUTO_S;

typedef struct hiISP_NR_MANUAL_S {
    HI_U8   au8ChromaStr[ISP_BAYER_CHN_NUM]; /*RW;Range:[0x0,0x3];Format:2.0;Strength of Chrmoa noise reduction for R/Gr/Gb/B channel*/
    HI_U8   u8FineStr;                       /*RW;Range:[0x0,0x80];Format:8.0;Strength of Luma noise reduction*/
    HI_U16  au16CoarseStr[ISP_BAYER_CHN_NUM];
    HI_U16  u16CoringWgt;                    /*RW;Range:[0x0,0xc80];Format:12.0;Strength of reserving the random noise*/
} ISP_NR_MANUAL_S;

typedef struct hiIISP_NR_WDR_S {
    HI_U8    au8WDRFrameStr[WDR_MAX_FRAME_NUM];        /*RW;Range:[0x0,0x50];Format:7.0; Strength of each frame in wdr mode*/
    HI_U8    au8FusionFrameStr[WDR_MAX_FRAME_NUM];     /*RW;Range:[0x0,0x50];Format:7.0; Strength of each frame in wdr-fusion mode*/
} ISP_NR_WDR_S;

typedef struct hiISP_BAYERNR_S {
    HI_BOOL  bInit;
    HI_BOOL  bEnable;
    HI_BOOL  bNrLscEnable;
    HI_BOOL  bBnrMonoSensorEn;
    HI_BOOL  bLutUpdate;
    HI_BOOL  bCenterWgtEn;
    HI_BOOL  bWdrModeEn;
    HI_BOOL  bWdrFusionEn;

    HI_U8    u8WdrMapGain;
    HI_U8    u8WdrFramesMerge;
    HI_U8    u8FineStr;
    HI_U8    u8NrLscRatio;
    HI_U8    u8BnrLscMaxGain;
    HI_U16   u16CenterLmt;
    HI_U16   u16BnrLscCmpStrength;
    HI_U16   u16WDRBlcThr;
    HI_U16   u16CoringLow;
    HI_U16   u16LmtNpThresh;
    HI_U16   u16LmtNpThreshS;
    HI_U32   u32EdgeDetThr;

    HI_U8    au8JnlmLimitLut[HI_ISP_BAYERNR_LMTLUTNUM]; //u8.0
    HI_U8    au8LutChromaRatio[ISP_BAYER_CHN_NUM][ISP_AUTO_ISO_STRENGTH_NUM];
    HI_U8    au8LutWDRChromaRatio[ISP_BAYER_CHN_NUM][ISP_AUTO_ISO_STRENGTH_NUM];
    HI_U8    au8LutAmedMode[ISP_BAYER_CHN_NUM];          //u1.0
    HI_U8    au8LutGCoringGain[HI_ISP_BAYERNR_LUT_LENGTH];   //u4.4
    HI_U16   au16LutCenterLmt[ISP_AUTO_ISO_STRENGTH_NUM];    //u1.8
    HI_U16   au16LmtStrength[ISP_BAYER_CHN_NUM];
    HI_U16   au16WDRFrameThr[WDR_MAX_FRAME_NUM + 2];
    HI_U16   au16LutCoringLow[ISP_AUTO_ISO_STRENGTH_NUM];           //u14.0
    HI_U16   au16LutCoringHig[ISP_AUTO_ISO_STRENGTH_NUM];           //u14.0
    HI_U16   au16LutCoringRatio[HI_ISP_BAYERNR_LUT_LENGTH];
    HI_U16   au16CoarseStr[ISP_BAYER_CHN_NUM];
    HI_U32   au32JnlmLimitMultGain[ISP_BAYER_CHN_NUM];  //u21.0
    HI_FLOAT afExpoValues[WDR_MAX_FRAME_NUM];
    HI_U32   au32ExpoValues[WDR_MAX_FRAME_NUM];
    HI_U32   au32LutCoringHig[ISP_AUTO_ISO_STRENGTH_NUM];
    HI_U32   au32LutCoringRatio[HI_ISP_BAYERNR_LUT_LENGTH];
    HI_U8    u8nLimitGainBlt;
    HI_U8     u8WindowSizeSel;
    HI_BOOL bPatternNoiseReduction;
    HI_U8     u8Gainsad;
    HI_BOOL bImplsnrEnable1;
    HI_BOOL bImplsnrEnable2;
    HI_BOOL bImplsnrEnable3;
    HI_U8     u8ImplsnrStrength;
    HI_BOOL bEnableSymmsad;
    HI_U8     u8EnablePostProc;
    HI_U8     u8BilateralEnable;
    HI_U16  au16WDRSyncFrameThr[CFG2VLD_DLY_LIMIT][WDR_MAX_FRAME_NUM + 2];

    ISP_OP_TYPE_E enOpType;
    ISP_NR_AUTO_S stAuto;
    ISP_NR_MANUAL_S stManual;
    ISP_NR_WDR_S  stWDR;
} ISP_BAYERNR_S;

ISP_BAYERNR_S *g_pastBayerNrCtx[ISP_MAX_PIPE_NUM] = {HI_NULL};

#define BAYERNR_GET_CTX(dev, pstCtx)   (pstCtx = g_pastBayerNrCtx[dev])
#define BAYERNR_SET_CTX(dev, pstCtx)   (g_pastBayerNrCtx[dev] = pstCtx)
#define BAYERNR_RESET_CTX(dev)         (g_pastBayerNrCtx[dev] = HI_NULL)

HI_S32 BayerNrCtxInit(VI_PIPE ViPipe)
{
    ISP_BAYERNR_S *pastBayerNrCtx = HI_NULL;

    BAYERNR_GET_CTX(ViPipe, pastBayerNrCtx);

    if (pastBayerNrCtx == HI_NULL) {
        pastBayerNrCtx = (ISP_BAYERNR_S *)ISP_MALLOC(sizeof(ISP_BAYERNR_S));
        if (pastBayerNrCtx == HI_NULL) {
            ISP_TRACE(HI_DBG_ERR, "Isp[%d] BayerNrCtx malloc memory failed!\n", ViPipe);
            return HI_ERR_ISP_NOMEM;
        }
    }

    memset(pastBayerNrCtx, 0, sizeof(ISP_BAYERNR_S));

    BAYERNR_SET_CTX(ViPipe, pastBayerNrCtx);

    return HI_SUCCESS;
}

HI_VOID BayerNrCtxExit(VI_PIPE ViPipe)
{
    ISP_BAYERNR_S *pastBayerNrCtx = HI_NULL;

    BAYERNR_GET_CTX(ViPipe, pastBayerNrCtx);
    ISP_FREE(pastBayerNrCtx);
    BAYERNR_RESET_CTX(ViPipe);
}

static HI_VOID  NrInitFw(VI_PIPE ViPipe)
{
    ISP_BAYERNR_S *pstBayernr = HI_NULL;

    HI_U8   au8LutChromaRatio[ISP_BAYER_CHN_NUM][HI_ISP_NR_LUT_LEN] = {
        {0, 0, 0, 0, 0, 2, 5, 8, 10, 18, 27, 32, 32, 32, 32, 32}, //ChromaRatioR
        {0, 0, 0, 0, 1, 4, 10, 16, 19, 24, 27, 32, 32, 32, 32, 32}, //ChromaRatioGr
        {0, 0, 0, 0, 1, 4, 10, 16, 19, 24, 27, 32, 32, 32, 32, 32}, //ChromaRatioGb
        {0, 0, 0, 0, 0, 2, 5, 8, 10, 18, 27, 32, 32, 32, 32, 32}//ChromaRatioB
    };
    HI_U8   au8LutWDRChromaRatio[ISP_BAYER_CHN_NUM][HI_ISP_NR_LUT_LEN] = {
        {0, 0, 0, 2, 4, 6, 8, 9, 10, 10, 10, 10, 10, 10, 10, 10}, //ChromaRatioR
        {0, 0, 0, 2, 4, 6, 8, 9, 10, 10, 10, 10, 10, 10, 10, 10}, //ChromaRatioGr
        {0, 0, 0, 2, 4, 6, 8, 9, 10, 10, 10, 10, 10, 10, 10, 10}, //ChromaRatioGb
        {0, 0, 0, 2, 4, 6, 8, 9, 10, 10, 10, 10, 10, 10, 10, 10}//ChromaRatioB
    };
    HI_U8  au8LutGCoringGain[HI_ISP_BAYERNR_LUT_LENGTH]  = {16, 16, 16, 16, 16, 16, 16, 16, 20, 24, 32, 32, 32, 34, 34, 36, 36, 38, 38, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41};
    HI_U16  au16LutCenterLmt[HI_ISP_NR_LUT_LEN] = {128, 128, 154, 205, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256};
    HI_U16  au16LutCoringHig[HI_ISP_NR_LUT_LEN] = {3200, 3200, 3200, 3200, 3200, 6400, 6400, 6400, 6400, 6400, 6400, 6400, 6400, 6400, 6400, 6400};

    BAYERNR_GET_CTX(ViPipe, pstBayernr);

    memcpy(pstBayernr->au8LutChromaRatio,    au8LutChromaRatio,    sizeof(HI_U8)*HI_ISP_NR_LUT_LEN * ISP_BAYER_CHN_NUM);
    memcpy(pstBayernr->au8LutWDRChromaRatio, au8LutWDRChromaRatio, sizeof(HI_U8)*HI_ISP_NR_LUT_LEN * ISP_BAYER_CHN_NUM);
    memcpy(pstBayernr->au8LutGCoringGain,    au8LutGCoringGain,    sizeof(HI_U8)*HI_ISP_BAYERNR_LUT_LENGTH);
    memcpy(pstBayernr->au16LutCenterLmt,     au16LutCenterLmt,     sizeof(HI_U16)*HI_ISP_NR_LUT_LEN);
    memcpy(pstBayernr->au16LutCoringHig,     au16LutCoringHig,     sizeof(HI_U16)*HI_ISP_NR_LUT_LEN);


}

static HI_S32 BayernrCheckCmosParam(VI_PIPE ViPipe, const ISP_CMOS_BAYERNR_S *pstBayerNr)
{
    HI_U8 i, j;

    ISP_CHECK_BOOL(pstBayerNr->bEnable);
    ISP_CHECK_BOOL(pstBayerNr->bBnrMonoSensorEn);
    ISP_CHECK_BOOL(pstBayerNr->bNrLscEnable);

    if (pstBayerNr->u16BnrLscCmpStrength > 0x100) {
        ISP_TRACE(HI_DBG_ERR, "err u16BnrLscCmpStrength!\n");
        return HI_ERR_ISP_ILLEGAL_PARAM;
    }

    if (pstBayerNr->u8BnrLscMaxGain > 0xBF) {
        ISP_TRACE(HI_DBG_ERR, "err u8BnrLscMaxGain!\n");
        return HI_ERR_ISP_ILLEGAL_PARAM;
    }
    for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++) {
        if (pstBayerNr->au8LutFineStr[i] > 128) {
            ISP_TRACE(HI_DBG_ERR, "err au8LutFineStr[%d]!\n", i);
            return HI_ERR_ISP_ILLEGAL_PARAM;
        }

        if (pstBayerNr->au16LutCoringWgt[i] > 3200) {
            ISP_TRACE(HI_DBG_ERR, "err au16CoringWgt[%d]!\n", i);
            return HI_ERR_ISP_ILLEGAL_PARAM;
        }
    }

    for (i = 0; i < HI_ISP_BAYERNR_LUT_LENGTH; i++) {
        if (pstBayerNr->au16LutCoringRatio[i] > 0x3ff) {
            ISP_TRACE(HI_DBG_ERR, "err au16CoringRatio[%d]!\n", i);
            return HI_ERR_ISP_ILLEGAL_PARAM;
        }
    }

    for (j = 0; j < ISP_BAYER_CHN_NUM; j++) {
        for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++) {
            if (pstBayerNr->au8ChromaStr[j][i] > 3) {
                ISP_TRACE(HI_DBG_ERR, "err au8ChromaStr[%d][%d]!\n", j, i);
                return HI_ERR_ISP_ILLEGAL_PARAM;
            }

            if (pstBayerNr->au16CoarseStr[j][i] > 0x360) {
                ISP_TRACE(HI_DBG_ERR, "err au16CoarseStr[%d][%d]!\n", j, i);
                return HI_ERR_ISP_ILLEGAL_PARAM;
            }
        }
    }

    for (i = 0; i < WDR_MAX_FRAME_NUM; i++) {
        if (pstBayerNr->au8WDRFrameStr[i] > 80) {
            ISP_TRACE(HI_DBG_ERR, "err au8WDRFrameStr[%d]!\n", i);
            return HI_ERR_ISP_ILLEGAL_PARAM;
        }

        if (pstBayerNr->au8FusionFrameStr[i] > 80) {
            ISP_TRACE(HI_DBG_ERR, "err au8FusionFrameStr[%d]!\n", i);
            return HI_ERR_ISP_ILLEGAL_PARAM;
        }
    }

    return HI_SUCCESS;
}

static HI_S32 BayernrExtRegsInitialize(VI_PIPE ViPipe)
{
    HI_U8  i;
    HI_S32 s32Ret;
    ISP_BAYERNR_S      *pstBayernr = HI_NULL;
    ISP_CMOS_DEFAULT_S *pstSnsDft  = HI_NULL;


    BAYERNR_GET_CTX(ViPipe, pstBayernr);
    ISP_SensorGetDefault(ViPipe, &pstSnsDft);

    NrInitFw(ViPipe);

    hi_ext_system_bayernr_manual_mode_write(ViPipe, HI_EXT_SYSTEM_BAYERNR_MANU_MODE_DEFAULT);
    hi_ext_system_bayernr_enable_write(ViPipe, HI_EXT_SYSTEM_BAYERNR_ENABLE_DEFAULT);
    hi_ext_system_bayernr_lsc_enable_write(ViPipe, HI_EXT_SYSTEM_BAYERNR_LSC_ENABLE_DEFAULT);
    hi_ext_system_bayernr_lsc_max_gain_write(ViPipe, HI_EXT_SYSTEM_BAYERNR_LSC_MAX_GAIN_DEFAULT);
    hi_ext_system_bayernr_lsc_cmp_strength_write(ViPipe, HI_EXT_SYSTEM_BAYERNR_LSC_CMP_STRENGTH_DEFAULT);
    hi_ext_system_bayernr_mono_sensor_write(ViPipe, HI_EXT_SYSTEM_BAYERNR_MONO_SENSOR_ENABLE_DEFAULT);
#ifndef VREG_OLD
    for (i = 0; i < HI_ISP_BAYERNR_LUT_LENGTH; i++) {
        pstBayernr->au16LutCoringRatio[i] = g_au16LutCoringRatio[i];
        hi_ext_system_bayernr_coring_ratio_write(ViPipe, i, pstBayernr->au16LutCoringRatio[i]);
    }
#else
    memcpy(pstBayernr->au16LutCoringRatio, g_au16LutCoringRatio, sizeof(HI_U16)*HI_ISP_BAYERNR_LUT_LENGTH);
    HI_U32 u32Addr;
    HI_U8 *pVirtAddr = HI_NULL;
    HI_U8 *pVirtAddrTmp = HI_NULL;
    u32Addr   = VREG_BNR_BASE(ViPipe);
    pVirtAddr = (HI_U8 *)VReg_GetVirtAddr(u32Addr) + 0x290;
    memcpy((HI_U16 *)pVirtAddr, pstBayernr->au16LutCoringRatio, HI_ISP_BAYERNR_LUT_LENGTH * sizeof(HI_U16));
#endif

#ifndef VREG_OLD
    for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++) {
        pstBayernr->stAuto.au8FineStr[i]       = g_au8LutFineStr[i];
        pstBayernr->stAuto.au8ChromaStr[0][i]  = g_au8ChromaStr[0][i];
        pstBayernr->stAuto.au8ChromaStr[1][i]  = g_au8ChromaStr[1][i];
        pstBayernr->stAuto.au8ChromaStr[2][i]  = g_au8ChromaStr[2][i];
        pstBayernr->stAuto.au8ChromaStr[3][i]  = g_au8ChromaStr[3][i];
        pstBayernr->stAuto.au16CoarseStr[0][i] = g_au16CoarseStr[0][i];
        pstBayernr->stAuto.au16CoarseStr[1][i] = g_au16CoarseStr[1][i];
        pstBayernr->stAuto.au16CoarseStr[2][i] = g_au16CoarseStr[2][i];
        pstBayernr->stAuto.au16CoarseStr[3][i] = g_au16CoarseStr[3][i];
        pstBayernr->stAuto.au16CoringWgt[i]    = g_au16LutCoringWgt[i];

    }
#else
    memcpy(pstBayernr->stAuto.au8FineStr, g_au8LutFineStr, ISP_AUTO_ISO_STRENGTH_NUM * sizeof(HI_U8));
    memcpy(pstBayernr->stAuto.au8ChromaStr, g_au8ChromaStr, ISP_AUTO_ISO_STRENGTH_NUM * 4 * sizeof(HI_U8));
    memcpy(pstBayernr->stAuto.au16CoarseStr, g_au16CoarseStr, ISP_AUTO_ISO_STRENGTH_NUM * 4 * sizeof(HI_U16));
    memcpy(pstBayernr->stAuto.au16CoringWgt, g_au16LutCoringWgt, ISP_AUTO_ISO_STRENGTH_NUM * sizeof(HI_U16));

#endif
    for (i = 0; i < WDR_MAX_FRAME_NUM; i++) {
        pstBayernr->stWDR.au8WDRFrameStr[i]    = g_au8WDRFrameStr[i];
        pstBayernr->stWDR.au8FusionFrameStr[i] = g_au8FusionFrameStr[i];
        hi_ext_system_bayernr_wdr_frame_strength_write(ViPipe, i, pstBayernr->stWDR.au8WDRFrameStr[i]);
        hi_ext_system_bayernr_fusion_frame_strength_write(ViPipe, i, pstBayernr->stWDR.au8FusionFrameStr[i]);
    }

    //Manual
    pstBayernr->stManual.u8FineStr        = HI_EXT_SYSTEM_BAYERNR_MANU_FINE_STRENGTH_DEFAULT;
    pstBayernr->stManual.au8ChromaStr[0]  = HI_EXT_SYSTEM_BAYERNR_MANU_CHROMA_STRENGTH_DEFAULT;
    pstBayernr->stManual.au8ChromaStr[1]  = HI_EXT_SYSTEM_BAYERNR_MANU_CHROMA_STRENGTH_DEFAULT;
    pstBayernr->stManual.au8ChromaStr[2]  = HI_EXT_SYSTEM_BAYERNR_MANU_CHROMA_STRENGTH_DEFAULT;
    pstBayernr->stManual.au8ChromaStr[3]  = HI_EXT_SYSTEM_BAYERNR_MANU_CHROMA_STRENGTH_DEFAULT;
    pstBayernr->stManual.au16CoarseStr[0] = HI_EXT_SYSTEM_BAYERNR_MANU_COARSE_STRENGTH_DEFAULT;
    pstBayernr->stManual.au16CoarseStr[1] = HI_EXT_SYSTEM_BAYERNR_MANU_COARSE_STRENGTH_DEFAULT;
    pstBayernr->stManual.au16CoarseStr[2] = HI_EXT_SYSTEM_BAYERNR_MANU_COARSE_STRENGTH_DEFAULT;
    pstBayernr->stManual.au16CoarseStr[3] = HI_EXT_SYSTEM_BAYERNR_MANU_COARSE_STRENGTH_DEFAULT;
    pstBayernr->stManual.u16CoringWgt     = HI_EXT_SYSTEM_BAYERNR_MANU_CORING_WEIGHT_DEFAULT;

    hi_ext_system_bayernr_manual_fine_strength_write(ViPipe, pstBayernr->stManual.u8FineStr);
    hi_ext_system_bayernr_manual_chroma_strength_write(ViPipe, 0, pstBayernr->stManual.au8ChromaStr[0]);
    hi_ext_system_bayernr_manual_chroma_strength_write(ViPipe, 1, pstBayernr->stManual.au8ChromaStr[1]);
    hi_ext_system_bayernr_manual_chroma_strength_write(ViPipe, 2, pstBayernr->stManual.au8ChromaStr[2]);
    hi_ext_system_bayernr_manual_chroma_strength_write(ViPipe, 3, pstBayernr->stManual.au8ChromaStr[3]);
    hi_ext_system_bayernr_manual_coarse_strength_write(ViPipe, 0, pstBayernr->stManual.au16CoarseStr[0]);
    hi_ext_system_bayernr_manual_coarse_strength_write(ViPipe, 1, pstBayernr->stManual.au16CoarseStr[1]);
    hi_ext_system_bayernr_manual_coarse_strength_write(ViPipe, 2, pstBayernr->stManual.au16CoarseStr[2]);
    hi_ext_system_bayernr_manual_coarse_strength_write(ViPipe, 3, pstBayernr->stManual.au16CoarseStr[3]);
    hi_ext_system_bayernr_manual_coring_weight_write(ViPipe, pstBayernr->stManual.u16CoringWgt);

    for (i = 0; i < CFG2VLD_DLY_LIMIT; i++) {
        pstBayernr->au16WDRSyncFrameThr[i][0] = hi_ext_system_wdr_longthr_read(ViPipe);
        pstBayernr->au16WDRSyncFrameThr[i][1] = hi_ext_system_wdr_shortthr_read(ViPipe);
    }

    if (pstSnsDft->unKey.bit1BayerNr) {
        ISP_CHECK_POINTER(pstSnsDft->pstBayerNr);

        s32Ret = BayernrCheckCmosParam(ViPipe, pstSnsDft->pstBayerNr);
        if (s32Ret != HI_SUCCESS) {
            return s32Ret;
        }

        hi_ext_system_bayernr_enable_write(ViPipe, pstSnsDft->pstBayerNr->bEnable);
        hi_ext_system_bayernr_lsc_enable_write(ViPipe, pstSnsDft->pstBayerNr->bNrLscEnable);
        hi_ext_system_bayernr_lsc_max_gain_write(ViPipe, pstSnsDft->pstBayerNr->u8BnrLscMaxGain);
        hi_ext_system_bayernr_lsc_cmp_strength_write(ViPipe, pstSnsDft->pstBayerNr->u16BnrLscCmpStrength);
        hi_ext_system_bayernr_mono_sensor_write(ViPipe, pstSnsDft->pstBayerNr->bBnrMonoSensorEn);

        for (i = 0; i < HI_ISP_BAYERNR_LUT_LENGTH; i++) {
            pstBayernr->au16LutCoringRatio[i] = pstSnsDft->pstBayerNr->au16LutCoringRatio[i];
            hi_ext_system_bayernr_coring_ratio_write(ViPipe, i, pstBayernr->au16LutCoringRatio[i]);
        }
#ifndef VREG_OLD
        for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++) { //Auto
            pstBayernr->stAuto.au8FineStr[i]       = pstSnsDft->pstBayerNr->au8LutFineStr[i];
            pstBayernr->stAuto.au8ChromaStr[0][i]  = pstSnsDft->pstBayerNr->au8ChromaStr[0][i];
            pstBayernr->stAuto.au8ChromaStr[1][i]  = pstSnsDft->pstBayerNr->au8ChromaStr[1][i];
            pstBayernr->stAuto.au8ChromaStr[2][i]  = pstSnsDft->pstBayerNr->au8ChromaStr[2][i];
            pstBayernr->stAuto.au8ChromaStr[3][i]  = pstSnsDft->pstBayerNr->au8ChromaStr[3][i];
            pstBayernr->stAuto.au16CoarseStr[0][i] = pstSnsDft->pstBayerNr->au16CoarseStr[0][i];
            pstBayernr->stAuto.au16CoarseStr[1][i] = pstSnsDft->pstBayerNr->au16CoarseStr[1][i];
            pstBayernr->stAuto.au16CoarseStr[2][i] = pstSnsDft->pstBayerNr->au16CoarseStr[2][i];
            pstBayernr->stAuto.au16CoarseStr[3][i] = pstSnsDft->pstBayerNr->au16CoarseStr[3][i];
            pstBayernr->stAuto.au16CoringWgt[i]    = pstSnsDft->pstBayerNr->au16LutCoringWgt[i];
        }
#else
        memcpy(pstBayernr->stAuto.au8FineStr, g_au8LutFineStr, ISP_AUTO_ISO_STRENGTH_NUM * sizeof(HI_U8));
        memcpy(pstBayernr->stAuto.au8ChromaStr, g_au8ChromaStr, ISP_AUTO_ISO_STRENGTH_NUM * 4 * sizeof(HI_U8));
        memcpy(pstBayernr->stAuto.au16CoarseStr, g_au16CoarseStr, ISP_AUTO_ISO_STRENGTH_NUM * 4 * sizeof(HI_U16));
        memcpy(pstBayernr->stAuto.au16CoringWgt, g_au16LutCoringWgt, ISP_AUTO_ISO_STRENGTH_NUM * sizeof(HI_U16));

#endif
        for (i = 0; i < WDR_MAX_FRAME_NUM; i++) {
            pstBayernr->stWDR.au8WDRFrameStr[i]    = pstSnsDft->pstBayerNr->au8WDRFrameStr[i];
            pstBayernr->stWDR.au8FusionFrameStr[i] = pstSnsDft->pstBayerNr->au8FusionFrameStr[i];

            hi_ext_system_bayernr_wdr_frame_strength_write(ViPipe, i, pstBayernr->stWDR.au8WDRFrameStr[i]);
            hi_ext_system_bayernr_fusion_frame_strength_write(ViPipe, i, pstBayernr->stWDR.au8FusionFrameStr[i]);
        }
    }
#ifndef VREG_OLD
    for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++) {


        hi_ext_system_bayernr_auto_fine_strength_write(ViPipe, i, pstBayernr->stAuto.au8FineStr[i]);
        hi_ext_system_bayernr_auto_chroma_strength_r_write(ViPipe, i, pstBayernr->stAuto.au8ChromaStr[0][i]);    //ChromaStrR
        hi_ext_system_bayernr_auto_chroma_strength_gr_write(ViPipe, i, pstBayernr->stAuto.au8ChromaStr[1][i]);  //ChromaStrGr
        hi_ext_system_bayernr_auto_chroma_strength_gb_write(ViPipe, i, pstBayernr->stAuto.au8ChromaStr[2][i]);  //ChromaStrGb
        hi_ext_system_bayernr_auto_chroma_strength_b_write(ViPipe, i, pstBayernr->stAuto.au8ChromaStr[3][i]);    //ChromaStrB
        hi_ext_system_bayernr_auto_coarse_strength_r_write(ViPipe, i, pstBayernr->stAuto.au16CoarseStr[0][i]);
        hi_ext_system_bayernr_auto_coarse_strength_gr_write(ViPipe, i, pstBayernr->stAuto.au16CoarseStr[1][i]);
        hi_ext_system_bayernr_auto_coarse_strength_gb_write(ViPipe, i, pstBayernr->stAuto.au16CoarseStr[2][i]);
        hi_ext_system_bayernr_auto_coarse_strength_b_write(ViPipe, i, pstBayernr->stAuto.au16CoarseStr[3][i]);
        hi_ext_system_bayernr_auto_coring_weight_write(ViPipe, i, pstBayernr->stAuto.au16CoringWgt[i]);
    }
#else
    u32Addr   = VREG_BNR_BASE(ViPipe);
    pVirtAddrTmp = (HI_U8 *)VReg_GetVirtAddr(u32Addr);
    pVirtAddr    =  pVirtAddrTmp + 0xe0;
    memcpy((HI_U8 *)pVirtAddr, pstBayernr->stAuto.au8FineStr, ISP_AUTO_ISO_STRENGTH_NUM * sizeof(HI_U8));

    pVirtAddr    =  pVirtAddrTmp + 0xa0;
    memcpy((HI_U8 *)pVirtAddr, pstBayernr->stAuto.au8ChromaStr, ISP_AUTO_ISO_STRENGTH_NUM * 4 * sizeof(HI_U8));

    pVirtAddr    =  pVirtAddrTmp + 0x20;
    memcpy((HI_U16 *)pVirtAddr, pstBayernr->stAuto.au16CoarseStr, ISP_AUTO_ISO_STRENGTH_NUM * 4 * sizeof(HI_U16));

    pVirtAddr    =  pVirtAddrTmp + 0x100;
    memcpy((HI_U16 *)pVirtAddr, pstBayernr->stAuto.au16CoringWgt, ISP_AUTO_ISO_STRENGTH_NUM * sizeof(HI_U16));
#endif
    pstBayernr->bInit    = HI_TRUE;

    return HI_SUCCESS;
}

static HI_VOID BayernrStaticRegsInitialize(VI_PIPE ViPipe, ISP_BAYERNR_STATIC_CFG_S *pstBayernrStaticRegCfg, HI_U8 i)
{
    ISP_CMOS_BLACK_LEVEL_S *pstSnsBlackLevel = HI_NULL;
    ISP_SensorGetBlc(ViPipe, &pstSnsBlackLevel);

    pstBayernrStaticRegCfg->u8JnlmSel           = HI_ISP_BNR_DEFAULT_JNLM_SEL;
    pstBayernrStaticRegCfg->u8SADWinSizeSel     = HI_ISP_BNR_DEFAULT_SAD_WINSIZE_SEL;
    pstBayernrStaticRegCfg->u16JnlmMaxWtCoef    = HI_ISP_BNR_DEFAULT_JNLM_MAX_WT_COEF;
    pstBayernrStaticRegCfg->bLumaSel            = HI_ISP_BNR_DEFAULT_LUMA_SEL;
    pstBayernrStaticRegCfg->u8WindowSizeSel     = HI_ISP_BNR_DEFAULT_WINDOW_SIZE_SEL;
    pstBayernrStaticRegCfg->bPatternNoiseReduction = HI_ISP_BNR_DEFAULT_PATTERN_NOISE_REDUCTION;
    pstBayernrStaticRegCfg->u8Gainsad              =  HI_ISP_BNR_DEFAULT_GAIN_SAD;
    pstBayernrStaticRegCfg->bImplsnrEnable1        =  HI_ISP_BNR_DEFAULT_IMPLS_NR_ENABLE1;
    pstBayernrStaticRegCfg->bImplsnrEnable2        =  HI_ISP_BNR_DEFAULT_IMPLS_NR_ENABLE2;
    pstBayernrStaticRegCfg->bImplsnrEnable3        =  HI_ISP_BNR_DEFAULT_IMPLS_NR_ENABLE3;
    pstBayernrStaticRegCfg->u8ImplsnrStrength      =  HI_ISP_BNR_DEFAULT_IMPLS_NR_STRENGTH;
    pstBayernrStaticRegCfg->bEnableSymmsad         =  HI_ISP_BNR_DEFAULT_SYMMSAD;
    pstBayernrStaticRegCfg->u8EnablePostProc       =  HI_ISP_BNR_DEFAULT_ENABLE_POST_PROC;
    pstBayernrStaticRegCfg->u8BilateralEnable      =  HI_ISP_BNR_DEFAULT_BILATERAL_ENABLE;

    pstBayernrStaticRegCfg->bResh = HI_TRUE;

    return;
}

static HI_VOID BayernrDynaRegsInitialize(VI_PIPE ViPipe, ISP_BAYERNR_DYNA_CFG_S *pstBayernrDynaRegCfg, ISP_CTX_S *pstIspCtx)
{
    HI_U8  u8WDRMode;
    HI_U16 j;

    ISP_BAYERNR_S *pstBayernr = HI_NULL;
    BAYERNR_GET_CTX(ViPipe, pstBayernr);

    u8WDRMode = pstIspCtx->u8SnsWDRMode;

    pstBayernrDynaRegCfg->bMedcEnable           = HI_TRUE;
    pstBayernrDynaRegCfg->bWdrModeEn            = HI_FALSE;
    pstBayernrDynaRegCfg->bWdrFusionEn          = HI_FALSE;
    pstBayernrDynaRegCfg->bCenterWgtEn          = HI_FALSE;
    pstBayernrDynaRegCfg->u8WdrMapFltMode       = HI_ISP_BNR_DEFAULT_LINEAR_WDRMAP_FLTMODE;
    pstBayernrDynaRegCfg->au8BnrCRatio[0]       = HI_ISP_BNR_DEFAULT_C_RATIO_R;
    pstBayernrDynaRegCfg->au8BnrCRatio[1]       = HI_ISP_BNR_DEFAULT_C_RATIO_GR;
    pstBayernrDynaRegCfg->au8BnrCRatio[2]       = HI_ISP_BNR_DEFAULT_C_RATIO_GB;
    pstBayernrDynaRegCfg->au8BnrCRatio[3]       = HI_ISP_BNR_DEFAULT_C_RATIO_B;
    pstBayernrDynaRegCfg->au8AmedMode[0]        = HI_ISP_BNR_DEFAULT_AMED_MODE_R;
    pstBayernrDynaRegCfg->au8AmedMode[1]        = HI_ISP_BNR_DEFAULT_AMED_MODE_GR;
    pstBayernrDynaRegCfg->au8AmedMode[2]        = HI_ISP_BNR_DEFAULT_AMED_MODE_GB;
    pstBayernrDynaRegCfg->au8AmedMode[3]        = HI_ISP_BNR_DEFAULT_AMED_MODE_B;
    pstBayernrDynaRegCfg->au8AmedLevel[0]       = HI_ISP_BNR_DEFAULT_AMED_LEVEL_R;
    pstBayernrDynaRegCfg->au8AmedLevel[1]       = HI_ISP_BNR_DEFAULT_AMED_LEVEL_GR;
    pstBayernrDynaRegCfg->au8AmedLevel[2]       = HI_ISP_BNR_DEFAULT_AMED_LEVEL_GB;
    pstBayernrDynaRegCfg->au8AmedLevel[3]       = HI_ISP_BNR_DEFAULT_AMED_LEVEL_B;
    pstBayernrDynaRegCfg->u8JnlmGain            = HI_ISP_BNR_DEFAULT_JNLM_GAIN;
    pstBayernrDynaRegCfg->u16JnlmCoringHig      = HI_ISP_BNR_DEFAULT_JNLM_CORING_HIGH;
    pstBayernrDynaRegCfg->u16ShotRatio          = HI_ISP_BNR_DEFAULT_SHOT_RATIO;
    pstBayernrDynaRegCfg->u16RLmtRgain          = HI_ISP_BNR_DEFAULT_RLMT_RGAIN;
    pstBayernrDynaRegCfg->u16RLmtBgain          = HI_ISP_BNR_DEFAULT_RLMT_BGAIN;
    pstBayernrDynaRegCfg->u32EdgeDetThr         = HI_ISP_BNR_DEFAULT_EDGE_DET_THRESHOLD_LINEAR;
    pstBayernr->u32EdgeDetThr                   = HI_ISP_BNR_DEFAULT_EDGE_DET_THRESHOLD_LINEAR;
    pstBayernr->u8WdrMapGain                    = HI_ISP_BNR_DEFAULT_LINEAR_WDRMAP_GAIN;

#ifndef VREG_OLD
    for (j = 0; j < HI_ISP_BAYERNR_LMTLUTNUM; j++) {
        pstBayernrDynaRegCfg->au8JnlmLimitLut[j] = 0;
        pstBayernrDynaRegCfg->au8JnlmLimitSLut[j] = 0;
    }
#else
    memset(pstBayernrDynaRegCfg->au8JnlmLimitLut, 0, HI_ISP_BAYERNR_LMTLUTNUM);
    memset(pstBayernrDynaRegCfg->au8JnlmLimitSLut, 0, HI_ISP_BAYERNR_LMTLUTNUM);
#endif

    for (j = 0; j < HI_ISP_BAYERNR_LUT_LENGTH; j++) {
        pstBayernrDynaRegCfg->au16JnlmCoringLowLut[j] = 0;
        pstBayernrDynaRegCfg->au8GcoringLut[j] = 16;
    }


    for (j = 0; j < ISP_BAYER_CHN_NUM; j++) {
        pstBayernrDynaRegCfg->au32JnlmLimitMultGain[j] = 0;
        pstBayernrDynaRegCfg->au32JnlmLimitMultGainS[j] = 0;
    }
    //pstBayernrDynaRegCfg->bBnrLutUpdateEn = HI_TRUE;


    if (IS_2to1_WDR_MODE(u8WDRMode)) {
        pstBayernr->u8WdrFramesMerge          = 2;
        pstBayernr->au32ExpoValues[0]         = 64;
        pstBayernr->u8WdrMapGain              = HI_ISP_BNR_DEFAULT_WDR_WDRMAP_GAIN;
        pstBayernrDynaRegCfg->u8WdrMapGain    = HI_ISP_BNR_DEFAULT_WDR_WDRMAP_GAIN;
        pstBayernrDynaRegCfg->u8WdrMapFltMode = HI_ISP_BNR_DEFAULT_WDR_WDRMAP_FLTMODE;
        pstBayernrDynaRegCfg->u8JnlmSymCoef   = HI_ISP_BNR_DEFAULT_JNLM_SYMCOEF_WDR;
        pstBayernrDynaRegCfg->u32EdgeDetThr   = HI_ISP_BNR_DEFAULT_EDGE_DET_THRESHOLD_WDR;
        pstBayernr->u32EdgeDetThr             = HI_ISP_BNR_DEFAULT_EDGE_DET_THRESHOLD_WDR;
    } else {
        pstBayernr->u8WdrFramesMerge        = 1;
        pstBayernr->u16WDRBlcThr            = 0;
        pstBayernrDynaRegCfg->u8JnlmSymCoef = HI_ISP_BNR_DEFAULT_JNLM_SYMCOEF_LINEAR;
        pstBayernrDynaRegCfg->u32EdgeDetThr = HI_ISP_BNR_DEFAULT_EDGE_DET_THRESHOLD_LINEAR;
        pstBayernr->u32EdgeDetThr           = HI_ISP_BNR_DEFAULT_EDGE_DET_THRESHOLD_LINEAR;
    }

    pstBayernrDynaRegCfg->bResh = HI_TRUE;
}

static HI_VOID BayernrUsrRegsInitialize(ISP_BAYERNR_USR_CFG_S *pstBayernrUsrRegCfg, ISP_CMOS_DEFAULT_S *pstSnsDft)
{
    ISP_CHECK_POINTER_VOID(pstSnsDft->pstBayerNr);

    pstBayernrUsrRegCfg->bBnrLscEn            = (pstSnsDft->pstBayerNr->bNrLscEnable == 0) ? 0 : 3;
    pstBayernrUsrRegCfg->bBnrMonoSensorEn     = pstSnsDft->pstBayerNr->bBnrMonoSensorEn;
    pstBayernrUsrRegCfg->u8BnrLscMaxGain      = pstSnsDft->pstBayerNr->u8BnrLscMaxGain + 64;
    pstBayernrUsrRegCfg->u16BnrLscCmpStrength = pstSnsDft->pstBayerNr->u16BnrLscCmpStrength;
    pstBayernrUsrRegCfg->bResh                = HI_TRUE;
}

static HI_VOID BayernrRegsInitialize(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfg)
{
    HI_U8  i;
    HI_U8  u8BlockNum;
    ISP_CTX_S   *pstIspCtx = HI_NULL;
    ISP_CMOS_DEFAULT_S       *pstSnsDft              = HI_NULL;
    ISP_CMOS_BLACK_LEVEL_S   *pstSnsBlackLevel       = HI_NULL;
    ISP_BAYERNR_STATIC_CFG_S *pstBayernrStaticRegCfg = HI_NULL;
    ISP_BAYERNR_DYNA_CFG_S   *pstBayernrDynaRegCfg   = HI_NULL;
    ISP_BAYERNR_USR_CFG_S    *pstBayernrUsrRegCfg    = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);
    ISP_SensorGetDefault(ViPipe, &pstSnsDft);
    ISP_SensorGetBlc(ViPipe, &pstSnsBlackLevel);

    u8BlockNum = pstIspCtx->stBlockAttr.u8BlockNum;

    for (i = 0; i < u8BlockNum; i++) {

        pstBayernrStaticRegCfg = &pstRegCfg->stAlgRegCfg[i].stBnrRegCfg.stStaticRegCfg;
        pstBayernrDynaRegCfg   = &pstRegCfg->stAlgRegCfg[i].stBnrRegCfg.stDynaRegCfg;
        pstBayernrUsrRegCfg    = &pstRegCfg->stAlgRegCfg[i].stBnrRegCfg.stUsrRegCfg;

        pstRegCfg->stAlgRegCfg[i].stBnrRegCfg.bBnrEnable               = HI_TRUE;
        //pstRegCfg->stAlgRegCfg[i].stDeRegCfg.stKernelRegCfg.bBnrEnable = HI_TRUE;
        BayernrStaticRegsInitialize(ViPipe, pstBayernrStaticRegCfg, i);
        BayernrDynaRegsInitialize(ViPipe, pstBayernrDynaRegCfg, pstIspCtx);
        BayernrUsrRegsInitialize(pstBayernrUsrRegCfg, pstSnsDft);
    }

    pstRegCfg->unKey.bit1BayernrCfg = 1;
}

static HI_S32 BayernrReadExtregs(VI_PIPE ViPipe, ISP_BE_BLC_CFG_S *pstBeBlcCfg)
{
    HI_U8  i, u8WDRMode;
    HI_U32 au32ExpRatio[3] = {0};
    ISP_BAYERNR_S *pstBayernr = HI_NULL;
    ISP_CTX_S     *pstIspCtx  = HI_NULL;

    BAYERNR_GET_CTX(ViPipe, pstBayernr);
    ISP_GET_CTX(ViPipe, pstIspCtx);

    pstBayernr->enOpType             = hi_ext_system_bayernr_manual_mode_read(ViPipe);
    pstBayernr->bNrLscEnable         = hi_ext_system_bayernr_lsc_enable_read(ViPipe);
    pstBayernr->bBnrMonoSensorEn     = hi_ext_system_bayernr_mono_sensor_read(ViPipe);
    pstBayernr->bWdrModeEn           = hi_ext_system_wdr_en_read(ViPipe);
    pstBayernr->bWdrFusionEn         = hi_ext_system_fusion_mode_read(ViPipe);
    pstBayernr->u8BnrLscMaxGain      = hi_ext_system_bayernr_lsc_max_gain_read(ViPipe);
    pstBayernr->u16BnrLscCmpStrength = hi_ext_system_bayernr_lsc_cmp_strength_read(ViPipe);

    //pstBayernr->u8WindowSizeSel = hi_ext_system_bayernr_window_size_sel_read(ViPipe);
    //pstBayernr->bPatternNoiseReduction = hi_ext_system_bayernr_pattern_noise_reduction_read(ViPipe);
    //pstBayernr->u8Gainsad           = hi_ext_system_bayernr_gain_sad_read(ViPipe);
    //pstBayernr->bImplsnrEnable1 = hi_ext_system_bayernr_impls_nr_enable1_read(ViPipe);
    //pstBayernr->bImplsnrEnable2 = hi_ext_system_bayernr_impls_nr_enable2_read(ViPipe);
    //pstBayernr->bImplsnrEnable3 = hi_ext_system_bayernr_impls_nr_enable3_read(ViPipe);
    //pstBayernr->u8ImplsnrStrength = hi_ext_system_bayernr_impls_nr_strength_read(ViPipe);
    //pstBayernr->bEnableSymmsad = hi_ext_system_bayernr_symmsad_read(ViPipe);
    //pstBayernr->u8EnablePostProc = hi_ext_system_bayernr_enable_post_proc_read(ViPipe);
    //pstBayernr->u8nLimitGainBlt  = hi_ext_system_bayernr_nlimit_gain_blt_read(ViPipe);
    //pstBayernr->u8BilateralEnable = hi_ext_system_bayernr_bilateral_enable_read(ViPipe);


    for (i = 0; i < HI_ISP_BAYERNR_LUT_LENGTH; i++) {
        pstBayernr->au16LutCoringRatio[i] = hi_ext_system_bayernr_coring_ratio_read(ViPipe, i);
    }

    //for (i = 0; i < HI_ISP_BAYERNR_LUT_LENGTH; i++)
    //{
    //    pstBayernr->au8LutGCoringGain[i] = hi_ext_system_bayernr_gcoring_gain_read(ViPipe, i);
    //}

    for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++) {
        pstBayernr->stAuto.au8FineStr[i]       = hi_ext_system_bayernr_auto_fine_strength_read(ViPipe, i);
        pstBayernr->stAuto.au16CoringWgt[i]    = hi_ext_system_bayernr_auto_coring_weight_read(ViPipe, i);
        pstBayernr->stAuto.au8ChromaStr[0][i]  = hi_ext_system_bayernr_auto_chroma_strength_r_read(ViPipe, i);
        pstBayernr->stAuto.au8ChromaStr[1][i]  = hi_ext_system_bayernr_auto_chroma_strength_gr_read(ViPipe, i);
        pstBayernr->stAuto.au8ChromaStr[2][i]  = hi_ext_system_bayernr_auto_chroma_strength_gb_read(ViPipe, i);
        pstBayernr->stAuto.au8ChromaStr[3][i]  = hi_ext_system_bayernr_auto_chroma_strength_b_read(ViPipe, i);
        pstBayernr->stAuto.au16CoarseStr[0][i] = hi_ext_system_bayernr_auto_coarse_strength_r_read(ViPipe, i);
        pstBayernr->stAuto.au16CoarseStr[1][i] = hi_ext_system_bayernr_auto_coarse_strength_gr_read(ViPipe, i);
        pstBayernr->stAuto.au16CoarseStr[2][i] = hi_ext_system_bayernr_auto_coarse_strength_gb_read(ViPipe, i);
        pstBayernr->stAuto.au16CoarseStr[3][i] = hi_ext_system_bayernr_auto_coarse_strength_b_read(ViPipe, i);
    }

    pstBayernr->stManual.u8FineStr        = hi_ext_system_bayernr_manual_fine_strength_read(ViPipe);
    pstBayernr->stManual.u16CoringWgt     = hi_ext_system_bayernr_manual_coring_weight_read(ViPipe);
    pstBayernr->stManual.au8ChromaStr[0]  = hi_ext_system_bayernr_manual_chroma_strength_read(ViPipe, 0);
    pstBayernr->stManual.au8ChromaStr[1]  = hi_ext_system_bayernr_manual_chroma_strength_read(ViPipe, 1);
    pstBayernr->stManual.au8ChromaStr[2]  = hi_ext_system_bayernr_manual_chroma_strength_read(ViPipe, 2);
    pstBayernr->stManual.au8ChromaStr[3]  = hi_ext_system_bayernr_manual_chroma_strength_read(ViPipe, 3);
    pstBayernr->stManual.au16CoarseStr[0] = hi_ext_system_bayernr_manual_coarse_strength_read(ViPipe, 0);
    pstBayernr->stManual.au16CoarseStr[1] = hi_ext_system_bayernr_manual_coarse_strength_read(ViPipe, 1);
    pstBayernr->stManual.au16CoarseStr[2] = hi_ext_system_bayernr_manual_coarse_strength_read(ViPipe, 2);
    pstBayernr->stManual.au16CoarseStr[3] = hi_ext_system_bayernr_manual_coarse_strength_read(ViPipe, 3);

    for (i = 0; i < WDR_MAX_FRAME_NUM; i++) {
        pstBayernr->stWDR.au8WDRFrameStr[i]    =  hi_ext_system_bayernr_wdr_frame_strength_read(ViPipe, i);
        pstBayernr->stWDR.au8FusionFrameStr[i] =  hi_ext_system_bayernr_fusion_frame_strength_read(ViPipe, i);
    }

    u8WDRMode = pstIspCtx->u8SnsWDRMode;
    memcpy(au32ExpRatio, pstIspCtx->stLinkage.au32ExpRatio, sizeof(au32ExpRatio));

    if (IS_2to1_WDR_MODE(u8WDRMode)) {
        pstBayernr->au32ExpoValues[1] = (HI_U32)au32ExpRatio[0];
        pstBayernr->u16WDRBlcThr      = pstBeBlcCfg->stWdrBlc[0].stUsrRegCfg.u16OutBlc;
    }


    return HI_SUCCESS;
}

static HI_S32 BayernrReadProMode(VI_PIPE ViPipe)
{
    HI_U8 i;
    ISP_BAYERNR_S *pstBayernr = HI_NULL;
    ISP_CTX_S *pstIspCtx = HI_NULL;
    HI_U8 u8Index = 0;
    HI_U8 u8IndexMaxValue = 0;
    BAYERNR_GET_CTX(ViPipe, pstBayernr);
    ISP_CHECK_POINTER(pstBayernr);
    ISP_GET_CTX(ViPipe, pstIspCtx);
    if (pstIspCtx->stProNrParamCtrl.pstProNrParam->bEnable == HI_TRUE) {
        u8Index = pstIspCtx->stLinkage.u8ProIndex;
        u8IndexMaxValue = MIN2(pstIspCtx->stProShpParamCtrl.pstProShpParam->u32ParamNum, PRO_MAX_FRAME_NUM);
        if (u8Index > u8IndexMaxValue) {
            u8Index =  u8IndexMaxValue;
        }

        if (u8Index < 1) {
            return HI_SUCCESS;
        }
        u8Index -= 1;
    } else {
        return HI_SUCCESS;
    }
    pstBayernr->enOpType = OP_TYPE_AUTO;
    for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++) {
        pstBayernr->stAuto.au8FineStr[i]       = pstIspCtx->stProNrParamCtrl.pstProNrParam->astNrAttr[u8Index].au8FineStr[i];
        pstBayernr->stAuto.au16CoringWgt[i]    = pstIspCtx->stProNrParamCtrl.pstProNrParam->astNrAttr[u8Index].au16CoringWgt[i];
        pstBayernr->stAuto.au8ChromaStr[0][i]  = pstIspCtx->stProNrParamCtrl.pstProNrParam->astNrAttr[u8Index].au8ChromaStr[0][i];
        pstBayernr->stAuto.au8ChromaStr[1][i]  = pstIspCtx->stProNrParamCtrl.pstProNrParam->astNrAttr[u8Index].au8ChromaStr[1][i];
        pstBayernr->stAuto.au8ChromaStr[2][i]  = pstIspCtx->stProNrParamCtrl.pstProNrParam->astNrAttr[u8Index].au8ChromaStr[2][i];
        pstBayernr->stAuto.au8ChromaStr[3][i]  = pstIspCtx->stProNrParamCtrl.pstProNrParam->astNrAttr[u8Index].au8ChromaStr[3][i];
        pstBayernr->stAuto.au16CoarseStr[0][i] = pstIspCtx->stProNrParamCtrl.pstProNrParam->astNrAttr[u8Index].au16CoarseStr[0][i];
        pstBayernr->stAuto.au16CoarseStr[1][i] = pstIspCtx->stProNrParamCtrl.pstProNrParam->astNrAttr[u8Index].au16CoarseStr[1][i];
        pstBayernr->stAuto.au16CoarseStr[2][i] = pstIspCtx->stProNrParamCtrl.pstProNrParam->astNrAttr[u8Index].au16CoarseStr[2][i];
        pstBayernr->stAuto.au16CoarseStr[3][i] = pstIspCtx->stProNrParamCtrl.pstProNrParam->astNrAttr[u8Index].au16CoarseStr[3][i];
    }
    return HI_SUCCESS;
}
HI_S32 BayernrProcWrite(VI_PIPE ViPipe, ISP_CTRL_PROC_WRITE_S *pstProc)
{
    ISP_CTRL_PROC_WRITE_S stProcTmp;
    ISP_BAYERNR_S *pstBayernr = HI_NULL;

    BAYERNR_GET_CTX(ViPipe, pstBayernr);
    ISP_CHECK_POINTER(pstBayernr);

    if ((pstProc->pcProcBuff == HI_NULL)
        || (pstProc->u32BuffLen == 0)) {
        return HI_FAILURE;
    }

    stProcTmp.pcProcBuff = pstProc->pcProcBuff;
    stProcTmp.u32BuffLen = pstProc->u32BuffLen;

    ISP_PROC_PRINTF(&stProcTmp, pstProc->u32WriteLen,
                    "-----BAYERNR INFO----------------------------------------------------------------------------\n");

    ISP_PROC_PRINTF(&stProcTmp, pstProc->u32WriteLen,
                    "%16s"      "%16s"             "%16s"      "%16s"       "%16s"        "%16s\n",
                    "Enable",  "NrLscEnable",   "CoarseStr0", "CoarseStr1", "CoarseStr2", "CoarseStr3" );

    ISP_PROC_PRINTF(&stProcTmp, pstProc->u32WriteLen,
                    "%16u"   "%16u"   "%16u"   "%16u"  "%16u"   "%16u\n",
                    pstBayernr->bEnable,
                    (HI_U16)pstBayernr->bNrLscEnable,
                    (HI_U16)pstBayernr->au16LmtStrength[0],
                    (HI_U16)pstBayernr->au16LmtStrength[1],
                    (HI_U16)pstBayernr->au16LmtStrength[2],
                    (HI_U16)pstBayernr->au16LmtStrength[3]
                   );

    pstProc->u32WriteLen += 1;

    return HI_SUCCESS;
}


HI_S32 ISP_BayernrInit(VI_PIPE ViPipe, HI_VOID *pRegCfg)
{
    HI_S32 s32Ret = HI_SUCCESS;
    ISP_REG_CFG_S *pstRegCfg = (ISP_REG_CFG_S *)pRegCfg;

    s32Ret = BayerNrCtxInit(ViPipe);
    if (s32Ret != HI_SUCCESS) {
        return s32Ret;
    }

    s32Ret = BayernrExtRegsInitialize(ViPipe);
    if (s32Ret != HI_SUCCESS) {
        return s32Ret;
    }
    BayernrRegsInitialize(ViPipe, pstRegCfg);

    return HI_SUCCESS;
}

HI_U16 NRGetValueFromLut_fix(HI_U32 x, HI_U32 *pLutX, HI_U16 *pLutY, HI_U32 length)
{
    HI_S32 j;

    if (x <= pLutX[0]) {
        return pLutY[0];
    }
    for (j = 1; j < length; j++) {
        if (x <= pLutX[j]) {
            if (pLutY[j] < pLutY[j - 1]) {
                return (HI_U16)(pLutY[j - 1] - (pLutY[j - 1] - pLutY[j]) * (HI_U16)(x - pLutX[j - 1]) / DIV_0_TO_1((HI_U16)(pLutX[j] - pLutX[j - 1])));
            } else {
                return (HI_U16)(pLutY[j - 1] + (pLutY[j] - pLutY[j - 1]) * (HI_U16)(x - pLutX[j - 1]) / DIV_0_TO_1((HI_U16)(pLutX[j] - pLutX[j - 1])));
            }
        }
    }
    return pLutY[length - 1];
}

#define  BNR_EPS (0.000001f)
#define  BNR_COL_ISO      0
#define  BNR_COL_K        1
#define  BNR_COL_B        2

static HI_FLOAT Bayernr_getKfromNoiseLut(HI_FLOAT (*pRecord)[3], HI_U16 recordNum, HI_S32 iso)
{
    HI_S32 i = 0;
    HI_FLOAT  y_diff = 0, x_diff = 0;
    HI_FLOAT k = 0.0f;

    if (recordNum > BAYER_CALIBTAION_MAX_NUM) {
        k = pRecord[BAYER_CALIBTAION_MAX_NUM - 1][BNR_COL_K];
        return k;
    }

    // record: iso - k
    if (iso <= pRecord[0][BNR_COL_ISO]) {
        k = pRecord[0][BNR_COL_K];
        //return k;
    }

    if (iso >= pRecord[recordNum - 1][BNR_COL_ISO]) {
        k = pRecord[recordNum - 1][BNR_COL_K];
        //return k;
    }

    for (i = 0; i < recordNum - 1; i++) {
        if (iso >= pRecord[i][BNR_COL_ISO] && iso <= pRecord[i + 1][BNR_COL_ISO]) {
            x_diff = pRecord[i + 1][BNR_COL_ISO] - pRecord[i][BNR_COL_ISO];  // iso diff
            y_diff = pRecord[i + 1][BNR_COL_K]  - pRecord[i][BNR_COL_K];     // k diff
            k = pRecord[i][BNR_COL_K] + y_diff * (iso - pRecord[i][BNR_COL_ISO]) / DIV_0_TO_1_FLOAT(x_diff + BNR_EPS);
            //return k;
        }
    }
    return k;
}

static HI_FLOAT Bayernr_getBfromNoiseLut(HI_FLOAT (*pRecord)[3], HI_U16 recordNum, HI_S32 iso)
{
    HI_S32 i = 0;
    HI_FLOAT  y_diff = 0, x_diff = 0;
    HI_FLOAT b = 0.0f;

    if (recordNum > BAYER_CALIBTAION_MAX_NUM) {
        b = pRecord[BAYER_CALIBTAION_MAX_NUM - 1][BNR_COL_B];
        return b;
    }
    // record: iso - b
    if (iso <= pRecord[0][BNR_COL_ISO]) {
        b = pRecord[0][BNR_COL_B];
        //return b;
    }

    if (iso >= pRecord[recordNum - 1][BNR_COL_ISO]) {
        b = pRecord[recordNum - 1][BNR_COL_B];
        //return b;
    }

    for (i = 0; i < recordNum - 1; i++) {
        if (iso >= pRecord[i][BNR_COL_ISO] && iso <= pRecord[i + 1][BNR_COL_ISO]) {
            x_diff = pRecord[i + 1][BNR_COL_ISO] - pRecord[i][BNR_COL_ISO];  // iso diff
            y_diff = pRecord[i + 1][BNR_COL_B]  - pRecord[i][BNR_COL_B];     // k diff
            b = pRecord[i][BNR_COL_B] + y_diff * (iso - pRecord[i][BNR_COL_ISO]) / DIV_0_TO_1_FLOAT(x_diff + BNR_EPS);
            //return b;
        }
    }
    return b;
}


HI_U32 NRSqrt64(HI_U64 u64Arg)
{
    HI_U64 u64Mask = (HI_U64)1 << 31;
    HI_U32 u32Res  = 0;
    HI_U32 i = 0;

    for (i = 0; i < 32; i++) {
        if ((u32Res + (u64Mask >> i)) * (u32Res + (u64Mask >> i)) <= u64Arg) {
            u32Res = u32Res + (u64Mask >> i);
        }
    }

    /* rounding */
    if ((HI_U64)u32Res * u32Res + u32Res < u64Arg) {
        ++u32Res;
    }

    return u32Res;
}

static HI_S32 BayerNR_SetLongFrameMode(VI_PIPE ViPipe, ISP_BAYERNR_DYNA_CFG_S *pstBayernrDynaCfg, ISP_BAYERNR_S *pstBayernr, HI_U8 u8IsoIndexUpper, HI_U8 u8IsoIndexLower, HI_U32 u32ISO2, HI_U32 u32ISO1, HI_U32 u32Iso )
{
    ISP_CTX_S          *pstIspCtx   = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);

    if ( (pstIspCtx->stLinkage.enFSWDRMode == ISP_FSWDR_LONG_FRAME_MODE) || (pstIspCtx->stLinkage.enFSWDRMode == ISP_FSWDR_AUTO_LONG_FRAME_MODE)) {
        pstBayernrDynaCfg->au8BnrCRatio[BAYER_RGGB]  = (HI_U8)LinearInter(u32Iso, u32ISO1, pstBayernr->au8LutChromaRatio[BAYER_RGGB][u8IsoIndexLower], \
                                                                                  u32ISO2, pstBayernr->au8LutChromaRatio[BAYER_RGGB][u8IsoIndexUpper]);
        pstBayernrDynaCfg->au8BnrCRatio[BAYER_GRBG]  = (HI_U8)LinearInter(u32Iso, u32ISO1, pstBayernr->au8LutChromaRatio[BAYER_GRBG][u8IsoIndexLower], \
                                                                                  u32ISO2, pstBayernr->au8LutChromaRatio[BAYER_GRBG][u8IsoIndexUpper]);
        pstBayernrDynaCfg->au8BnrCRatio[BAYER_GBRG]  = (HI_U8)LinearInter(u32Iso, u32ISO1, pstBayernr->au8LutChromaRatio[BAYER_GBRG][u8IsoIndexLower], \
                                                                                  u32ISO2, pstBayernr->au8LutChromaRatio[BAYER_GBRG][u8IsoIndexUpper]);
        pstBayernrDynaCfg->au8BnrCRatio[BAYER_BGGR]  = (HI_U8)LinearInter(u32Iso, u32ISO1, pstBayernr->au8LutChromaRatio[BAYER_BGGR][u8IsoIndexLower], \
                                                                                  u32ISO2, pstBayernr->au8LutChromaRatio[BAYER_BGGR][u8IsoIndexUpper]);

    } else {
        pstBayernrDynaCfg->au8BnrCRatio[BAYER_RGGB]  = (HI_U8)LinearInter(u32Iso, u32ISO1, pstBayernr->au8LutWDRChromaRatio[BAYER_RGGB][u8IsoIndexLower], \
                                                                                  u32ISO2, pstBayernr->au8LutWDRChromaRatio[BAYER_RGGB][u8IsoIndexUpper]);
        pstBayernrDynaCfg->au8BnrCRatio[BAYER_GRBG]  = (HI_U8)LinearInter(u32Iso, u32ISO1, pstBayernr->au8LutWDRChromaRatio[BAYER_GRBG][u8IsoIndexLower], \
                                                                                  u32ISO2, pstBayernr->au8LutWDRChromaRatio[BAYER_GRBG][u8IsoIndexUpper]);
        pstBayernrDynaCfg->au8BnrCRatio[BAYER_GBRG]  = (HI_U8)LinearInter(u32Iso, u32ISO1, pstBayernr->au8LutWDRChromaRatio[BAYER_GBRG][u8IsoIndexLower], \
                                                                                  u32ISO2, pstBayernr->au8LutWDRChromaRatio[BAYER_GBRG][u8IsoIndexUpper]);
        pstBayernrDynaCfg->au8BnrCRatio[BAYER_BGGR]  = (HI_U8)LinearInter(u32Iso, u32ISO1, pstBayernr->au8LutWDRChromaRatio[BAYER_BGGR][u8IsoIndexLower], \
                                                                                  u32ISO2, pstBayernr->au8LutWDRChromaRatio[BAYER_BGGR][u8IsoIndexUpper]);
    }

    return HI_SUCCESS;
}


static HI_S32 NRCfg(ISP_BAYERNR_DYNA_CFG_S *pstBayernrDynaCfg, ISP_BAYERNR_S *pstBayernr, HI_U8 u8IsoIndexUpper, HI_U8 u8IsoIndexLower, HI_U32 u32ISO2, HI_U32 u32ISO1, HI_U32 u32Iso)
{
    HI_U8  u8MaxCRaio, u8MaxCRaio01, u8MaxCRaio23;

    if (pstBayernr->u8WdrFramesMerge > 1) {
        pstBayernrDynaCfg->au8BnrCRatio[BAYER_RGGB]  = (HI_U8)LinearInter(u32Iso, u32ISO1, pstBayernr->au8LutWDRChromaRatio[BAYER_RGGB][u8IsoIndexLower], \
                                                                                  u32ISO2, pstBayernr->au8LutWDRChromaRatio[BAYER_RGGB][u8IsoIndexUpper]);
        pstBayernrDynaCfg->au8BnrCRatio[BAYER_GRBG]  = (HI_U8)LinearInter(u32Iso, u32ISO1, pstBayernr->au8LutWDRChromaRatio[BAYER_GRBG][u8IsoIndexLower], \
                                                                                  u32ISO2, pstBayernr->au8LutWDRChromaRatio[BAYER_GRBG][u8IsoIndexUpper]);
        pstBayernrDynaCfg->au8BnrCRatio[BAYER_GBRG]  = (HI_U8)LinearInter(u32Iso, u32ISO1, pstBayernr->au8LutWDRChromaRatio[BAYER_GBRG][u8IsoIndexLower], \
                                                                                  u32ISO2, pstBayernr->au8LutWDRChromaRatio[BAYER_GBRG][u8IsoIndexUpper]);
        pstBayernrDynaCfg->au8BnrCRatio[BAYER_BGGR]  = (HI_U8)LinearInter(u32Iso, u32ISO1, pstBayernr->au8LutWDRChromaRatio[BAYER_BGGR][u8IsoIndexLower], \
                                                                                  u32ISO2, pstBayernr->au8LutWDRChromaRatio[BAYER_BGGR][u8IsoIndexUpper]);
    } else {
        pstBayernrDynaCfg->au8BnrCRatio[BAYER_RGGB]  = (HI_U8)LinearInter(u32Iso, u32ISO1, pstBayernr->au8LutChromaRatio[BAYER_RGGB][u8IsoIndexLower], \
                                                                                  u32ISO2, pstBayernr->au8LutChromaRatio[BAYER_RGGB][u8IsoIndexUpper]);
        pstBayernrDynaCfg->au8BnrCRatio[BAYER_GRBG]  = (HI_U8)LinearInter(u32Iso, u32ISO1, pstBayernr->au8LutChromaRatio[BAYER_GRBG][u8IsoIndexLower], \
                                                                                  u32ISO2, pstBayernr->au8LutChromaRatio[BAYER_GRBG][u8IsoIndexUpper]);
        pstBayernrDynaCfg->au8BnrCRatio[BAYER_GBRG]  = (HI_U8)LinearInter(u32Iso, u32ISO1, pstBayernr->au8LutChromaRatio[BAYER_GBRG][u8IsoIndexLower], \
                                                                                  u32ISO2, pstBayernr->au8LutChromaRatio[BAYER_GBRG][u8IsoIndexUpper]);
        pstBayernrDynaCfg->au8BnrCRatio[BAYER_BGGR]  = (HI_U8)LinearInter(u32Iso, u32ISO1, pstBayernr->au8LutChromaRatio[BAYER_BGGR][u8IsoIndexLower], \
                                                                                  u32ISO2, pstBayernr->au8LutChromaRatio[BAYER_BGGR][u8IsoIndexUpper]);
    }

    pstBayernr->u16CenterLmt  = (HI_U16)LinearInter(u32Iso, u32ISO1, pstBayernr->au16LutCenterLmt[u8IsoIndexLower], \
                                                            u32ISO2, pstBayernr->au16LutCenterLmt[u8IsoIndexUpper]);
    pstBayernrDynaCfg->u16JnlmCoringHig  = (HI_U16)LinearInter(u32Iso, u32ISO1, pstBayernr->au16LutCoringHig[u8IsoIndexLower], \
                                                                       u32ISO2, pstBayernr->au16LutCoringHig[u8IsoIndexUpper]);
    pstBayernrDynaCfg->u16JnlmCoringHig  = (HI_U16)(256 * ((HI_FLOAT)pstBayernrDynaCfg->u16JnlmCoringHig / (HI_FLOAT)HI_ISP_BAYERNR_STRENGTH_DIVISOR));

    pstBayernrDynaCfg->au8AmedMode[BAYER_RGGB] = (u32Iso < 5000) ? 0 : 1;
    pstBayernrDynaCfg->au8AmedMode[BAYER_GRBG] = 0;
    pstBayernrDynaCfg->au8AmedMode[BAYER_GBRG] = 0;
    pstBayernrDynaCfg->au8AmedMode[BAYER_BGGR] = (u32Iso < 5000) ? 0 : 1;

    u8MaxCRaio01 = MAX2(pstBayernrDynaCfg->au8BnrCRatio[BAYER_RGGB], pstBayernrDynaCfg->au8BnrCRatio[BAYER_GRBG]);
    u8MaxCRaio23 = MAX2(pstBayernrDynaCfg->au8BnrCRatio[BAYER_GBRG], pstBayernrDynaCfg->au8BnrCRatio[BAYER_BGGR]);
    u8MaxCRaio   = MAX2(u8MaxCRaio01, u8MaxCRaio23);

    if (u8MaxCRaio <= 4) {
        pstBayernrDynaCfg->bMedcEnable = HI_FALSE;
    } else {
        pstBayernrDynaCfg->bMedcEnable = HI_TRUE;
    }

    return HI_SUCCESS;
}

static HI_S32 NRExtCfg( VI_PIPE ViPipe,  ISP_BAYERNR_DYNA_CFG_S *pstBayernrDynaCfg, ISP_BAYERNR_S *pstBayernr, HI_U8 u8IsoIndexUpper, HI_U8 u8IsoIndexLower, HI_U32 u32ISO2, HI_U32 u32ISO1, HI_U32 u32Iso)
{
    HI_U32 i = 0;
    HI_U32 u32CoringLow = 1;

    if (pstBayernr->enOpType == OP_TYPE_AUTO) {
        pstBayernr->u16CoringLow      = (HI_U16)LinearInter(u32Iso, u32ISO1, pstBayernr->stAuto.au16CoringWgt[u8IsoIndexLower], \
                                                                    u32ISO2, pstBayernr->stAuto.au16CoringWgt[u8IsoIndexUpper]);
        u32CoringLow                  = 256 * (HI_U32)pstBayernr->u16CoringLow;
        pstBayernrDynaCfg->u8JnlmGain = (HI_U8)LinearInter(u32Iso, u32ISO1, pstBayernr->stAuto.au8FineStr[u8IsoIndexLower], \
                                                                   u32ISO2, pstBayernr->stAuto.au8FineStr[u8IsoIndexUpper]);
        pstBayernr->u8FineStr         = pstBayernrDynaCfg->u8JnlmGain;

        pstBayernrDynaCfg->au8AmedLevel[BAYER_RGGB] = (HI_U8)LinearInter(u32Iso,  u32ISO1, pstBayernr->stAuto.au8ChromaStr[BAYER_RGGB][u8IsoIndexLower], \
                                                                                  u32ISO2, pstBayernr->stAuto.au8ChromaStr[BAYER_RGGB][u8IsoIndexUpper]);
        pstBayernrDynaCfg->au8AmedLevel[BAYER_GRBG] = (HI_U8)LinearInter(u32Iso,  u32ISO1, pstBayernr->stAuto.au8ChromaStr[BAYER_GRBG][u8IsoIndexLower], \
                                                                                  u32ISO2, pstBayernr->stAuto.au8ChromaStr[BAYER_GRBG][u8IsoIndexUpper]);
        pstBayernrDynaCfg->au8AmedLevel[BAYER_GBRG] = (HI_U8)LinearInter(u32Iso,  u32ISO1, pstBayernr->stAuto.au8ChromaStr[BAYER_GBRG][u8IsoIndexLower], \
                                                                                  u32ISO2, pstBayernr->stAuto.au8ChromaStr[BAYER_GBRG][u8IsoIndexUpper]);
        pstBayernrDynaCfg->au8AmedLevel[BAYER_BGGR] = (HI_U8)LinearInter(u32Iso,  u32ISO1, pstBayernr->stAuto.au8ChromaStr[BAYER_BGGR][u8IsoIndexLower], \
                                                                                  u32ISO2, pstBayernr->stAuto.au8ChromaStr[BAYER_BGGR][u8IsoIndexUpper]);
        pstBayernr->au16LmtStrength[BAYER_RGGB]     = (HI_U16)LinearInter(u32Iso, u32ISO1, pstBayernr->stAuto.au16CoarseStr[BAYER_RGGB][u8IsoIndexLower], \
                                                                                  u32ISO2, pstBayernr->stAuto.au16CoarseStr[BAYER_RGGB][u8IsoIndexUpper]);
        pstBayernr->au16LmtStrength[BAYER_GRBG]     = (HI_U16)LinearInter(u32Iso, u32ISO1, pstBayernr->stAuto.au16CoarseStr[BAYER_GRBG][u8IsoIndexLower], \
                                                                                  u32ISO2, pstBayernr->stAuto.au16CoarseStr[BAYER_GRBG][u8IsoIndexUpper]);
        pstBayernr->au16LmtStrength[BAYER_GBRG]     = (HI_U16)LinearInter(u32Iso, u32ISO1, pstBayernr->stAuto.au16CoarseStr[BAYER_GBRG][u8IsoIndexLower], \
                                                                                  u32ISO2, pstBayernr->stAuto.au16CoarseStr[BAYER_GBRG][u8IsoIndexUpper]);
        pstBayernr->au16LmtStrength[BAYER_BGGR]     = (HI_U16)LinearInter(u32Iso, u32ISO1, pstBayernr->stAuto.au16CoarseStr[BAYER_BGGR][u8IsoIndexLower], \
                                                                                  u32ISO2, pstBayernr->stAuto.au16CoarseStr[BAYER_BGGR][u8IsoIndexUpper]);
    } else if (pstBayernr->enOpType == OP_TYPE_MANUAL) {
        pstBayernr->u16CoringLow  = pstBayernr->stManual.u16CoringWgt;
        u32CoringLow  = 256 * (HI_U32)pstBayernr->u16CoringLow;
        pstBayernrDynaCfg->u8JnlmGain                = pstBayernr->stManual.u8FineStr;
        pstBayernrDynaCfg->au8AmedLevel[BAYER_RGGB]  = pstBayernr->stManual.au8ChromaStr[BAYER_RGGB];
        pstBayernrDynaCfg->au8AmedLevel[BAYER_GRBG]  = pstBayernr->stManual.au8ChromaStr[BAYER_GRBG];
        pstBayernrDynaCfg->au8AmedLevel[BAYER_GBRG]  = pstBayernr->stManual.au8ChromaStr[BAYER_GBRG];
        pstBayernrDynaCfg->au8AmedLevel[BAYER_BGGR]  = pstBayernr->stManual.au8ChromaStr[BAYER_BGGR];

        pstBayernr->au16LmtStrength[BAYER_RGGB] = pstBayernr->stManual.au16CoarseStr[BAYER_RGGB];
        pstBayernr->au16LmtStrength[BAYER_GRBG] = pstBayernr->stManual.au16CoarseStr[BAYER_GRBG];
        pstBayernr->au16LmtStrength[BAYER_GBRG] = pstBayernr->stManual.au16CoarseStr[BAYER_GBRG];
        pstBayernr->au16LmtStrength[BAYER_BGGR] = pstBayernr->stManual.au16CoarseStr[BAYER_BGGR];
    }

    hi_ext_system_bayernr_actual_coring_weight_write(ViPipe, pstBayernr->u16CoringLow);
    hi_ext_system_bayernr_actual_fine_strength_write(ViPipe, pstBayernrDynaCfg->u8JnlmGain);
    hi_ext_system_bayernr_actual_nr_lsc_ratio_write(ViPipe, pstBayernr->u8NrLscRatio);

    for (i = 0; i < 4; i++) {
        hi_ext_system_bayernr_actual_coarse_strength_write(ViPipe, i, pstBayernr->au16LmtStrength[i]);
        hi_ext_system_bayernr_actual_chroma_strength_write(ViPipe, i, 0);
        hi_ext_system_bayernr_actual_wdr_frame_strength_write(ViPipe, i, pstBayernr->stWDR.au8WDRFrameStr[i]);
    }

    for (i = 0; i < HI_ISP_BAYERNR_LUT_LENGTH; i++) {
        pstBayernrDynaCfg->au16JnlmCoringLowLut[i] = (HI_U16)(pstBayernr->au16LutCoringRatio[i] * u32CoringLow / HI_ISP_BAYERNR_CORINGLOW_STRENGTH_DIVISOR);
        pstBayernrDynaCfg->au16JnlmCoringLowLut[i] = MIN2(16383, pstBayernrDynaCfg->au16JnlmCoringLowLut[i]);
        pstBayernrDynaCfg->au8GcoringLut[i]    = pstBayernr->au8LutGCoringGain[i];
    }

    return HI_SUCCESS;
}

HI_S32 NRMultiGainCfg( VI_PIPE ViPipe,  ISP_BAYERNR_DYNA_CFG_S *pstBayernrDynaCfg, ISP_BAYERNR_S *pstBayernr)
{
    HI_U32 i = 0;
    HI_U8  u8SadFac = 1;
    HI_U16 u16JnlmScale = 49;
    HI_U16 u16JnlmShotScale;
    HI_U16 u16ShotCoef = 2;
    HI_U32 u32CenterLmtRatio = 40;
    HI_U32 u32MultGainL = 0, u32MultGainS = 0;

    HI_U8  u8nLimitGainBlt = 200;


    u16JnlmShotScale   = 128 + CLIP3((u16JnlmScale * u16ShotCoef), 0, 255);
    u16JnlmScale       = u16JnlmScale + 128;

    if (pstBayernr->bWdrModeEn == HI_TRUE) {
        for (i = 0; i < ISP_BAYER_CHN_NUM; i++) {
            u32MultGainL = ((HI_U32)pstBayernr->u16LmtNpThresh * pstBayernr->au16LmtStrength[i] * u8SadFac) >> 7;
            u32MultGainS = ((HI_U32)pstBayernr->u16LmtNpThreshS * pstBayernr->au16LmtStrength[i] * u8SadFac) >> 7;

            u32MultGainL = (u32MultGainL * u16JnlmScale )  >> 7;
            u32MultGainS = (u32MultGainS * u16JnlmScale )  >> 7;

            u32MultGainL = (u32MultGainL * u8nLimitGainBlt )  >> 6;
            u32MultGainS = (u32MultGainS * u8nLimitGainBlt )  >> 6;

            pstBayernrDynaCfg->au32JnlmLimitMultGain[i]  = MIN2(u32MultGainL >> 4, 0x01fffff);   //u21
            pstBayernrDynaCfg->au32JnlmLimitMultGainS[i] = MIN2(u32MultGainS >> 4, 0x01fffff);   //u21
        }
    } else {
        for (i = 0; i < ISP_BAYER_CHN_NUM; i++) {
            pstBayernrDynaCfg->au32JnlmLimitMultGain[i]  = (pstBayernr->u16LmtNpThresh * pstBayernr->au16LmtStrength[i] * u8SadFac) >> 7;
            pstBayernrDynaCfg->au32JnlmLimitMultGain[i]  = (pstBayernrDynaCfg->au32JnlmLimitMultGain[i] * u8nLimitGainBlt) >> 6;
            pstBayernrDynaCfg->au32JnlmLimitMultGain[i]  = (pstBayernrDynaCfg->au32JnlmLimitMultGain[i] * u16JnlmScale) >> 7;
            pstBayernrDynaCfg->au32JnlmLimitMultGain[i]  = MIN2(pstBayernrDynaCfg->au32JnlmLimitMultGain[i], 0x01ffffff);
            pstBayernrDynaCfg->au32JnlmLimitMultGain[i]  = pstBayernrDynaCfg->au32JnlmLimitMultGain[i] >> 4;  //u21.0
            pstBayernrDynaCfg->au32JnlmLimitMultGainS[i] = pstBayernrDynaCfg->au32JnlmLimitMultGain[i];
        }
    }

    pstBayernrDynaCfg->u32EdgeDetThr = ( pstBayernr->u32EdgeDetThr * u8SadFac) >> 5;

    if (pstBayernr->bCenterWgtEn == HI_TRUE) {
        u32CenterLmtRatio = u32CenterLmtRatio * (HI_U32)pstBayernr->u16CenterLmt;
        pstBayernrDynaCfg->u16ShotRatio = u32CenterLmtRatio >> 4;
    } else {
        pstBayernrDynaCfg->u16ShotRatio = u16JnlmShotScale * 256 / DIV_0_TO_1(u16JnlmScale);    // u2.8
    }
    pstBayernrDynaCfg->u16ShotRatio = MIN2(pstBayernrDynaCfg->u16ShotRatio, 1023);

    return HI_SUCCESS;
}

HI_S32 NRLimitLut(VI_PIPE ViPipe, ISP_BAYERNR_DYNA_CFG_S *pstBayernrDynaCfg, ISP_BAYERNR_S *pstBayernr, HI_U32 u32Iso, HI_U16 u16BlackLevel)
{
    HI_U16 str;
    HI_U32 u32LmtNpThresh;
    HI_U32 i = 0, n = 0;
    HI_U16 u16BitMask = ((1 << (HI_ISP_BAYERNR_BITDEP - 1)) - 1);
    HI_U32 lutN[2] = {16, 45};
    HI_U32 k = 0, b = 0;
    HI_U32 sigma = 0, sigma_max = 0;
    HI_U16 DarkStrength = 230;   //1.8f*128
    HI_U16 lutStr[2] = {96, 128};  //{0.75f, 1.0f}*128
    HI_FLOAT fCalibrationCoef = 0.0f;

    ISP_CMOS_DEFAULT_S *pstSnsDft = HI_NULL;
    ISP_SensorGetDefault(ViPipe, &pstSnsDft);

    fCalibrationCoef = Bayernr_getKfromNoiseLut(pstSnsDft->stNoiseCalibration.afCalibrationCoef, pstSnsDft->stNoiseCalibration.u16CalibrationLutNum, u32Iso);
    k     = (HI_U32)(fCalibrationCoef * ISP_BITFIX(14));
    fCalibrationCoef = Bayernr_getBfromNoiseLut(pstSnsDft->stNoiseCalibration.afCalibrationCoef, pstSnsDft->stNoiseCalibration.u16CalibrationLutNum, u32Iso);
    b     = (HI_U32)(fCalibrationCoef * ISP_BITFIX(14));
    sigma_max = (HI_U32)(k * (HI_U32)MAX2(((HI_S32)(255 - (u16BlackLevel >> 4))), 0) + b);
    sigma_max = (HI_U32)Sqrt32(sigma_max);

    u32LmtNpThresh = (HI_U32)(sigma_max * (1 << (HI_ISP_BAYERNR_BITDEP - 8 - 7))); //sad win size, move to hw
    pstBayernr->u16LmtNpThresh = (HI_U16)((u32LmtNpThresh > u16BitMask) ? u16BitMask : u32LmtNpThresh);

    lutStr[0] = DarkStrength;

    for (i = 0; i < HI_ISP_BAYERNR_LMTLUTNUM; i++) {
        sigma = (HI_U32)(k * (HI_U32)MAX2((((HI_S32)(i * 255 - 128 * (u16BlackLevel >> 4))) / (HI_S32)128), 0) + b);
        sigma = (HI_U32)Sqrt32(sigma);
        str = NRGetValueFromLut_fix(2 * i, lutN, lutStr, 2);
        sigma = sigma * str;

        pstBayernrDynaCfg->au8JnlmLimitLut[i] = (HI_U8)((sigma + sigma_max / 2) / DIV_0_TO_1(sigma_max));
    }

    //copy the first non-zero value to its left side
    for (i = 0; i < HI_ISP_BAYERNR_LMTLUTNUM; i++) {
        if (pstBayernrDynaCfg->au8JnlmLimitLut[i] > 0) {
            n = i;
            break;
        }
    }

    for (i = 0; i < n; i++) {
        pstBayernrDynaCfg->au8JnlmLimitLut[i] = pstBayernrDynaCfg->au8JnlmLimitLut[n];
    }

#ifndef VREG_OLD
    for (i = 0; i < HI_ISP_BAYERNR_LMTLUTNUM; i++) {
        pstBayernrDynaCfg->au8JnlmLimitSLut[i] = pstBayernrDynaCfg->au8JnlmLimitLut[i];
    }
#else
    memcpy(pstBayernrDynaCfg->au8JnlmLimitSLut, pstBayernrDynaCfg->au8JnlmLimitLut, HI_ISP_BAYERNR_LMTLUTNUM);
#endif
    return HI_SUCCESS;
}

HI_S32 hiisp_bayernr_fw(HI_U32 u32Iso, HI_U32 u32SensorIso, VI_PIPE ViPipe, ISP_BAYERNR_DYNA_CFG_S *pstBayernrDynaCfg,
                        ISP_BAYERNR_USR_CFG_S *pstBayernrUsrCfg, HI_U16 u16BlackLevel)
{
    HI_U8  u8IsoIndexUpper, u8IsoIndexLower;
    HI_U32 u32ISO1 = 0, u32ISO2 = 0;
    static HI_U32 RgainIIR = 65536, BgainIIR = 65536;

    ISP_BAYERNR_S      *pstBayernr = HI_NULL;
    ISP_CMOS_DEFAULT_S *pstSnsDft  = HI_NULL;
    ISP_CTX_S          *pstIspCtx  = HI_NULL;

    ISP_SensorGetDefault(ViPipe, &pstSnsDft);
    BAYERNR_GET_CTX(ViPipe, pstBayernr);
    ISP_GET_CTX(ViPipe, pstIspCtx);

    pstBayernrUsrCfg->bBnrMonoSensorEn     = pstBayernr->bBnrMonoSensorEn;     //MonoSensor, waiting to get
    pstBayernrUsrCfg->bBnrLscEn            = (pstBayernr->bNrLscEnable == 0) ? 0 : 3;
    pstBayernrUsrCfg->u8BnrLscMaxGain      = pstBayernr->u8BnrLscMaxGain + 64;
    pstBayernrUsrCfg->u16BnrLscCmpStrength = pstBayernr->u16BnrLscCmpStrength;

    u8IsoIndexUpper = GetIsoIndex(u32Iso);
    u8IsoIndexLower = MAX2((HI_S8)u8IsoIndexUpper - 1, 0);
    u32ISO1     = g_au32IsoLut[u8IsoIndexLower];
    u32ISO2     = g_au32IsoLut[u8IsoIndexUpper];

    NRLimitLut(ViPipe, pstBayernrDynaCfg, pstBayernr, u32SensorIso, u16BlackLevel);
    NRCfg(pstBayernrDynaCfg, pstBayernr, u8IsoIndexUpper, u8IsoIndexLower, u32ISO2, u32ISO1, u32Iso);
    NRExtCfg(ViPipe,  pstBayernrDynaCfg, pstBayernr, u8IsoIndexUpper, u8IsoIndexLower, u32ISO2, u32ISO1, u32Iso);
    NRMultiGainCfg(ViPipe, pstBayernrDynaCfg, pstBayernr);

    if (pstBayernrUsrCfg->bBnrMonoSensorEn == HI_TRUE) {
        pstBayernrDynaCfg->au8BnrCRatio[BAYER_RGGB] = HI_FALSE;
        pstBayernrDynaCfg->au8BnrCRatio[BAYER_GRBG] = HI_FALSE;
        pstBayernrDynaCfg->au8BnrCRatio[BAYER_GBRG] = HI_FALSE;
        pstBayernrDynaCfg->au8BnrCRatio[BAYER_BGGR] = HI_FALSE;
        pstBayernrDynaCfg->au8AmedLevel[BAYER_RGGB] = HI_FALSE;
        pstBayernrDynaCfg->au8AmedLevel[BAYER_GRBG] = HI_FALSE;
        pstBayernrDynaCfg->au8AmedLevel[BAYER_GBRG] = HI_FALSE;
        pstBayernrDynaCfg->au8AmedLevel[BAYER_BGGR] = HI_FALSE;
        pstBayernrDynaCfg->bMedcEnable              = HI_FALSE;
    }

    //pstBayernrDynaCfg->bBnrLutUpdateEn = HI_TRUE;
    RgainIIR = (7 * RgainIIR + pstIspCtx->stLinkage.au32WhiteBalanceGain[0]) >> 3;
    BgainIIR = (7 * BgainIIR + pstIspCtx->stLinkage.au32WhiteBalanceGain[3]) >> 3;

    pstBayernrDynaCfg->u16RLmtRgain    = (RgainIIR + 128) >> 8;
    pstBayernrDynaCfg->u16RLmtBgain    = (BgainIIR + 128) >> 8;
    pstBayernrDynaCfg->u16RLmtRgain = (pstBayernrDynaCfg->u16RLmtRgain > 1023) ? 1023 : pstBayernrDynaCfg->u16RLmtRgain;
    pstBayernrDynaCfg->u16RLmtBgain = (pstBayernrDynaCfg->u16RLmtBgain > 1023) ? 1023 : pstBayernrDynaCfg->u16RLmtBgain;

    pstBayernrDynaCfg->bResh = HI_TRUE;
    pstBayernrUsrCfg->bResh  = HI_TRUE;

    return  HI_SUCCESS;
}

// WDR FW: ADJ_C(2) + ADJ_D(4) = 6
#define  ADJ_C  2
#define  ADJ_D  4

HI_U16 BCOM(HI_U64 x)
{
    HI_U64 out = (x << 22) / DIV_0_TO_1((x << 6) + ((HI_U32)((1 << 18) - x) << ADJ_C));
    return (HI_U16)out;
}

// 16bit -> 20bit
HI_U32 BDEC(HI_U64 y)
{
    HI_U64 out = (y << 18) / DIV_0_TO_1(y + ((HI_U32)((1 << 16) - y) << ADJ_D));
    return (HI_U32)out;
}

HI_S32 NRLimitLut_WDR2to1(ISP_BAYERNR_DYNA_CFG_S *pstBayernrDynaCfg, ISP_BAYERNR_S *pstBayernr, HI_U32 k, HI_U32 b, HI_U16 u16BlackLevel)
{
    HI_U32 i;
    HI_U16 u16BitMask = ((1 << (HI_ISP_BAYERNR_BITDEP - 1)) - 1);
    HI_U64 u64Pvb = 0;
    HI_U32 u32Pvb = 0;
    HI_U16 au16WDRFrameThr[WDR_MAX_FRAME_NUM + 2];
    HI_U16 u16WDRFrameThrDiff;
    HI_U32 u32NrmGain, u32BcomGain;
    HI_U32 u32PvbLong, u32PvbShort;
    HI_U16 u16SigmaL, u16SigmaS, u16SigmaLMax = 0, u16SigmaSMax = 0;
    HI_U32 u32SigmaL, u32SigmaS;
    HI_U16 au16SigmaL[HI_ISP_BAYERNR_LMTLUTNUM], au16SigmaS[HI_ISP_BAYERNR_LMTLUTNUM];
    HI_U8  u8StrengthS;

    pstBayernr->au32ExpoValues[1] = (pstBayernr->au32ExpoValues[1] == 0) ? 64 : pstBayernr->au32ExpoValues[1];

    au16WDRFrameThr[0] = ISP_BITFIX(4) * MAX2(0, (pstBayernr->au16WDRFrameThr[0] - u16BlackLevel));    //low
    au16WDRFrameThr[1] = ISP_BITFIX(4) * MAX2(0, (pstBayernr->au16WDRFrameThr[1] - u16BlackLevel));    //high

    /*nrmgain:4bit fraction part*/

    u32NrmGain = (HI_U64)(ISP_BITFIX(18) - pstBayernr->u16WDRBlcThr) * ISP_BITFIX(10) / DIV_0_TO_1((ISP_BITFIX(12) - u16BlackLevel) * pstBayernr->au32ExpoValues[1]);

    for (i = 0; i < HI_ISP_BAYERNR_LMTLUTNUM; i++) {
        u64Pvb = i * 512;
        u32Pvb = (HI_U32)BDEC(u64Pvb);

        if (i == 0) {
            u32BcomGain = 128;
        } else {
            u32BcomGain = u64Pvb *  ISP_BITFIX(7) / DIV_0_TO_1(u32Pvb);
        }

        u32Pvb = MAX2(((HI_S32)u32Pvb - pstBayernr->u16WDRBlcThr), 0);
        u32PvbLong  = u32Pvb * ISP_BITFIX(8) / DIV_0_TO_1(u32NrmGain);
        u32PvbShort = (HI_U32)((HI_U64)u32Pvb * ISP_BITFIX(14) / DIV_0_TO_1(u32NrmGain * pstBayernr->au32ExpoValues[1]));

        u32SigmaL = (HI_U32)Sqrt32(u32PvbLong * k / ISP_BITFIX(8) + b) * 16;
        u32SigmaS = (HI_U32)Sqrt32(u32PvbShort * k / ISP_BITFIX(8) + b) * (pstBayernr->au32ExpoValues[1] >> 2);

        u16SigmaL = (HI_U16)MIN2(0xffff, ((HI_U64)u32SigmaL * u32NrmGain * u32BcomGain * u32BcomGain / ISP_BITFIX(25)));
        u16SigmaS = (HI_U16)MIN2(0xffff, ((HI_U64)u32SigmaS * u32NrmGain * u32BcomGain * u32BcomGain / ISP_BITFIX(25)));

        if (u32PvbLong < au16WDRFrameThr[0]) {
            u8StrengthS = pstBayernr->stWDR.au8WDRFrameStr[2];
        } else if (u32PvbLong > au16WDRFrameThr[1]) {
            u8StrengthS = pstBayernr->stWDR.au8WDRFrameStr[0];
        } else {
            u16WDRFrameThrDiff = au16WDRFrameThr[1] - au16WDRFrameThr[0];
            if (u16WDRFrameThrDiff < 25) {
                u8StrengthS = pstBayernr->stWDR.au8WDRFrameStr[0];
            } else {
                u8StrengthS = ((au16WDRFrameThr[1] - u32PvbLong) * pstBayernr->stWDR.au8WDRFrameStr[2] + (u32PvbLong - au16WDRFrameThr[0]) * pstBayernr->stWDR.au8WDRFrameStr[0]) / u16WDRFrameThrDiff;
            }
        }

        u16SigmaL = MIN2(65535, u16SigmaL * pstBayernr->stWDR.au8WDRFrameStr[1] / ISP_BITFIX(2));
        u16SigmaS = MIN2(65535, u16SigmaS * u8StrengthS / ISP_BITFIX(2));
        u16SigmaLMax = (u16SigmaLMax < u16SigmaL) ? u16SigmaL : u16SigmaLMax;
        u16SigmaSMax = (u16SigmaSMax < u16SigmaS) ? u16SigmaS : u16SigmaSMax;

        au16SigmaL[i] = u16SigmaL;
        au16SigmaS[i] = u16SigmaS;
    }

    pstBayernr->u16LmtNpThresh = ((u16SigmaLMax / 4) > u16BitMask) ? u16BitMask : (u16SigmaLMax / 4);
    pstBayernr->u16LmtNpThreshS = ((u16SigmaSMax / 4) > u16BitMask) ? u16BitMask : (u16SigmaSMax / 4);

    for (i = 0; i < HI_ISP_BAYERNR_LMTLUTNUM; i++) {
        pstBayernrDynaCfg->au8JnlmLimitLut[i]   = (HI_U8)MIN2(0xff, ((HI_U64)au16SigmaL[i] * 128 + u16SigmaLMax / 2) / DIV_0_TO_1(u16SigmaLMax));
        pstBayernrDynaCfg->au8JnlmLimitSLut[i] = (HI_U8)MIN2(0xff, ((HI_U64)au16SigmaS[i] * 128 + u16SigmaSMax / 2) / DIV_0_TO_1((HI_U32)u16SigmaSMax));
    }

    pstBayernrDynaCfg->bCenterWgtEn = HI_FALSE;

    return HI_SUCCESS;
}

HI_S32 NRLimitLut_Fusion(ISP_BAYERNR_DYNA_CFG_S *pstBayernrDynaCfg, ISP_BAYERNR_S *pstBayernr, HI_U32 k, HI_U32 b, HI_U16 u16BlackLevel)
{
    HI_U32 i;
    HI_U16 u16BitMask = ((1 << (HI_ISP_BAYERNR_BITDEP - 1)) - 1);
    HI_U64 u64Pvb = 0;
    HI_U32 u32Pvb = 0;
    HI_U32 u32NrmGain, u32BcomGain;
    HI_U32 u32PvbLong, u32PvbShort;
    HI_U16 u16SigmaM, u16SigmaS, u16SigmaMMax = 0, u16SigmaSMax = 0;
    HI_U32 u32SigmaL, u32SigmaM, u32SigmaS;
    HI_U16 au16SigmaM[HI_ISP_BAYERNR_LMTLUTNUM], au16SigmaS[HI_ISP_BAYERNR_LMTLUTNUM];

    pstBayernr->au32ExpoValues[1] = (pstBayernr->au32ExpoValues[1] == 0) ? 64 : pstBayernr->au32ExpoValues[1];
    u32NrmGain = (HI_U64)(ISP_BITFIX(18) - pstBayernr->u16WDRBlcThr) * ISP_BITFIX(10) / DIV_0_TO_1((ISP_BITFIX(12) - (HI_U32)u16BlackLevel) * (pstBayernr->au32ExpoValues[1] + 64));

    for (i = 0; i < HI_ISP_BAYERNR_LMTLUTNUM; i++) {
        u64Pvb = i * 512;
        u32Pvb = (HI_U32)BDEC(u64Pvb);

        if (i == 0) {
            u32BcomGain = 128;
        } else {
            u32BcomGain = u64Pvb *  ISP_BITFIX(7) / u32Pvb;
        }

        u32Pvb = MAX2(((HI_S32)u32Pvb - pstBayernr->u16WDRBlcThr), 0);
        u32PvbLong  = (HI_U32)MIN2(0xffffffff, (HI_U64)u32Pvb * pstBayernr->au32ExpoValues[1] * ISP_BITFIX(8) / DIV_0_TO_1(u32NrmGain * (pstBayernr->au32ExpoValues[1] + 64)));
        u32PvbShort = (HI_U32)MIN2(0xffffffff, (HI_U64)u32Pvb * ISP_BITFIX(14) / DIV_0_TO_1(u32NrmGain * (pstBayernr->au32ExpoValues[1] + 64)));

        u32SigmaL = (HI_U32)Sqrt32(u32PvbLong * k / ISP_BITFIX(8) + b) * 16;
        u32SigmaS = (HI_U32)Sqrt32(u32PvbShort * k / ISP_BITFIX(8) + b) * 16;
        u32SigmaM = NRSqrt64((HI_U64)u32SigmaL * u32SigmaL + (HI_U64)u32SigmaS * u32SigmaS);     //long+short
        u32SigmaS = u32SigmaS * (pstBayernr->au32ExpoValues[1] + 64) / ISP_BITFIX(6);

        u16SigmaM = (HI_U16)MIN2(0xffff, ((HI_U64)u32SigmaM * u32NrmGain * u32BcomGain * u32BcomGain / ISP_BITFIX(25)));
        u16SigmaS = (HI_U16)MIN2(0xffff, ((HI_U64)u32SigmaS * u32NrmGain * u32BcomGain * u32BcomGain / ISP_BITFIX(25)));

        u16SigmaM = MIN2(65535, u16SigmaM * pstBayernr->stWDR.au8FusionFrameStr[1] / ISP_BITFIX(4));
        u16SigmaS = MIN2(65535, u16SigmaS * pstBayernr->stWDR.au8FusionFrameStr[0] / ISP_BITFIX(4));

        u16SigmaMMax = (u16SigmaMMax < u16SigmaM) ? u16SigmaM : u16SigmaMMax;
        u16SigmaSMax = (u16SigmaSMax < u16SigmaS) ? u16SigmaS : u16SigmaSMax;

        au16SigmaM[i] = u16SigmaM;
        au16SigmaS[i] = u16SigmaS;
    }

    pstBayernr->u16LmtNpThresh = (u16SigmaMMax  > (HI_U32)u16BitMask) ? u16BitMask : u16SigmaMMax;
    pstBayernr->u16LmtNpThreshS = (u16SigmaSMax  > (HI_U32)u16BitMask) ? u16BitMask : u16SigmaSMax;

    for (i = 0; i < HI_ISP_BAYERNR_LMTLUTNUM; i++) {
        pstBayernrDynaCfg->au8JnlmLimitLut[i]   = (HI_U8)MIN2(0xff, ((HI_U64)au16SigmaM[i] * 128 + u16SigmaMMax / 2) / DIV_0_TO_1(u16SigmaMMax));
        pstBayernrDynaCfg->au8JnlmLimitSLut[i] = (HI_U8)MIN2(0xff, ((HI_U64)au16SigmaS[i] * 128 + u16SigmaSMax / 2) / DIV_0_TO_1(u16SigmaSMax));
    }

    pstBayernrDynaCfg->bCenterWgtEn = HI_TRUE;
    pstBayernrDynaCfg->u8WdrMapFltMode = HI_FALSE;

    return HI_SUCCESS;
}

static HI_VOID isp_bayernr_get_sync_framethr(VI_PIPE ViPipe, ISP_CTX_S *pstIspCtx, ISP_BAYERNR_S *pstBayernr)
{
    HI_U8 u8SyncIndex;
    HI_S8 i;
    ISP_SNS_REGS_INFO_S *pstSnsRegsInfo = NULL;
    ISP_SensorGetSnsReg(ViPipe, &pstSnsRegsInfo);

    if (IS_OFFLINE_MODE(pstIspCtx->stBlockAttr.enIspRunningMode) ||
        IS_STRIPING_MODE(pstIspCtx->stBlockAttr.enIspRunningMode)) {    /* offline mode */
        if (IS_HALF_WDR_MODE(pstIspCtx->u8SnsWDRMode)) {
            u8SyncIndex = MIN2(pstSnsRegsInfo->u8Cfg2ValidDelayMax, CFG2VLD_DLY_LIMIT - 1);
        } else {
            u8SyncIndex = MIN2(pstSnsRegsInfo->u8Cfg2ValidDelayMax + 1, CFG2VLD_DLY_LIMIT - 1);
        }
    } else {    /* online mode */
        if (IS_HALF_WDR_MODE(pstIspCtx->u8SnsWDRMode)) {
            u8SyncIndex = CLIP3((HI_S8)pstSnsRegsInfo->u8Cfg2ValidDelayMax - 1, 0, CFG2VLD_DLY_LIMIT - 1);
        } else {
            u8SyncIndex = MIN2(pstSnsRegsInfo->u8Cfg2ValidDelayMax, CFG2VLD_DLY_LIMIT - 1);
        }
        //online,stt load lut in ,delay one frame to acitive
        if(u8SyncIndex >=1) {
            u8SyncIndex = u8SyncIndex - 1;
        }

    }

    for (i = CFG2VLD_DLY_LIMIT - 2; i >= 0; i--) {
        pstBayernr->au16WDRSyncFrameThr[i + 1][0] = pstBayernr->au16WDRSyncFrameThr[i][0];
        pstBayernr->au16WDRSyncFrameThr[i + 1][1] = pstBayernr->au16WDRSyncFrameThr[i][1];
    }

    pstBayernr->au16WDRSyncFrameThr[0][0] = hi_ext_system_wdr_longthr_read(ViPipe);
    pstBayernr->au16WDRSyncFrameThr[0][1] = hi_ext_system_wdr_shortthr_read(ViPipe);

    memcpy(pstBayernr->au16WDRFrameThr, pstBayernr->au16WDRSyncFrameThr[u8SyncIndex], (WDR_MAX_FRAME_NUM + 2) * sizeof(HI_U16));
}

HI_S32 hiisp_bayernr_fw_wdr(HI_U32 u32Iso, HI_U32 u32SensorIso, VI_PIPE ViPipe, ISP_BAYERNR_DYNA_CFG_S *pstBayernrDynaCfg, ISP_BAYERNR_USR_CFG_S *pstBayernrUsrCfg, HI_U16 u16BlackLevel)
{
    HI_U8  i, u8IsoIndexUpper, u8IsoIndexLower;
    HI_U32 u32ISO1 = 0, u32ISO2 = 0;
    HI_U32 k, b;
    HI_FLOAT fCalibrationCoef = 0.0f;
    static HI_U32 RgainIIR = 65536, BgainIIR = 65536;

    ISP_BAYERNR_S      *pstBayernr = HI_NULL;
    ISP_CMOS_DEFAULT_S *pstSnsDft  = HI_NULL;
    ISP_CTX_S          *pstIspCtx  = HI_NULL;

    ISP_SensorGetDefault(ViPipe, &pstSnsDft);
    BAYERNR_GET_CTX(ViPipe, pstBayernr);
    ISP_GET_CTX(ViPipe, pstIspCtx);

    isp_bayernr_get_sync_framethr(ViPipe, pstIspCtx, pstBayernr);

    pstBayernrDynaCfg->bWdrModeEn          = pstBayernr->bWdrModeEn;
    pstBayernrDynaCfg->bWdrFusionEn        = pstBayernr->bWdrFusionEn;
    pstBayernrUsrCfg->bBnrMonoSensorEn     = pstBayernr->bBnrMonoSensorEn;     //MonoSensor, waiting to get
    pstBayernrUsrCfg->bBnrLscEn            = (pstBayernr->bNrLscEnable == 0) ? 0 : 3;
    pstBayernrUsrCfg->u8BnrLscMaxGain      = pstBayernr->u8BnrLscMaxGain + 64;
    pstBayernrUsrCfg->u16BnrLscCmpStrength = pstBayernr->u16BnrLscCmpStrength;

    // Noise LUT
    fCalibrationCoef = Bayernr_getKfromNoiseLut(pstSnsDft->stNoiseCalibration.afCalibrationCoef, pstSnsDft->stNoiseCalibration.u16CalibrationLutNum, u32SensorIso);
    k     = (HI_U32)(fCalibrationCoef * ISP_BITFIX(14));
    fCalibrationCoef = Bayernr_getBfromNoiseLut(pstSnsDft->stNoiseCalibration.afCalibrationCoef, pstSnsDft->stNoiseCalibration.u16CalibrationLutNum, u32SensorIso);
    b     = (HI_U32)(fCalibrationCoef * ISP_BITFIX(14));

    u8IsoIndexUpper = GetIsoIndex(u32Iso);
    u8IsoIndexLower = MAX2((HI_S8)u8IsoIndexUpper - 1, 0);
    u32ISO1 = g_au32IsoLut[u8IsoIndexLower];
    u32ISO2 = g_au32IsoLut[u8IsoIndexUpper];

    NRCfg(pstBayernrDynaCfg, pstBayernr, u8IsoIndexUpper, u8IsoIndexLower, u32ISO2, u32ISO1, u32Iso);
    BayerNR_SetLongFrameMode(ViPipe, pstBayernrDynaCfg, pstBayernr, u8IsoIndexUpper, u8IsoIndexLower, u32ISO2, u32ISO1, u32Iso);
    NRExtCfg(ViPipe, pstBayernrDynaCfg, pstBayernr, u8IsoIndexUpper, u8IsoIndexLower, u32ISO2, u32ISO1, u32Iso);

    switch (pstBayernr->bWdrFusionEn) {
        case 0:  //WDR mode
            NRLimitLut_WDR2to1(pstBayernrDynaCfg, pstBayernr, k, b, u16BlackLevel);
            break;
        case 1:  //Fusion mode
            NRLimitLut_Fusion(pstBayernrDynaCfg, pstBayernr, k, b, u16BlackLevel);
            break;
        default:
            break;
    }

    NRMultiGainCfg(ViPipe, pstBayernrDynaCfg, pstBayernr);

    if (0 == HI_WDR_EINIT_BLCNR) {
        pstBayernrDynaCfg->au8JnlmLimitLut[0] = pstBayernrDynaCfg->au8JnlmLimitLut[3];
        pstBayernrDynaCfg->au8JnlmLimitLut[1] = pstBayernrDynaCfg->au8JnlmLimitLut[3];
        pstBayernrDynaCfg->au8JnlmLimitLut[2] = pstBayernrDynaCfg->au8JnlmLimitLut[3];
    }

    if (pstBayernr->bWdrFusionEn == HI_TRUE) { //Fusion
        pstBayernrDynaCfg->u8WdrMapGain = (HI_U8)(((HI_U16)pstBayernr->u8WdrMapGain * 512 + 25) / 50);
    } else {
        pstBayernrDynaCfg->u8WdrMapGain = (HI_U8)((pstBayernr->u8WdrMapGain * 128 + 7) / 14);
    }
    if (pstBayernrUsrCfg->bBnrMonoSensorEn == 1) {
        for (i = 0; i < ISP_BAYER_CHN_NUM; i++) {
            pstBayernrDynaCfg->bMedcEnable     = HI_FALSE;
            pstBayernrDynaCfg->au8BnrCRatio[i] = 0;
            pstBayernrDynaCfg->au8AmedLevel[i] = 0;
        }
    }

    //pstBayernrDynaCfg->bBnrLutUpdateEn = HI_TRUE;
    RgainIIR = (7 * RgainIIR + pstIspCtx->stLinkage.au32WhiteBalanceGain[0]) >> 3;
    BgainIIR = (7 * BgainIIR + pstIspCtx->stLinkage.au32WhiteBalanceGain[3]) >> 3;

    pstBayernrDynaCfg->u16RLmtRgain    = (RgainIIR + 128) >> 8;
    pstBayernrDynaCfg->u16RLmtBgain    = (BgainIIR + 128) >> 8;

    pstBayernrDynaCfg->u16RLmtRgain = (pstBayernrDynaCfg->u16RLmtRgain > 1023) ? 1023 : pstBayernrDynaCfg->u16RLmtRgain;
    pstBayernrDynaCfg->u16RLmtBgain = (pstBayernrDynaCfg->u16RLmtBgain > 1023) ? 1023 : pstBayernrDynaCfg->u16RLmtBgain;

    pstBayernrDynaCfg->bResh = HI_TRUE;
    pstBayernrUsrCfg->bResh  = HI_TRUE;

    return HI_SUCCESS;
}

static HI_BOOL __inline CheckBnrOpen(ISP_BAYERNR_S *pstBayernr)
{
    return (pstBayernr->bEnable == HI_TRUE);
}

HI_S32 ISP_BayernrRun(VI_PIPE ViPipe, const HI_VOID *pStatInfo,
                      HI_VOID *pRegCfg, HI_S32 s32Rsv)
{
    HI_U8 i;
    HI_U16 u16BlackLevel;
    HI_U16 u16WDRInBLC;
    ISP_REG_CFG_S *pstRegCfg  = (ISP_REG_CFG_S *)pRegCfg;
    ISP_CTX_S     *pstIspCtx  = HI_NULL;
    ISP_BAYERNR_S *pstBayernr = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);
    BAYERNR_GET_CTX(ViPipe, pstBayernr);
    ISP_CHECK_POINTER(pstBayernr);

    /* calculate every two interrupts */
    if (!pstBayernr->bInit) {
        return HI_SUCCESS;
    }

    pstBayernr->bEnable = hi_ext_system_bayernr_enable_read(ViPipe);
    for (i = 0; i < pstRegCfg->u8CfgNum; i++) {
        pstRegCfg->stAlgRegCfg[i].stBnrRegCfg.bBnrEnable               = pstBayernr->bEnable;
    }

    pstRegCfg->unKey.bit1BayernrCfg = 1;

    /*check hardware setting*/
    if (!CheckBnrOpen(pstBayernr)) {
        return HI_SUCCESS;
    }

    BayernrReadExtregs(ViPipe, &pstRegCfg->stAlgRegCfg[0].stBeBlcCfg);
    BayernrReadProMode(ViPipe);

    u16BlackLevel = pstRegCfg->stAlgRegCfg[0].stBeBlcCfg.stBnrBlc.stUsrRegCfg.au16Blc[0];   // //12bit
    u16WDRInBLC   = pstRegCfg->stAlgRegCfg[0].stBeBlcCfg.stWdrBlc[0].stUsrRegCfg.au16Blc[0];

    if (pstBayernr->u8WdrFramesMerge > 1) {
        hiisp_bayernr_fw_wdr(pstIspCtx->stLinkage.u32Iso, pstIspCtx->stLinkage.u32SensorIso, ViPipe, &pstRegCfg->stAlgRegCfg[0].stBnrRegCfg.stDynaRegCfg,
                             &pstRegCfg->stAlgRegCfg[0].stBnrRegCfg.stUsrRegCfg, u16WDRInBLC);
    } else {
        hiisp_bayernr_fw(pstIspCtx->stLinkage.u32Iso, pstIspCtx->stLinkage.u32SensorIso, ViPipe, &pstRegCfg->stAlgRegCfg[0].stBnrRegCfg.stDynaRegCfg,
                         &pstRegCfg->stAlgRegCfg[0].stBnrRegCfg.stUsrRegCfg, u16BlackLevel);
    }

    for (i = 1; i < pstIspCtx->stBlockAttr.u8BlockNum; i++) {
        memcpy(&pstRegCfg->stAlgRegCfg[i].stBnrRegCfg.stDynaRegCfg, &pstRegCfg->stAlgRegCfg[0].stBnrRegCfg.stDynaRegCfg,
               sizeof(ISP_BAYERNR_DYNA_CFG_S));

        memcpy(&pstRegCfg->stAlgRegCfg[i].stBnrRegCfg.stUsrRegCfg, &pstRegCfg->stAlgRegCfg[0].stBnrRegCfg.stUsrRegCfg,
               sizeof(ISP_BAYERNR_USR_CFG_S));
    }

    return HI_SUCCESS;
}

HI_S32 ISP_BayernrCtrl(VI_PIPE ViPipe, HI_U32 u32Cmd, HI_VOID *pValue)
{
    ISP_BAYERNR_S *pstBayernr = HI_NULL;
    ISP_REGCFG_S  *pRegCfg    = HI_NULL;

    switch (u32Cmd) {
        case ISP_WDR_MODE_SET :
            ISP_REGCFG_GET_CTX(ViPipe, pRegCfg);
            BAYERNR_GET_CTX(ViPipe, pstBayernr);
            ISP_CHECK_POINTER(pRegCfg);
            ISP_CHECK_POINTER(pstBayernr);

            pstBayernr->bInit = HI_FALSE;
            ISP_BayernrInit(ViPipe, (HI_VOID *)&pRegCfg->stRegCfg);
            break;
        case ISP_PROC_WRITE:
            BayernrProcWrite(ViPipe, (ISP_CTRL_PROC_WRITE_S *)pValue);
            break;
        default :
            break;
    }
    return HI_SUCCESS;
}

HI_S32 ISP_BayernrExit(VI_PIPE ViPipe)
{
    HI_U8 i;
    ISP_REGCFG_S *pRegCfg   = HI_NULL;

    ISP_REGCFG_GET_CTX(ViPipe, pRegCfg);

    for (i = 0; i < pRegCfg->stRegCfg.u8CfgNum; i++) {
        pRegCfg->stRegCfg.stAlgRegCfg[i].stBnrRegCfg.bBnrEnable = HI_FALSE;
    }

    pRegCfg->stRegCfg.unKey.bit1BayernrCfg = 1;

    BayerNrCtxExit(ViPipe);

    return HI_SUCCESS;
}

HI_S32 ISP_AlgRegisterBayernr(VI_PIPE ViPipe)
{
    ISP_CTX_S *pstIspCtx = HI_NULL;
    ISP_ALG_NODE_S *pstAlgs = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);
    ISP_ALG_CHECK(pstIspCtx->unAlgKey.bit1Bayernr);
    pstAlgs = ISP_SearchAlg(pstIspCtx->astAlgs);
    ISP_CHECK_POINTER(pstAlgs);

    pstAlgs->enAlgType = ISP_ALG_BAYERNR;
    pstAlgs->stAlgFunc.pfn_alg_init = ISP_BayernrInit;
    pstAlgs->stAlgFunc.pfn_alg_run  = ISP_BayernrRun;
    pstAlgs->stAlgFunc.pfn_alg_ctrl = ISP_BayernrCtrl;
    pstAlgs->stAlgFunc.pfn_alg_exit = ISP_BayernrExit;
    pstAlgs->bUsed = HI_TRUE;

    return HI_SUCCESS;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
