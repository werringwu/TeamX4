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

#include "hi_osal.h"
#include "proc_ext.h"

#include "hi_errno.h"
#include "hi_debug.h"
#include "hifb_vou_graphics.h"


typedef struct hiVOU_DEV_S
{
    HI_BOOL        bVoEnable;                           /* Device enable flag */

    VO_INTF_TYPE_E enIntfType;                          /* Device interface type */

    HI_U32         u32MaxWidth;                         /* Device resolution maximum width */
    HI_U32         u32MaxHeight;                        /* Device resolution maximum height */

}VOU_DEV_S;

VOU_DEV_S g_astVoDev[VO_MAX_DEV_NUM];

VO_GFXLAYER_CONTEXT_S s_astGfxLayerCtx[VO_MAX_GRAPHICS_LAYER_NUM];

HI_VOID HIFB_GetGfxDcmpPixel(VO_DISP_PIXEL_FORMAT_E enPixelFmt, HAL_DISP_PIXEL_FORMAT_E *penPixelFmt)
{
    switch (enPixelFmt)
    {
        case VO_INPUTFMT_ARGB_4444 :
            *penPixelFmt = HAL_INPUTFMT_ARGB_4444;
            break;
        case VO_INPUTFMT_ARGB_1555 :
            *penPixelFmt = HAL_INPUTFMT_ARGB_1555;
            break;
        case VO_INPUTFMT_ARGB_8888 :
            *penPixelFmt = HAL_INPUTFMT_ARGB_8888;
            break;
        default:
           GRAPHICS_DRV_TRACE(HI_DBG_ERR, "pixel format(%d) is invalid!\n", enPixelFmt);
            break;

    }
}
#ifdef MDDRDETECT
HI_U32                g_u32DectectZone = 0;
HI_VOID VOU_GraphicsInitMDDRDetect(HI_VOID)
{
    MDDRC_DRV_InitMDDRDetect();
}

HI_VOID VOU_GraphicsSetMDDRDetectZone(HI_U32 u32StartID, HI_U32 u32Cnt, VO_MDDRC_ZONE_ADDR_S *pstZoneAddr)
{
    MDDRC_ZONE_ADDR_S  stZoneAddr;
    stZoneAddr.u32StartPhyAddr = pstZoneAddr->u32StartPhyAddr;
    stZoneAddr.u32EndPhyAddr = pstZoneAddr->u32EndPhyAddr;
    MDDRC_DRV_SetZoneAddr(u32StartID, u32Cnt, &stZoneAddr);
}

HI_VOID VOU_GraphicsGetMDDRStatus(HI_U32 u32StartID, HI_U32 u32Cnt, HI_U32 *pu32Status)
{
    Graphics_DRV_GetMDDRStatus(u32StartID, u32Cnt, pu32Status);
}

HI_VOID VOU_GraphicsClearDDRDectectZone(HI_U32 u32StartID, HI_U32 u32Cnt)
{
    HI_U32 u32SectionID;
    HI_U32 i;

    HI_ASSERT((u32StartID < MDDRC_ZONE_MAX_NUM)
        && ((u32Cnt <= MDDRC_ZONE_MAX_NUM))
        && (u32StartID + u32Cnt) <= MDDRC_ZONE_MAX_NUM);

    for (i = 0; i < u32Cnt; ++i)
    {
        u32SectionID = u32StartID + i;
        g_u32DectectZone &= ~(1 << u32SectionID);
    }
}

HI_S32 VOU_GraphicsCheckDDRDectectZone(GRAPHIC_LAYER u32Layer,
    HI_U32 u32NewStartID, HI_U32 u32NewCnt, HI_U32 u32PreStartID, HI_U32 u32PreCnt)
{
    HI_U32 u32SectionID;
    HI_U32 i;
    HAL_DISP_LAYER_E gfxLayer;

    HIFB_DRV_GetLayerID(u32Layer, &gfxLayer);
    if ((u32NewStartID >= MDDRC_ZONE_MAX_NUM) || ((u32NewStartID + u32NewCnt) > MDDRC_ZONE_MAX_NUM))
    {
        GRAPHICS_DRV_TRACE(HI_DBG_ERR, "Detect zone startid%d or cnt%d err. \n", u32NewStartID, u32NewCnt);

        return HI_FAILURE;
    }

    for (i = 0; i < u32NewCnt; ++i)
    {
        u32SectionID = u32NewStartID + i;
        if ((u32SectionID >= u32PreStartID) && (u32SectionID  < (u32PreStartID + u32PreCnt)))
        {
            continue;
        }
        if (g_u32DectectZone & (1 << u32SectionID))
        {
            GRAPHICS_DRV_TRACE(HI_DBG_ERR, "DDR dectect zone is conflicted with others!\n");

            return HI_FAILURE;
        }
    }

    return HI_SUCCESS;
}

HI_VOID VOU_GraphicsGetDDRZoneCfg(GRAPHIC_LAYER u32Layer, HI_U32 *pu32StartID, HI_U32 *pu32Cnt)
{
    HI_U32              u32LayerIndex;
    GFX_SPIN_LOCK_FLAG  lockFlag;
    HAL_DISP_LAYER_E    gfxLayer = HAL_DISP_LAYER_BUTT;

    HIFB_DRV_GetLayerID(u32Layer, &gfxLayer);
    if (HI_SUCCESS != HIFB_DRV_GetLayerIndex(gfxLayer, &u32LayerIndex))
    {
        GRAPHICS_DRV_TRACE(HI_DBG_ERR, "gfxLayer(%u) is invalid!\n", (HI_U32)gfxLayer);
        return ;
    }

    GFX_SPIN_LOCK_IRQSAVE(&s_astGfxLayerCtx[u32LayerIndex].spinLock, &lockFlag);
    *pu32StartID = s_astGfxLayerCtx[u32LayerIndex].u32StartSection;
    *pu32Cnt = s_astGfxLayerCtx[u32LayerIndex].u32ZoneNums;
    GFX_SPIN_UNLOCK_IRQRESTORE(&s_astGfxLayerCtx[u32LayerIndex].spinLock, &lockFlag);
}
#endif
/*
* Name : VOU_GraphicsSetGfxKeyMode
* Desc : set color key mode
*/
HI_BOOL HIFB_GraphicsSetGfxKeyMode(GRAPHIC_LAYER u32Layer, HI_U32 u32KeyOut)
{
    HAL_DISP_LAYER_E enLayer = HAL_DISP_LAYER_BUTT;

    HIFB_DRV_GetLayerID(u32Layer, &enLayer);
    return HIFB_GRAPHIC_DRV_SetGfxKeyMode(enLayer, u32KeyOut);
}

/*
* Name : VOU_GraphicsSetGfxExt
* Desc :
*/
HI_BOOL  HIFB_GraphicsSetGfxExt(GRAPHIC_LAYER u32Layer,
                                VO_GFX_BITEXTEND_E enMode)
{
    HAL_DISP_LAYER_E enLayer = HAL_DISP_LAYER_BUTT;

    HIFB_DRV_GetLayerID(u32Layer, &enLayer);
    return HIFB_DRV_SetGfxExt(enLayer, enMode);
}

/*
* Name : VOU_GraphicsSetGfxPalpha
* Desc : set point alpha
*/
HI_BOOL HIFB_GraphicsSetGfxPalpha(GRAPHIC_LAYER u32Layer,
                                   HI_U32 bAlphaEn,HI_U32 bArange,
                                   HI_U8 u8Alpha0,HI_U8 u8Alpha1)
{
    HAL_DISP_LAYER_E enLayer = HAL_DISP_LAYER_BUTT;

    HIFB_DRV_GetLayerID(u32Layer, &enLayer);
    return HIFB_DRV_SetGfxPalpha(enLayer, bAlphaEn, bArange,
                   u8Alpha0, u8Alpha1);
}

/*
* Name : VOU_GraphicsSetLayerGalpha
* Desc : set golbal alpha
*/
HI_BOOL HIFB_GraphicsSetLayerGalpha(GRAPHIC_LAYER u32Layer,
                                     HI_U8 u8Alpha0)
{
    HAL_DISP_LAYER_E enLayer = HAL_DISP_LAYER_BUTT;

    HIFB_DRV_GetLayerID(u32Layer, &enLayer);
    return HIFB_DRV_LAYER_SetLayerGalpha(enLayer, u8Alpha0);
}

/*
* Name : VOU_GraphicsSetCscEn
* Desc :
*/
HI_BOOL HIFB_GraphicsSetCscEn(GRAPHIC_LAYER u32Layer, HI_BOOL bCscEn)
{
    HAL_DISP_LAYER_E  enLayer = HAL_DISP_LAYER_BUTT;

    HIFB_DRV_GetLayerID(u32Layer, &enLayer);
    return HIFB_DRV_LAYER_SetCscEn(enLayer, bCscEn);
}

HI_BOOL HIFB_GraphicsSetGfxAddr(GRAPHIC_LAYER u32Layer, HI_U64 u64LAddr)
{
    HAL_DISP_LAYER_E  enLayer= HAL_DISP_LAYER_BUTT;

    HIFB_DRV_GetLayerID(u32Layer, &enLayer);
    return HIFB_Graphics_DRV_SetLayerAddr(enLayer, u64LAddr);
}

HI_BOOL HIFB_GraphicsSetGfxStride(GRAPHIC_LAYER u32Layer, HI_U16 u16pitch)
{
    HAL_DISP_LAYER_E enLayer = HAL_DISP_LAYER_BUTT;

    HIFB_DRV_GetLayerID(u32Layer, &enLayer);
    return HIFB_DRV_SetGfxStride(enLayer, u16pitch);
}

/*
* Name : VOU_GraphicsGetGfxPreMult
* Desc :
*/
HI_BOOL HIFB_GraphicsGetGfxPreMult(GRAPHIC_LAYER u32Layer, HI_U32 *pbEnable)
{
    HAL_DISP_LAYER_E enLayer = HAL_DISP_LAYER_BUTT;

    HIFB_DRV_GetLayerID(u32Layer, &enLayer);
    return HIFB_DRV_GetGfxPreMult(enLayer, pbEnable);
}

/*
* Name : VOU_GraphicsSetGfxPreMult
* Desc :
*/
HI_BOOL HIFB_GraphicsSetGfxPreMult(GRAPHIC_LAYER u32Layer, HI_U32 bEnable)
{
    HAL_DISP_LAYER_E enLayer = HAL_DISP_LAYER_BUTT;

    HIFB_DRV_GetLayerID(u32Layer, &enLayer);
    return HIFB_DRV_SetGfxPreMult(enLayer, bEnable);
}

/*
* Name : VOU_GraphicsSetLayerDataFmt
* Desc :
*/
HI_BOOL HIFB_GraphicsSetLayerDataFmt(GRAPHIC_LAYER u32Layer,
                                        VO_DISP_PIXEL_FORMAT_E  enDataFmt)
{
    HAL_DISP_LAYER_E         enLayer    = HAL_DISP_LAYER_BUTT;
    HAL_DISP_PIXEL_FORMAT_E  enPixFmt   = HAL_INPUTFMT_ARGB_1555;

    HIFB_GetGfxDcmpPixel(enDataFmt, &enPixFmt);

    HIFB_DRV_GetLayerID(u32Layer, &enLayer);
    return HIFB_Graphics_DRV_SetLayerDataFmt(enLayer, enPixFmt);
}

HI_BOOL HIFB_GraphicsSetLayerInRect(GRAPHIC_LAYER u32Layer, HIFB_RECT *pstRect)
{
    HAL_DISP_LAYER_E enLayer = HAL_DISP_LAYER_BUTT;

    HIFB_DRV_GetLayerID(u32Layer, &enLayer);
    return HIFB_DRV_SetLayerInRect(enLayer, pstRect);
}

HI_BOOL HIFB_GraphicsSetLayerSrcImageReso(GRAPHIC_LAYER u32Layer, HIFB_RECT *pstRect)
{
    HAL_DISP_LAYER_E enLayer = HAL_DISP_LAYER_BUTT;

    HIFB_DRV_GetLayerID(u32Layer, &enLayer);
    return HIFB_DRV_SetSrcImageResolution(enLayer, pstRect);
}

HI_BOOL HIFB_GraphicsSetLayerOutRect(GRAPHIC_LAYER u32Layer, HIFB_RECT *pstRect)
{
    HAL_DISP_LAYER_E enLayer = HAL_DISP_LAYER_BUTT;

    HIFB_DRV_GetLayerID(u32Layer, &enLayer);
    return HIFB_DRV_SetLayerOutRect(enLayer, pstRect);
}

/*
* Desc : set color key
*/
HI_BOOL HIFB_GraphicsSetColorKeyValue(GRAPHIC_LAYER u32Layer,
        VO_GFX_KEY_MAX_S stVoKeyMax, VO_GFX_KEY_MIN_S stVoKeyMin)
{
    HAL_DISP_LAYER_E  enLayer = HAL_DISP_LAYER_BUTT;

    HAL_GFX_KEY_MAX_S stKeyMax;
    HAL_GFX_KEY_MIN_S stKeyMin;
    stKeyMax.u8KeyMax_R = stVoKeyMax.u8KeyMax_R;
    stKeyMax.u8KeyMax_G = stVoKeyMax.u8KeyMax_G;
    stKeyMax.u8KeyMax_B = stVoKeyMax.u8KeyMax_B;
    stKeyMin.u8KeyMin_R = stVoKeyMin.u8KeyMin_R;
    stKeyMin.u8KeyMin_G = stVoKeyMin.u8KeyMin_G;
    stKeyMin.u8KeyMin_B = stVoKeyMin.u8KeyMin_B;
    HIFB_DRV_GetLayerID(u32Layer, &enLayer);
    return HIFB_DRV_SetColorKeyValue(enLayer, stKeyMax, stKeyMin);
}

HI_BOOL HIFB_GraphicsSetColorKeyMask(GRAPHIC_LAYER u32Layer, VO_GFX_MASK_S stVoMsk)
{
    HAL_DISP_LAYER_E    enLayer  = HAL_DISP_LAYER_BUTT;

    HAL_GFX_MASK_S      stMsk ;
    HIFB_DRV_GetLayerID(u32Layer, &enLayer);
    stMsk.u8Mask_r = stVoMsk.u8Mask_r;
    stMsk.u8Mask_g = stVoMsk.u8Mask_g;
    stMsk.u8Mask_b = stVoMsk.u8Mask_b;
    return HIFB_DRV_SetColorKeyMask(enLayer, stMsk);
}


HI_BOOL HIFB_GraphicsSetGfxKeyEn(GRAPHIC_LAYER u32Layer, HI_U32 u32KeyEnable)
{
    HAL_DISP_LAYER_E  enLayer = HAL_DISP_LAYER_BUTT;

    HIFB_DRV_GetLayerID(u32Layer, &enLayer);
    return HIFB_DRV_SetGfxKeyEn(enLayer, u32KeyEnable);
}


HI_BOOL  HIFB_GraphicsSetRegUp(GRAPHIC_LAYER u32Layer)
{
    HAL_DISP_LAYER_E enLayer = HAL_DISP_LAYER_BUTT;

    HIFB_DRV_GetLayerID(u32Layer, &enLayer);
    return HIFB_DRV_SetRegUp(enLayer);
}


HI_S32 HIFB_GraphicsInit(HI_VOID)
{
    HI_S32              s32ret     = HI_FAILURE;
    VO_DEV              VoDev      = VO_DEV_DHD0;
    HAL_DISP_SYNCINFO_S stSyncInfo = {0};
    HI_BOOL             bRet       = HI_FALSE;
    HI_BOOL             bVoEnable  = HI_FALSE;
    VO_INTF_TYPE_E      enIntfType = HAL_DISP_INTF_BT1120;
    HI_S32              i          = 0;

    s32ret = HIFB_GRAPHIC_DRV_Init();

    for(i = 0; i < VO_MAX_DEV_NUM; i++)
    {
        VoDev = i;
        bRet = HIFB_DRV_GetDevEnable(VoDev, &bVoEnable);
        if(HI_FALSE == bRet)
        {
            continue;
        }
        else
        {
            g_astVoDev[VoDev].bVoEnable = bVoEnable;
        }

        bRet = HIFB_DRV_GetIntfSync(VoDev,&stSyncInfo);
        if(HI_FALSE == bRet)
        {
            continue;
        }
        else
        {
            g_astVoDev[VoDev].u32MaxWidth = stSyncInfo.u16Hact;
            g_astVoDev[VoDev].u32MaxHeight = (stSyncInfo.bIop) ? stSyncInfo.u16Vact : stSyncInfo.u16Vact * 2;
        }

        bRet = HIFB_DRV_GetIntfMuxSel(VoDev,&enIntfType);
        if(HI_FALSE == bRet)
        {
            continue;
        }
        else
        {
            g_astVoDev[VoDev].enIntfType = enIntfType;
        }
     }

     return s32ret;
}

HI_S32 HIFB_GraphicsDeInit(HI_VOID)
{
    return HIFB_DRV_Exit();
}


HI_BOOL HIFB_GraphicsGetLayerGalpha(GRAPHIC_LAYER u32Layer,HI_U8 *pu8Alpha0)
{
    HAL_DISP_LAYER_E enLayer = HAL_DISP_LAYER_BUTT;

    HIFB_DRV_GetLayerID(u32Layer, &enLayer);
    return HIFB_DRV_GetLayerGalpha(enLayer, pu8Alpha0);
}

HI_BOOL HIFB_GraphicsGetLayerDataFmt(GRAPHIC_LAYER u32Layer,HI_U32 *pu32Fmt)
{
    HAL_DISP_LAYER_E enLayer = HAL_DISP_LAYER_BUTT;

    HIFB_DRV_GetLayerID(u32Layer, &enLayer);
    return HIFB_DRV_GetLayerDataFmt(enLayer, pu32Fmt);
}


HI_BOOL HIFB_GraphicsGetGfxAddr(GRAPHIC_LAYER u32Layer, HI_U64 *pu64GfxAddr)
{
    HAL_DISP_LAYER_E enLayer = HAL_DISP_LAYER_BUTT;

    HIFB_DRV_GetLayerID(u32Layer, &enLayer);
    return HIFB_DRV_GetGfxAddr(enLayer, pu64GfxAddr);
}


HI_BOOL HIFB_GraphicsGetGfxStride(GRAPHIC_LAYER u32Layer, HI_U32 *pu32GfxStride)
{
    HAL_DISP_LAYER_E enLayer = HAL_DISP_LAYER_BUTT;

    HIFB_DRV_GetLayerID(u32Layer, &enLayer);
    return HIFB_DRV_GetGfxStride(enLayer, pu32GfxStride);
}


HI_S32 HIFB_GraphicsGetDevMode(HAL_DISP_LAYER_E u32Layer, VOU_SCAN_MODE_E *pScanMode, HI_BOOL *pbFeildUpdate)

{
    VO_DEV                 VoDev;
    HI_U32                 u32LayerIndex;
    GFX_SPIN_LOCK_FLAG     lockFlag;
    VO_GFXLAYER_CONTEXT_S* pstVoGfxLayerCtx = NULL;
    HAL_DISP_LAYER_E       gfxLayer         = HAL_DISP_LAYER_BUTT;

    HIFB_DRV_GetLayerID(u32Layer, &gfxLayer);
    if (HI_SUCCESS != HIFB_DRV_GetLayerIndex(gfxLayer, &u32LayerIndex))
    {
        GRAPHICS_DRV_TRACE(HI_DBG_ERR, "gfxLayer(%u) is invalid!\n", (HI_U32)gfxLayer);
        return HI_ERR_VO_GFX_INVALID_ID;
    }

    pstVoGfxLayerCtx = &s_astGfxLayerCtx[u32LayerIndex];

    GFX_SPIN_LOCK_IRQSAVE(&pstVoGfxLayerCtx->spinLock, &lockFlag);
    if (!pstVoGfxLayerCtx->bBinded)
    {
        GFX_SPIN_UNLOCK_IRQRESTORE(&pstVoGfxLayerCtx->spinLock, &lockFlag);
        HIFB_GRAPHICS_TRACE(HI_DBG_ERR, "graphics layer %d has not been binded!\n", u32LayerIndex);
        return HI_ERR_VO_GFX_NOT_BIND;
    }
    VoDev = pstVoGfxLayerCtx->s32BindedDev;

    if (!g_astVoDev[VoDev].bVoEnable)
    {
        GFX_SPIN_UNLOCK_IRQRESTORE(&pstVoGfxLayerCtx->spinLock, &lockFlag);
        HIFB_GRAPHICS_TRACE(HI_DBG_ERR, "vodev %d for graphics layer %d has been disable!\n",\
            VoDev, u32LayerIndex);
        return HI_ERR_VO_DEV_NOT_ENABLE;
    }

    if (HI_FALSE == HIFB_DRV_GetScanMode(VoDev, (HI_BOOL *)pScanMode))
    {
        GFX_SPIN_UNLOCK_IRQRESTORE(&pstVoGfxLayerCtx->spinLock, &lockFlag);
        HIFB_GRAPHICS_TRACE(HI_DBG_ERR, "get vodev:%d scan mode failed!\n", VoDev);
        return HI_FAILURE;
    }

    if (HI_FALSE == HIFB_DRV_GetVtThdMode(VoDev, pbFeildUpdate))
    {
        GFX_SPIN_UNLOCK_IRQRESTORE(&pstVoGfxLayerCtx->spinLock, &lockFlag);
        HIFB_GRAPHICS_TRACE(HI_DBG_ERR, "get vodev:%d scan mode failed!\n", VoDev);
        return HI_FAILURE;
    }
    GFX_SPIN_UNLOCK_IRQRESTORE(&pstVoGfxLayerCtx->spinLock, &lockFlag);
    return HI_SUCCESS;
}
/*
* Name : VO_GraphicsVtthIntProcess
* Desc :
*/
HI_VOID HIFB_GraphicsVtthIntProcess(VO_DEV VoDev)
{
    GFX_SPIN_LOCK_FLAG     lockFlag;
    HI_S32                 i                = 0;
    HI_BOOL                bProgressvie     = HI_FALSE;
    HI_BOOL                bBottomInt       = HI_FALSE;
    FB_IntCallBack         pfVoCallBack;
    VO_GFXLAYER_CONTEXT_S* pstVoGfxLayerCtx = NULL;

    HIFB_DRV_GetScanMode(VoDev, &bProgressvie);
    HIFB_DRV_GetIntState(VoDev, &bBottomInt);


    for (i = 0; i < VO_MAX_GRAPHICS_LAYER_NUM; ++i)
    {
        pstVoGfxLayerCtx = &s_astGfxLayerCtx[i];
        GFX_SPIN_LOCK_IRQSAVE(&pstVoGfxLayerCtx->spinLock, &lockFlag);
        if (!pstVoGfxLayerCtx->bOpened)
        {
            GFX_SPIN_UNLOCK_IRQRESTORE(&pstVoGfxLayerCtx->spinLock, &lockFlag);
            continue;
        }

        if (pstVoGfxLayerCtx->bBinded && pstVoGfxLayerCtx->s32BindedDev == VoDev)
        {
            if (!bProgressvie && !bBottomInt)
            {
                /* do nothing */
            }
            else if (HI_NULL != pstVoGfxLayerCtx->pfVoCallBack)
            {
                pfVoCallBack = pstVoGfxLayerCtx->pfVoCallBack;
                GFX_SPIN_UNLOCK_IRQRESTORE(&pstVoGfxLayerCtx->spinLock, &lockFlag);

                pfVoCallBack(pstVoGfxLayerCtx->pVoCallBackArg, HI_NULL);
                continue;
            }
        }
        GFX_SPIN_UNLOCK_IRQRESTORE(&pstVoGfxLayerCtx->spinLock, &lockFlag);
    }

}

HI_S32 HIFB_GraphicsResourceInit(HI_VOID)
{
    HI_S32 s32Ret = 0;

    s32Ret = HIFB_DRV_Resource_Init();
    if(HI_SUCCESS != s32Ret)
    {
       HIFB_GRAPHICS_TRACE(HI_DBG_ERR, "graphic drv resource init failed.\n");
       return s32Ret;
    }

    return s32Ret;
}

HI_S32 HIFB_GraphicsResourceDeInit(HI_VOID)
{
    HI_S32 s32Ret = 0;
    s32Ret = HIFB_DRV_Resource_Exit();
    return s32Ret;
}

/*
* Name : VOU_GraphicsEnableLayer
* Desc :
*/
HI_S32 HIFB_GraphicsEnableLayer(GRAPHIC_LAYER u32Layer, HI_BOOL bEnable)
{
    HAL_DISP_LAYER_E gfxLayer = HAL_DISP_LAYER_BUTT;

    HIFB_DRV_GetLayerID(u32Layer, &gfxLayer);
    if((gfxLayer < LAYER_GFX_START) || (gfxLayer > LAYER_GFX_END))
    {
        return HI_ERR_VO_GFX_INVALID_ID;
    }
    return HIFB_GRAPHIC_DRV_EnableLayer(gfxLayer, bEnable);
}

/*
* Name : VOU_GraphicsSetCallback
* Desc :
* See  : VO_GraphicsVtthIntProcess
*/
HI_S32 HIFB_GraphicsSetCallback(GRAPHIC_LAYER u32Layer, VO_FB_INT_TYPE_E enType,
    VO_FB_IntCallBack pCallBack, HI_VOID *pArg)
{
    HAL_DISP_LAYER_E           gfxLayer         = HAL_DISP_LAYER_BUTT;

    GFX_SPIN_LOCK_FLAG         lockFlag;
    HI_U32                     u32LayerIndex;
    HI_S32                     s32Ret           = HI_SUCCESS;
    VO_GFXLAYER_CONTEXT_S*     pstVoGfxLayerCtx = NULL;

    HIFB_DRV_GetLayerID(u32Layer, &gfxLayer);
    if (HI_SUCCESS != HIFB_DRV_GetLayerIndex(gfxLayer, &u32LayerIndex))
    {
        GRAPHICS_DRV_TRACE(HI_DBG_ERR, "gfxLayer(%u) is invalid!\n", (HI_U32)gfxLayer);
        return HI_ERR_VO_GFX_INVALID_ID;
    }

    pstVoGfxLayerCtx = &s_astGfxLayerCtx[u32LayerIndex];

    GFX_SPIN_LOCK_IRQSAVE(&pstVoGfxLayerCtx->spinLock, &lockFlag);
    switch (enType)
    {
        case HIFB_INTTYPE_VO:
        {
            pstVoGfxLayerCtx->pfVoCallBack = (FB_IntCallBack)pCallBack;
            pstVoGfxLayerCtx->pVoCallBackArg = pArg;
            break ;
        }
        case HIFB_INTTYPE_WBC:
        {
            pstVoGfxLayerCtx->pfWbcCallBack = (FB_IntCallBack)pCallBack;
            pstVoGfxLayerCtx->pWbcCallBackArg = pArg;
            break ;
        }
        default :
        {
            s32Ret = HI_FAILURE;
            break ;
        }
    }
    GFX_SPIN_UNLOCK_IRQRESTORE(&pstVoGfxLayerCtx->spinLock, &lockFlag);

    return s32Ret;
}

/*
* Name : VOU_GraphicsGetIntfSize
* Desc :
* See  : HIFB_DRV_GetOSDData
*/
HI_S32 HIFB_GraphicsGetIntfSize(GRAPHIC_LAYER u32Layer, HI_U32 *pu32Width, HI_U32 *pu32Height)
{
    VO_DEV                     VoDev;
    HI_U32                     u32LayerIndex;
    VO_GFXLAYER_CONTEXT_S*     pstVoGfxLayerCtx = NULL;
    VOU_DEV_S*                  pstVoDev         = NULL;
    HAL_DISP_LAYER_E           gfxLayer         = HAL_DISP_LAYER_BUTT;

    HIFB_DRV_GetLayerID(u32Layer, &gfxLayer);
    if (HI_SUCCESS != HIFB_DRV_GetLayerIndex(gfxLayer, &u32LayerIndex))
    {
        GRAPHICS_DRV_TRACE(HI_DBG_ERR, "gfxLayer(%u) is invalid!\n", (HI_U32)gfxLayer);
        return HI_ERR_VO_GFX_INVALID_ID;
    }

    pstVoGfxLayerCtx = &s_astGfxLayerCtx[u32LayerIndex];

    if (!pstVoGfxLayerCtx->bBinded)
    {
        HIFB_GRAPHICS_TRACE(HI_DBG_ERR, "Graphics layer %d# has not been binded!\n", u32LayerIndex);
        return HI_FAILURE;
    }

    VoDev    = pstVoGfxLayerCtx->s32BindedDev;
    pstVoDev = &g_astVoDev[VoDev];

    if (!pstVoDev->bVoEnable)
    {
        HIFB_GRAPHICS_TRACE(HI_DBG_ERR, "The vo device (%d) for graphics layer %d has been disable!\n", VoDev, u32LayerIndex);
        return HI_FAILURE;
    }

    *pu32Width = pstVoDev->u32MaxWidth;
    *pu32Height= pstVoDev->u32MaxHeight;

    return HI_SUCCESS;
}

/*
* Name : VOU_GraphicsGetIntfType
* Desc :
*/
HI_S32 HIFB_GraphicsGetIntfType(GRAPHIC_LAYER u32Layer, VO_INTF_TYPE_E *penIntfType)
{
    HAL_DISP_OUTPUTCHANNEL_E enChan = HAL_DISP_CHANNEL_DHD0;
    HI_BOOL             bRet       = HI_FALSE;

    bRet = HIFB_HAL_DISP_GetIntfMuxSel(enChan , penIntfType);
    if (bRet==HI_FALSE)
    {
        GRAPHICS_DRV_TRACE(HI_DBG_ERR, "Get IntfMuxSel failed!\n");
        return HI_FAILURE;
    }
    return HI_SUCCESS;
}

extern HI_S32 HIFB_DRV_CalcCscMatrix(HI_U32 u32Luma, HI_U32 u32Contrast,
    HI_U32 u32Hue, HI_U32 u32Satuature, HAL_CSC_MODE_E enCscMode, CscCoef_S *pstCstCoef);

/*
* Name : VOU_GraphicsSetCscCoef
* Desc :
*/
HI_S32 HIFB_GraphicsSetCscCoef(GRAPHIC_LAYER u32Layer)
{
    HI_U32             u32LayerIndex ;
    HAL_DISP_LAYER_E   gfxLayer  = HAL_DISP_LAYER_BUTT;

    HIFB_DRV_GetLayerID(u32Layer, &gfxLayer);

    if (HI_SUCCESS != HIFB_DRV_GetLayerIndex(gfxLayer, &u32LayerIndex))
    {
        GRAPHICS_DRV_TRACE(HI_DBG_ERR, "gfxLayer(%u) is invalid!\n", (HI_U32)gfxLayer);
        return HI_ERR_VO_GFX_INVALID_ID;
    }

    return HIFB_DRV_SetCscCoef(gfxLayer, &s_astGfxLayerCtx[u32LayerIndex].stGfxCsc,&s_astGfxLayerCtx[u32LayerIndex].stCscCoefParam);
}

HI_S32 HIFB_GraphicsGetCSC(HI_U32 u32Layer, VO_CSC_S * pstCsc)
{
    HI_S32 s32Ret;
    HAL_DISP_LAYER_E gfxLayer;

    if (!HIFB_DRV_IsChipSupportCsc())
    {
        HIFB_GRAPHICS_TRACE(HI_DBG_ERR, "graphics layer %d is not support to get csc!\n", u32Layer);
        return HI_ERR_VO_NOT_SUPPORT;
    }

    GRAPHICS_CHECK_NULL_PTR(pstCsc);

    s32Ret = HIFB_DRV_GetHalLayer(u32Layer, &gfxLayer);
    if (HI_SUCCESS != s32Ret)
    {
        HIFB_GRAPHICS_TRACE(HI_DBG_ERR, "graphics layer %d is illegal!\n", u32Layer);
        return s32Ret;
    }

    /*get CSC*/
    osal_memcpy(pstCsc, &s_astGfxLayerCtx[u32Layer].stGfxCsc, sizeof(VO_CSC_S));

    return HI_SUCCESS;
}

#if 1

/* open the first time,check binding if has been build*/
HI_S32 HIFB_GraphicsOpenLayer(HAL_DISP_LAYER_E gfxLayer)
{
    VO_DEV VoDev;
    GFX_SPIN_LOCK_FLAG lockFlag = 0;
    HI_U32 u32LayerIndex;

    if (HI_SUCCESS != HIFB_DRV_GetLayerIndex(gfxLayer, &u32LayerIndex))
    {
        GRAPHICS_DRV_TRACE(HI_DBG_ERR, "gfxLayer(%u) is invalid!\n", (HI_U32)gfxLayer);
        return HI_ERR_VO_GFX_INVALID_ID;
    }

    GFX_SPIN_LOCK_IRQSAVE(&s_astGfxLayerCtx[u32LayerIndex].spinLock, &lockFlag);
    if (!s_astGfxLayerCtx[u32LayerIndex].bBinded)
    {
        GFX_SPIN_UNLOCK_IRQRESTORE(&s_astGfxLayerCtx[u32LayerIndex].spinLock, &lockFlag);
        HIFB_GRAPHICS_TRACE(HI_DBG_ERR, "graphics layer %d has not been binded!\n", u32LayerIndex);
        return HI_ERR_VO_GFX_NOT_BIND;
    }

    VoDev = s_astGfxLayerCtx[u32LayerIndex].s32BindedDev;

    if (!g_astVoDev[VoDev].bVoEnable)
    {
        GFX_SPIN_UNLOCK_IRQRESTORE(&s_astGfxLayerCtx[u32LayerIndex].spinLock, &lockFlag);
        HIFB_GRAPHICS_TRACE(HI_DBG_ERR, "vodev %d for graphics layer %d has not been enable!\n",\
            VoDev, u32LayerIndex);
        return HI_ERR_VO_DEV_NOT_ENABLE;
    }
    s_astGfxLayerCtx[u32LayerIndex].bOpened = HI_TRUE;
    GFX_SPIN_UNLOCK_IRQRESTORE(&s_astGfxLayerCtx[u32LayerIndex].spinLock, &lockFlag);
    return HI_SUCCESS;
}

HI_S32 HIFB_GraphicsCloseLayer(HAL_DISP_LAYER_E gfxLayer)
{
    GFX_SPIN_LOCK_FLAG lockFlag;
    HI_U32 u32LayerIndex;

    if (HI_SUCCESS != HIFB_DRV_GetLayerIndex(gfxLayer, &u32LayerIndex))
    {
        GRAPHICS_DRV_TRACE(HI_DBG_ERR, "gfxLayer(%u) is invalid!\n", (HI_U32)gfxLayer);
        return HI_ERR_VO_GFX_INVALID_ID;
    }

    GFX_SPIN_LOCK_IRQSAVE(&s_astGfxLayerCtx[u32LayerIndex].spinLock, &lockFlag);
    s_astGfxLayerCtx[u32LayerIndex].bOpened = HI_FALSE;
    GFX_SPIN_UNLOCK_IRQRESTORE(&s_astGfxLayerCtx[u32LayerIndex].spinLock, &lockFlag);

    return HI_SUCCESS;
}
#endif
HI_S32 HIFB_GraphicsShowProc(osal_proc_entry_t *s)
{
    VO_CSC_S stCSC;
    HI_S32 s32Dev;
    HI_S32 i;

    HIFB_DRV_ShowProc(s);

    osal_seq_printf(s,"\r\n");
    osal_seq_printf(s, "-----GRAPHIC LAYER CSC PARAM-----------------------------\n");
    /****************  1 2 3 4  5 6 7 8  9 0 1 2  3 4 5 6*****************************/
    osal_seq_printf(s, "%s%s%s%s%s%s\n",
        " LAYERID",
        "  Matrix",
        "    Luma",
        "    Cont",
        "     Hue",
        "    Satu"
        );
    for (i = 0; i < VO_MAX_GRAPHICS_LAYER_NUM; i++)
    {
        s32Dev = s_astGfxLayerCtx[i].s32BindedDev;
        if(g_astVoDev[s32Dev].bVoEnable)
        {
            stCSC = s_astGfxLayerCtx[i].stGfxCsc;
            osal_seq_printf(s, "%8u%8u%8u%8u%8u%8u\n"
            ,i
            ,stCSC.enCscMatrix
            ,stCSC.u32Luma
            ,stCSC.u32Contrast
            ,stCSC.u32Hue
            ,stCSC.u32Satuature
            );
        }
    }

    return HI_SUCCESS;
}

HI_S32 HIFB_GraphicsEnableInt(HI_U32 u32LayerIndex, HI_BOOL bEnable)
{
    VO_GFXLAYER_CONTEXT_S* pstVoGfxLayerCtx = NULL;
    VOU_DEV_S *             pstVoDev         = NULL;
    VO_DEV                 VoDev            = VO_DEV_DHD0;
    pstVoGfxLayerCtx = &s_astGfxLayerCtx[u32LayerIndex];

    if (!pstVoGfxLayerCtx->bBinded)
    {
        HIFB_GRAPHICS_TRACE(HI_DBG_ERR, "graphics layer %d has not been binded!\n", u32LayerIndex);
        return HI_ERR_VO_GFX_NOT_BIND;
    }
    VoDev = pstVoGfxLayerCtx->s32BindedDev;
    pstVoDev = &g_astVoDev[VoDev];

    if (HI_FALSE == pstVoDev->bVoEnable)
    {
        HIFB_GRAPHICS_TRACE(HI_DBG_ERR, "Open Int Error:The vo device (%d) for graphics layer %d has been disable!\n", VoDev, u32LayerIndex);
        return HI_FAILURE;
    }

    HIFB_DRV_DevIntEnable(VoDev, bEnable);
    return HI_SUCCESS;
}

HI_BOOL HIFB_GraphicsClearInt(HI_U32 u32IntClear, HI_S32 s32Irq)
{
    HIFB_DRV_IntClear(u32IntClear, s32Irq);
    return HI_SUCCESS;
}

HI_BOOL HIFB_GraphicsGetInt(HI_U32 * pu32IntStaus)
{
    *pu32IntStaus = HIFB_DRV_IntGetStatus();
    return HI_SUCCESS;
}

HI_BOOL HIFB_GraphicsClearIntStatus(HI_U32 u32IntStatus)
{
    HIFB_DRV_ClrIntStatus(u32IntStatus);
    return HI_SUCCESS;
}

HI_S32 HIFB_GraphicsGetInterruptDev(HI_U32 IntStatus, VO_DEV* pVoDev)
{
    return HIFB_DRV_GetInterruptDev(IntStatus, pVoDev);
}
