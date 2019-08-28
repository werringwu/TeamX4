/******************************************************************************

  Copyright (C), 2016, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : isp_green_equalization.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2013/04/24
  Description   :
  History       :
  1.Date        : 2016/09/20
    Modification: Created file

******************************************************************************/
#include <math.h>
#include "isp_alg.h"
#include "isp_config.h"
#include "isp_ext_config.h"
#include "isp_sensor.h"
#include "isp_math_utils.h"
#include "isp_proc.h"
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define HI_MINIISP_BITDEPTH        (16)
#define ISP_GE_SLOPE_DEFAULT       (13)
#define ISP_GE_SENSI_SLOPE_DEFAULT (13)
#define ISP_GE_SENSI_THR_DEFAULT   (4800)


static const  HI_U16 g_au16Threshold[ISP_AUTO_ISO_STRENGTH_NUM] = { 4800, 4800, 4800, 4800, 4960, 4960, 4960, 4960, 5120, 5120, 5120, 5120, 5280, 5280, 5280, 5280};
static const  HI_U16 g_au16Strength[ISP_AUTO_ISO_STRENGTH_NUM]  = { 128,  128,  128,  128,  129,  129,  129,  129,  130,  130,  130,  130,  131,  131,  131,  131};

static const  HI_U16 g_au16NpOffset[ISP_AUTO_ISO_STRENGTH_NUM]  = {16384, 16384, 16384, 16384, 16384, 32768, 32768, 32768, 32768, 32768, 32768, 32768, 32768, 32768, 32768, 32768};

typedef struct hiISP_GE_S {
    HI_BOOL bEnable;
    HI_BOOL bCoefUpdateEn;

    HI_U32  bitDepth;
    HI_U16 u16GeProcCtTh1;
    HI_U16 u16GeProcCtTh3;
    HI_U16 u16GeProcStrength;

    ISP_CMOS_GE_S stCmosGe;
} ISP_GE_S;

ISP_GE_S *g_pastGeCtx[ISP_MAX_PIPE_NUM] = {HI_NULL};

#define GE_GET_CTX(dev, pstCtx)   (pstCtx = g_pastGeCtx[dev])
#define GE_SET_CTX(dev, pstCtx)   (g_pastGeCtx[dev] = pstCtx)
#define GE_RESET_CTX(dev)         (g_pastGeCtx[dev] = HI_NULL)

static HI_S32 GeCtxInit(VI_PIPE ViPipe)
{
    ISP_GE_S *pastGeCtx = HI_NULL;

    GE_GET_CTX(ViPipe, pastGeCtx);

    if (pastGeCtx == HI_NULL) {
        pastGeCtx = (ISP_GE_S *)ISP_MALLOC(sizeof(ISP_GE_S));
        if (pastGeCtx == HI_NULL) {
            ISP_TRACE(HI_DBG_ERR, "Isp[%d] GeCtx malloc memory failed!\n", ViPipe);
            return HI_ERR_ISP_NOMEM;
        }
    }

    memset(pastGeCtx, 0, sizeof(ISP_GE_S));

    GE_SET_CTX(ViPipe, pastGeCtx);

    return HI_SUCCESS;
}

static HI_VOID GeCtxExit(VI_PIPE ViPipe)
{
    ISP_GE_S *pastGeCtx = HI_NULL;

    GE_GET_CTX(ViPipe, pastGeCtx);
    ISP_FREE(pastGeCtx);
    GE_RESET_CTX(ViPipe);
}

static HI_S32 GeCheckCmosParam(VI_PIPE ViPipe, const ISP_CMOS_GE_S *pstCmosGe)
{
    HI_U8 i;

    ISP_CHECK_BOOL(pstCmosGe->bEnable);

    if (pstCmosGe->u8Slope > HI_ISP_CR_SLOPE_MAX) {
        ISP_TRACE(HI_DBG_ERR, "err u8Slope!\n");
        return HI_ERR_ISP_ILLEGAL_PARAM;
    }

    if (pstCmosGe->u8SensiSlope > HI_ISP_CR_SLOPE_MAX) {
        ISP_TRACE(HI_DBG_ERR, "err u8SensiSlope!\n");
        return HI_ERR_ISP_ILLEGAL_PARAM;
    }

    if (pstCmosGe->u16SensiThr > HI_ISP_CR_THR_MAX) {
        ISP_TRACE(HI_DBG_ERR, "err u16SensiThr!\n");
        return HI_ERR_ISP_ILLEGAL_PARAM;
    }

    for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++) {
        if (pstCmosGe->au16Strength[i] > 0x100) {
            ISP_TRACE(HI_DBG_ERR, "err au16Strength[%d]!\n", i);
            return HI_ERR_ISP_ILLEGAL_PARAM;
        }

        if (pstCmosGe->au16NpOffset[i] > HI_ISP_CR_NPOFFSET_MAX || pstCmosGe->au16NpOffset[i] < HI_ISP_CR_NPOFFSET_MIN) {
            ISP_TRACE(HI_DBG_ERR, "err au16NpOffset[%d]!\n", i);
            return HI_ERR_ISP_ILLEGAL_PARAM;
        }

        if (pstCmosGe->au16Threshold[i] > HI_ISP_CR_THR_MAX) {
            ISP_TRACE(HI_DBG_ERR, "err au16Threshold[%d]!\n", i);
            return HI_ERR_ISP_ILLEGAL_PARAM;
        }
    }

    return HI_SUCCESS;
}

static HI_S32 GeInitialize(VI_PIPE ViPipe)
{
    HI_S32     s32Ret;
    ISP_GE_S   *pstGe = HI_NULL;
    ISP_CMOS_DEFAULT_S  *pstSnsDft = HI_NULL;

    GE_GET_CTX(ViPipe, pstGe);
    ISP_SensorGetDefault(ViPipe, &pstSnsDft);


    if (pstSnsDft->unKey.bit1Ge) {
        ISP_CHECK_POINTER(pstSnsDft->pstGe);

        s32Ret = GeCheckCmosParam(ViPipe, pstSnsDft->pstGe);
        if (s32Ret != HI_SUCCESS) {
            return s32Ret;
        }

        memcpy(&pstGe->stCmosGe, pstSnsDft->pstGe, sizeof(ISP_CMOS_GE_S));
    } else {
        pstGe->stCmosGe.bEnable      = HI_TRUE;
        pstGe->stCmosGe.u8Slope      = ISP_GE_SLOPE_DEFAULT;
        pstGe->stCmosGe.u8SensiSlope = ISP_GE_SENSI_SLOPE_DEFAULT;
        pstGe->stCmosGe.u16SensiThr  = ISP_GE_SENSI_THR_DEFAULT;
        memcpy(pstGe->stCmosGe.au16Strength,  g_au16Strength,  ISP_AUTO_ISO_STRENGTH_NUM * sizeof(HI_U16));
        memcpy(pstGe->stCmosGe.au16Threshold, g_au16Threshold, ISP_AUTO_ISO_STRENGTH_NUM * sizeof(HI_U16));
        memcpy(pstGe->stCmosGe.au16NpOffset,  g_au16NpOffset,  ISP_AUTO_ISO_STRENGTH_NUM * sizeof(HI_U16));
    }

    pstGe->bEnable = pstGe->stCmosGe.bEnable;

    pstGe->bitDepth   = HI_MINIISP_BITDEPTH;

    return HI_SUCCESS;
}

static HI_VOID GeExtRegsInitialize(VI_PIPE ViPipe)
{
    HI_U32 i;
    ISP_GE_S *pstGe = HI_NULL;

    GE_GET_CTX(ViPipe, pstGe);


    /* initial ext register of Ge */
    hi_ext_system_ge_enable_write(ViPipe, pstGe->bEnable);
    hi_ext_system_ge_slope_write(ViPipe, pstGe->stCmosGe.u8Slope);
    hi_ext_system_ge_sensitivity_write(ViPipe, pstGe->stCmosGe.u8SensiSlope);
    hi_ext_system_ge_sensithreshold_write(ViPipe, pstGe->stCmosGe.u16SensiThr);
    hi_ext_system_ge_coef_update_en_write(ViPipe, HI_TRUE);

    for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++) {
        hi_ext_system_ge_strength_write(ViPipe, i, pstGe->stCmosGe.au16Strength[i]);
        hi_ext_system_ge_npoffset_write(ViPipe, i, pstGe->stCmosGe.au16NpOffset[i]);
        hi_ext_system_ge_threshold_write(ViPipe, i, pstGe->stCmosGe.au16Threshold[i]);
    }

    return;
}

static HI_VOID GeStaticRegsInitialize(VI_PIPE ViPipe, ISP_GE_STATIC_CFG_S *pstGeStaticRegCfg)
{
    ISP_GE_S   *pstGe = HI_NULL;

    GE_GET_CTX(ViPipe, pstGe);
    ISP_CHECK_POINTER_VOID(pstGe);

    pstGeStaticRegCfg->bGeGrGbEn           = HI_TRUE;
    pstGeStaticRegCfg->bGeGrEn             = HI_FALSE;
    pstGeStaticRegCfg->bGeGbEn             = HI_FALSE;

    pstGeStaticRegCfg->bStaticResh = HI_TRUE;
}

static HI_VOID GeUsrRegsInitialize(VI_PIPE ViPipe, HI_U8 i, ISP_GE_USR_CFG_S *pstGeUsrRegCfg, ISP_CTX_S *pstIspCtx)
{
    pstGeUsrRegCfg->au16GeCtTh2   = HI_ISP_GE_SENSITHR_DEFAULT;
    pstGeUsrRegCfg->au8GeCtSlope1 = HI_ISP_GE_SENSISLOPE_DEFAULT;
    pstGeUsrRegCfg->au8GeCtSlope2 = HI_ISP_GE_SLOPE_DEFAULT;


    pstGeUsrRegCfg->bResh = HI_TRUE;
}

static HI_VOID GeDynaRegsInitialize(ISP_GE_DYNA_CFG_S *pstGeDynaRegCfg)
{
    pstGeDynaRegCfg->au16GeCtTh1 = HI_ISP_GE_NPOFFSET_DEFAULT;
    pstGeDynaRegCfg->au16GeCtTh3 = HI_ISP_GE_THRESHOLD_DEFAULT;
    pstGeDynaRegCfg->u16GeStrength = HI_ISP_GE_STRENGTH_DEFAULT;
    pstGeDynaRegCfg->bResh         = HI_TRUE;
}

static HI_VOID GeRegsInitialize(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfg)
{
    HI_U8      i;
    ISP_CTX_S  *pstIspCtx    = HI_NULL;
    ISP_GE_S   *pstGe = HI_NULL;


    GE_GET_CTX(ViPipe, pstGe);
    ISP_GET_CTX(ViPipe, pstIspCtx);


    for (i = 0; i < pstRegCfg->u8CfgNum; i++) {
        GeStaticRegsInitialize(ViPipe, &pstRegCfg->stAlgRegCfg[i].stGeRegCfg.stStaticRegCfg);
        GeUsrRegsInitialize(ViPipe, i, &pstRegCfg->stAlgRegCfg[i].stGeRegCfg.stUsrRegCfg, pstIspCtx);
        GeDynaRegsInitialize(&pstRegCfg->stAlgRegCfg[i].stGeRegCfg.stDynaRegCfg);

        pstRegCfg->stAlgRegCfg[i].stGeRegCfg.abGeEn = pstGe->bEnable;
    }

    pstRegCfg->unKey.bit1GeCfg = 1;

    return;
}

static HI_S32 GeReadExtregs(VI_PIPE ViPipe)
{
    HI_U32 i;
    ISP_GE_S *pstGe = HI_NULL;

    GE_GET_CTX(ViPipe, pstGe);
    ISP_CHECK_POINTER(pstGe);

    /* read ext register of Ge */
    pstGe->bCoefUpdateEn         = hi_ext_system_ge_coef_update_en_read(ViPipe);
    hi_ext_system_ge_coef_update_en_write(ViPipe, HI_FALSE);

    if (pstGe->bCoefUpdateEn) {
        pstGe->stCmosGe.u8Slope      = hi_ext_system_ge_slope_read(ViPipe);
        pstGe->stCmosGe.u8SensiSlope = hi_ext_system_ge_sensitivity_read(ViPipe);
        pstGe->stCmosGe.u16SensiThr  = hi_ext_system_ge_sensithreshold_read(ViPipe);

        for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++) {
            pstGe->stCmosGe.au16Strength[i] = hi_ext_system_ge_strength_read(ViPipe, i);
            pstGe->stCmosGe.au16NpOffset[i] = hi_ext_system_ge_npoffset_read(ViPipe, i);
            pstGe->stCmosGe.au16Threshold[i] = hi_ext_system_ge_threshold_read(ViPipe, i);
        }
    }

    return HI_SUCCESS;
}

static HI_VOID Ge_Usr_Fw(ISP_GE_S *pstGe, ISP_GE_USR_CFG_S *pstGeUsrRegCfg)
{
    pstGeUsrRegCfg->au16GeCtTh2   = MIN2(pstGe->stCmosGe.u16SensiThr, (1 << pstGe->bitDepth));
    pstGeUsrRegCfg->au8GeCtSlope1 = MIN2(pstGe->stCmosGe.u8SensiSlope, pstGe->bitDepth);
    pstGeUsrRegCfg->au8GeCtSlope2 = MIN2(pstGe->stCmosGe.u8Slope, pstGe->bitDepth);

    pstGeUsrRegCfg->bResh    = HI_TRUE;
}

static HI_VOID Ge_Dyna_Fw(HI_U32 u32Iso, ISP_GE_S *pstGe, ISP_GE_DYNA_CFG_S *pstGeDynaRegCfg)
{

    HI_U8 u8IndexUpper = GetIsoIndex(u32Iso);
    HI_U8 u8IndexLower = MAX2((HI_S8)u8IndexUpper - 1, 0);


    pstGeDynaRegCfg->au16GeCtTh3 = CLIP3(LinearInter(u32Iso, g_au32IsoLut[u8IndexLower], pstGe->stCmosGe.au16Threshold[u8IndexLower], \
                                                     g_au32IsoLut[u8IndexUpper], pstGe->stCmosGe.au16Threshold[u8IndexUpper]), 0, (1 << pstGe->bitDepth));

    pstGeDynaRegCfg->au16GeCtTh1 = (HI_U16)LinearInter(u32Iso, g_au32IsoLut[u8IndexLower], pstGe->stCmosGe.au16NpOffset[u8IndexLower], \
                                                       g_au32IsoLut[u8IndexUpper], pstGe->stCmosGe.au16NpOffset[u8IndexUpper]);

    pstGeDynaRegCfg->u16GeStrength  = (HI_U16)LinearInter(u32Iso, g_au32IsoLut[u8IndexLower], pstGe->stCmosGe.au16Strength[u8IndexLower], \
                                                          g_au32IsoLut[u8IndexUpper], pstGe->stCmosGe.au16Strength[u8IndexUpper]);

    pstGe->u16GeProcCtTh1    = pstGeDynaRegCfg->au16GeCtTh1;
    pstGe->u16GeProcCtTh3    = pstGeDynaRegCfg->au16GeCtTh3;
    pstGe->u16GeProcStrength = pstGeDynaRegCfg->u16GeStrength;

    pstGeDynaRegCfg->bResh          = HI_TRUE;
}

static HI_BOOL __inline CheckGeOpen(ISP_GE_S *pstGe)
{
    return (pstGe->bEnable == HI_TRUE);
}

static HI_VOID Ge_Bypass(ISP_REG_CFG_S *pstReg, ISP_GE_S *pstGe)
{
    HI_U8 i;

    for (i = 0; i < pstReg->u8CfgNum; i++) {
        pstReg->stAlgRegCfg[i].stGeRegCfg.abGeEn = HI_FALSE;
    }

    pstReg->unKey.bit1GeCfg = 1;
}

HI_S32 ISP_GeInit(VI_PIPE ViPipe, HI_VOID *pRegCfg)
{
    HI_S32 s32Ret = HI_SUCCESS;
    ISP_REG_CFG_S *pstRegCfg = (ISP_REG_CFG_S *)pRegCfg;

    s32Ret = GeCtxInit(ViPipe);
    if (s32Ret != HI_SUCCESS) {
        return s32Ret;
    }

    s32Ret = GeInitialize(ViPipe);
    if (s32Ret != HI_SUCCESS) {
        return s32Ret;
    }

    GeRegsInitialize(ViPipe, pstRegCfg);
    GeExtRegsInitialize(ViPipe);

    return HI_SUCCESS;
}

HI_S32 ISP_GeRun(VI_PIPE ViPipe, const HI_VOID *pStatInfo,
                 HI_VOID *pRegCfg, HI_S32 s32Rsv)
{
    HI_U8 i;
    ISP_GE_S  *pstGe = HI_NULL;
    ISP_CTX_S *pstIspCtx  = HI_NULL;
    ISP_REG_CFG_S *pstReg = (ISP_REG_CFG_S *)pRegCfg;

    ISP_GET_CTX(ViPipe, pstIspCtx);

    GE_GET_CTX(ViPipe, pstGe);
    ISP_CHECK_POINTER(pstGe);

    /* calculate every two interrupts */

    if ((pstIspCtx->u32FrameCnt % 2 != 0) && (pstIspCtx->stLinkage.bSnapState != HI_TRUE)) {
        return HI_SUCCESS;
    }

    if (pstIspCtx->stLinkage.bDefectPixel) {
        Ge_Bypass(pstReg, pstGe);
        return HI_SUCCESS;
    }

    pstGe->bEnable = hi_ext_system_ge_enable_read(ViPipe);

    for (i = 0; i < pstReg->u8CfgNum; i++) {
        pstReg->stAlgRegCfg[i].stGeRegCfg.abGeEn = pstGe->bEnable;
    }

    pstReg->unKey.bit1GeCfg = 1;

    /* check hardware setting */
    if (!CheckGeOpen(pstGe)) {
        return HI_SUCCESS;
    }

    GeReadExtregs(ViPipe);

    if (pstGe->bCoefUpdateEn) {
        for (i = 0; i < pstReg->u8CfgNum; i++) {
            Ge_Usr_Fw(pstGe, &pstReg->stAlgRegCfg[i].stGeRegCfg.stUsrRegCfg);
        }
    }

    for (i = 0; i < pstReg->u8CfgNum; i++) {
        Ge_Dyna_Fw(pstIspCtx->stLinkage.u32Iso, pstGe, &pstReg->stAlgRegCfg[i].stGeRegCfg.stDynaRegCfg);
    }

    return HI_SUCCESS;
}

HI_S32 GeProcWrite(VI_PIPE ViPipe, ISP_CTRL_PROC_WRITE_S *pstProc)
{
    ISP_CTRL_PROC_WRITE_S stProcTmp;
    ISP_GE_S  *pstGe = HI_NULL;


    GE_GET_CTX(ViPipe, pstGe);
    ISP_CHECK_POINTER(pstGe);

    if ((pstProc->pcProcBuff == HI_NULL)
        || (pstProc->u32BuffLen == 0)) {
        return HI_FAILURE;
    }

    stProcTmp.pcProcBuff = pstProc->pcProcBuff;
    stProcTmp.u32BuffLen = pstProc->u32BuffLen;

    ISP_PROC_PRINTF(&stProcTmp, pstProc->u32WriteLen,
                    "-----GE INFO-------------------------------------------------------------\n");

    ISP_PROC_PRINTF(&stProcTmp, pstProc->u32WriteLen,
                    "%12s" "%12s" "%12s" "%12s\n",
                    "Enable", "Threshold1", "Threshold3", "Strength");

    ISP_PROC_PRINTF(&stProcTmp, pstProc->u32WriteLen,
                    "%12u" "%12u" "%12u" "%12u\n",
                    pstGe->bEnable, pstGe->u16GeProcCtTh1, pstGe->u16GeProcCtTh3, pstGe->u16GeProcStrength);

    pstProc->u32WriteLen += 1;

    return HI_SUCCESS;
}

HI_S32 ISP_GeCtrl(VI_PIPE ViPipe, HI_U32 u32Cmd, HI_VOID *pValue)
{
    ISP_REGCFG_S  *pRegCfg = HI_NULL;


    switch (u32Cmd) {
        case ISP_WDR_MODE_SET :
            ISP_REGCFG_GET_CTX(ViPipe, pRegCfg);
            ISP_CHECK_POINTER(pRegCfg);
            ISP_GeInit(ViPipe, (HI_VOID *)&pRegCfg->stRegCfg);
            break;
        case ISP_CHANGE_IMAGE_MODE_SET:
            ISP_REGCFG_GET_CTX(ViPipe, pRegCfg);
            ISP_CHECK_POINTER(pRegCfg);
            break;
        case ISP_PROC_WRITE:
            GeProcWrite(ViPipe, (ISP_CTRL_PROC_WRITE_S *)pValue);
            break;

        default :
            break;
    }
    return HI_SUCCESS;
}

HI_S32 ISP_GeExit(VI_PIPE ViPipe)
{
    HI_U8 i;
    ISP_REGCFG_S  *pRegCfg = HI_NULL;


    ISP_REGCFG_GET_CTX(ViPipe, pRegCfg);

    for (i = 0; i < pRegCfg->stRegCfg.u8CfgNum; i++) {
        pRegCfg->stRegCfg.stAlgRegCfg[i].stGeRegCfg.abGeEn = HI_FALSE;
    }

    pRegCfg->stRegCfg.unKey.bit1GeCfg = 1;

    GeCtxExit(ViPipe);

    return HI_SUCCESS;
}

HI_S32 ISP_AlgRegisterGe(VI_PIPE ViPipe)
{
    ISP_CTX_S *pstIspCtx = HI_NULL;
    ISP_ALG_NODE_S *pstAlgs = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);
    ISP_ALG_CHECK(pstIspCtx->unAlgKey.bit1Ge);
    pstAlgs = ISP_SearchAlg(pstIspCtx->astAlgs);
    ISP_CHECK_POINTER(pstAlgs);

    pstAlgs->enAlgType = ISP_ALG_GE;
    pstAlgs->stAlgFunc.pfn_alg_init = ISP_GeInit;
    pstAlgs->stAlgFunc.pfn_alg_run  = ISP_GeRun;
    pstAlgs->stAlgFunc.pfn_alg_ctrl = ISP_GeCtrl;
    pstAlgs->stAlgFunc.pfn_alg_exit = ISP_GeExit;
    pstAlgs->bUsed = HI_TRUE;

    return HI_SUCCESS;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
