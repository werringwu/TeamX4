/******************************************************************************

  Copyright (C), 2016, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : isp_mcds.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2013/01/16
  Description   :
  History       :
  1.Date        : 2014/01/16
    Author      :
    Modification: Created file

******************************************************************************/
#include "isp_config.h"
#include "hi_isp_debug.h"
#include "isp_ext_config.h"
#include "isp_math_utils.h"
#include "isp_alg.h"
#include "isp_sensor.h"
#include "isp_proc.h"


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define MCDS_EN          (1)
#define MCDS_FILTER_MODE (1)     // 1: filter mode; 0: discard mode

static HI_VOID McdsStaticRegInit(VI_PIPE ViPipe, ISP_MCDS_STATIC_REG_CFG_S *pstStaticRegCfg, ISP_CTX_S *pstIspCtx)
{
    static const HI_S32 Coeff_Filter_8tap_H[2][8] = {{ -16, 0, 145, 254, 145, 0, -16, 0}, {0, 0, 0, 256, 256, 0, 0, 0}};
    static const HI_S32 Coeff_Discard_8pixel_H[8] = {0, 0, 0, 512, 0, 0, 0, 0};

    HI_S32 s32HCoef[8];
    HI_S32 s32EnFilterV = 1;
    HI_U32  i;

    if (MCDS_FILTER_MODE) {         // Filter Mode
        if (pstIspCtx->stHdrAttr.enDynamicRange == DYNAMIC_RANGE_XDR) {     // HDR mode
            for (i = 0; i < 8; i++) {

                s32HCoef[i] = Coeff_Filter_8tap_H[1][i];
            }
        } else { // SDR mode
            for (i = 0; i < 8; i++) {
                s32HCoef[i] = Coeff_Filter_8tap_H[0][i];
            }
        }
        s32EnFilterV = 1;
    } else {    // discard Mode
        for (i = 0; i < 8; i++) {
            s32HCoef[i] = Coeff_Discard_8pixel_H[i];
        }
        s32EnFilterV = 0;
    }

    pstStaticRegCfg->bHcdsEn    = 1;
    pstStaticRegCfg->s32HCoef0  = s32HCoef[0];
    pstStaticRegCfg->s32HCoef1  = s32HCoef[1];
    pstStaticRegCfg->s32HCoef2  = s32HCoef[2];
    pstStaticRegCfg->s32HCoef3  = s32HCoef[3];
    pstStaticRegCfg->s32HCoef4  = s32HCoef[4];
    pstStaticRegCfg->s32HCoef5  = s32HCoef[5];
    pstStaticRegCfg->s32HCoef6  = s32HCoef[6];
    pstStaticRegCfg->s32HCoef7  = s32HCoef[7];
    pstStaticRegCfg->s32EnFilterV = s32EnFilterV;
    pstStaticRegCfg->bStaticResh    = HI_TRUE;
}

static HI_VOID McdsDynaRegInit(ISP_MCDS_DYNA_REG_CFG_S *pstDynaRegCfg, ISP_CTX_S *pstIspCtx)
{
    pstDynaRegCfg->bDynaResh = 1;

    if (pstIspCtx->stHdrAttr.enFormat == PIXEL_FORMAT_YVU_SEMIPLANAR_422) {
        pstDynaRegCfg->bVcdsEn    = HI_FALSE;
    } else { // 400 is the same with 420
        pstDynaRegCfg->bVcdsEn    = HI_TRUE;
    }
}

static HI_VOID McdsRegsInitialize(VI_PIPE ViPipe, ISP_REG_CFG_S *pRegCfg)
{
    HI_U32 i;

    ISP_CTX_S *pstIspCtx = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);

    for (i = 0; i < pRegCfg->u8CfgNum; i++) {
        pRegCfg->stAlgRegCfg[i].stMcdsRegCfg.bMCDSen = HI_TRUE;
        McdsStaticRegInit(ViPipe, &(pRegCfg->stAlgRegCfg[i].stMcdsRegCfg.stStaticRegCfg), pstIspCtx);
        McdsDynaRegInit(&(pRegCfg->stAlgRegCfg[i].stMcdsRegCfg.stDynaRegCfg), pstIspCtx);
    }

    pRegCfg->unKey.bit1McdsCfg = 1;

    return;
}

HI_S32 McdsProcWrite(VI_PIPE ViPipe, ISP_CTRL_PROC_WRITE_S *pstProc)
{
    return HI_SUCCESS;
}

HI_S32 ISP_McdsInit(VI_PIPE ViPipe, HI_VOID *pRegCfg)
{
    ISP_REG_CFG_S *pstRegCfg = (ISP_REG_CFG_S *)pRegCfg;

    McdsRegsInitialize(ViPipe, pstRegCfg);

    return HI_SUCCESS;
}

HI_S32 ISP_McdsRun(VI_PIPE ViPipe, const HI_VOID *pStatInfo, HI_VOID *pRegCfg, HI_S32 s32Rsv)
{
    HI_U8  i;
    ISP_CTX_S *pstIspCtx = HI_NULL;
    ISP_REG_CFG_S *pstRegCfg = (ISP_REG_CFG_S *)pRegCfg;

    ISP_GET_CTX(ViPipe, pstIspCtx);
    ISP_ALG_CHECK(pstIspCtx->unAlgKey.bit1Mcds);

    if (pstIspCtx->stLinkage.bDefectPixel) {
        return HI_SUCCESS;
    }

    /* calculate every two interrupts */
    if ((pstIspCtx->u32FrameCnt % 2 != 0) && (pstIspCtx->stLinkage.bSnapState != HI_TRUE)) {
        return HI_SUCCESS;
    }

    for (i = 0; i < pstRegCfg->u8CfgNum; i++) {
        pstRegCfg->stAlgRegCfg[i].stMcdsRegCfg.bMCDSen = HI_TRUE;
        pstRegCfg->stAlgRegCfg[i].stMcdsRegCfg.stDynaRegCfg.bDynaResh = 1;
        if (pstIspCtx->stHdrAttr.enFormat == PIXEL_FORMAT_YVU_SEMIPLANAR_422) {
            pstRegCfg->stAlgRegCfg[i].stMcdsRegCfg.stDynaRegCfg.bVcdsEn = HI_FALSE;
        } else { // 400 is the same with 420
            pstRegCfg->stAlgRegCfg[i].stMcdsRegCfg.stDynaRegCfg.bVcdsEn    = HI_TRUE;
        }
    }
    pstRegCfg->unKey.bit1McdsCfg = 1;

    return HI_SUCCESS;
}

HI_S32 ISP_McdsCtrl(VI_PIPE ViPipe, HI_U32 u32Cmd, HI_VOID *pValue)
{
    ISP_REGCFG_S  *pRegCfg = HI_NULL;
    switch (u32Cmd) {
        case ISP_PROC_WRITE:
            McdsProcWrite(ViPipe, (ISP_CTRL_PROC_WRITE_S *)pValue);
            break;
        case  ISP_CHANGE_IMAGE_MODE_SET:
            ISP_REGCFG_GET_CTX(ViPipe, pRegCfg);
            ISP_CHECK_POINTER(pRegCfg);
            ISP_McdsInit(ViPipe, &pRegCfg->stRegCfg);
        default :
            break;
    }
    return HI_SUCCESS;
}

HI_S32 ISP_McdsExit(VI_PIPE ViPipe)
{
    HI_U8 i;
    ISP_REGCFG_S  *pRegCfg = HI_NULL;

    ISP_REGCFG_GET_CTX(ViPipe, pRegCfg);

    for (i = 0; i < pRegCfg->stRegCfg.u8CfgNum; i++) {
        pRegCfg->stRegCfg.stAlgRegCfg[i].stMcdsRegCfg.bMCDSen = HI_FALSE;
    }

    pRegCfg->stRegCfg.unKey.bit1McdsCfg = 1;
    return HI_SUCCESS;
}

HI_S32 ISP_AlgRegisterMcds(VI_PIPE ViPipe)
{
    ISP_CTX_S *pstIspCtx = HI_NULL;
    ISP_ALG_NODE_S *pstAlgs = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);
    ISP_ALG_CHECK(pstIspCtx->unAlgKey.bit1Mcds);
    pstAlgs = ISP_SearchAlg(pstIspCtx->astAlgs);
    ISP_CHECK_POINTER(pstAlgs);

    pstAlgs->enAlgType = ISP_ALG_MCDS;
    pstAlgs->stAlgFunc.pfn_alg_init = ISP_McdsInit;
    pstAlgs->stAlgFunc.pfn_alg_run  = ISP_McdsRun;
    pstAlgs->stAlgFunc.pfn_alg_ctrl = ISP_McdsCtrl;
    pstAlgs->stAlgFunc.pfn_alg_exit = ISP_McdsExit;
    pstAlgs->bUsed = HI_TRUE;

    return HI_SUCCESS;
}
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
