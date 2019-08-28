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

#ifndef __HIFB_VOU_GRAPHICS_H__
#define __HIFB_VOU_GRAPHICS_H__

#include "hi_type.h"
#include "hifb.h"
#include "hifb_vou_drv.h"
#include "hifb_graphic_hal.h"
#include "hifb_graphics_drv.h"


#define MDDRC_ZONE_MAX_NUM          32
#define HIFB_LINE_BUF          1920
#define HIFB_MAX_LAYER_NUM 1

typedef enum hiVOU_BITEXT_MODE_E
{
    VOU_BITEXT_LOW_ZERO         = 0x0,
    VOU_BITEXT_LOW_HIGHBIT      = 0x2,
    VOU_BITEXT_LOW_HIGHBITS     = 0x3,
    VOU_BITEXT_BUTT
} VOU_BITEXT_MODE_E;


typedef enum hiVOU_COLORKEY_MODE_E
{
    VOU_COLORKEY_IN     = 0x0,
    VOU_COLORKEY_OUT    = 0x1,
    VOU_COLORKEY_BUTT
} VOU_COLORKEY_MODE_E;
typedef struct tagVO_GFX_KEY_MAX_S
{
    HI_U8 u8KeyMax_R;
    HI_U8 u8KeyMax_G;
    HI_U8 u8KeyMax_B;

}VO_GFX_KEY_MAX_S;

typedef struct tagVO_GFX_KEY_MIN_S
{
    HI_U8 u8KeyMin_R;
    HI_U8 u8KeyMin_G;
    HI_U8 u8KeyMin_B;

} VO_GFX_KEY_MIN_S;

typedef struct tagVO_GFX_MASK_S
{
    HI_U8 u8Mask_r;
    HI_U8 u8Mask_g;
    HI_U8 u8Mask_b;

} VO_GFX_MASK_S;

typedef enum hiVOU_SCAN_MODE_E
{
    VOU_SCAN_MODE_INTERLACE = 0x0,
    VOU_SCAN_MODE_PROGRESSIVE = 0x1,
    VOU_SCAN_MODE_BUTT
}VOU_SCAN_MODE_E;

typedef enum
{
    VO_GFX_BITEXTEND_1ST =   0,
    VO_GFX_BITEXTEND_2ND = 0x2,
    VO_GFX_BITEXTEND_3RD = 0x3,

    VO_GFX_BITEXTEND_BUTT
}VO_GFX_BITEXTEND_E;
typedef HI_S32 (*VO_FB_IntCallBack)(const HI_VOID *pParaml, HI_VOID *pParamr);
//typedef struct hiVO_GRAPHIC_LAYER_

HI_VOID HIFB_GraphicsEnableWbc(HAL_DISP_LAYER_E gfxLayer, HI_BOOL bEnable);
HI_VOID HIFB_GraphicsSetMDDRDetectZone(HI_U32 u32StartID, HI_U32 u32Cnt, VO_MDDRC_ZONE_ADDR_S *pstZoneAddr);
HI_VOID HIFB_GraphicsGetMDDRStatus(HI_U32 u32StartID, HI_U32 u32Cnt, HI_U32 *pu32Status);
HI_VOID HIFB_GraphicsClearDDRDectectZone(HI_U32 u32StartID, HI_U32 u32Cnt);
HI_S32 HIFB_GraphicsCheckDDRDectectZone(GRAPHIC_LAYER gfxLayer, HI_U32 u32NewStartID, HI_U32 u32NewCnt, HI_U32 u32PreStartID, HI_U32 u32PreCnt);
HI_VOID HIFB_GraphicsGetDDRZoneCfg(GRAPHIC_LAYER gfxLayer, HI_U32 *pu32StartID, HI_U32 *pu32Cnt);
HI_VOID HIFB_GraphicsInitMDDRDetect(HI_VOID);

HI_BOOL  HIFB_GraphicsSetGfxExt(GRAPHIC_LAYER gfxLayer, VO_GFX_BITEXTEND_E enMode);
HI_BOOL HIFB_GraphicsSetGfxPalpha(GRAPHIC_LAYER gfxLayer, HI_U32 bAlphaEn,HI_U32 bArange,HI_U8 u8Alpha0,HI_U8 u8Alpha1);
HI_BOOL HIFB_GraphicsSetLayerGalpha(GRAPHIC_LAYER gfxLayer, HI_U8 u8Alpha0);
HI_BOOL HIFB_GraphicsSetCscEn(GRAPHIC_LAYER gfxLayer, HI_BOOL bCscEn);
HI_BOOL HIFB_GraphicsSetGfxAddr(GRAPHIC_LAYER gfxLayer, HI_U64 u64LAddr);
HI_BOOL HIFB_GraphicsSetGfxStride(GRAPHIC_LAYER gfxLayer, HI_U16 u16pitch);

HI_BOOL HIFB_GraphicsGetGfxPreMult(GRAPHIC_LAYER gfxLayer, HI_U32 *pbEnable);
HI_BOOL HIFB_GraphicsSetGfxPreMult(GRAPHIC_LAYER gfxLayer, HI_U32 bEnable);
HI_BOOL HIFB_GraphicsSetLayerDataFmt(GRAPHIC_LAYER gfxLayer, VO_DISP_PIXEL_FORMAT_E  enDataFmt);
HI_BOOL HIFB_GraphicsSetLayerInRect(GRAPHIC_LAYER gfxLayer, HIFB_RECT *pstRect);
HI_BOOL HIFB_GraphicsSetLayerSrcImageReso(GRAPHIC_LAYER gfxLayer, HIFB_RECT *pstRect);
HI_BOOL HIFB_GraphicsSetLayerOutRect(GRAPHIC_LAYER gfxLayer, HIFB_RECT *pstRect);
HI_BOOL HIFB_GraphicsSetColorKeyValue(GRAPHIC_LAYER gfxLayer, VO_GFX_KEY_MAX_S stKeyMax, VO_GFX_KEY_MIN_S stKeyMin);
HI_BOOL HIFB_GraphicsSetColorKeyMask(GRAPHIC_LAYER gfxLayer, VO_GFX_MASK_S stMsk);
HI_BOOL HIFB_DRV_GetDevEnable(VO_DEV VoDev, HI_BOOL *pbIntfEn);
HI_BOOL HIFB_DRV_GetIntfSync(VO_DEV VoDev,HAL_DISP_SYNCINFO_S *pstSyncInfo);
HI_BOOL HIFB_DRV_GetIntfMuxSel(VO_DEV VoDev,VO_INTF_TYPE_E *pbenIntfType);
HI_BOOL HIFB_GraphicsSetGfxKeyEn(GRAPHIC_LAYER gfxLayer, HI_U32 u32KeyEnable);
HI_BOOL HIFB_GraphicsSetGfxKeyMode(GRAPHIC_LAYER gfxLayer, HI_U32 u32KeyOut);

HI_BOOL  HIFB_GraphicsSetRegUp(GRAPHIC_LAYER gfxLayer);
HI_BOOL HIFB_GraphicsGetLayerGalpha(GRAPHIC_LAYER gfxLayer,HI_U8 *pu8Alpha0);
HI_BOOL HIFB_GraphicsGetLayerDataFmt(GRAPHIC_LAYER gfxLayer,HI_U32 *pu32Fmt);
HI_BOOL HIFB_GraphicsGetGfxStride(GRAPHIC_LAYER gfxLayer, HI_U32 *pu32GfxStride);
HI_BOOL HIFB_GraphicsGetGfxAddr(GRAPHIC_LAYER gfxLayer, HI_U64 *pu64GfxAddr);

HI_S32 HIFB_GraphicsGetDevMode(HAL_DISP_LAYER_E u32Layer, VOU_SCAN_MODE_E *pScanMode, HI_BOOL *pbFeildUpdate);

HI_VOID HIFB_GraphicsVtthIntProcess(VO_DEV VoDev);
HI_S32 HIFB_GraphicsGetCSC(HI_U32 u32Layer, VO_CSC_S * pstCsc);

HI_S32 HIFB_GraphicsInit(HI_VOID);
HI_S32 HIFB_GraphicsDeInit(HI_VOID);
HI_S32 HIFB_GraphicsResourceInit(HI_VOID);
HI_S32 HIFB_GraphicsResourceDeInit(HI_VOID);

HI_S32 HIFB_GraphicsOpenLayer(GRAPHIC_LAYER  gfxLayer);
HI_S32 HIFB_GraphicsCloseLayer(GRAPHIC_LAYER gfxLayer);
HI_S32 HIFB_GraphicsEnableLayer(GRAPHIC_LAYER gfxLayer, HI_BOOL bEnable);
HI_S32 HIFB_GraphicsSetCallback(GRAPHIC_LAYER gfxLayer, VO_FB_INT_TYPE_E enIntType, VO_FB_IntCallBack pCallBack, HI_VOID *pArg);
HI_S32 HIFB_GraphicsGetIntfSize(GRAPHIC_LAYER gfxLayer, HI_U32 *pu32Width, HI_U32 *pu32Height);
HI_S32 HIFB_GraphicsGetIntfType(GRAPHIC_LAYER gfxLayer, VO_INTF_TYPE_E *penIntfType);


HI_S32 HIFB_GraphicsSetCscCoef(GRAPHIC_LAYER gfxLayer);

HI_S32  HIFB_GraphicsShowProc(osal_proc_entry_t *s);
HI_S32  HIFB_GraphicsEnableInt(HI_U32 u32LayerIndex, HI_BOOL bEnable);
HI_BOOL HIFB_GraphicsClearInt(HI_U32 u32IntClear, HI_S32 s32Irq);
HI_BOOL HIFB_GraphicsGetInt(HI_U32 * pu32IntStaus);
HI_BOOL HIFB_GraphicsClearIntStatus(HI_U32 u32IntStatus);
HI_S32 HIFB_GraphicsGetInterruptDev(HI_U32 IntStatus, VO_DEV* pVoDev);
typedef enum hiVO_DITHER_OUT_BITWIDTH_E
{
    DITHER_OUT_BITWIDTH_8  = 0x0,    /* dither output 8bit */
    DITHER_OUT_BITWIDTH_10 = 0x1,    /* dither output 10bit */

    DITHER_OUT_BITWIDTH_BUTT
}VO_DITHER_OUT_BITWIDTH_E;

typedef enum hiVO_SCAN_MODE_E
{
    VO_SCAN_MODE_INTERLACE = 0x0,
    VO_SCAN_MODE_PROGRESSIVE = 0x1,
    VO_SCAN_MODE_BUTT
}VO_SCAN_MODE_E;

typedef HI_S32 (*VO_FB_IntCallBack)(const HI_VOID *pParaml, HI_VOID *pParamr);

/* initial and exit service of VO module,called by SYS module */
typedef HI_S32  FN_VOU_Init(HI_VOID);
typedef HI_VOID FN_VOU_Exit(HI_VOID);
/* send pic to VO ,called by internal modules */
typedef HI_S32 FN_VOU_ChnSendPic(VO_DEV VoDev, VO_CHN VoChn, VIDEO_FRAME_INFO_S *pVFrame, HI_VOID *pAppendArg);
/* clear buffer of VO channle */
typedef HI_S32 FN_VOU_ClearChnBuf(VO_DEV VoDev, VO_CHN VoChn, HI_BOOL bClear);
/* set vou channel buffer deep */
typedef HI_S32 FN_VOU_ChnSetBufLen(VO_DEV VoDev, VO_CHN VoChn, HI_U32 u32BufLen);
typedef HI_BOOL FN_VOU_CheckHdmiEn(HI_VOID);
/* set vo dither output bitwidth */
typedef HI_S32 FN_VOU_SetDitherOutBitWidth(MOD_ID_E enModId, VO_DEV VoDev, VO_DITHER_OUT_BITWIDTH_E enOutBitWidth);


typedef HI_S32 FN_VOU_GraphicsGetDevMode(GRAPHIC_LAYER gfxLayer, VO_SCAN_MODE_E *pScanMode, HI_BOOL *pbFeildUpdate);
typedef HI_S32 FN_VOU_GraphicsOpenLayer(GRAPHIC_LAYER gfxLayer);
typedef HI_S32 FN_VOU_GraphicsCloseLayer(GRAPHIC_LAYER gfxLayer);
typedef HI_S32 FN_VOU_GraphicsEnableLayer(GRAPHIC_LAYER gfxLayer,  HI_BOOL bEnable);
typedef HI_S32 FN_VOU_GraphicsSetCallback(GRAPHIC_LAYER gfxLayer, VO_FB_INT_TYPE_E enIntType, VO_FB_IntCallBack pCallBack, HI_VOID *pArg);
typedef HI_S32 FN_VOU_GraphicsGetIntfSize(GRAPHIC_LAYER gfxLayer, HI_U32 *pu32Width, HI_U32 *pu32Height);
typedef HI_S32 FN_VOU_GraphicsGetIntfType(GRAPHIC_LAYER gfxLayer, VO_INTF_TYPE_E *penIntfType);
typedef HI_S32 FN_VOU_GraphicsSetCscCoef(GRAPHIC_LAYER gfxLayer);
typedef HI_BOOL FN_VOU_GraphicsSetGfxKeyMode(GRAPHIC_LAYER enLayer, HI_U32 u32KeyOut);
typedef HI_BOOL FN_VOU_GraphicsSetGfxPalpha(GRAPHIC_LAYER enLayer, HI_U32 bAlphaEn,HI_U32 bArange,HI_U8 u8Alpha0,HI_U8 u8Alpha1);
typedef HI_BOOL FN_VOU_GraphicsSetLayerGalpha(GRAPHIC_LAYER enLayer, HI_U8 u8Alpha0);
typedef HI_BOOL FN_VOU_GraphicsSetCscEn(GRAPHIC_LAYER enLayer,  HI_BOOL bCscEn);
typedef HI_BOOL FN_VOU_GraphicsSetGfxAddr(GRAPHIC_LAYER enLayer, HI_U64 u64LAddr);
typedef HI_BOOL FN_VOU_GraphicsSetGfxStride(GRAPHIC_LAYER enLayer, HI_U16 u16pitch);
typedef HI_BOOL FN_VOU_GraphicsGetGfxPreMult(GRAPHIC_LAYER enLayer, HI_U32 *pbEnable);
typedef HI_BOOL FN_VOU_GraphicsSetGfxPreMult(GRAPHIC_LAYER enLayer, HI_U32 bEnable);
typedef HI_BOOL FN_VOU_GraphicsSetLayerDataFmt(GRAPHIC_LAYER enLayer, VO_DISP_PIXEL_FORMAT_E  enDataFmt);
typedef HI_BOOL FN_VOU_GraphicsSetLayerInRect(GRAPHIC_LAYER enLayer, RECT_S *pstRect);
typedef HI_BOOL FN_VOU_GraphicsSetLayerSrcImageReso(GRAPHIC_LAYER enLayer, RECT_S *pstRect);
typedef HI_BOOL FN_VOU_GraphicsSetLayerOutRect(GRAPHIC_LAYER enLayer, RECT_S *pstRect);
typedef HI_BOOL FN_VOU_GraphicsSetColorKeyValue(GRAPHIC_LAYER enLayer, VO_GFX_KEY_MAX_S stKeyMax, VO_GFX_KEY_MIN_S stKeyMin);
typedef HI_BOOL FN_VOU_GraphicsSetColorKeyMask(GRAPHIC_LAYER enLayer, VO_GFX_MASK_S stMsk);
typedef HI_BOOL FN_VOU_GraphicsSetGfxKeyEn(GRAPHIC_LAYER enLayer, HI_U32 u32KeyEnable);
typedef HI_BOOL  FN_VOU_GraphicsSetRegUp(GRAPHIC_LAYER enLayer);
typedef HI_BOOL FN_VOU_GraphicsGetLayerGalpha(GRAPHIC_LAYER enLayer,HI_U8 *pu8Alpha0);
typedef HI_BOOL FN_VOU_GraphicsGetLayerDataFmt(GRAPHIC_LAYER enLayer,HI_U32 *pu32Fmt);
typedef HI_BOOL  FN_VOU_GraphicsSetGfxExt(GRAPHIC_LAYER enLayer, VO_GFX_BITEXTEND_E enMode);
typedef HI_BOOL FN_VOU_GraphicsGetGfxAddr(GRAPHIC_LAYER enLayer, HI_U64 *pu64GfxAddr);
typedef HI_BOOL FN_VOU_GraphicsGetGfxStride(GRAPHIC_LAYER enLayer, HI_U32 *pu32GfxStride);
typedef HI_VOID FN_VOU_GraphicsGetDDRZoneCfg(GRAPHIC_LAYER gfxLayer, HI_U32 *pu32StartID, HI_U32 *pu32Cnt);
typedef HI_VOID FN_VOU_GraphicsClearDDRDectectZone(HI_U32 u32StartID, HI_U32 u32Cnt);
typedef HI_S32 FN_VOU_GraphicsCheckDDRDectectZone(GRAPHIC_LAYER gfxLayer, HI_U32 u32NewStartID, HI_U32 u32NewCnt, HI_U32 u32PreStartID, HI_U32 u32PreCnt);
typedef HI_VOID FN_VOU_GraphicsGetMDDRStatus(HI_U32 u32StartID, HI_U32 u32Cnt, HI_U32 *pu32Status);
typedef HI_VOID FN_VOU_GraphicsSetMDDRDetectZone(HI_U32 u32StartID, HI_U32 u32Cnt, VO_MDDRC_ZONE_ADDR_S *pstZoneAddr);

// For graphics compress
typedef HI_BOOL FN_VOU_GraphicsEnableDcmp(GRAPHIC_LAYER gfxLayer, HI_BOOL bEnable);
typedef HI_BOOL FN_VOU_GraphicsGetDcmpEnableState(GRAPHIC_LAYER gfxLayer, HI_BOOL *pbEnable);
typedef HI_BOOL FN_VOU_GraphicsSetDcmpInfo(GRAPHIC_LAYER enLayer, VO_GRAPHIC_DCMP_INFO_S *pstDcmpInfo);

// For graphics hdr(GHDR)
typedef HI_BOOL FN_VOU_GraphicsEnableGHDR(GRAPHIC_LAYER enLayer,HI_BOOL bEnable);
// For graphics ZME
typedef HI_BOOL FN_VOU_GraphicsEnableZME(GRAPHIC_LAYER enLayer, RECT_S *stInRect,RECT_S *stOutRect,HI_BOOL bEnable);

typedef struct hiVOU_EXPORT_FUNC_S
{
    FN_VOU_Init *pfnInit;
    FN_VOU_Exit *pfnExit;
    FN_VOU_ChnSendPic *pfnVouChnSendPic;
    FN_VOU_ClearChnBuf *pfnVouClearChnBuf;
    FN_VOU_ChnSetBufLen *pfnVouChnSetBufLen;
    FN_VOU_CheckHdmiEn *pfnVouCheckHdmiEn;
    FN_VOU_SetDitherOutBitWidth *pfnVouSetDitherOutBitWidth;

    FN_VOU_GraphicsGetDevMode           *pfnVOU_GraphicsGetDevMode;
    FN_VOU_GraphicsOpenLayer            *pfnVOU_GraphicsOpenLayer;
    FN_VOU_GraphicsCloseLayer           *pfnVOU_GraphicsCloseLayer;
    FN_VOU_GraphicsEnableLayer          *pfnVOU_GraphicsEnableLayer;
    FN_VOU_GraphicsSetCallback          *pfnVOU_GraphicsSetCallback;
    FN_VOU_GraphicsGetIntfSize          *pfnVOU_GraphicsGetIntfSize;
    FN_VOU_GraphicsGetIntfType          *pfnVOU_GraphicsGetIntfType;
    FN_VOU_GraphicsSetCscCoef           *pfnVOU_GraphicsSetCscCoef;
    FN_VOU_GraphicsSetGfxKeyMode        *pfnVOU_GraphicsSetGfxKeyMode;
    FN_VOU_GraphicsSetGfxPalpha         *pfnVOU_GraphicsSetGfxPalpha;
    FN_VOU_GraphicsSetLayerGalpha       *pfnVOU_GraphicsSetLayerGalpha;
    FN_VOU_GraphicsSetCscEn             *pfnVOU_GraphicsSetCscEn;
    FN_VOU_GraphicsSetGfxAddr           *pfnVOU_GraphicsSetGfxAddr;
    FN_VOU_GraphicsSetGfxStride         *pfnVOU_GraphicsSetGfxStride;
    FN_VOU_GraphicsGetGfxPreMult        *pfnVOU_GraphicsGetGfxPreMult;
    FN_VOU_GraphicsSetGfxPreMult        *pfnVOU_GraphicsSetGfxPreMult;
    FN_VOU_GraphicsSetLayerDataFmt      *pfnVOU_GraphicsSetLayerDataFmt;
    FN_VOU_GraphicsSetLayerInRect       *pfnVOU_GraphicsSetLayerInRect;
    FN_VOU_GraphicsSetLayerSrcImageReso *pfnVOU_GraphicsSetLayerSrcImageReso;
    FN_VOU_GraphicsSetLayerOutRect      *pfnVOU_GraphicsSetLayerOutRect;
    FN_VOU_GraphicsSetColorKeyValue     *pfnVOU_GraphicsSetColorKeyValue;
    FN_VOU_GraphicsSetColorKeyMask      *pfnVOU_GraphicsSetColorKeyMask;
    FN_VOU_GraphicsSetGfxKeyEn          *pfnVOU_GraphicsSetGfxKeyEn;
    FN_VOU_GraphicsSetRegUp             *pfnVOU_GraphicsSetRegUp;
    FN_VOU_GraphicsGetLayerGalpha       *pfnVOU_GraphicsGetLayerGalpha;
    FN_VOU_GraphicsGetLayerDataFmt      *pfnVOU_GraphicsGetLayerDataFmt;
    FN_VOU_GraphicsSetGfxExt            *pfnVOU_GraphicsSetGfxExt;
    FN_VOU_GraphicsGetGfxAddr           *pfnVOU_GraphicsGetGfxAddr;
    FN_VOU_GraphicsGetGfxStride         *pfnVOU_GraphicsGetGfxStride;
    FN_VOU_GraphicsGetDDRZoneCfg        *pfnVOU_GraphicsGetDDRZoneCfg;
    FN_VOU_GraphicsClearDDRDectectZone  *pfnVOU_GraphicsClearDDRDectectZone;
    FN_VOU_GraphicsCheckDDRDectectZone  *pfnVOU_GraphicsCheckDDRDectectZone;
    FN_VOU_GraphicsGetMDDRStatus        *pfnVOU_GraphicsGetMDDRStatus;
    FN_VOU_GraphicsSetMDDRDetectZone    *pfnVOU_GraphicsSetMDDRDetectZone;
    // For compress
    FN_VOU_GraphicsEnableDcmp           *pfnVOU_GraphicsEnableDcmp;
    FN_VOU_GraphicsGetDcmpEnableState   *pfnVOU_GraphicsGetDcmpEnableState;
    FN_VOU_GraphicsSetDcmpInfo          *pfnVOU_GraphicsSetDcmpInfo;
    // For GHDR
    FN_VOU_GraphicsEnableGHDR           *pfnVOU_GraphicsEnableGHDR;
    //FN_VO_GraphicsGetLayerAttr          *pfnVO_GraphicsGetLayerAttr;
    // For ZME
    FN_VOU_GraphicsEnableZME            *pfnVOU_GraphicsEnableZME;
}VOU_EXPORT_FUNC_S;


#endif /* __VOU_GRAPHICS_H__ */
