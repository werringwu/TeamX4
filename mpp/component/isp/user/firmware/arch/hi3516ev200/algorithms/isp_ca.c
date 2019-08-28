/******************************************************************************

  Copyright (C), 2016, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : isp_ca.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       :
  Description   :
  History       :
  1.Date        :
    Author      :
    Modification: Created file

******************************************************************************/

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

#define HI_ISP_CA_CSC_DC_LEN       (3)
#define HI_ISP_CA_CSC_COEF_LEN     (9)
#define HI_ISP_CA_CSC_TYPE_DEFAULT (0)
#define HI_ISP_CA_RATIO_MAX        (2047)

static const  HI_U16 g_au16YRatioLut[HI_ISP_CA_YRATIO_LUT_LENGTH] = {
    516, 525, 534, 544, 554, 563, 573, 583, 594, 604, 614, 624, 634, 644, 654, 664, 674, 684, 694,
    704, 713, 723, 732, 741, 750, 758, 766, 775, 782, 790, 797, 804, 811, 817, 823, 828, 834,
    839, 844, 848, 853, 857, 861, 865, 868, 872, 875, 878, 881, 884, 887, 890, 892, 895, 898,
    900, 903, 905, 908, 910, 913, 915, 918, 921, 924, 926, 929, 932, 935, 937, 940, 943, 945,
    948, 950, 952, 955, 957, 959, 961, 964, 966, 968, 970, 972, 974, 976, 978, 979, 981, 983,
    985, 987, 988, 990, 992, 993, 994, 995, 996, 997, 998, 998, 999, 1000, 1001, 1002, 1002,
    1003, 1004, 1005, 1006, 1006, 1007, 1008, 1009, 1010, 1011, 1012, 1013, 1014, 1015, 1016, 1018, 1019, 1020, 1022, 1024
};
static const  HI_S16 g_as16ISORatio[ISP_AUTO_ISO_STRENGTH_NUM] = {1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024};

typedef struct hiISP_CA_S {
    HI_BOOL bCaEn;      // u1.0
    HI_BOOL bCaCoefUpdateEn;
    HI_U16 u16LumaThdHigh;
    HI_U16 u16CaLumaThrLow;
    HI_S16 s16CaLumaRatioLow;
    HI_U16 u16CaSkinRatioThrHigh;
    HI_U16 u16CaSkinRatioThrMid;
    HI_S16 s16SaturationRatio;
    HI_U16 au16YRatioLut[HI_ISP_CA_YRATIO_LUT_LENGTH];
    HI_S16 as16CaIsoRatio[ISP_AUTO_ISO_STRENGTH_NUM]; // 16
    HI_S16 s16ProcInfoIsoRatio;
} ISP_CA_S;

ISP_CA_S *g_pastCaCtx[ISP_MAX_PIPE_NUM] = {HI_NULL};

#define CA_GET_CTX(dev, pstCtx)   (pstCtx = g_pastCaCtx[dev])
#define CA_SET_CTX(dev, pstCtx)   (g_pastCaCtx[dev] = pstCtx)
#define CA_RESET_CTX(dev)         (g_pastCaCtx[dev] = HI_NULL)

HI_S32 CaCtxInit(VI_PIPE ViPipe)
{
    ISP_CA_S *pastCaCtx = HI_NULL;

    CA_GET_CTX(ViPipe, pastCaCtx);

    if (pastCaCtx == HI_NULL) {
        pastCaCtx = (ISP_CA_S *)ISP_MALLOC(sizeof(ISP_CA_S));
        if (pastCaCtx == HI_NULL) {
            ISP_TRACE(HI_DBG_ERR, "Isp[%d] CaCtx malloc memory failed!\n", ViPipe);
            return HI_ERR_ISP_NOMEM;
        }
    }

    memset(pastCaCtx, 0, sizeof(ISP_CA_S));

    CA_SET_CTX(ViPipe, pastCaCtx);

    return HI_SUCCESS;
}

HI_VOID CaCtxExit(VI_PIPE ViPipe)
{
    ISP_CA_S *pastCaCtx = HI_NULL;

    CA_GET_CTX(ViPipe, pastCaCtx);
    ISP_FREE(pastCaCtx);
    CA_RESET_CTX(ViPipe);
}

static HI_VOID CaExtRegsInitialize(VI_PIPE ViPipe)
{
    HI_U16 i;
    ISP_CA_S  *pstCA     = HI_NULL;

    CA_GET_CTX(ViPipe, pstCA);

    hi_ext_system_ca_en_write(ViPipe, pstCA->bCaEn);

    hi_ext_system_ca_luma_thd_high_write(ViPipe, HI_ISP_EXT_CA_LUMA_THD_HIGH_DEFAULT);
    hi_ext_system_ca_saturation_ratio_write(ViPipe, HI_ISP_EXT_CA_SATURATION_RATIO_DEFAULT);
    hi_ext_system_ca_coef_update_en_write(ViPipe, HI_TRUE);

    for (i = 0; i < HI_ISP_CA_YRATIO_LUT_LENGTH; i++) {
        hi_ext_system_ca_y_ratio_lut_write(ViPipe, i, pstCA->au16YRatioLut[i]);
    }

    for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++) {
        hi_ext_system_ca_iso_ratio_lut_write(ViPipe, i, pstCA->as16CaIsoRatio[i]);
    }

    return;
}

static HI_VOID CaStaticRegsInitialize(HI_U8 i, ISP_CA_STATIC_CFG_S *pstStaticRegCfg)
{
    pstStaticRegCfg->bCaLlhcProcEn  = HI_TRUE;
    pstStaticRegCfg->bCaSkinProcEn  = HI_TRUE;
    pstStaticRegCfg->bCaSatuAdjEn   = HI_TRUE;

    pstStaticRegCfg->u16CaLumaThrLow         = HI_ISP_CA_LUMA_THD_LOW_DEFAULT;
    pstStaticRegCfg->u16CaDarkChromaThrLow   = HI_ISP_CA_DARKCHROMA_THD_LOW_DEFAULT;
    pstStaticRegCfg->u16CaDarkChromaThrHigh  = HI_ISP_CA_DARKCHROMA_THD_HIGH_DEFAULT;
    pstStaticRegCfg->u16CaSDarkChromaThrLow  = HI_ISP_CA_SDARKCHROMA_THD_LOW_DEFAULT;
    pstStaticRegCfg->u16CaSDarkChromaThrHigh = HI_ISP_CA_SDARKCHROMA_THD_HIGH_DEFAULT;
    pstStaticRegCfg->s16CaLumaRatioLow       = HI_ISP_CA_LUMA_RATIO_LOW_DEFAULT;

    pstStaticRegCfg->u16CaSkinLowLumaMinU    = HI_ISP_CA_SKINLOWLUAM_UMIN_DEFAULT;
    pstStaticRegCfg->u16CaSkinLowLumaMaxU    = HI_ISP_CA_SKINLOWLUAM_UMAX_DEFAULT;
    pstStaticRegCfg->u16CaSkinLowLumaMinUy   = HI_ISP_CA_SKINLOWLUAM_UYMIN_DEFAULT;
    pstStaticRegCfg->u16CaSkinLowLumaMaxUy   = HI_ISP_CA_SKINLOWLUAM_UYMAX_DEFAULT;
    pstStaticRegCfg->u16CaSkinHighLumaMinU   = HI_ISP_CA_SKINHIGHLUAM_UMIN_DEFAULT;
    pstStaticRegCfg->u16CaSkinHighLumaMaxU   = HI_ISP_CA_SKINHIGHLUAM_UMAX_DEFAULT;
    pstStaticRegCfg->u16CaSkinHighLumaMinUy  = HI_ISP_CA_SKINHIGHLUAM_UYMIN_DEFAULT;
    pstStaticRegCfg->u16CaSkinHighLumaMaxUy  = HI_ISP_CA_SKINHIGHLUAM_UYMAX_DEFAULT;
    pstStaticRegCfg->u16CaSkinLowLumaMinV    = HI_ISP_CA_SKINLOWLUAM_VMIN_DEFAULT;
    pstStaticRegCfg->u16CaSkinLowLumaMaxV    = HI_ISP_CA_SKINLOWLUAM_VMAX_DEFAULT;
    pstStaticRegCfg->u16CaSkinLowLumaMinVy   = HI_ISP_CA_SKINLOWLUAM_VYMIN_DEFAULT;
    pstStaticRegCfg->u16CaSkinLowLumaMaxVy   = HI_ISP_CA_SKINLOWLUAM_VYMAX_DEFAULT;
    pstStaticRegCfg->u16CaSkinHighLumaMinV   = HI_ISP_CA_SKINHIGHLUAM_VMIN_DEFAULT;
    pstStaticRegCfg->u16CaSkinHighLumaMaxV   = HI_ISP_CA_SKINHIGHLUAM_VMAX_DEFAULT;
    pstStaticRegCfg->u16CaSkinHighLumaMinVy  = HI_ISP_CA_SKINHIGHLUAM_VYMIN_DEFAULT;
    pstStaticRegCfg->u16CaSkinHighLumaMaxVy  = HI_ISP_CA_SKINHIGHLUAM_VYMAX_DEFAULT;
    pstStaticRegCfg->s16CaSkinUvDiff         = HI_ISP_CA_SKINUVDIFF_DEFAULT;
    pstStaticRegCfg->u16CaSkinRatioThrLow    = HI_ISP_CA_SKINRATIOTHD_LOW_DEFAULT;
    pstStaticRegCfg->u16CaSkinRatioThrMid    = HI_ISP_CA_SKINRATIOTHD_MID_DEFAULT;
    pstStaticRegCfg->u16CaSkinRatioThrHigh   = HI_ISP_CA_SKINRATIOTHD_HIGH_DEFAULT;

    pstStaticRegCfg->bStaticResh = HI_TRUE;
}

static HI_VOID CaUsrRegsInitialize(ISP_CA_USR_CFG_S *pstUsrRegCfg, ISP_CA_S  *pstCA)
{
    HI_U16 u16Index;

    pstUsrRegCfg->u16CaLumaThrHigh  = HI_ISP_EXT_CA_LUMA_THD_HIGH_DEFAULT;
    u16Index = (pstUsrRegCfg->u16CaLumaThrHigh >> 3);
    u16Index = (u16Index >= HI_ISP_CA_YRATIO_LUT_LENGTH) ? (HI_ISP_CA_YRATIO_LUT_LENGTH - 1) : u16Index;
    pstUsrRegCfg->s16CaLumaRatioHigh = pstCA->au16YRatioLut[u16Index];

    memcpy(pstUsrRegCfg->au16YRatioLUT, pstCA->au16YRatioLut, HI_ISP_CA_YRATIO_LUT_LENGTH * sizeof(HI_U16));

    pstUsrRegCfg->bCaLutUpdateEn = HI_TRUE;
    pstUsrRegCfg->bResh          = HI_TRUE;
    pstUsrRegCfg->u32UpdateIndex = 1;
    pstUsrRegCfg->u8BufId        = 0;
}

static HI_VOID CaDynaRegsInitialize(ISP_CA_DYNA_CFG_S *pstDynaRegCfg)
{
    pstDynaRegCfg->u16CaISORatio  = 1024;
    pstDynaRegCfg->bResh          = HI_TRUE;

    return;
}

static HI_VOID CaRegsInitialize(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfg)
{
    HI_U8 i;
    ISP_CA_S  *pstCA     = HI_NULL;

    CA_GET_CTX(ViPipe, pstCA);

    for (i = 0; i < pstRegCfg->u8CfgNum; i++) {
        pstRegCfg->stAlgRegCfg[i].stCaRegCfg.bCaEn      = pstCA->bCaEn;
        pstRegCfg->stAlgRegCfg[i].stCaRegCfg.bLut2SttEn = HI_TRUE;

        CaStaticRegsInitialize(i, &pstRegCfg->stAlgRegCfg[i].stCaRegCfg.stStaticRegCfg);
        CaUsrRegsInitialize(&pstRegCfg->stAlgRegCfg[i].stCaRegCfg.stUsrRegCfg, pstCA);
        CaDynaRegsInitialize(&pstRegCfg->stAlgRegCfg[i].stCaRegCfg.stDynaRegCfg);
    }
    pstCA->u16CaLumaThrLow = pstRegCfg->stAlgRegCfg[0].stCaRegCfg.stStaticRegCfg.u16CaLumaThrLow;
    pstCA->s16CaLumaRatioLow = pstRegCfg->stAlgRegCfg[0].stCaRegCfg.stStaticRegCfg.s16CaLumaRatioLow;
    pstCA->u16CaSkinRatioThrHigh = pstRegCfg->stAlgRegCfg[0].stCaRegCfg.stStaticRegCfg.u16CaSkinRatioThrHigh;
    pstCA->u16CaSkinRatioThrMid = pstRegCfg->stAlgRegCfg[0].stCaRegCfg.stStaticRegCfg.u16CaSkinRatioThrMid;

    pstRegCfg->unKey.bit1CaCfg = 1;
}

static HI_S32 CaCheckCmosParam(VI_PIPE ViPipe, const ISP_CMOS_CA_S *pstCmosCa)
{
    HI_U16 i;

    ISP_CHECK_BOOL(pstCmosCa->bEnable);

    for (i = 0; i < HI_ISP_CA_YRATIO_LUT_LENGTH; i++) {
        if (pstCmosCa->au16YRatioLut[i] > HI_ISP_CA_RATIO_MAX) {
            ISP_TRACE(HI_DBG_ERR, "Err au16YRatioLut[%d]!\n", i);
            return HI_ERR_ISP_ILLEGAL_PARAM;
        }
    }

    for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++) {
        if (pstCmosCa->as16ISORatio[i] > HI_ISP_CA_RATIO_MAX) {
            ISP_TRACE(HI_DBG_ERR, "Err as16ISORatio[%d]!\n", i);
            return HI_ERR_ISP_ILLEGAL_PARAM;
        }
    }

    return HI_SUCCESS;
}

static HI_S32 CaInInitialize(VI_PIPE ViPipe)
{
    HI_S32             s32Ret;
    ISP_CA_S           *pstCA     = HI_NULL;
    ISP_CMOS_DEFAULT_S *pstSnsDft = HI_NULL;

    CA_GET_CTX(ViPipe, pstCA);
    ISP_SensorGetDefault(ViPipe, &pstSnsDft);

    if (pstSnsDft->unKey.bit1Ca) {
        ISP_CHECK_POINTER(pstSnsDft->pstCa);

        s32Ret = CaCheckCmosParam(ViPipe, pstSnsDft->pstCa);
        if (s32Ret != HI_SUCCESS) {
            return s32Ret;
        }

        pstCA->bCaEn  = pstSnsDft->pstCa->bEnable;
        memcpy(pstCA->au16YRatioLut, pstSnsDft->pstCa->au16YRatioLut, HI_ISP_CA_YRATIO_LUT_LENGTH * sizeof(HI_U16));
        memcpy(pstCA->as16CaIsoRatio, pstSnsDft->pstCa->as16ISORatio, ISP_AUTO_ISO_STRENGTH_NUM * sizeof(HI_S16));
    } else {
        pstCA->bCaEn  = HI_TRUE;
        memcpy(pstCA->au16YRatioLut,  g_au16YRatioLut, HI_ISP_CA_YRATIO_LUT_LENGTH * sizeof(HI_U16));
        memcpy(pstCA->as16CaIsoRatio, g_as16ISORatio,  ISP_AUTO_ISO_STRENGTH_NUM * sizeof(HI_S16));
    }

    return HI_SUCCESS;
}

static HI_S32 CaReadExtregs(VI_PIPE ViPipe)
{
    HI_U16 i;
    ISP_CA_S *pstCA = HI_NULL;

    CA_GET_CTX(ViPipe, pstCA);

    pstCA->bCaCoefUpdateEn = hi_ext_system_ca_coef_update_en_read(ViPipe);
    hi_ext_system_ca_coef_update_en_write(ViPipe, HI_FALSE);

    if (pstCA->bCaCoefUpdateEn) {

        for (i = 0; i < HI_ISP_CA_YRATIO_LUT_LENGTH; i++) {
            pstCA->au16YRatioLut[i] = hi_ext_system_ca_y_ratio_lut_read(ViPipe, i);
        }


        for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++) {
            pstCA->as16CaIsoRatio[i] = hi_ext_system_ca_iso_ratio_lut_read(ViPipe, i);
        }

        pstCA->u16LumaThdHigh     = hi_ext_system_ca_luma_thd_high_read(ViPipe);
        pstCA->s16SaturationRatio = (HI_S16)hi_ext_system_ca_saturation_ratio_read(ViPipe);
    }

    return HI_SUCCESS;
}

static HI_S32 CaGetValueFromLut(HI_S32 x, HI_U32 const *pLutX, HI_S16 *pLutY, HI_S32 length)
{
    HI_S32 n = 0;

    if (x <= pLutX[0]) {
        return pLutY[0];
    }

    for (n = 1; n < length; n++) {
        if (x <= pLutX[n]) {
            return (pLutY[n - 1] + (HI_S64)(pLutY[n] - pLutY[n - 1]) * (HI_S64)(x - (HI_S32)pLutX[n - 1]) / DIV_0_TO_1((HI_S32)pLutX[n] - (HI_S32)pLutX[n - 1]));
        }
    }

    return pLutY[length - 1];
}

static HI_BOOL __inline CheckCaOpen(ISP_CA_S *pstCA)
{
    return (pstCA->bCaEn == HI_TRUE);
}

static HI_VOID Isp_Ca_Usr_Fw(ISP_CA_S *pstCA, ISP_CA_USR_CFG_S *pstUsrRegCfg)
{
    HI_U16 j, i;
    HI_U16 u16lum_min = 0;
    HI_U16 u16lum_max = 0;
    HI_U16 u16lmin = 0;
    HI_U16 u16lmax = 0;
    HI_U16 u16lum_a = 0;
    HI_U16 u16lum_b = 0;
    HI_U16 u16weight = 200;

    for (j = 0; j < HI_ISP_CA_YRATIO_LUT_LENGTH; j++) {
        pstUsrRegCfg->au16YRatioLUT[j] = MIN2((HI_S32)pstCA->au16YRatioLut[j] * pstCA->s16SaturationRatio / 1000, 2047);
    }

    u16lum_min = pstUsrRegCfg->au16YRatioLUT[0];
    u16lum_max = pstUsrRegCfg->au16YRatioLUT[0];
    u16lmin = pstUsrRegCfg->au16YRatioLUT[0];
    u16lmax = pstUsrRegCfg->au16YRatioLUT[0];
    u16lum_a = pstUsrRegCfg->au16YRatioLUT[0];
    u16lum_b = pstUsrRegCfg->au16YRatioLUT[0];

    for (i = 0; i < pstCA->u16CaLumaThrLow / 8; i = i + 2) {
        if (pstUsrRegCfg->au16YRatioLUT[i] >= pstUsrRegCfg->au16YRatioLUT[i + 1]) {
            u16lmin = pstUsrRegCfg->au16YRatioLUT[i + 1];
            u16lmax = pstUsrRegCfg->au16YRatioLUT[i];
        } else {
            u16lmax = pstUsrRegCfg->au16YRatioLUT[i + 1];
            u16lmin = pstUsrRegCfg->au16YRatioLUT[i];
        }

        if (u16lmax >= u16lum_max) {
            u16lum_max = u16lmax;
        }

        if (u16lmin <= u16lum_min) {
            u16lum_min = u16lmin;
        }
    }

    if (ABS((HI_S16)u16lum_max - 1024) >= ABS((HI_S16)u16lum_min - 1024)) {
        u16lum_a = u16lum_min;
        u16lum_b = u16lum_max;
    } else {
        u16lum_a = u16lum_max;
        u16lum_b = u16lum_min;
    }

    pstUsrRegCfg->s16CaLumaRatioLow = (u16weight * (u16lum_a - u16lum_b) + 1024 * u16lum_b) >> 10;

    u16lum_min = pstUsrRegCfg->au16YRatioLUT[pstCA->u16CaLumaThrLow / 8];
    u16lum_max = pstUsrRegCfg->au16YRatioLUT[pstCA->u16CaLumaThrLow / 8];

    u16lmin = pstUsrRegCfg->au16YRatioLUT[pstCA->u16CaLumaThrLow / 8];
    u16lmax = pstUsrRegCfg->au16YRatioLUT[pstCA->u16CaLumaThrLow / 8];

    for (i = pstCA->u16CaLumaThrLow / 8; i < pstCA->u16LumaThdHigh / 8; i = i + 2) {
        if (pstUsrRegCfg->au16YRatioLUT[i] >= pstUsrRegCfg->au16YRatioLUT[i + 1]) {
            u16lmin = pstUsrRegCfg->au16YRatioLUT[i + 1];
            u16lmax = pstUsrRegCfg->au16YRatioLUT[i];
        } else {
            u16lmax = pstUsrRegCfg->au16YRatioLUT[i + 1];
            u16lmin = pstUsrRegCfg->au16YRatioLUT[i];
        }

        if (u16lmax >= u16lum_max) {
            u16lum_max = u16lmax;
        }

        if (u16lmin <= u16lum_min) {
            u16lum_min = u16lmin;
        }
    }

    if (ABS((HI_S16)u16lum_max - 1024) >= ABS((HI_S16)u16lum_min - 1024)) {
        u16lum_a = u16lum_min;
        u16lum_b = u16lum_max;
    } else {
        u16lum_a = u16lum_max;
        u16lum_b = u16lum_min;
    }
    pstUsrRegCfg->s16CaLumaRatioHigh = (u16weight * (u16lum_a - u16lum_b) + 1024 * u16lum_b) >> 10;

    if ((pstCA->u16LumaThdHigh - pstCA->u16CaLumaThrLow) < 4) {
        pstUsrRegCfg->s16CaYLumaRatio = (pstUsrRegCfg->s16CaLumaRatioHigh - pstUsrRegCfg->s16CaLumaRatioLow) * 32 / 4;
    } else {
        pstUsrRegCfg->s16CaYLumaRatio = ((pstUsrRegCfg->s16CaLumaRatioHigh - pstUsrRegCfg->s16CaLumaRatioLow) * 32 / DIV_0_TO_1(pstCA->u16LumaThdHigh - pstCA->u16CaLumaThrLow));
    }

    if ((pstCA->u16CaSkinRatioThrHigh - pstCA->u16CaSkinRatioThrMid) < 4) {
        pstUsrRegCfg->s16CaSkinBetaRatio = (1024) * 32 / 4;
    } else {
        pstUsrRegCfg->s16CaSkinBetaRatio = ((1024) * 32 / DIV_0_TO_1(pstCA->u16CaSkinRatioThrHigh - pstCA->u16CaSkinRatioThrMid));
    }

    pstUsrRegCfg->bCaLutUpdateEn     = HI_TRUE;

    pstUsrRegCfg->bResh              = HI_TRUE;
    pstUsrRegCfg->u32UpdateIndex    += 1;

    return;
}

static HI_VOID Isp_Ca_Dyna_Fw(HI_S32 s32Iso, ISP_CA_DYNA_CFG_S *pstDynaRegCfg, ISP_CA_S *pstCA)
{
    HI_S32 s32IsoRatio;

    s32IsoRatio = CaGetValueFromLut(s32Iso, g_au32IsoLut, pstCA->as16CaIsoRatio, ISP_AUTO_ISO_STRENGTH_NUM);
    pstDynaRegCfg->u16CaISORatio = CLIP3(s32IsoRatio, 0, HI_ISP_CA_RATIO_MAX);
    pstCA->s16ProcInfoIsoRatio = pstDynaRegCfg->u16CaISORatio;
    pstDynaRegCfg->bResh         = HI_TRUE;
}

static HI_S32 ISP_CaInit(VI_PIPE ViPipe, HI_VOID *pRegCfg)
{
    HI_S32 s32Ret = HI_SUCCESS;
    ISP_REG_CFG_S *pstRegCfg = (ISP_REG_CFG_S *)pRegCfg;

    s32Ret = CaCtxInit(ViPipe);
    if (s32Ret != HI_SUCCESS) {
        return s32Ret;
    }

    s32Ret = CaInInitialize(ViPipe);
    if (s32Ret != HI_SUCCESS) {
        return s32Ret;
    }

    CaRegsInitialize(ViPipe, pstRegCfg);
    CaExtRegsInitialize(ViPipe);

    return HI_SUCCESS;
}

static HI_S32 ISP_CaRun(VI_PIPE ViPipe, const HI_VOID *pStatInfo,
                        HI_VOID *pRegCfg, HI_S32 s32Rsv)
{
    HI_U8  i;
    ISP_CA_S  *pstCA = HI_NULL;
    ISP_CTX_S *pstIspCtx = HI_NULL;
    ISP_REG_CFG_S *pstReg = (ISP_REG_CFG_S *)pRegCfg;

    ISP_GET_CTX(ViPipe, pstIspCtx);
    CA_GET_CTX(ViPipe, pstCA);
    ISP_CHECK_POINTER(pstCA);

    /* calculate every two interrupts */
    if ((pstIspCtx->u32FrameCnt % 2 != 0) && (pstIspCtx->stLinkage.bSnapState != HI_TRUE)) {
        return HI_SUCCESS;
    }

    pstCA->bCaEn = hi_ext_system_ca_en_read(ViPipe);

    for (i = 0; i < pstReg->u8CfgNum; i++) {
        pstReg->stAlgRegCfg[i].stCaRegCfg.bCaEn = pstCA->bCaEn;
    }

    pstReg->unKey.bit1CaCfg = 1;

    /* check hardware setting */
    if (!CheckCaOpen(pstCA)) {
        return HI_SUCCESS;
    }

    CaReadExtregs(ViPipe);

    if (pstCA->bCaCoefUpdateEn) {
        for (i = 0; i < pstReg->u8CfgNum; i++) {
            Isp_Ca_Usr_Fw(pstCA, &pstReg->stAlgRegCfg[i].stCaRegCfg.stUsrRegCfg);
        }
    }

    for (i = 0; i < pstReg->u8CfgNum; i++) {
        Isp_Ca_Dyna_Fw((HI_S32)pstIspCtx->stLinkage.u32Iso, &pstReg->stAlgRegCfg[i].stCaRegCfg.stDynaRegCfg, pstCA);
    }

    return HI_SUCCESS;
}

HI_S32 CaProcWrite(VI_PIPE ViPipe, ISP_CTRL_PROC_WRITE_S *pstProc)
{
    ISP_CTRL_PROC_WRITE_S stProcTmp;
    ISP_CA_S  *pstCa = HI_NULL;
    CA_GET_CTX(ViPipe, pstCa);
    ISP_CHECK_POINTER(pstCa);

    if ((pstProc->pcProcBuff == HI_NULL)
        || (pstProc->u32BuffLen == 0)) {
        return HI_FAILURE;
    }

    stProcTmp.pcProcBuff = pstProc->pcProcBuff;
    stProcTmp.u32BuffLen = pstProc->u32BuffLen;

    ISP_PROC_PRINTF(&stProcTmp, pstProc->u32WriteLen,
                    "-----CA INFO-----------------------------------\n");

    ISP_PROC_PRINTF(&stProcTmp, pstProc->u32WriteLen,
                    "%12s" "%12s\n",
                    "Enable", "isoRatio");

    ISP_PROC_PRINTF(&stProcTmp, pstProc->u32WriteLen,
                    "%12u" "%12u\n",
                    pstCa->bCaEn, pstCa->s16ProcInfoIsoRatio);


    pstProc->u32WriteLen += 1;

    return HI_SUCCESS;
}


static HI_S32 ISP_CaCtrl(VI_PIPE ViPipe, HI_U32 u32Cmd, HI_VOID *pValue)
{
    switch (u32Cmd) {
        case ISP_PROC_WRITE:
            CaProcWrite(ViPipe, (ISP_CTRL_PROC_WRITE_S *)pValue);
            break;
        default :
            break;
    }
    return HI_SUCCESS;
}

static HI_S32 ISP_CaExit(VI_PIPE ViPipe)
{
    HI_U8 i;
    ISP_REGCFG_S  *pRegCfg = HI_NULL;

    ISP_REGCFG_GET_CTX(ViPipe, pRegCfg);

    for (i = 0; i < pRegCfg->stRegCfg.u8CfgNum; i++) {
        pRegCfg->stRegCfg.stAlgRegCfg[i].stCaRegCfg.bCaEn = HI_FALSE;
    }

    pRegCfg->stRegCfg.unKey.bit1CaCfg = 1;

    CaCtxExit(ViPipe);

    return HI_SUCCESS;
}

HI_S32 ISP_AlgRegisterCa(VI_PIPE ViPipe)
{
    ISP_CTX_S *pstIspCtx = HI_NULL;
    ISP_ALG_NODE_S *pstAlgs = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);
    ISP_ALG_CHECK(pstIspCtx->unAlgKey.bit1Ca);
    pstAlgs = ISP_SearchAlg(pstIspCtx->astAlgs);
    ISP_CHECK_POINTER(pstAlgs);

    pstAlgs->enAlgType = ISP_ALG_CA;
    pstAlgs->stAlgFunc.pfn_alg_init = ISP_CaInit;
    pstAlgs->stAlgFunc.pfn_alg_run  = ISP_CaRun;
    pstAlgs->stAlgFunc.pfn_alg_ctrl = ISP_CaCtrl;
    pstAlgs->stAlgFunc.pfn_alg_exit = ISP_CaExit;
    pstAlgs->bUsed = HI_TRUE;

    return HI_SUCCESS;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
