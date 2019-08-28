/*
* Copyright (c) 2018 HiSilicon Technologies Co., Ltd.
*
* This program is free software; you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the
* Free Software Foundation; either version 2 of the License, or (at your
* option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*
*/

#ifndef __HIFB_HAL_H__
#define __HIFB_HAL_H__

#include "hifb.h"
#include "hifb_reg.h"
#include "hifb_def.h"
#include "hifb_coef_org.h"
#include "hi_comm_vo.h"


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

HI_VOID HIFB_HAL_VOU_Init(HI_VOID);
HI_VOID HIFB_HAL_VOU_Exit(HI_VOID);
HI_VOID HIFB_HAL_WriteReg(HI_U32* pAddress, HI_U32 Value);
HI_U32 HIFB_HAL_ReadReg(HI_U32* pAddress);

/*****************************************************************************
 Prototype       : sys Relative
*****************************************************************************/
HI_VOID HAL_SYS_SetArbMode(HI_U32 bMode);
HI_VOID HAL_SYS_VdpResetClk(HI_U32 sel);

/*****************************************************************************
 Prototype       : device Relative
 Description     : device Relative
*****************************************************************************/
HI_BOOL HIFB_HAL_DISP_GetIntfEnable(HAL_DISP_OUTPUTCHANNEL_E enChan, HI_BOOL* pbIntfEn);

HI_BOOL HIFB_HAL_DISP_GetIntState(HAL_DISP_OUTPUTCHANNEL_E enChan, HI_BOOL* pbBottom);
HI_BOOL HIFB_HAL_DISP_GetDispIoP(HAL_DISP_OUTPUTCHANNEL_E enChan, HI_BOOL* pbIop);
HI_BOOL HIFB_HAL_DISP_SetBt1120Sel(HAL_DISP_OUTPUTCHANNEL_E enChan);
HI_BOOL HIFB_HAL_DISP_SetIntfClipEnable(HAL_DISP_INTF_E enIntf, HI_BOOL enClip);
HI_BOOL HIFB_HAL_DISP_GetVtThdMode(HAL_DISP_OUTPUTCHANNEL_E enChan, HI_BOOL* pbFieldMode);
HI_BOOL HIFB_HAL_DISP_SetIntMask(HI_U32 u32MaskEn);
HI_BOOL HIFB_HAL_DISP_ClrIntMask(HI_U32 u32MaskEn);
HI_U32  HIFB_HAL_DISP_GetIntStatus(HI_U32 u32IntMsk);
HI_BOOL HIFB_HAL_DISP_ClearIntStatus(HI_U32 u32IntMsk);
HI_U32  HIFB_HAL_DISP_GetRawIntStatus(HI_U32 u32IntMsk);
HI_U32  HIFB_HAL_DISP_GetRawIntStatus1(HI_U32 u32IntMsk);

HI_BOOL HIFB_HAL_DISP_SetClkGateEnable(HI_U32 u32Data);
HI_VOID HIFB_HAL_DISP_DATE_OutCtrl(HAL_DISP_OUTPUTCHANNEL_E enChan, HI_U32 u32OutCtrl);
HI_BOOL HIFB_HAL_DISP_SetDateCoeff(HAL_DISP_OUTPUTCHANNEL_E enChan, HI_U32 u32Data);
HI_VOID HIFB_HAL_DISP_SetDateCoeffByIdx(HI_U32 u32Idx, HI_U32 u32Data);
HI_VOID HIFB_HAL_DISP_SetRegUp (HAL_DISP_OUTPUTCHANNEL_E enChan);
HI_U32  HIFB_HAL_DISP_GetRegUp (HAL_DISP_OUTPUTCHANNEL_E enChan);
HI_BOOL HIFB_HAL_DISP_GetIntfSync(HAL_DISP_OUTPUTCHANNEL_E enChan,HAL_DISP_SYNCINFO_S *pstSyncInfo);
HI_BOOL HIFB_HAL_DISP_GetIntfMuxSel(HAL_DISP_OUTPUTCHANNEL_E enChan,VO_INTF_TYPE_E *pbenIntfType);

/*****************************************************************************
 Prototype       : video layer Relative
 Description     : video layer Relative
*****************************************************************************/

HI_BOOL HIFB_HAL_VIDEO_SetLayerDispRect(HAL_DISP_LAYER_E enLayer, HIFB_RECT* pstRect);
HI_BOOL HIFB_HAL_VIDEO_SetLayerVideoRect(HAL_DISP_LAYER_E enLayer, HIFB_RECT* pstRect);
HI_BOOL HAL_VIDEO_SetMultiAreaLAddr  (HAL_DISP_LAYER_E enLayer, HI_U32 area_num,
                                      HI_UL u32LAddr, HI_U16 stride);
HI_BOOL HAL_VIDEO_SetLayerRimWidth(HAL_DISP_LAYER_E enLayer, HI_U32 u32RimWidth);
HI_BOOL HAL_VIDEO_SetLayerRimCol0(HAL_DISP_LAYER_E enLayer, VDP_BKG_S  *pstRimCol0);
HI_BOOL HAL_VIDEO_SetLayerRimCol1(HAL_DISP_LAYER_E enLayer, VDP_BKG_S  *pstRimCol1);
HI_BOOL HAL_VIDEO_SetMultiAreaCAddr  (HAL_DISP_LAYER_E enLayer, HI_U32 area_num,
                                      HI_UL u32CAddr, HI_U16 stride);
HI_BOOL HAL_VIDEO_GetMultiAreaLAddr  (HAL_DISP_LAYER_E enLayer, HI_U32 area_num,
                                      HI_U32 *pu32LAddrLow, HI_U32 *pu32LAddrHigh);
HI_BOOL HAL_VIDEO_GetMultiAreaCAddr  (HAL_DISP_LAYER_E enLayer, HI_U32 area_num,
                                      HI_U32 *pu32CAddrLow, HI_U32 *pu32CAddrHigh);


HI_BOOL HAL_VIDEO_SetMultiAreaVDcmpOffset(HAL_DISP_LAYER_E enLayer, HI_U32 area_num,
        HI_U64 u64LHOffset, HI_U64 u64CHOffset);

HI_BOOL HAL_VIDEO_SetMultiAreaVDcmpEnable(HAL_DISP_LAYER_E enLayer, HI_U32 bEnable);
HI_BOOL HAL_VIDEO_SetMultiAreaReso(HAL_DISP_LAYER_E enLayer,HI_U32 u32AreaNum,
                                              HI_U32 u32Width);
HI_BOOL HAL_VIDEO_SetMultiAreaRect(HAL_DISP_LAYER_E enLayer,HI_U32 u32AreaNum,RECT_S *pstVideoAreaRect);
HI_BOOL HAL_VIDEO_GetMRGState(HAL_DISP_LAYER_E enLayer, HI_U32 *pu32Mrg);
HI_BOOL HAL_VIDEO_CLrMRGState(HAL_DISP_LAYER_E enLayer);
HI_BOOL HAL_VIDEO_GetVDcmpLumaState(HAL_DISP_LAYER_E enLayer, HI_U32* pu32DcmpLumaStat);
HI_BOOL HAL_VIDEO_GetVDcmpChromaState(HAL_DISP_LAYER_E enLayer, HI_U32* pu32DcmpChromaStat);
HI_BOOL HAL_VIDEO_ClrVDcmpLumaState(HAL_DISP_LAYER_E enLayer);
HI_BOOL HAL_VIDEO_ClrVDcmpChromaState(HAL_DISP_LAYER_E enLayer);

HI_BOOL HAL_VIDEO_SetAllAreaDisable  (HAL_DISP_LAYER_E enLayer);

/* Video layer CVFIR relative hal functions */
HI_VOID HAL_VIDEO_CVFIR_SetOutHeight(HAL_DISP_LAYER_E enLayer, HI_U32 out_height);
HI_VOID HAL_VIDEO_CVFIR_SetOutFmt(HAL_DISP_LAYER_E enLayer, HI_U32 out_fmt);
HI_VOID HAL_VIDEO_CVFIR_SetOutPro(HAL_DISP_LAYER_E enLayer, HI_U32 out_pro);
HI_VOID HAL_VIDEO_CVFIR_SetVzmeCkGtEn(HAL_DISP_LAYER_E enLayer, HI_BOOL vzme_ck_gt_en);

HI_VOID HAL_VIDEO_CVFIR_SetCvfirEn(HAL_DISP_LAYER_E enLayer, HI_U32 cvfir_en);
HI_VOID HAL_VIDEO_CVFIR_SetCvmidEn(HAL_DISP_LAYER_E enLayer, HI_U32 cvmid_en);
HI_VOID HAL_VIDEO_CVFIR_SetCvfirMode(HAL_DISP_LAYER_E enLayer, HI_U32 cvfir_mode);
HI_VOID HAL_VIDEO_CVFIR_SetVratio(HAL_DISP_LAYER_E enLayer, HI_U32 vratio);

HI_VOID HAL_VIDEO_CVFIR_SetVChromaOffset(HAL_DISP_LAYER_E enLayer, HI_U32 vchroma_offset);
HI_VOID HAL_VIDEO_CVFIR_SetVbChromaOffset(HAL_DISP_LAYER_E enLayer, HI_U32 vbchroma_offset);

HI_VOID HAL_VIDEO_CVFIR_SetCoef(HAL_DISP_LAYER_E enLayer, CvfirCoef_S *pstCvfirCoef);

/* Video layer HFIR relative hal functions */
HI_VOID HAL_VIDEO_HFIR_SetCkGtEn(HAL_DISP_LAYER_E enLayer, HI_U32 ck_gt_en);
HI_VOID HAL_VIDEO_HFIR_SetMidEn(HAL_DISP_LAYER_E enLayer, HI_U32 mid_en);
HI_VOID HAL_VIDEO_HFIR_SetHfirMode(HAL_DISP_LAYER_E enLayer, HI_U32 hfir_mode);
HI_VOID HAL_VIDEO_HFIR_SetHfirEn(HAL_DISP_LAYER_E enLayer, HI_U32 hfir_en);
HI_VOID HAL_VIDEO_HFIR_SetCoef(HAL_DISP_LAYER_E enLayer, HfirCoef_S *pstHfirCoef);

/*****************************************************************************
 Prototype       : layer Relative
 Description     : layer Relative

*****************************************************************************/
/* Video layer CSC relative hal functions. */
HI_VOID HIFB_HAL_LAYER_CSC_SetEnable(HAL_DISP_LAYER_E enLayer, HI_BOOL csc_en);
HI_VOID HIFB_HAL_LAYER_CSC_SetCkGtEn(HAL_DISP_LAYER_E enLayer, HI_BOOL ck_gt_en);
HI_VOID HIFB_HAL_LAYER_CSC_SetCoef(HAL_DISP_LAYER_E enLayer, VDP_CSC_COEF_S *pstCscCoef);
HI_VOID HIFB_HAL_LAYER_CSC_SetDcCoef(HAL_DISP_LAYER_E enLayer, VDP_CSC_DC_COEF_S *pstCscDcCoef);


HI_BOOL HIFB_HAL_LAYER_GetCoefAddr(HAL_DISP_LAYER_E enLayer,
                              HAL_DISP_COEFMODE_E enMode,
                              HI_U32* pu32Addr);
HI_BOOL HIFB_HAL_LAYER_SetCoefAddr(HAL_DISP_LAYER_E enLayer,
                              HAL_DISP_COEFMODE_E enMode,
                              HI_U32 u32Addr);
HI_BOOL HIFB_HAL_LAYER_SetLayerParaUpd(HAL_DISP_LAYER_E enLayer,
                                  HAL_DISP_COEFMODE_E enMode);
HI_BOOL HIFB_HAL_LAYER_SetDitherMode(HAL_DISP_LAYER_E enLayer);
HI_BOOL HIFB_HAL_LAYER_SetDitherSeed(HAL_DISP_LAYER_E enLayer, HI_S32* s32Seed);

HI_BOOL HIFB_HAL_LAYER_SetLinkCtrl(HAL_DISP_LAYER_E enLayer, HI_U8 u8CbmId);

HI_BOOL HIFB_HAL_LAYER_EnableLayer(HAL_DISP_LAYER_E enLayer, HI_U32 bEnable);
HI_BOOL HIFB_HAL_LAYER_GetLayerEnable(HAL_DISP_LAYER_E enLayer, HI_U32* pu32Enable);
HI_BOOL HIFB_HAL_LAYER_SetLayerDataFmt(HAL_DISP_LAYER_E enLayer,
                                  HAL_DISP_PIXEL_FORMAT_E enDataFmt);
HI_BOOL HIFB_HAL_LAYER_GetLayerDataFmt(HAL_DISP_LAYER_E enLayer, HI_U32* pu32Fmt);
HI_BOOL HIFB_HAL_LAYER_SetLayerBitWidth(HAL_DISP_LAYER_E enLayer,
                                   HAL_DISP_BIT_WIDTH_E  enBitWide);
HI_BOOL HIFB_HAL_LAYER_SetCscCoef(HAL_DISP_LAYER_E enLayer, CscCoef_S* pstCscCoef);
HI_BOOL HIFB_HAL_LAYER_SetCscMode(HAL_DISP_LAYER_E enLayer, HI_BOOL bIsHCModeBy709);
HI_BOOL HIFB_HAL_LAYER_SetCscEn(HAL_DISP_LAYER_E enLayer, HI_BOOL bCscEn);

HI_BOOL HIFB_HAL_LAYER_SetSrcResolution(HAL_DISP_LAYER_E enLayer, HIFB_RECT *  pstRect);
HI_BOOL HIFB_HAL_LAYER_SetLayerInRect(HAL_DISP_LAYER_E enLayer, HIFB_RECT *pstRect);
HI_BOOL HIFB_HAL_LAYER_SetLayerGAlpha(HAL_DISP_LAYER_E enLayer,
                                 HI_U8 u8Alpha0);
HI_BOOL HIFB_HAL_LAYER_GetLayerGAlpha(HAL_DISP_LAYER_E enLayer, HI_U8* pu8Alpha0);
HI_BOOL HIFB_HAL_LAYER_SetRegUp(HAL_DISP_LAYER_E enLayer);
HI_BOOL HIFB_HAL_LAYER_SetLayerBgColor(HAL_DISP_LAYER_E enLayer, HAL_VIDEO_LAYER_BKCOLOR_S *stBgColor);
HI_BOOL HIFB_HAL_DISP_SetLayerBoundaryColor(HAL_DISP_LAYER_E enLayer, HAL_DISP_BOUNDARY_COLOR_S *stBoundaryColor);
HI_BOOL HIFB_HAL_DISP_SetLayerBoundaryWidth(HAL_DISP_LAYER_E enLayer, HI_U32 u32Width);
HI_BOOL HIFB_HAL_DISP_SetChnBoundary(HAL_DISP_LAYER_E enLayer, HI_U32 u32AreaNum, const VO_CHN_BOUNDARY_S *pstChnBoundary);


/*****************************************************************************
 Prototype       : graphic layer Relative
 Description     : graphic layer Relative

*****************************************************************************/
HI_BOOL HIFB_HAL_GRAPHIC_SetGfxAddr(HAL_DISP_LAYER_E enLayer, HI_U64 u64LAddr);
HI_BOOL HIFB_HAL_GRAPHIC_GetGfxAddr(HAL_DISP_LAYER_E enLayer, HI_U64* pu64GfxAddr);
HI_BOOL HIFB_HAL_GRAPHIC_SetGfxStride(HAL_DISP_LAYER_E enLayer, HI_U16 u16pitch);
HI_BOOL HIFB_HAL_GRAPHIC_GetGfxStride(HAL_DISP_LAYER_E enLayer, HI_U32* pu32GfxStride);
HI_BOOL HIFB_HAL_GRAPHIC_SetGfxExt(HAL_DISP_LAYER_E enLayer,
                              HAL_GFX_BITEXTEND_E enMode);
HI_BOOL HIFB_HAL_GRAPHIC_SetGfxPreMult(HAL_DISP_LAYER_E enLayer, HI_U32 bEnable);
HI_BOOL HIFB_HAL_GRAPHIC_SetGfxPalpha(HAL_DISP_LAYER_E enLayer,
                                 HI_U32 bAlphaEn, HI_U32 bArange,
                                 HI_U8 u8Alpha0, HI_U8 u8Alpha1);
HI_BOOL HIFB_HAL_GRAPHIC_GetGfxPalpha(HAL_DISP_LAYER_E enLayer, HI_U32* pbAlphaEn,
                                 HI_U8* pu8Alpha0, HI_U8* pu8Alpha1);
HI_BOOL HIFB_HAL_GRAPHIC_SetGfxPalphaRange(HAL_DISP_LAYER_E enLayer, HI_U32 bArange);

HI_BOOL HIFB_HAL_GRAPHIC_SetGfxKeyEn(HAL_DISP_LAYER_E enLayer, HI_U32 u32KeyEnable);
HI_BOOL HIFB_HAL_GRAPHIC_SetGfxKeyMode(HAL_DISP_LAYER_E enLayer, HI_U32 u32KeyOut);
HI_BOOL HIFB_HAL_GRAPHIC_SetColorKeyValue(HAL_DISP_LAYER_E enLayer,
                                     HAL_GFX_KEY_MAX_S stKeyMax, HAL_GFX_KEY_MIN_S stKeyMin);
HI_BOOL HIFB_HAL_GRAPHIC_SetColorKeyMask(HAL_DISP_LAYER_E enLayer, HAL_GFX_MASK_S stMsk);

HI_BOOL HIFB_HAL_GRAPHIC_GetGfxPreMult(HAL_DISP_LAYER_E enLayer, HI_U32* pbEnable);

/***************************************************************************************************
*  Begin : Parameter Address distribute
***************************************************************************************************/
HI_VOID HAL_PARA_SetParaAddrVhdChn00(HI_U64 para_addr_vhd_chn00);
HI_VOID HAL_PARA_SetParaAddrVhdChn01(HI_U64 para_addr_vhd_chn01);
HI_VOID HAL_PARA_SetParaAddrVhdChn02(HI_U64 para_addr_vhd_chn02);
HI_VOID HAL_PARA_SetParaAddrVhdChn03(HI_U64 para_addr_vhd_chn03);
HI_VOID HAL_PARA_SetParaAddrVhdChn04(HI_U64 para_addr_vhd_chn04);
HI_VOID HAL_PARA_SetParaAddrVhdChn05(HI_U64 para_addr_vhd_chn05);
HI_VOID HAL_PARA_SetParaAddrVhdChn06(HI_U64 para_addr_vhd_chn06);
HI_VOID HAL_PARA_SetParaAddrVhdChn07(HI_U64 para_addr_vhd_chn07);
HI_VOID HAL_PARA_SetParaAddrVhdChn08(HI_U64 para_addr_vhd_chn08);
HI_VOID HAL_PARA_SetParaAddrVhdChn09(HI_U64 para_addr_vhd_chn09);
HI_VOID HAL_PARA_SetParaAddrVhdChn10(HI_U64 para_addr_vhd_chn10);
HI_VOID HAL_PARA_SetParaAddrVhdChn11(HI_U64 para_addr_vhd_chn11);
HI_VOID HAL_PARA_SetParaAddrVhdChn12(HI_U64 para_addr_vhd_chn12);
HI_VOID HAL_PARA_SetParaAddrVhdChn13(HI_U64 para_addr_vhd_chn13);
HI_VOID HAL_PARA_SetParaAddrVhdChn14(HI_U32 para_addr_vhd_chn14);
HI_VOID HAL_PARA_SetParaAddrVhdChn15(HI_U32 para_addr_vhd_chn15);
HI_VOID HAL_PARA_SetParaAddrVhdChn16(HI_U32 para_addr_vhd_chn16);
HI_VOID HAL_PARA_SetParaAddrVhdChn17(HI_U32 para_addr_vhd_chn17);
HI_VOID HAL_PARA_SetParaAddrVhdChn18(HI_U32 para_addr_vhd_chn18);
HI_VOID HAL_PARA_SetParaAddrVhdChn19(HI_U32 para_addr_vhd_chn19);
HI_VOID HAL_PARA_SetParaAddrVhdChn20(HI_U32 para_addr_vhd_chn20);
HI_VOID HAL_PARA_SetParaAddrVhdChn21(HI_U32 para_addr_vhd_chn21);
HI_VOID HAL_PARA_SetParaAddrVhdChn22(HI_U32 para_addr_vhd_chn22);
HI_VOID HAL_PARA_SetParaAddrVhdChn23(HI_U32 para_addr_vhd_chn23);
HI_VOID HAL_PARA_SetParaAddrVhdChn24(HI_U32 para_addr_vhd_chn24);
HI_VOID HAL_PARA_SetParaAddrVhdChn25(HI_U32 para_addr_vhd_chn25);
HI_VOID HAL_PARA_SetParaAddrVhdChn26(HI_U32 para_addr_vhd_chn26);
HI_VOID HAL_PARA_SetParaAddrVhdChn27(HI_U32 para_addr_vhd_chn27);
HI_VOID HAL_PARA_SetParaAddrVhdChn28(HI_U32 para_addr_vhd_chn28);
HI_VOID HAL_PARA_SetParaAddrVhdChn29(HI_U32 para_addr_vhd_chn29);
HI_VOID HAL_PARA_SetParaAddrVhdChn30(HI_U32 para_addr_vhd_chn30);
HI_VOID HAL_PARA_SetParaAddrVhdChn31(HI_U32 para_addr_vhd_chn31);

HI_VOID HAL_PARA_SetParaUpVhdChn(HI_U32 u32ChnNum);
HI_VOID HAL_PARA_SetParaUpVhdChnAll(void);

HI_VOID HAL_PARA_SetParaAddrVsdChn00(HI_U32 para_addr_vsd_chn00);
HI_VOID HAL_PARA_SetParaAddrVsdChn01(HI_U32 para_addr_vsd_chn01);
HI_VOID HAL_PARA_SetParaAddrVsdChn02(HI_U32 para_addr_vsd_chn02);
HI_VOID HAL_PARA_SetParaAddrVsdChn03(HI_U32 para_addr_vsd_chn03);
HI_VOID HAL_PARA_SetParaAddrVsdChn04(HI_U32 para_addr_vsd_chn04);
HI_VOID HAL_PARA_SetParaAddrVsdChn05(HI_U32 para_addr_vsd_chn05);
HI_VOID HAL_PARA_SetParaAddrVsdChn06(HI_U32 para_addr_vsd_chn06);
HI_VOID HAL_PARA_SetParaAddrVsdChn07(HI_U32 para_addr_vsd_chn07);

HI_VOID HAL_PARA_SetParaUpVsdChn(HI_U32 u32ChnNum);
HI_VOID HAL_PARA_SetParaUpVsdChnAll(void);
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of __VOU_HAL_H__ */