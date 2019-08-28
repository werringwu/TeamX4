
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "mkp_isp.h"
#include "mpi_isp.h"
#include "isp_main.h"
#include "hi_comm_isp.h"
#include "isp_ext_config.h"

#define EXP_STABLE_FRM (16)
#define IR_ABS(a)                   (((a) > 0) ? (a) : (-(a)))
#define IR_CHECK_RANGE(x, max, min) (((x) <= (max)) && ((x) >= (min)))

typedef struct hiISP_IR_S {
    HI_U32 u32IrStatusFrm;
    HI_U32 au32Iso[EXP_STABLE_FRM];
} ISP_IR_S;

ISP_IR_S g_astIrCtx[ISP_MAX_PIPE_NUM] = {{0}};
#define IR_AUTO_GET_CTX(dev, pstCtx)   pstCtx = &g_astIrCtx[dev]

HI_BOOL ISP_CheckExpStable(HI_U32 u32Iso, HI_U32 *pu32Iso)
{
    HI_U32 i = 0;
    HI_U64 u64AveIso = 0;
    HI_U64 u64AveIsoErr = 0;
    HI_U64 u64AveIsoErrSum = 0;

    for (i = 0; i < EXP_STABLE_FRM - 1; i++) {
        pu32Iso[i] = pu32Iso[i + 1];
    }
    pu32Iso[i] = u32Iso;

    for (i = 0; i < EXP_STABLE_FRM; i++) {
        u64AveIso += pu32Iso[i];
    }
    u64AveIso = u64AveIso / EXP_STABLE_FRM;

    for (i = 0; i < EXP_STABLE_FRM; i++) {
        u64AveIsoErr = (u64AveIso > pu32Iso[i]) ? (u64AveIso - pu32Iso[i]) : (pu32Iso[i] - u64AveIso);
        u64AveIsoErrSum += (i + 1) * u64AveIsoErr;
    }

    for (i = 0; i < EXP_STABLE_FRM; i++) {
        if (pu32Iso[i] == 0) {
            return HI_FALSE;
        }
    }

    if (u64AveIsoErrSum > (u64AveIso * EXP_STABLE_FRM * EXP_STABLE_FRM >> 5)) {
        return HI_FALSE;
    }
    return HI_TRUE;
}

/*                                                                Normal --> IR                     */
/* MinExp ----------------------------------------------------------------|----------------> MaxExp */
/*                                                                 u64Normal2IrExpThr               */
/*                                                                                                  */
/*                Normal <-- IR                                                                     */
/* MinExp <----------------|---------------------------------------------------------------- MaxExp */
/*                u64Ir2NormalExpThr                                                                */
/*                                BG                                                                */
/*                                 ^                                                                */
/*                                 |                                                                */
/*                            BGmax-      -|---------|-                                             */
/*                                 |       |         |                                              */
/*                                 |       |   IR    |                                              */
/*                                 |       |         |                                              */
/*                            BGmin-      -|---------|-                                             */
/*                                 |                                                                */
/*                                 |-------|---------|--------------> RG                            */
/*                                       RGmin     RGmax                                            */
HI_S32 ISP_IrAuto(VI_PIPE ViPipe, HI_U32 u32Iso, HI_U32 u32RG, HI_U32 u32BG, ISP_IR_AUTO_ATTR_S *pstIrAttr)
{
    ISP_IR_S *pstCtx = HI_NULL;
    IR_AUTO_GET_CTX(ViPipe, pstCtx);

    if (pstIrAttr->bEnable == HI_FALSE) {
        return HI_SUCCESS;
    }

    if (pstCtx->u32IrStatusFrm++ < EXP_STABLE_FRM) {
        pstIrAttr->enIrSwitch = ISP_IR_SWITCH_NONE;
        return HI_SUCCESS;
    } else {
        pstCtx->u32IrStatusFrm = EXP_STABLE_FRM;
    }

    /* check if Exp is stable */
    if (ISP_CheckExpStable(u32Iso, pstCtx->au32Iso) != HI_TRUE) {
        pstIrAttr->enIrSwitch = ISP_IR_SWITCH_NONE;
        return HI_SUCCESS;
    }

    /* Normal to IR */
    if (pstIrAttr->enIrStatus == ISP_IR_STATUS_NORMAL) {
        if (u32Iso > pstIrAttr->u32Normal2IrIsoThr) {
            pstIrAttr->enIrSwitch = ISP_IR_SWITCH_TO_IR;
            pstCtx->u32IrStatusFrm = 0;
            return HI_SUCCESS;
        } else {
            pstIrAttr->enIrSwitch = ISP_IR_SWITCH_NONE;
            return HI_SUCCESS;
        }
    }

    /* IR to Normal */
    if (pstIrAttr->enIrStatus == ISP_IR_STATUS_IR) {
        if (u32Iso < pstIrAttr->u32Ir2NormalIsoThr) {
            pstIrAttr->enIrSwitch = ISP_IR_SWITCH_TO_NORMAL;
            pstCtx->u32IrStatusFrm = 0;
            return HI_SUCCESS;
        } else {
            if (IR_CHECK_RANGE(u32RG, pstIrAttr->u32RGMax, pstIrAttr->u32RGMin)
                && IR_CHECK_RANGE(u32BG, pstIrAttr->u32BGMax, pstIrAttr->u32BGMin)) {
                pstIrAttr->enIrSwitch = ISP_IR_SWITCH_NONE;
                return HI_SUCCESS;
            } else {
                pstIrAttr->enIrSwitch = ISP_IR_SWITCH_TO_NORMAL;
                pstCtx->u32IrStatusFrm = 0;
                return HI_SUCCESS;
            }
        }
    }

    return HI_SUCCESS;
}

HI_S32 HI_MPI_ISP_IrAutoRunOnce(VI_PIPE ViPipe, ISP_IR_AUTO_ATTR_S *pstIrAttr)
{
    HI_S32 s32Ret = HI_SUCCESS;
    ISP_WB_STATISTICS_S stStat;
    HI_U32 u32Iso, u32RG, u32BG;

    ISP_CHECK_PIPE(ViPipe);
    ISP_CHECK_POINTER(pstIrAttr);
    ISP_CHECK_OPEN(ViPipe);
    ISP_CHECK_MEM_INIT(ViPipe);

    ISP_CHECK_BOOL(pstIrAttr->bEnable);
    if (pstIrAttr->u32RGMax > 0xFFF) {
        ISP_TRACE(HI_DBG_ERR, "Invalid u32RGMax :%u!\n", pstIrAttr->u32RGMax);
        return HI_ERR_ISP_ILLEGAL_PARAM;
    }
    if (pstIrAttr->u32RGMin > 0xFFF) {
        ISP_TRACE(HI_DBG_ERR, "Invalid u32RGMin :%u!\n", pstIrAttr->u32RGMin);
        return HI_ERR_ISP_ILLEGAL_PARAM;
    }
    if (pstIrAttr->u32BGMax > 0xFFF) {
        ISP_TRACE(HI_DBG_ERR, "Invalid u32BGMax :%u!\n", pstIrAttr->u32BGMax);
        return HI_ERR_ISP_ILLEGAL_PARAM;
    }
    if (pstIrAttr->u32BGMin > 0xFFF) {
        ISP_TRACE(HI_DBG_ERR, "Invalid u32BGMin :%u!\n", pstIrAttr->u32BGMin);
        return HI_ERR_ISP_ILLEGAL_PARAM;
    }
    if (pstIrAttr->u32RGMax < pstIrAttr->u32RGMin) {
        ISP_TRACE(HI_DBG_ERR, "u32RGMax(%u) should not be less than u32RGMin(%u)!\n", pstIrAttr->u32RGMax, pstIrAttr->u32RGMin);
        return HI_ERR_ISP_ILLEGAL_PARAM;
    }
    if (pstIrAttr->u32BGMax < pstIrAttr->u32BGMin) {
        ISP_TRACE(HI_DBG_ERR, "u32BGMax(%u) should not be less than u32BGMin(%u)!\n", pstIrAttr->u32BGMax, pstIrAttr->u32BGMin);
        return HI_ERR_ISP_ILLEGAL_PARAM;
    }
    if (pstIrAttr->enIrStatus >= ISP_IR_BUTT) {
        ISP_TRACE(HI_DBG_ERR, "Invalid enIrStatus :%u!\n", pstIrAttr->enIrStatus);
        return HI_ERR_ISP_ILLEGAL_PARAM;
    }

    s32Ret = HI_MPI_ISP_GetWBStatistics(ViPipe, &stStat);
    if (s32Ret != HI_SUCCESS) {
        printf("HI_MPI_ISP_GetWBStatistics failed\n");
        return s32Ret;
    }

    u32Iso = hi_ext_system_sys_iso_read(ViPipe);
    u32RG = ((HI_U32)stStat.u16GlobalR << 8) / DIV_0_TO_1(stStat.u16GlobalG);
    u32BG = ((HI_U32)stStat.u16GlobalB << 8) / DIV_0_TO_1(stStat.u16GlobalG);
    return ISP_IrAuto(ViPipe, u32Iso, u32RG, u32BG, pstIrAttr);
}
