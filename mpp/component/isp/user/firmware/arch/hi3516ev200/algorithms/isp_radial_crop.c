/******************************************************************************

  Copyright (C), 2016, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : isp_radial_crop.c
  Version       : Initial Draft
  Author        :
  Created       : 2016/10/09
  Last Modified :
  Description   : Radial crop Algorithms
  Function List :
  History       :
  1.Date        : 2016/10/09
    Modification: Created file

******************************************************************************/
#include <math.h>
#include <stdio.h>
#include "isp_alg.h"
#include "isp_ext_config.h"
#include "isp_config.h"
#include "isp_sensor.h"


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

typedef struct {
    HI_BOOL bRcEn;
    HI_BOOL bCoefUpdateEn;
    HI_U16  u16CenterVerCoor;
    HI_U16  u16CenterHorCoor;
    HI_U32  u32Radius;
} ISP_RC_S;

ISP_RC_S g_astRcCtx[ISP_MAX_PIPE_NUM] = {{0}};
#define RC_GET_CTX(dev, pstCtx)   pstCtx = &g_astRcCtx[dev]

static HI_VOID RcUsrRegsInitialize(ISP_RC_USR_CFG_S *pstUsrRegCfg, ISP_RC_S *pstRc)
{
    pstUsrRegCfg->u16CenterHorCoor = pstRc->u16CenterHorCoor;
    pstUsrRegCfg->u16CenterVerCoor = pstRc->u16CenterVerCoor;
    pstUsrRegCfg->u32SquareRadius  = pstRc->u32Radius * pstRc->u32Radius;
    pstUsrRegCfg->bResh            = HI_TRUE;
}

static HI_VOID RcRegsInitialize(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfg)
{
    ISP_RC_S   *pstRc = HI_NULL;

    RC_GET_CTX(ViPipe, pstRc);

    RcUsrRegsInitialize(&pstRegCfg->stAlgRegCfg[0].stRcRegCfg.stUsrRegCfg, pstRc);

    pstRegCfg->stAlgRegCfg[0].stRcRegCfg.bRcEn = pstRc->bRcEn;

    pstRegCfg->unKey.bit1RcCfg = 1;

    return;
}

static HI_VOID RcExtRegsInitialize(VI_PIPE ViPipe)
{
    ISP_RC_S   *pstRc = HI_NULL;

    RC_GET_CTX(ViPipe, pstRc);

    hi_ext_system_rc_en_write(ViPipe, pstRc->bRcEn);
    hi_ext_system_rc_center_hor_coor_write(ViPipe, pstRc->u16CenterHorCoor);
    hi_ext_system_rc_center_ver_coor_write(ViPipe, pstRc->u16CenterVerCoor);
    hi_ext_system_rc_radius_write(ViPipe, pstRc->u32Radius);
    hi_ext_system_rc_coef_update_en_write(ViPipe, HI_FALSE);

    return;
}

static HI_VOID RcInitialize(VI_PIPE ViPipe)
{
    HI_U32     u32HorCoor, u32VerCoor;
    ISP_RC_S   *pstRc = HI_NULL;
    ISP_CTX_S  *pstIspCtx = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);
    RC_GET_CTX(ViPipe, pstRc);

    u32HorCoor  = pstIspCtx->stBlockAttr.stFrameRect.u32Width >> 1;
    u32VerCoor  = pstIspCtx->stBlockAttr.stFrameRect.u32Height >> 1;
    pstRc->u16CenterHorCoor = u32HorCoor;
    pstRc->u16CenterVerCoor = u32VerCoor;
    pstRc->u32Radius        = Sqrt32(u32HorCoor * u32HorCoor + u32VerCoor * u32VerCoor) + 1;
    pstRc->bRcEn   = HI_FALSE;
}

static HI_BOOL __inline CheckRcOpen(ISP_RC_S  *pstRc)
{
    return (pstRc->bRcEn == HI_TRUE);
}

static HI_VOID RcReadExtRegs(VI_PIPE ViPipe)
{
    ISP_RC_S *pstRc      = HI_NULL;
    RC_GET_CTX(ViPipe, pstRc);

    pstRc->bCoefUpdateEn    = hi_ext_system_rc_coef_update_en_read(ViPipe);

    hi_ext_system_rc_coef_update_en_write(ViPipe, HI_FALSE);

    if (pstRc->bCoefUpdateEn) {
        pstRc->u16CenterHorCoor = hi_ext_system_rc_center_hor_coor_read(ViPipe);
        pstRc->u16CenterVerCoor = hi_ext_system_rc_center_ver_coor_read(ViPipe);
        pstRc->u32Radius        = hi_ext_system_rc_radius_read(ViPipe);
    }
}

HI_VOID Rc_Usr_Fw(ISP_RC_USR_CFG_S *pstUsrRegCfg, ISP_RC_S *pstRc)
{
    pstUsrRegCfg->u16CenterHorCoor = pstRc->u16CenterHorCoor;
    pstUsrRegCfg->u16CenterVerCoor = pstRc->u16CenterVerCoor;
    pstUsrRegCfg->u32SquareRadius  = pstRc->u32Radius * pstRc->u32Radius;
    pstUsrRegCfg->bResh            = HI_TRUE;
}

static HI_VOID ISP_RcWdrModeSet(VI_PIPE ViPipe, HI_VOID *pRegCfg)
{
    ISP_REG_CFG_S *pstRegCfg = (ISP_REG_CFG_S *)pRegCfg;
    HI_U8  u8WDRMode;
    ISP_CTX_S *pstIspCtx = HI_NULL;
    ISP_RC_S  *pstRc       = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);
    u8WDRMode = pstIspCtx->u8SnsWDRMode;
    RC_GET_CTX(ViPipe, pstRc);

    if(IS_2to1_WDR_MODE(u8WDRMode) && (pstRc->bRcEn)) {
        ISP_TRACE(HI_DBG_ERR, "Rc not support in Wdr mode\n");
    }
    pstRegCfg->unKey.bit1RcCfg = 1;
    pstRegCfg->stAlgRegCfg[0].stRcRegCfg.stUsrRegCfg.bResh = HI_TRUE;
}

HI_S32 ISP_RcInit(VI_PIPE ViPipe, HI_VOID *pRegCfg)
{
    ISP_REG_CFG_S *pstRegCfg = (ISP_REG_CFG_S *)pRegCfg;

    RcInitialize(ViPipe);
    RcRegsInitialize(ViPipe, pstRegCfg);
    RcExtRegsInitialize(ViPipe);

    return HI_SUCCESS;
}

HI_S32 ISP_RcRun(VI_PIPE ViPipe, const HI_VOID *pStatInfo,
                 HI_VOID *pRegCfg, HI_S32 s32Rsv)
{
    ISP_RC_S  *pstRc       = HI_NULL;
    ISP_CTX_S *pstIspCtx   = HI_NULL;
    ISP_REG_CFG_S *pstRegCfg  = (ISP_REG_CFG_S *)pRegCfg;

    ISP_GET_CTX(ViPipe, pstIspCtx);
    RC_GET_CTX(ViPipe, pstRc);

    /* calculate every two interrupts */
    if ((pstIspCtx->u32FrameCnt % 2 != 0) && (pstIspCtx->stLinkage.bSnapState != HI_TRUE)) {
        return HI_SUCCESS;
    }

    pstRc->bRcEn = hi_ext_system_rc_en_read(ViPipe);
    pstRegCfg->stAlgRegCfg[0].stRcRegCfg.bRcEn = pstRc->bRcEn;
    pstRegCfg->unKey.bit1RcCfg = 1;

    /* check hardware setting */
    if (!CheckRcOpen(pstRc)) {
        return HI_SUCCESS;
    }

    RcReadExtRegs(ViPipe);

    if (pstRc->bCoefUpdateEn) {
        Rc_Usr_Fw(&pstRegCfg->stAlgRegCfg[0].stRcRegCfg.stUsrRegCfg, pstRc);
    }

    return HI_SUCCESS;
}

HI_S32 ISP_RcCtrl(VI_PIPE ViPipe, HI_U32 u32Cmd, HI_VOID *pValue)
{
    ISP_REGCFG_S  *pRegCfg = HI_NULL;

    switch (u32Cmd) {
        case ISP_WDR_MODE_SET :
            ISP_REGCFG_GET_CTX(ViPipe, pRegCfg);
            ISP_CHECK_POINTER(pRegCfg);
            ISP_RcWdrModeSet(ViPipe, (HI_VOID *)&pRegCfg->stRegCfg);
            break;
        default :
            break;
    }
    return HI_SUCCESS;
}

HI_S32 ISP_RcExit(VI_PIPE ViPipe)
{
    ISP_REGCFG_S  *pRegCfg = HI_NULL;

    ISP_REGCFG_GET_CTX(ViPipe, pRegCfg);

    pRegCfg->stRegCfg.stAlgRegCfg[0].stRcRegCfg.bRcEn = HI_FALSE;
    pRegCfg->stRegCfg.unKey.bit1RcCfg                 = 1;

    return HI_SUCCESS;
}

HI_S32 ISP_AlgRegisterRc(VI_PIPE ViPipe)
{
    ISP_CTX_S *pstIspCtx = HI_NULL;
    ISP_ALG_NODE_S *pstAlgs = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);

    pstAlgs = ISP_SearchAlg(pstIspCtx->astAlgs);
    ISP_CHECK_POINTER(pstAlgs);

    pstAlgs->enAlgType = ISP_ALG_RC;
    pstAlgs->stAlgFunc.pfn_alg_init = ISP_RcInit;
    pstAlgs->stAlgFunc.pfn_alg_run  = ISP_RcRun;
    pstAlgs->stAlgFunc.pfn_alg_ctrl = ISP_RcCtrl;
    pstAlgs->stAlgFunc.pfn_alg_exit = ISP_RcExit;
    pstAlgs->bUsed = HI_TRUE;

    return HI_SUCCESS;
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */
