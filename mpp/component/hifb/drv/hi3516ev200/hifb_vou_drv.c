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


#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/poll.h>

#ifdef __HuaweiLite__
#include <fb.h>
#else
#include <linux/fb.h>
#endif

#include <linux/interrupt.h>
#include <asm/io.h>

#include "hifb_vou_drv.h"
#include "hifb_vou_graphics.h"
#include "hi_tde_ext.h"
#include "mod_ext.h"



#define HIFB_MAX_LAYER_WIDTH_G0   1920
#define HIFB_MAX_LAYER_HEIGHT_G0  1080
#define HIFB_MIN_LAYER_WIDTH_G0   32
#define HIFB_MIN_LAYER_HEIGH_G0   32

HIFB_LAYER_S g_stLayer[HIFB_MAX_LAYER_NUM];

char* apszLayerMmzNames[HIFB_MAX_LAYER_NUM] = {[0 ... HIFB_MAX_LAYER_NUM-1] = NULL};

module_param_array(apszLayerMmzNames, charp, NULL, S_IRUGO);
MODULE_PARM_DESC(apszLayerMmzNames, "The mmz names for the graphics layers.");


#define CHECK_LAYER_SUPPORT(layer)   if (layer >= HIFB_MAX_LAYER_NUM) {return HI_FAILURE;}
#define CHECK_NULL_POINTER(pointer)  if (pointer == NULL) {return HI_FAILURE;}

/*****************************************************************************
 Prototype       : Variable Definition
 Description     :

*****************************************************************************/

/*****************************************************************************
 Prototype       : Variable Definition
 Description     : hifb: a definition of the graphics layer capability set.
*****************************************************************************/
const HIFB_CAPABILITY_S s_astHifbGfxCap[HIFB_MAX_LAYER_NUM] =
{
    /* G0 */
    {
        .bKeyRgb          = 1,
        .bKeyAlpha        = 0,
        .bGlobalAlpha     = 1,
        .bCmap            = 0,
        .bColFmt          = {0,0,0,0,  0,1,1,0,  0,0,0,0,   0,0,0,0,   0,0,0,0,   0,0,0,0,  0,0,0,0,  0,0,0,0,  0,0}, //25
        .u32MaxWidth      = HIFB_MAX_LAYER_WIDTH_G0,
        .u32MaxHeight     = HIFB_MAX_LAYER_HEIGHT_G0,
        .u32MinWidth      = HIFB_MIN_LAYER_WIDTH_G0,
        .u32MinHeight     = HIFB_MIN_LAYER_HEIGH_G0,
        .u32VDefLevel     = 0,
        .u32HDefLevel     = 0,
        .bLayerSupported  = HI_TRUE,
        .bVoScale         = HI_FALSE,
        .bHasCmapReg      = 0,
        .bDcmp            = 0,
        .bPreMul          = 1,
        .bGHDR            = 0,
    },
};


char video[64] = "hifb:vram0_size:8100";

/*the index of array means the number of framebuffer layer, and the value of the array means the maping harware graphic layer*/
/*
* the index number of fb and the actual graphics layer
* 0 - HAL_DISP_LAYER_GFX0
* 1 - HAL_DISP_LAYER_GFX1
* 2 - HAL_DISP_LAYER_GFX3
*/
static HAL_DISP_LAYER_E s_HifbLayerToHwLayer[HIFB_MAX_LAYER_NUM] = {
        HAL_DISP_LAYER_GFX0,
};

HI_BOOL HIFB_DRV_SupportRotate()
{
    return HI_TRUE;
}
HI_BOOL IS_HD_LAYER(HI_U32 LayerId)
{
    return (0 == LayerId);
}

HI_BOOL IS_SD_LAYER(HI_U32 LayerId)
{
    return HI_FALSE;
}

HI_BOOL IS_AD_LAYER(HI_U32 LayerId)
{
    return HI_FALSE;
}

HI_BOOL IS_CURSOR_LAYER(HI_U32 LayerId)
{
    return (2 == LayerId);
}

VO_DISP_PIXEL_FORMAT_E HIFB_DRV_ConvertPixelFormat(HIFB_COLOR_FMT_E enColFmt)
{
    switch ( enColFmt )
    {
        case HIFB_FMT_ARGB8888 :
        {
            return HAL_INPUTFMT_ARGB_8888;
        }
        case HIFB_FMT_ARGB1555 :
        {
            return HAL_INPUTFMT_ARGB_1555;
        }
        default:
        {
            return HAL_DISP_PIXELFORMAT_BUTT;
        }
    }
}


/* Frame decompression needs to be offset by 16 bytes;
line decompression does not need */
HI_U64 HIFB_DRV_GetDcmpOffsetAddr(HI_U64 u64DcmpAddr)
{
    return u64DcmpAddr;
}

HI_S32 HIFB_DRV_EnableLayer(HI_U32 u32LayerId, HI_BOOL bEnable)
{
	CHECK_LAYER_SUPPORT(u32LayerId);
    return HIFB_GraphicsEnableLayer(s_HifbLayerToHwLayer[u32LayerId], bEnable);
}

/*
* Name : HIFB_DRV_LayerDefaultSetting
* Desc : The default settings for the layer, including the following:
            1.Color key mode
            2.Bit extension mode
            3.alpha, global alpha,CSC,CSC enable, get interface type
*/
HI_S32 HIFB_DRV_LayerDefaultSetting(HI_U32 u32LayerId)
{
    HAL_DISP_LAYER_E enDispLayer = HAL_DISP_LAYER_BUTT;
    VO_INTF_TYPE_E enIntfType = {0};
    CHECK_LAYER_SUPPORT(u32LayerId);
    enDispLayer = s_HifbLayerToHwLayer[u32LayerId];

    if (s_astHifbGfxCap[u32LayerId].bKeyRgb
        || s_astHifbGfxCap[u32LayerId].bKeyAlpha)
    {
        HIFB_GraphicsSetGfxKeyMode(enDispLayer, VOU_COLORKEY_IN);
    }

    HIFB_GraphicsSetGfxExt(enDispLayer, VOU_BITEXT_LOW_HIGHBITS);

    HIFB_GraphicsSetGfxPalpha(enDispLayer, HI_TRUE, HI_TRUE, GRAPHIC_ALPHA_OPACITY, GRAPHIC_ALPHA_OPACITY);

    HIFB_GraphicsSetLayerGalpha(enDispLayer, GRAPHIC_ALPHA_OPACITY);

    HIFB_GraphicsSetCscCoef(enDispLayer);

    HIFB_GraphicsGetIntfType(enDispLayer, &enIntfType);

    if (HAL_DISP_INTF_LCD == enIntfType)
    {
        HIFB_GraphicsSetCscEn(enDispLayer, HI_FALSE);
    }
    else
    {
        HIFB_GraphicsSetCscEn(enDispLayer, HI_TRUE);
    }

    return HI_SUCCESS;
}

HI_S32 HIFB_DRV_SetLayerAddr(HI_U32 u32LayerId, HI_U64 u64Addr)
{
    CHECK_LAYER_SUPPORT(u32LayerId);
    if (HI_FALSE == HIFB_GraphicsSetGfxAddr(s_HifbLayerToHwLayer[u32LayerId], u64Addr))
    {
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

HI_S32 HIFB_DRV_SetLayerStride(HI_U32 u32LayerId, HI_U32 u32Stride)
{
    CHECK_LAYER_SUPPORT(u32LayerId);
    if (HI_FALSE == HIFB_GraphicsSetGfxStride(s_HifbLayerToHwLayer[u32LayerId], u32Stride))
    {
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

HI_S32 HIFB_DRV_GetLayerStrideAlign(HI_U32 u32LayerId, HI_U32 *pu32StrideAlign)
{
    CHECK_LAYER_SUPPORT(u32LayerId);
    CHECK_NULL_POINTER(pu32StrideAlign);

    *pu32StrideAlign = 16;

    return HI_SUCCESS;
}


HI_S32 HIFB_DRV_SetLayerAlpha(HI_U32 u32LayerId, HIFB_ALPHA_S stAlpha)
{
    HI_U32 u32PreMultMode = 0;
    HAL_DISP_LAYER_E enDispLayer = 0;
    CHECK_LAYER_SUPPORT(u32LayerId);
    enDispLayer = s_HifbLayerToHwLayer[u32LayerId];
    /* When the premultiply is enabled, the global alpha cannot be 1*/
    if (1 == stAlpha.u8GlobalAlpha)
    {
        if(HI_FALSE == HIFB_GraphicsGetGfxPreMult(enDispLayer, &u32PreMultMode))
        {
            return HI_FAILURE;
        }
        if (1 == u32PreMultMode)
        {
            HI_TRACE_FB(HI_DBG_ERR, "the global alpha can not set to 1 when the pre-mult mode is enable\n");
            return HI_FAILURE;
        }
    }
    if (stAlpha.bAlphaChannel)
    {
        HIFB_GraphicsSetLayerGalpha(s_HifbLayerToHwLayer[u32LayerId], stAlpha.u8GlobalAlpha);
    }
    else
    {
        HIFB_GraphicsSetLayerGalpha(s_HifbLayerToHwLayer[u32LayerId], GRAPHIC_ALPHA_OPACITY);
    }

    HIFB_GraphicsSetGfxPalpha(s_HifbLayerToHwLayer[u32LayerId], stAlpha.bAlphaEnable,
        HI_TRUE, stAlpha.u8Alpha0, stAlpha.u8Alpha1);

    return HI_SUCCESS;
}

HI_S32 HIFB_DRV_SetLayerDataFmt(HI_U32 u32LayerId, HIFB_COLOR_FMT_E enDataFmt)
{
    VO_DISP_PIXEL_FORMAT_E enPixelFormatForHal;
    HAL_DISP_LAYER_E enDispLayer = 0;
    CHECK_LAYER_SUPPORT(u32LayerId);
    enDispLayer = s_HifbLayerToHwLayer[u32LayerId];

    switch (enDataFmt)
    {
        case HIFB_FMT_ARGB1555:
        {
            enPixelFormatForHal = HAL_INPUTFMT_ARGB_1555;
            /* Pre-multiply enable is not supported when the data format is ARGB1555 */
            HIFB_GraphicsSetGfxPreMult(enDispLayer, HI_FALSE);
            break;
        }

        case HIFB_FMT_ARGB8888:
        {
            enPixelFormatForHal = HAL_INPUTFMT_ARGB_8888;
            break;
        }

        case HIFB_FMT_ARGB4444:
        {
            enPixelFormatForHal = HAL_INPUTFMT_ARGB_4444;
            break;
        }

        default:
        {
            HI_TRACE_FB(HI_DBG_ERR, "HiFB does not support this color format");
            return HI_FAILURE;
        }
    }

    if (HI_FALSE == HIFB_GraphicsSetLayerDataFmt(enDispLayer, enPixelFormatForHal))
    {
        return HI_FAILURE;
    }
    return HI_SUCCESS;
}

HI_S32 HIFB_DRV_SetLayerSrcImageReso(HI_U32 enDispLayer, const HIFB_RECT *pstHifbRect)
{
    HIFB_RECT stHifbRect = {0};
    CHECK_LAYER_SUPPORT(enDispLayer);
    CHECK_NULL_POINTER(pstHifbRect);
    stHifbRect.x = pstHifbRect->x;
    stHifbRect.y = pstHifbRect->y;
    stHifbRect.w = pstHifbRect->w;
    stHifbRect.h = pstHifbRect->h;
    if(HI_FALSE == HIFB_GraphicsSetLayerSrcImageReso(s_HifbLayerToHwLayer[enDispLayer], &stHifbRect))
    {
        return HI_FAILURE;
    }
    return HI_SUCCESS;
}

HI_S32 HIFB_DRV_SetLayerRect(HI_U32 u32LayerId, const HIFB_RECT *pstInputRect, const HIFB_RECT *pstOutputRect)
{
    HIFB_RECT      Rect;
    VOU_SCAN_MODE_E enScanMode = 0;
    HI_BOOL        bFieldUpdate;
    CHECK_LAYER_SUPPORT(u32LayerId);
    CHECK_NULL_POINTER(pstInputRect);
    CHECK_NULL_POINTER(pstOutputRect);

    Rect.x = pstOutputRect->x;
    Rect.y = pstOutputRect->y;
    Rect.w = pstOutputRect->w;
    Rect.h = pstOutputRect->h;

    if (HI_SUCCESS != HIFB_GraphicsGetDevMode(s_HifbLayerToHwLayer[u32LayerId], &enScanMode, &bFieldUpdate))
    {
        return HI_FAILURE;
    }

    if (VOU_SCAN_MODE_INTERLACE == enScanMode)
    {
        /* When interlaced display, the ordinate and height are required to be even */
        if ((1 == ((HI_U32)Rect.y & 0x1)) || (1 == ((HI_U32)Rect.h & 0x1)))
        {
            HI_TRACE_FB(HI_DBG_ERR, "the height must be even for interlaced device\n");
            return HI_FAILURE;
        }
    }

    HIFB_GraphicsSetLayerOutRect(s_HifbLayerToHwLayer[u32LayerId], &Rect);

    HIFB_GraphicsSetLayerInRect(s_HifbLayerToHwLayer[u32LayerId], &Rect);

    return HI_SUCCESS;
}

HI_S32 HIFB_DRV_SetLayKeyMask(HI_U32 u32LayerId, const HIFB_COLORKEYEX_S* pstColorkey)
{
    VO_GFX_KEY_MAX_S stKeyMaxRGB;
    VO_GFX_KEY_MIN_S stKeyMinRGB;
    VO_GFX_MASK_S stKeyMaskRGB;
    HAL_DISP_LAYER_E enDispLayer = 0;
    CHECK_LAYER_SUPPORT(u32LayerId);
    CHECK_NULL_POINTER(pstColorkey);
    enDispLayer = s_HifbLayerToHwLayer[u32LayerId];

    if (pstColorkey->bKeyEnable)
    {
        if (HI_FALSE == HIFB_GraphicsSetGfxKeyMode(enDispLayer, pstColorkey->u32KeyMode))
        {
            return HI_FAILURE;
        }

        stKeyMaxRGB.u8KeyMax_R = pstColorkey->u8RedMax;
        stKeyMaxRGB.u8KeyMax_G = pstColorkey->u8GreenMax;
        stKeyMaxRGB.u8KeyMax_B = pstColorkey->u8BlueMax;

        stKeyMinRGB.u8KeyMin_R = pstColorkey->u8RedMin;
        stKeyMinRGB.u8KeyMin_G = pstColorkey->u8GreenMin;
        stKeyMinRGB.u8KeyMin_B = pstColorkey->u8BlueMin;

        HIFB_GraphicsSetColorKeyValue(enDispLayer, stKeyMaxRGB, stKeyMinRGB);
        if (pstColorkey->bMaskEnable)
        {
            stKeyMaskRGB.u8Mask_r = pstColorkey->u8RedMask;
            stKeyMaskRGB.u8Mask_g = pstColorkey->u8GreenMask;
            stKeyMaskRGB.u8Mask_b = pstColorkey->u8BlueMask;
        }
        else
        {
            stKeyMaskRGB.u8Mask_r = 0;
            stKeyMaskRGB.u8Mask_g = 0;
            stKeyMaskRGB.u8Mask_b = 0;
        }
        HIFB_GraphicsSetColorKeyMask(enDispLayer, stKeyMaskRGB);
    }

    if (HI_FALSE == HIFB_GraphicsSetGfxKeyEn(enDispLayer, pstColorkey->bKeyEnable))
    {
        return HI_FAILURE;
    }

    return HI_FAILURE;
}

#ifdef __HuaweiLite__
HI_S32 HIFB_DRV_ColorConvert(const struct hifb_info *info, HIFB_COLORKEYEX_S *pCkey)
{
    HI_U8 r = 0;
    HI_U8 g = 0;
    HI_U8 b = 0;
    HI_U32 u32Key = 0;
    HI_U8 u8RMask = 0;
    HI_U8 u8GMask = 0;
    HI_U8 u8BMask = 0;
    CHECK_NULL_POINTER(info);
    CHECK_NULL_POINTER(pCkey);
    u32Key = pCkey->u32Key;

    if (info->vinfo.fmt == HIFB_FMT_ARGB1555)
    {
        r = (u32Key >> 10)   << (8-5) ;
        g = (u32Key >> 5) << (8-5) ;
        b = (u32Key >> 0)  << (8-5) ;

        u8RMask = (0xff >> 5);
        u8GMask= (0xff >> 5);
        u8BMask  = (0xff >> 5);
    }

    pCkey->u8BlueMin = b;
    pCkey->u8GreenMin = g;
    pCkey->u8RedMin = r;

    pCkey->u8BlueMax = b | u8BMask;
    pCkey->u8GreenMax = g | u8BMask;
    pCkey->u8RedMax = r | u8BMask;

   /*low bits not using when do compare, after extend bits*/
   pCkey->u8RedMask = ~u8RMask;
   pCkey->u8GreenMask = ~u8GMask;
   pCkey->u8BlueMask  = ~u8BMask;

   pCkey->u32KeyMode = 0;

   pCkey->bMaskEnable = HI_TRUE;

   return HI_SUCCESS;
}
#else
HI_S32 HIFB_DRV_ColorConvert(const struct fb_var_screeninfo *pstVar, HIFB_COLORKEYEX_S *pCkey)
{
    HI_U8 r, g, b;
    HI_U32 u32Key = 0;
    HI_U8 u8RMask, u8GMask, u8BMask;
    CHECK_NULL_POINTER(pstVar);
    CHECK_NULL_POINTER(pCkey);
    u32Key = pCkey->u32Key;

    r = (u32Key >> pstVar->red.offset)   << (8-pstVar->red.length) ;
    g = (u32Key >> pstVar->green.offset) << (8-pstVar->green.length) ;
    b = (u32Key >> pstVar->blue.offset)  << (8-pstVar->blue.length) ;

    u8RMask = (0xff >> pstVar->red.length);
    u8GMask= (0xff >> pstVar->green.length);
    u8BMask  = (0xff >> pstVar->blue.length);

    pCkey->u8BlueMin = b;
    pCkey->u8GreenMin = g;
    pCkey->u8RedMin = r;

    pCkey->u8BlueMax = b | u8BMask;
    pCkey->u8GreenMax = g | u8BMask;
    pCkey->u8RedMax = r | u8BMask;

   /*low bits not using when do compare, after extend bits*/
   pCkey->u8RedMask = ~u8RMask;
   pCkey->u8GreenMask = ~u8GMask;
   pCkey->u8BlueMask  = ~u8BMask;

   pCkey->u32KeyMode = 0;

   pCkey->bMaskEnable = HI_TRUE;

   return HI_SUCCESS;
}
#endif

HI_S32 HIFB_DRV_UpdataLayerReg(HI_U32 u32LayerId)
{
    CHECK_LAYER_SUPPORT(u32LayerId);
    if (HI_TRUE != HIFB_GraphicsSetRegUp(s_HifbLayerToHwLayer[u32LayerId]))
    {
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

HI_S32 HIFB_DRV_GammaEnable(HI_U32 u32LayerId, HI_BOOL bEnable)
{
    return HI_FAILURE;
}

HI_S32 HIFB_DRV_SetLayerDeFlicker(HI_U32 u32LayerId, HIFB_DEFLICKER_S stDeFlicker)
{
    return HI_FAILURE;
}

HI_S32 HIFB_DRV_SetColorReg(HI_U32 u32LayerId, HI_U32 u32OffSet, HI_U32 u32Color, HI_S32 UpFlag)
{
    return HI_FAILURE;
}

HI_S32 HIFB_DRV_Init(HI_VOID)
{
    HI_S32 ret;
    TDE_EXPORT_FUNC_S  *pTdeExportFunc = NULL;

    pTdeExportFunc = FUNC_ENTRY(TDE_EXPORT_FUNC_S, HI_ID_TDE);
    if ((NULL == pTdeExportFunc) || (NULL == pTdeExportFunc->pfnTdeOpen))
    {
        HI_TRACE_FB(HI_DBG_ERR, "TDE pTdeExportFunc is NULL!\n");
    }
    else
    {
        ret = pTdeExportFunc->pfnTdeOpen();
        if(ret)
        {
            HI_TRACE_FB(HI_DBG_ERR, "TDE open failed\n");
            return ret;
        }
    }

    ret = HIFB_GraphicsResourceInit();
    if (ret)
    {
        if ((pTdeExportFunc != NULL) && (pTdeExportFunc->pfnTdeClose != NULL)) {
            pTdeExportFunc->pfnTdeClose();
        }
        HI_TRACE_FB(HI_DBG_ERR, "resource init failed\n");
        return ret;
    }

    return HI_SUCCESS;
}

HI_S32 HIFB_DRV_DeInit(HI_VOID)
{
    TDE_EXPORT_FUNC_S  *pTdeExportFunc = NULL;

    HIFB_GraphicsResourceDeInit();

    pTdeExportFunc = FUNC_ENTRY(TDE_EXPORT_FUNC_S, HI_ID_TDE);
    if ((NULL == pTdeExportFunc) || (NULL == pTdeExportFunc->pfnTdeClose))
    {
        HI_TRACE_FB(HI_DBG_ERR, "TDE pTdeExportFunc is NULL!\n");
        return HI_FAILURE;
    }
    pTdeExportFunc->pfnTdeClose();

    return HI_SUCCESS;
}

/*set premul data*/
HI_S32 HIFB_DRV_SetPreMul(HI_U32 u32LayerId, HI_BOOL bPreMul)
{
    HAL_DISP_PIXEL_FORMAT_E enPixelFormat = HAL_INPUTFMT_ARGB_1555;
    HAL_DISP_LAYER_E enLayer = 0;
    HI_U8 u8GlobalAlpha;
    CHECK_LAYER_SUPPORT(u32LayerId);
    enLayer = s_HifbLayerToHwLayer[u32LayerId];

    if (HI_TRUE == bPreMul)
    {
        /* When the global alpha configuration is 1, pre-multiplication cannot be enabled. */
        if (HI_TRUE != HIFB_GraphicsGetLayerGalpha(enLayer, &u8GlobalAlpha))
        {
            return HI_FAILURE;
        }
        if (1 == u8GlobalAlpha)
        {
            HI_TRACE_FB(HI_DBG_ERR, "not allow to set pre-mult mode when the GlobalAlpha is 1\n");
            return HI_FAILURE;
        }

        /* Pre-multiply enable is not supported when the data format is ARGB1555 */
        if (HI_TRUE != HIFB_GraphicsGetLayerDataFmt(enLayer, &enPixelFormat))
        {
            return HI_FAILURE;
        }
        if (HAL_INPUTFMT_ARGB_1555 == enPixelFormat || HAL_INPUTFMT_ARGB_4444 == enPixelFormat)
        {
            HI_TRACE_FB(HI_DBG_ERR, "not allow to set pre-mult mode when the color format is ARGB1555 or ARGB4444\n");
            return HI_FAILURE;
        }
    }

    if (HI_TRUE != HIFB_GraphicsSetGfxPreMult(enLayer, bPreMul))
    {
        return HI_FAILURE;
    }
    return HI_SUCCESS;
}

HI_S32 HIFB_DRV_SetClutAddr(HI_U32 u32LayerId, HI_U32 u32PhyAddr)
{
    return HI_FAILURE;
}

#ifdef CONFIG_HI_VO_FB_SEPARATE
HI_S32 HIFB_DRV_SetIntCallback(VO_FB_INT_TYPE_E eIntType, IntCallBack pCallBack,
                        HI_U32 u32LayerId, HI_VOID *pCallBackArg)
{
    CHECK_LAYER_SUPPORT(u32LayerId);
    return VOU_GraphicsSetCallback(s_HifbLayerToHwLayer[u32LayerId], eIntType, pCallBack, pCallBackArg);
}
#else
HI_S32 HIFB_DRV_SetIntCallback(VO_FB_INT_TYPE_E eIntType, IntCallBack pCallBack,
                        HI_U32 u32LayerId, HI_VOID *pCallBackArg)
{
    VOU_EXPORT_FUNC_S  *pVouExportFunc = NULL;
    CHECK_LAYER_SUPPORT(u32LayerId);

    pVouExportFunc = FUNC_ENTRY(VOU_EXPORT_FUNC_S, HI_ID_VO);
    if ((NULL == pVouExportFunc) || (NULL == pVouExportFunc->pfnVOU_GraphicsSetCallback))
    {
        HI_TRACE_FB(HI_DBG_ERR, "VOU pVouExportFunc is NULL!\n");
        return HI_FAILURE;
    }

    if (u32LayerId >= HIFB_MAX_LAYER_NUM)
    {
        return HI_FAILURE;
    }
    // VOU_GraphicsSetCallback
    return pVouExportFunc->pfnVOU_GraphicsSetCallback(s_HifbLayerToHwLayer[u32LayerId], eIntType, pCallBack, pCallBackArg);
}

#endif


/* The premise is that the VO device is enabled and an error is returned when the VO device is not enabled.
For X5HD, the current configuration content of the graphics layer is obtained for the boot screen;
for the 3531, since the boot screen is not required, only the device resolution and scan information
can be obtained.*/
HI_S32 HIFB_DRV_GetOSDData(HI_U32 u32LayerId, HIFB_OSD_DATA_S *pstLayerData)
{
    HI_U32 u32Width = 0, u32Height = 0;
    HI_S32 ret = 0;
    HAL_DISP_LAYER_E eLayer = 0;
    VOU_SCAN_MODE_E enScanMode = 0;
    CHECK_LAYER_SUPPORT(u32LayerId);
    CHECK_NULL_POINTER(pstLayerData);
    eLayer = s_HifbLayerToHwLayer[u32LayerId];

    ret = HIFB_GraphicsGetIntfSize(eLayer, &u32Width, &u32Height);

    if (HI_SUCCESS != ret)
    {
        HI_TRACE_FB(HI_DBG_ERR, "failed to get screen width and height!\n");
        return HI_FAILURE;
    }

    ret = HIFB_GraphicsGetGfxAddr(eLayer, &pstLayerData->u64BufferPhyAddr);
    if (HI_TRUE!= ret)
    {
        return HI_FAILURE;
    }

    ret = HIFB_GraphicsGetGfxStride(eLayer, &pstLayerData->u32Stride);
    if (HI_TRUE != ret)
    {
        return HI_FAILURE;
    }

    ret = HIFB_GraphicsGetDevMode(eLayer, &enScanMode, &pstLayerData->bFeildUpdate);
    if (HI_SUCCESS != ret)
    {
        return HI_FAILURE;
    }
    if (VOU_SCAN_MODE_INTERLACE == enScanMode)
    {
        pstLayerData->eScanMode = HIFB_SCANMODE_I;
    }
    else
    {
        pstLayerData->eScanMode = HIFB_SCANMODE_P;
    }

    pstLayerData->u32ScreenHeight = u32Height;
    pstLayerData->u32ScreenWidth = u32Width;

    return HI_SUCCESS;
}
#ifdef CONFIG_HI_VO_FB_SEPARATE
HI_S32 HIFB_OpenLayer(HI_U32 u32LayerId)
{
    CHECK_LAYER_SUPPORT(u32LayerId);
    return VOU_GraphicsOpenLayer(s_HifbLayerToHwLayer[u32LayerId]);
}
#else
HI_S32 HIFB_OpenLayer(HI_U32 u32LayerId)
{
    VOU_EXPORT_FUNC_S  *pVouExportFunc = NULL;
    CHECK_LAYER_SUPPORT(u32LayerId);

    pVouExportFunc = FUNC_ENTRY(VOU_EXPORT_FUNC_S, HI_ID_VO);
    if ((NULL == pVouExportFunc) || (NULL == pVouExportFunc->pfnVOU_GraphicsOpenLayer))
    {
        HI_TRACE_FB(HI_DBG_ERR, "VOU pVouExportFunc is NULL!\n");
        return HI_FAILURE;
    }

    // See VOU_GraphicsOpenLayer
    return pVouExportFunc->pfnVOU_GraphicsOpenLayer(s_HifbLayerToHwLayer[u32LayerId]);
}
#endif


HI_S32 HIFB_CloseLayer(HI_U32 u32LayerId)
{
    CHECK_LAYER_SUPPORT(u32LayerId);
    return HIFB_GraphicsCloseLayer(s_HifbLayerToHwLayer[u32LayerId]);
}

/*open vo & close vo -- For internal debugging only
(for the API that does not adjust VO, the device can display the graphics layer)*/
HI_S32 HIFB_DRV_OpenDisplay(HI_VOID)
{
    return HI_SUCCESS;
}

/*close vou*/
HI_S32 HIFB_DRV_CloseDisplay(HI_VOID)
{
    return HI_SUCCESS;
}

HI_S32 TdeCalScaleRect_HIFB(const TDE2_RECT_S* pstSrcRect, const TDE2_RECT_S* pstDstRect,
                             TDE2_RECT_S* pstRectInSrc, TDE2_RECT_S* pstRectInDst)
{
    CHECK_NULL_POINTER(pstSrcRect);
    CHECK_NULL_POINTER(pstDstRect);
    CHECK_NULL_POINTER(pstRectInSrc);
    CHECK_NULL_POINTER(pstRectInDst);
    if (pstSrcRect->u32Width != 0) {
        pstRectInDst->s32Xpos = pstRectInSrc->s32Xpos * pstDstRect->u32Width / pstSrcRect->u32Width;
    }
    if (pstSrcRect->u32Height != 0) {
        pstRectInDst->s32Ypos = pstRectInSrc->s32Ypos * pstDstRect->u32Height/ pstSrcRect->u32Height;
    }
    pstRectInDst->u32Height = pstDstRect->u32Height;
    pstRectInDst->u32Width = pstDstRect->u32Width;
    return HI_SUCCESS;
}

HI_BOOL HIFB_DRV_EnableDcmp(HI_U32 u32LayerId, HI_BOOL bEnable)
{
    return HI_TRUE;
}

HI_BOOL HIFB_DRV_GetDcmpEnableState(HI_U32 u32LayerId, HI_BOOL *pbEnable)
{
    return HI_TRUE;
}


HI_BOOL HIFB_DRV_SetDcmpInfo(HI_U32 u32LayerId, VO_GRAPHIC_DCMP_INFO_S *pstDcmpInfo)
{
    return HI_TRUE;
}


HI_BOOL HIFB_DRV_EnableGHDR(HI_U32 u32LayerId, HI_BOOL bEnable)
{
    return HI_SUCCESS;
}
HI_VOID HIFB_DRV_EnableWbcInt(HI_U32 u32LayerId, HI_BOOL bEnable)
{
    return;
}

HI_VOID HIFB_DRV_SetWbcInfo(HI_U32 u32LayerId, VO_GRAPHIC_WBC_INFO_S *pstWbcInfo)
{
    return;
}

HI_VOID HIFB_DRV_EnableWbc(HI_U32 u32LayerId, HI_BOOL bEnable)
{
    return;
}

HI_VOID HIFB_DRV_GetWbcEnState(HI_U32 u32LayerId, HI_BOOL *pbWbcEn)
{
    return;
}

HI_VOID HIFB_DRV_GetWbcStopState(HI_U32 u32LayerId, HI_BOOL *pbWbcStop)
{
    return;
}
#ifdef MDDRDETECT

static HI_BOOL HIFB_DRV_IsLayerSupportDDRDetect(HI_U32 u32LayerId)
{
    return HI_FALSE;
}

static HI_VOID HIFB_DRV_SetDDR_DetectZone(HI_U32 u32StartID, HI_U32 u32Cnt, VO_MDDRC_ZONE_ADDR_S *pstZoneAddr)
{
    return;
}

static HI_VOID HIFB_DRV_GetMDDR_Status(HI_U32 u32StartID, HI_U32 u32Cnt, HI_U32 *pu32Status)
{
    return;
}

HI_VOID HIFB_DRV_ClearDDRDectectZone(HI_U32 u32StartID, HI_U32 u32Cnt)
{
    return;
}

HI_S32 HIFB_DRV_CheckDDR_DectectZone(HI_U32 u32LayerId, HI_U32 u32NewStartID,
    HI_U32 u32NewCnt, HI_U32 u32PreStartID, HI_U32 u32PreCnt)
{
    return HI_FAILURE;
}

HI_VOID HIFB_DRV_GetDDR_ZoneCfg(HI_U32 u32LayerId, HI_U32 *pu32StartID, HI_U32 *pu32Cnt)
{
    return;
}
#endif
HI_BOOL HIFB_DRV_GraphicsEnableInt(HI_U32 u32LayerId, HI_BOOL bEnable)
{
    return (HI_SUCCESS == HIFB_GraphicsEnableInt(u32LayerId,bEnable) ? HI_TRUE : HI_FALSE);
}

HI_BOOL HIFB_DRV_GraphicsClearInt(HI_U32 u32IntClear, HI_S32 s32Irq)
{
    return HIFB_GraphicsClearInt(u32IntClear, s32Irq);
}
HI_BOOL HIFB_DRV_GraphicsGetInt(HI_U32 * pu32IntStatus)
{
    return HIFB_GraphicsGetInt(pu32IntStatus);
}

HI_BOOL HIFB_DRV_GraphicsClearIntStatus(HI_U32 u32IntStatus)
{
    return HIFB_GraphicsClearIntStatus(u32IntStatus);
}


HI_S32 HIFB_DRV_GraphicsGetInterruptDev(HI_U32 IntStatus, VO_DEV* pVoDev)
{
    return HIFB_GraphicsGetInterruptDev(IntStatus,pVoDev);
}

/*****************************************************************************
 Prototype       : HIFB_DRV_GetOps
 Description     : fuction pointer list for hifb operation
 Input           : pstOps  **
 Output          : None
 Return Value    :
 Global Variable
    Read Only    :
    Read & Write :
  History
  1.Date         : 2009/3/13
    Author       : sdk
    Modification : Created function
*****************************************************************************/
HI_VOID HIFB_DRV_GetOps(HIFB_DRV_OPS_S *pstOps)
{
    if (pstOps == NULL) {
        return;
    }
    pstOps->u32LayerCount                   = HIFB_MAX_LAYER_NUM;
    pstOps->pstCapability = (HIFB_CAPABILITY_S *)s_astHifbGfxCap;

    /* implement */
    pstOps->HIFB_DRV_Init                   = HIFB_DRV_Init;
    pstOps->HIFB_DRV_DeInit                 = HIFB_DRV_DeInit;
    pstOps->HIFB_DRV_EnableLayer            = HIFB_DRV_EnableLayer;
    pstOps->HIFB_DRV_LayerDefaultSetting    = HIFB_DRV_LayerDefaultSetting;
    //pstOps->HIFB_DRV_SetOutDev              = HIFB_DRV_SetOutDev;
    pstOps->HIFB_DRV_SetLayerAddr           = HIFB_DRV_SetLayerAddr;
    pstOps->HIFB_DRV_SetLayerStride         = HIFB_DRV_SetLayerStride;
	//pstOps->HIFB_DRV_GetLayerStrideAlign	= HIFB_DRV_GetLayerStrideAlign;
    pstOps->HIFB_DRV_SetLayerAlpha          = HIFB_DRV_SetLayerAlpha;
    pstOps->HIFB_DRV_SetLayerDataFmt        = HIFB_DRV_SetLayerDataFmt;
    pstOps->HIFB_DRV_SetLayerRect           = HIFB_DRV_SetLayerRect;
    pstOps->HIFB_DRV_SetLayerSrcImageReso   = HIFB_DRV_SetLayerSrcImageReso;
    //pstOps->HIFB_DRV_SetLayerPriority       = HIFB_DRV_SetLayerPriority;
    pstOps->HIFB_DRV_SetLayerKeyMask       = HIFB_DRV_SetLayKeyMask;
    pstOps->HIFB_DRV_ColorConvert           = HIFB_DRV_ColorConvert;
    pstOps->HIFB_DRV_UpdataLayerReg         = HIFB_DRV_UpdataLayerReg;
	//pstOps->HIFB_DRV_GetLayerMaxRslt         = HIFB_DRV_GetLayerMaxRslt;
    pstOps->HIFB_DRV_SetIntCallback = HIFB_DRV_SetIntCallback;
    //pstOps->HIFB_DRV_WaitVBlank             = HIFB_DRV_WaitVBlank;
    pstOps->HIFB_DRV_SetLayerDeFlicker      = HIFB_DRV_SetLayerDeFlicker;
    pstOps->HIFB_DRV_SetColorReg            = HIFB_DRV_SetColorReg;
    pstOps->HIFB_DRV_SetPreMul = HIFB_DRV_SetPreMul;
    pstOps->HIFB_DRV_SetClutAddr = HIFB_DRV_SetClutAddr;
    pstOps->HIFB_DRV_GetOSDData = HIFB_DRV_GetOSDData;
    pstOps->HIFB_DRV_OpenDisplay = HIFB_DRV_OpenDisplay;
    pstOps->HIFB_DRV_CloseDisplay = HIFB_DRV_CloseDisplay;
    pstOps->HIFB_OpenLayer = HIFB_OpenLayer;
    pstOps->HIFB_CloseLayer = HIFB_CloseLayer;


    pstOps->HIFB_DRV_EnableWbcInt = HIFB_DRV_EnableWbcInt;
    pstOps->HIFB_DRV_GetWbcStopState = HIFB_DRV_GetWbcStopState;
    pstOps->HIFB_DRV_EnableWbc = HIFB_DRV_EnableWbc;
    pstOps->HIFB_DRV_SetWbcInfo = HIFB_DRV_SetWbcInfo;
    pstOps->HIFB_DRV_GetWbcEnState = HIFB_DRV_GetWbcEnState;
    pstOps->HIFB_DRV_SetDcmpInfo = HIFB_DRV_SetDcmpInfo;
    pstOps->HIFB_DRV_EnableDcmp = HIFB_DRV_EnableDcmp;
    pstOps->HIFB_DRV_GetDcmpEnableState = HIFB_DRV_GetDcmpEnableState;

#ifdef MDDRDETECT
    pstOps->HIFB_DRV_IsLayerSupportDDRDetect=HIFB_DRV_IsLayerSupportDDRDetect;
    pstOps->HIFB_DRV_GetMDDR_Status = HIFB_DRV_GetMDDR_Status;
    pstOps->HIFB_DRV_SetMDDRDetectZone = HIFB_DRV_SetDDR_DetectZone;
    pstOps->HIFB_DRV_ClearDDRDectectZone = HIFB_DRV_ClearDDRDectectZone;
    pstOps->HIFB_DRV_CheckDDR_DectectZone = HIFB_DRV_CheckDDR_DectectZone;
    pstOps->HIFB_DRV_GetDDR_ZoneCfg = HIFB_DRV_GetDDR_ZoneCfg;
    pstOps->HIFB_DRV_InitMDDR    = VOU_GraphicsInitMDDRDetect;
#endif
    // For GHDR
    pstOps->HIFB_DRV_EnableGHDR     = HIFB_DRV_EnableGHDR;
    // For ZME
    // for Int
    pstOps->HIFB_DRV_GraphicsEnableINT      = HIFB_DRV_GraphicsEnableInt;
    pstOps->HIFB_DRV_GraphicsClearINT       = HIFB_DRV_GraphicsClearInt;
    pstOps->HIFB_DRV_GraphicsGetINT         = HIFB_DRV_GraphicsGetInt;
    pstOps->HIFB_DRV_GraphicsClearINTStatus = HIFB_DRV_GraphicsClearIntStatus;
    pstOps->HIFB_DRV_GraphicsGetINTDev      = HIFB_DRV_GraphicsGetInterruptDev;
}
