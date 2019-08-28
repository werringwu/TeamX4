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
#include "hi_debug.h"
#include "hifb_graphics_drv.h"
#include "hifb_coef.h"
#include "mddrc_reg.h"
#include "hi_math.h"
#include "hifb_graphic_hal.h"
#include "hifb_def.h"

/****************************************************************************
 * MACRO DEFINITION                                                         *
 ****************************************************************************/
#define HAL_PRINT HI_PRINT

#define DDRC_BASE_ADDR  0x04605000
#define VOU_REGS_ADDR   0x11280000
#define VOU_REGS_SIZE   0x40000
// For CMP and DCMP
#define CMP_SEG_OFFSET  (0x80/4)
#define DCMP_SEG_OFFSET (0x20/4)

/****************************************************************************
 * EXTERN VARIABLES                                                         *
 ****************************************************************************/

/****************************************************************************
 * GLOBAL VARIABLES                                                         *
 ****************************************************************************/
volatile S_VDP_REGS_TYPE*   pHifbReg     = NULL;
volatile MDDRC_REGS_S*      pMddrcReg  = NULL;

HI_VOID HIFB_HAL_VOU_Init(HI_VOID)
{
    if (HI_NULL == pHifbReg)
    {
        pHifbReg = (volatile S_VDP_REGS_TYPE*)osal_ioremap(VOU_REGS_ADDR, (HI_U32)VOU_REGS_SIZE);
    }

    if (HI_NULL == pHifbReg)
    {
        osal_printk("ioremap_nocache failed\n");
    }
}

HI_VOID HIFB_HAL_VOU_Exit(HI_VOID)
{
    if(HI_NULL != pHifbReg)
    {
        osal_iounmap((void *)pHifbReg);
	pHifbReg = HI_NULL;
    }
}
extern HIFB_COEF_ADDR_S g_stHifbCoefBufAddr;


/*****************************************************************************
 Prototype         : HAL_WriteReg
 Description       : write reg
*****************************************************************************/
HI_VOID HIFB_HAL_WriteReg(HI_U32* pAddress, HI_U32 Value)
{
    if (pAddress != NULL) {
        *(volatile HI_U32*)pAddress = Value;
    }
    return;
}

/*****************************************************************************
 Prototype         : HAL_ReadReg
 Description       : read reg
*****************************************************************************/
HI_U32 HIFB_HAL_ReadReg(HI_U32* pAddress)
{
    if (pAddress != NULL) {
        return *(volatile HI_U32*)(pAddress);
    } else {
        return 0;
    }
}

/*****************************************************************************
 Prototype         : Vou_GetAbsAddr
 Description       : Get the absolute address of the layer (video layer and graphics layer)
*****************************************************************************/
HI_UL HIFB_GetAbsAddr(HAL_DISP_LAYER_E enLayer, HI_UL pReg)
{
    HI_UL RegAbsAddr;

    switch (enLayer)
    {
        case HAL_DISP_LAYER_VHD0 :
        case HAL_DISP_LAYER_VHD1 :
        case HAL_DISP_LAYER_VHD2 :
        {
            RegAbsAddr = (pReg) + (enLayer - HAL_DISP_LAYER_VHD0) * VHD_REGS_LEN;
            break;
        }

        case HAL_DISP_LAYER_GFX0:
        case HAL_DISP_LAYER_GFX1:
        case HAL_DISP_LAYER_GFX3:
        {
            RegAbsAddr = (pReg) + (enLayer - HAL_DISP_LAYER_GFX0) * GFX_REGS_LEN;
            break;
        }

        // one wbc dev
        case HAL_DISP_LAYER_WBC:
            RegAbsAddr = (pReg);
            break;

        default:
        {
            HAL_PRINT("Error channel id found in %s: L%d\n", __FUNCTION__, __LINE__);
            return 0;
        }
    }

    return RegAbsAddr;
}

/*****************************************************************************
 Prototype         : Vou_GetChnAbsAddr
 Description       : Get the absolute address of the video channel
*****************************************************************************/
HI_UL HIFB_GetChnAbsAddr(HAL_DISP_OUTPUTCHANNEL_E enChan, HI_UL pReg)
{
    volatile HI_UL RegAbsAddr;

    switch (enChan)
    {
        case HAL_DISP_CHANNEL_DHD0:
        case HAL_DISP_CHANNEL_DHD1:
        {
            RegAbsAddr = pReg + (enChan - HAL_DISP_CHANNEL_DHD0) * DHD_REGS_LEN;
            break;
        }

        default:
        {
            HAL_PRINT("Error channel id found in %s: L%d\n", __FUNCTION__, __LINE__);
            return 0;
        }
    }

    return RegAbsAddr;
}

HI_UL HIFB_GetGfxAbsAddr(HAL_DISP_LAYER_E enLayer, HI_UL pReg)
{
    volatile HI_UL RegAbsAddr;

    switch (enLayer)
    {
        case HAL_DISP_LAYER_GFX0 :
        case HAL_DISP_LAYER_GFX1 :
        case HAL_DISP_LAYER_GFX3 :
        {
            RegAbsAddr = pReg + (enLayer - HAL_DISP_LAYER_GFX0) * GRF_REGS_LEN;
            break;
        }
        default:
        {
            HAL_PRINT("Error layer id found in FUNC:%s,LINE:%d\n", __FUNCTION__, __LINE__);
            return 0;
        }
    }

    return RegAbsAddr;
}

/*
* Name : HAL_DISP_GetIntfEnable
* Desc : Get the status (enable,disable status) of display interface.
*/
HI_BOOL HIFB_HAL_DISP_GetIntfEnable(HAL_DISP_OUTPUTCHANNEL_E enChan, HI_BOOL* pbIntfEn)
{
    volatile U_DHD0_CTRL DHD0_CTRL;
    volatile HI_UL addr_REG;
    if (pbIntfEn == NULL) {
        return HI_FALSE;
    }

    switch (enChan)
    {
        case HAL_DISP_CHANNEL_DHD0:
        case HAL_DISP_CHANNEL_DHD1:
        {
            addr_REG = HIFB_GetChnAbsAddr(enChan, (HI_UL)(HI_UINTPTR_T)&(pHifbReg->DHD0_CTRL.u32));
            DHD0_CTRL.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
            *pbIntfEn = DHD0_CTRL.bits.intf_en;
            break;
        }

        default:
        {
            HAL_PRINT("Error channel id found in %s: L%d\n", __FUNCTION__, __LINE__);
            return HI_FALSE;
        }
    }

    return HI_TRUE;
}


HI_BOOL HIFB_HAL_DISP_GetIntfSync(HAL_DISP_OUTPUTCHANNEL_E enChan, HAL_DISP_SYNCINFO_S *pstSyncInfo)
{
    volatile U_DHD0_CTRL   DHD0_CTRL;
    volatile U_DHD0_VSYNC1 DHD0_VSYNC1;
    volatile U_DHD0_VSYNC2 DHD0_VSYNC2;
    volatile U_DHD0_HSYNC1 DHD0_HSYNC1;
    volatile U_DHD0_HSYNC2 DHD0_HSYNC2;
    volatile U_DHD0_VPLUS1 DHD0_VPLUS1;
    volatile U_DHD0_VPLUS2 DHD0_VPLUS2;
    volatile U_DHD0_PWR    DHD0_PWR;
    volatile HI_UL         addr_REG;
    if (pstSyncInfo == NULL) {
        return HI_FALSE;
    }
    switch (enChan)
    {
        case HAL_DISP_CHANNEL_DHD0:
        case HAL_DISP_CHANNEL_DHD1:
        {

            addr_REG = HIFB_GetChnAbsAddr(enChan,(HI_UL)(HI_UINTPTR_T)&(pHifbReg->DHD0_CTRL.u32));

            DHD0_CTRL.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);

            pstSyncInfo->bIop = DHD0_CTRL.bits.iop;

            addr_REG = HIFB_GetChnAbsAddr(enChan,(HI_UL)(HI_UINTPTR_T)&(pHifbReg->DHD0_HSYNC1.u32));
            DHD0_HSYNC1.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
            pstSyncInfo->u16Hact = DHD0_HSYNC1.bits.hact + 1;
            pstSyncInfo->u16Hbb = DHD0_HSYNC1.bits.hbb + 1;

            addr_REG = HIFB_GetChnAbsAddr(enChan,(HI_UL)(HI_UINTPTR_T)&(pHifbReg->DHD0_HSYNC2.u32));
            DHD0_HSYNC2.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
            pstSyncInfo->u16Hmid = (0 == DHD0_HSYNC2.bits.hmid)?0:DHD0_HSYNC2.bits.hmid + 1;
            pstSyncInfo->u16Hfb = DHD0_HSYNC2.bits.hfb + 1;

            addr_REG = HIFB_GetChnAbsAddr(enChan,(HI_UL)(HI_UINTPTR_T)&(pHifbReg->DHD0_VSYNC1.u32));
            DHD0_VSYNC1.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
            pstSyncInfo->u16Vact = DHD0_VSYNC1.bits.vact + 1;
            pstSyncInfo->u16Vbb = DHD0_VSYNC1.bits.vbb + 1;

            addr_REG = HIFB_GetChnAbsAddr(enChan, (HI_UL)(HI_UINTPTR_T)&(pHifbReg->DHD0_VSYNC2.u32));
            DHD0_VSYNC2.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
            pstSyncInfo->u16Vfb = DHD0_VSYNC2.bits.vfb + 1;

            //Config VHD interface veritical bottom timming,no use in progressive mode
            addr_REG = HIFB_GetChnAbsAddr(enChan,(HI_UL)(HI_UINTPTR_T)&(pHifbReg->DHD0_VPLUS1.u32));
            DHD0_VPLUS1.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
            pstSyncInfo->u16Bvact = DHD0_VPLUS1.bits.bvact + 1;
            pstSyncInfo->u16Bvbb = DHD0_VPLUS1.bits.bvbb + 1;

            addr_REG = HIFB_GetChnAbsAddr(enChan, (HI_UL)(HI_UINTPTR_T)&(pHifbReg->DHD0_VPLUS2.u32));
            DHD0_VPLUS2.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
            pstSyncInfo->u16Bvfb = DHD0_VPLUS2.bits.bvfb + 1;

            //Config VHD interface veritical bottom timming,
            addr_REG = HIFB_GetChnAbsAddr(enChan,(HI_UL)(HI_UINTPTR_T)&(pHifbReg->DHD0_PWR.u32));
            DHD0_PWR.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
            pstSyncInfo->u16Hpw = DHD0_PWR.bits.hpw + 1;
            pstSyncInfo->u16Vpw = DHD0_PWR.bits.vpw + 1;

            break;
        }

        default:
        {
            HAL_PRINT("Error channel id found in %s: L%d\n", __FUNCTION__, __LINE__);
            return HI_FALSE;
        }
    }

    return HI_TRUE;
}


HI_BOOL  HIFB_HAL_DISP_GetIntfMuxSel(HAL_DISP_OUTPUTCHANNEL_E enChan,VO_INTF_TYPE_E *pbenIntfType)
{
    volatile U_VO_MUX VO_MUX;
    if (pbenIntfType == NULL) {
        return HI_FALSE;
    }

    if(enChan > HAL_DISP_CHANNEL_DHD1)
     {
         HAL_PRINT("Error channel id found in %s: L%d\n",__FUNCTION__, __LINE__);
         return HI_FALSE;
     }

    VO_MUX.u32 = HIFB_HAL_ReadReg((HI_U32*)&(pHifbReg->VO_MUX.u32));

    switch(VO_MUX.bits.digital_sel)
    {
        case 0:
        {
            *pbenIntfType = HAL_DISP_INTF_BT1120;
            break;
        }
        case 1:
        {
            *pbenIntfType = HAL_DISP_INTF_BT656;
            break;
        }
        case 2:
        {
            *pbenIntfType = HAL_DISP_INTF_LCD;
            break;
        }

        default:
        {
            return HI_FALSE;
        }
    }

    return HI_TRUE;
}


HI_BOOL HIFB_HAL_DISP_GetIntState(HAL_DISP_OUTPUTCHANNEL_E enChan, HI_BOOL* pbBottom)
{
    volatile U_DHD0_STATE DHD0_STATE;
    volatile HI_UL addr_REG;
    if (pbBottom == NULL) {
        return HI_FALSE;
    }
    switch (enChan)
    {
        case HAL_DISP_CHANNEL_DHD0:
        case HAL_DISP_CHANNEL_DHD1:
        {
            addr_REG = HIFB_GetChnAbsAddr(enChan, (HI_UL)(HI_UINTPTR_T)&(pHifbReg->DHD0_STATE.u32));
            DHD0_STATE.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
            *pbBottom = DHD0_STATE.bits.bottom_field;
            break;
        }

        default:
        {
            HAL_PRINT("Error channel id found in %s: L%d\n", __FUNCTION__, __LINE__);
            return HI_FALSE;
        }
    }

    return HI_TRUE;
}


/*****************************************************************************
 Prototype         : HAL_DISP_GetDispIoP
 Description       : Interlace or Progressive
*****************************************************************************/
HI_BOOL  HIFB_HAL_DISP_GetDispIoP(HAL_DISP_OUTPUTCHANNEL_E enChan, HI_BOOL* pbIop)
{
    U_DHD0_CTRL DHD0_CTRL;
    volatile  HI_UL addr_REG;
    if (pbIop == NULL) {
        return HI_FALSE;
    }
    switch (enChan)
    {
        case HAL_DISP_CHANNEL_DHD0:
        case HAL_DISP_CHANNEL_DHD1:
        {
            addr_REG = HIFB_GetChnAbsAddr(enChan, (HI_UL)(HI_UINTPTR_T)&(pHifbReg->DHD0_CTRL.u32));
            DHD0_CTRL.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
            *pbIop = DHD0_CTRL.bits.iop;
            break;
        }

        default:
        {
            HAL_PRINT("Error channel id found in %s: L%d\n", __FUNCTION__, __LINE__);
            return HI_FALSE;
        }
    }

    return HI_TRUE;
}


HI_BOOL HIFB_HAL_DISP_GetVtThdMode(HAL_DISP_OUTPUTCHANNEL_E enChan, HI_BOOL* pbFieldMode)
{
    volatile U_DHD0_VTTHD  DHD0_VTTHD;
    volatile  HI_UL addr_REG;
    if (pbFieldMode == NULL) {
        return HI_FALSE;
    }
    switch (enChan)
    {
        case HAL_DISP_CHANNEL_DHD0:
        case HAL_DISP_CHANNEL_DHD1:
        {
            addr_REG = HIFB_GetChnAbsAddr(enChan, (HI_UL)(HI_UINTPTR_T)&(pHifbReg->DHD0_VTTHD.u32));
            DHD0_VTTHD.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
            *pbFieldMode = DHD0_VTTHD.bits.thd1_mode;
            break;
        }
        default:
        {
            HAL_PRINT("Error channel id found in %s: L%d\n", __FUNCTION__, __LINE__);
            return HI_FALSE;
        }
    }

    return HI_TRUE;
}

/*
* Name : HAL_DISP_SetIntMask
* Desc : Set intterupt mask to open or close intterupt.
*/
HI_BOOL HIFB_HAL_DISP_SetIntMask(HI_U32 u32MaskEn)
{
    volatile U_VOINTMSK1 VOINTMSK1;
    /* Dispaly interrupt mask enable */
    VOINTMSK1.u32 = HIFB_HAL_ReadReg((HI_U32*) & (pHifbReg->VOINTMSK1.u32));
    VOINTMSK1.u32 = VOINTMSK1.u32 | u32MaskEn;
    HIFB_HAL_WriteReg((HI_U32*) & (pHifbReg->VOINTMSK1.u32), VOINTMSK1.u32);
    VOINTMSK1.u32 = HIFB_HAL_ReadReg((HI_U32*) & (pHifbReg->VOINTMSK1.u32));
    return HI_TRUE;
}

HI_BOOL  HIFB_HAL_DISP_ClrIntMask(HI_U32 u32MaskEn)
{
    volatile U_VOINTMSK1 VOINTMSK1;

    /* Dispaly interrupt mask enable */
    VOINTMSK1.u32 = HIFB_HAL_ReadReg((HI_U32*) & (pHifbReg->VOINTMSK1.u32));
    VOINTMSK1.u32 = VOINTMSK1.u32 & (~u32MaskEn);
    HIFB_HAL_WriteReg((HI_U32*) & (pHifbReg->VOINTMSK1.u32), VOINTMSK1.u32);

    return HI_TRUE;
}

//Get interrupt status
HI_U32 HIFB_HAL_DISP_GetIntStatus(HI_U32 u32IntMsk)
{
    volatile U_VOMSKINTSTA1 VOMSKINTSTA1;

    /* read interrupt status */
    VOMSKINTSTA1.u32 = HIFB_HAL_ReadReg((HI_U32*) & (pHifbReg->VOMSKINTSTA1.u32));

    return (VOMSKINTSTA1.u32 & u32IntMsk);
}

/*
* Name : HAL_DISP_ClearIntStatus
* Desc : Clear interrupt status.
*/
HI_BOOL HIFB_HAL_DISP_ClearIntStatus(HI_U32 u32IntMsk)
{
    /* read interrupt status */
    HIFB_HAL_WriteReg((HI_U32*) & (pHifbReg->VOMSKINTSTA.u32), u32IntMsk);
    return HI_TRUE;
}

HI_U32 HAL_DISP_GetRawIntStatus(HI_U32 u32IntMsk)
{
    volatile U_VOINTSTA VOINTSTA;

    /* read interrupt status */
    VOINTSTA.u32 = HIFB_HAL_ReadReg((HI_U32*) & (pHifbReg->VOINTSTA.u32));

    return (VOINTSTA.u32 & u32IntMsk);
}

HI_U32 HIFB_HAL_DISP_GetRawIntStatus1(HI_U32 u32IntMsk)
{
    volatile U_VOINTSTA1 VOINTSTA1;

    /* read interrupt status */
    VOINTSTA1.u32 = HIFB_HAL_ReadReg((HI_U32*) & (pHifbReg->VOINTSTA1.u32));

    return (VOINTSTA1.u32 & u32IntMsk);
}


/*
* Name : HAL_DISP_SetClkGateEnable
* Desc : Set VO Clock gate enable
*/
HI_BOOL HIFB_HAL_DISP_SetClkGateEnable(HI_U32 u32Data)
{
    volatile U_VOCTRL VOCTRL;

    VOCTRL.u32 = HIFB_HAL_ReadReg((HI_U32*) & (pHifbReg->VOCTRL.u32));
    VOCTRL.bits.vo_ck_gt_en = u32Data;
    HIFB_HAL_WriteReg((HI_U32*) & (pHifbReg->VOCTRL.u32), VOCTRL.u32);

    return HI_TRUE;
}

/*
* Name : HAL_DISP_SetRegUp
* Desc : Set device register update.
*/
HI_VOID HIFB_HAL_DISP_SetRegUp (HAL_DISP_OUTPUTCHANNEL_E enChan)
{
    volatile U_DHD0_CTRL DHD0_CTRL;
    volatile  HI_UL addr_REG;

    if (enChan > HAL_DISP_CHANNEL_DHD1)
    {
        HI_PRINT("Error,HAL_DISP_SetRegUp Select Wrong CHANNEL ID\n");
        return ;
    }

    addr_REG = HIFB_GetChnAbsAddr(enChan, (HI_UL)(HI_UINTPTR_T)&(pHifbReg->DHD0_CTRL.u32));
    DHD0_CTRL.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
    DHD0_CTRL.bits.regup = 0x1;
    HIFB_HAL_WriteReg((HI_U32*)(HI_UINTPTR_T)addr_REG, DHD0_CTRL.u32);
    return;
}

/*
* Name : HAL_DISP_SetRegUp
* Desc : Get device register update.
*/
HI_U32 HIFB_HAL_DISP_GetRegUp (HAL_DISP_OUTPUTCHANNEL_E enChan)
{
    HI_U32 u32Data;
    volatile U_DHD0_CTRL DHD0_CTRL;
    volatile  HI_UL addr_REG;

    addr_REG = HIFB_GetChnAbsAddr(enChan, (HI_UL)(HI_UINTPTR_T)&(pHifbReg->DHD0_CTRL.u32));
    DHD0_CTRL.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
    u32Data = DHD0_CTRL.bits.regup;
    return u32Data & 0x1;
}

HI_BOOL HIFB_HAL_VIDEO_SetLayerDispRect(HAL_DISP_LAYER_E enLayer, HIFB_RECT* pstRect)
{
    volatile U_G0_DFPOS G0_DFPOS;
    volatile U_G0_DLPOS G0_DLPOS;
    volatile  HI_UL addr_REG;
    if (pstRect == NULL) {
        return HI_FALSE;
    }
    switch (enLayer)
    {
        case HAL_DISP_LAYER_GFX0:
        case HAL_DISP_LAYER_GFX1:
        case HAL_DISP_LAYER_GFX3:
        {
            addr_REG = HIFB_GetAbsAddr(enLayer, (HI_UL)(HI_UINTPTR_T)&(pHifbReg->G0_DFPOS.u32));
            G0_DFPOS.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
            G0_DFPOS.bits.disp_xfpos = pstRect->x;
            G0_DFPOS.bits.disp_yfpos = pstRect->y;
            HIFB_HAL_WriteReg((HI_U32*)(HI_UINTPTR_T)addr_REG, G0_DFPOS.u32);

            addr_REG = HIFB_GetAbsAddr(enLayer, (HI_UL)(HI_UINTPTR_T)&(pHifbReg->G0_DLPOS.u32));
            G0_DLPOS.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
            G0_DLPOS.bits.disp_xlpos = pstRect->x + pstRect->w - 1;
            G0_DLPOS.bits.disp_ylpos = pstRect->y + pstRect->h - 1;
            HIFB_HAL_WriteReg((HI_U32*)(HI_UINTPTR_T)addr_REG, G0_DLPOS.u32);
            break;
        }

        default:
        {
            HAL_PRINT("Error layer id found in %s: L%d\n", __FUNCTION__, __LINE__);
            return HI_FALSE;
        }
    }

    return HI_TRUE;
}

//Set the video image display area window
HI_BOOL HIFB_HAL_VIDEO_SetLayerVideoRect(HAL_DISP_LAYER_E enLayer, HIFB_RECT* pstRect)
{
    volatile U_G0_VFPOS G0_VFPOS;
    volatile U_G0_VLPOS G0_VLPOS;
    volatile  HI_UL addr_REG;
    if (pstRect == NULL) {
        return HI_FALSE;
    }
    switch (enLayer)
    {
        case HAL_DISP_LAYER_GFX0:
        case HAL_DISP_LAYER_GFX1:
        case HAL_DISP_LAYER_GFX3:
        {
            addr_REG = HIFB_GetAbsAddr(enLayer, (HI_UL)(HI_UINTPTR_T)&(pHifbReg->G0_VFPOS.u32));
            G0_VFPOS.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
            G0_VFPOS.bits.video_xfpos = pstRect->x;
            G0_VFPOS.bits.video_yfpos = pstRect->y;
            HIFB_HAL_WriteReg((HI_U32*)(HI_UINTPTR_T)addr_REG, G0_VFPOS.u32);

            addr_REG = HIFB_GetAbsAddr(enLayer, (HI_UL)(HI_UINTPTR_T)&(pHifbReg->G0_VLPOS.u32));
            G0_VLPOS.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
            G0_VLPOS.bits.video_xlpos = pstRect->x + pstRect->w - 1;
            G0_VLPOS.bits.video_ylpos = pstRect->y + pstRect->h - 1;
            HIFB_HAL_WriteReg((HI_U32*)(HI_UINTPTR_T)addr_REG, G0_VLPOS.u32);
            break;

        }

        default:
        {
            HAL_PRINT("Error layer id %d# found in %s,%s: L%d\n", enLayer, __FILE__, __FUNCTION__, __LINE__);
            return HI_FALSE;
        }
    }

    return HI_TRUE;
}
/*
* Name : HAL_LAYER_EnableLayer
* Desc : Set layer enable
*/
HI_BOOL HIFB_HAL_LAYER_EnableLayer(HAL_DISP_LAYER_E enLayer, HI_U32 bEnable)

{
    volatile U_V0_CTRL V0_CTRL;
    volatile U_G0_CTRL G0_CTRL;
    volatile  HI_UL addr_REG;

    switch (enLayer)
    {
        case HAL_DISP_LAYER_VHD0:
        case HAL_DISP_LAYER_VHD1:
        case HAL_DISP_LAYER_VHD2:
        {
            addr_REG = HIFB_GetAbsAddr(enLayer, (HI_UL)(HI_UINTPTR_T)&(pHifbReg->V0_CTRL.u32));
            V0_CTRL.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
            V0_CTRL.bits.surface_en = bEnable;
            HIFB_HAL_WriteReg((HI_U32*)(HI_UINTPTR_T)addr_REG, V0_CTRL.u32);
            break;
        }

        case HAL_DISP_LAYER_GFX0:
        case HAL_DISP_LAYER_GFX1:
        case HAL_DISP_LAYER_GFX3:
        {
            addr_REG = HIFB_GetAbsAddr(enLayer, (HI_UL)(HI_UINTPTR_T)&(pHifbReg->G0_CTRL.u32));
            G0_CTRL.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
            G0_CTRL.bits.surface_en = bEnable;
            HIFB_HAL_WriteReg((HI_U32*)(HI_UINTPTR_T)addr_REG, G0_CTRL.u32);
            break;
        }

        default:
        {
            HAL_PRINT("Error layer id found in %s: L%d\n", __FUNCTION__, __LINE__);
            return HI_FALSE;
        }
    }

    return HI_TRUE;
}

HI_BOOL HIFB_HAL_LAYER_GetLayerEnable(HAL_DISP_LAYER_E enLayer, HI_U32* pu32Enable)
{
    volatile U_V0_CTRL V0_CTRL;
    volatile U_G0_CTRL G0_CTRL;

    volatile  HI_UL addr_REG;
    if (pu32Enable == NULL) {
        return HI_FALSE;
    }
    switch (enLayer)
    {

        case HAL_DISP_LAYER_VHD0:
        case HAL_DISP_LAYER_VHD1:
        case HAL_DISP_LAYER_VHD2:
        {
            addr_REG = HIFB_GetAbsAddr(enLayer, (HI_UL)(HI_UINTPTR_T)&(pHifbReg->V0_CTRL.u32));
            V0_CTRL.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
            *pu32Enable = V0_CTRL.bits.surface_en;
            break;
        }

        case HAL_DISP_LAYER_GFX0:
        case HAL_DISP_LAYER_GFX1:
        case HAL_DISP_LAYER_GFX3:
        {
            addr_REG = HIFB_GetAbsAddr(enLayer, (HI_UL)(HI_UINTPTR_T)&(pHifbReg->G0_CTRL.u32));
            G0_CTRL.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
            *pu32Enable = G0_CTRL.bits.surface_en;
            break;
        }

        default:
        {
            HAL_PRINT("Error layer id found in %s: L%d\n", __FUNCTION__, __LINE__);
            return HI_FALSE;
        }
    }

    return HI_TRUE;
}

/*Desc : Set layer data type*/
HI_BOOL HIFB_HAL_LAYER_SetLayerDataFmt(HAL_DISP_LAYER_E enLayer,
                                  HAL_DISP_PIXEL_FORMAT_E  enDataFmt)
{
    volatile U_GFX_SRC_INFO GFX_SRC_INFO;
    volatile  HI_UL addr_REG;

    if (HAL_DISP_LAYER_GFX0 == enLayer ||
             HAL_DISP_LAYER_GFX1 == enLayer ||
             HAL_DISP_LAYER_GFX3 == enLayer)
    {
        addr_REG = HIFB_GetGfxAbsAddr(enLayer, (HI_UL)(HI_UINTPTR_T)&(pHifbReg->GFX_SRC_INFO.u32));
        GFX_SRC_INFO.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
        GFX_SRC_INFO.bits.ifmt = enDataFmt;
        HIFB_HAL_WriteReg((HI_U32*)(HI_UINTPTR_T)addr_REG, GFX_SRC_INFO.u32);
    }
    else
    {
        HAL_PRINT("Error layer id%d found in %s: L%d\n", enLayer, __FUNCTION__, __LINE__);
        return HI_FALSE;
    }

    return HI_TRUE;
}

HI_BOOL HIFB_HAL_LAYER_GetLayerDataFmt(HAL_DISP_LAYER_E enLayer, HI_U32* pu32Fmt)
{
    volatile U_GFX_SRC_INFO GFX_SRC_INFO;
    volatile  HI_UL addr_REG;
    if (pu32Fmt == NULL) {
        return HI_FALSE;
    }

    if(HAL_DISP_LAYER_GFX0 == enLayer ||
            HAL_DISP_LAYER_GFX1 == enLayer ||
            HAL_DISP_LAYER_GFX3 == enLayer)
    {
        addr_REG = HIFB_GetGfxAbsAddr(enLayer,(HI_UL)(HI_UINTPTR_T)&(pHifbReg->GFX_SRC_INFO.u32));
        GFX_SRC_INFO.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
        *pu32Fmt = GFX_SRC_INFO.bits.ifmt;
    }
    else
    {
        HAL_PRINT("Error layer id found in %s: L%d\n",__FUNCTION__, __LINE__);
        return HI_FALSE;
    }

    return HI_TRUE;
}

/**********************************************************************************
*  Begin  : Video   layer CSC relative hal functions.
**********************************************************************************/
HI_VOID HIFB_HAL_LAYER_CSC_SetEnable(HAL_DISP_LAYER_E enLayer, HI_BOOL csc_en)
{
    U_G0_HIPP_CSC_CTRL G0_HIPP_CSC_CTRL;
    volatile HI_U32 addr_REG;

    if (enLayer >= LAYER_GFX_START && enLayer <= LAYER_GFX_END)
    {
        addr_REG = HIFB_GetAbsAddr(enLayer,(HI_UL)(HI_UINTPTR_T)&(pHifbReg->G0_HIPP_CSC_CTRL.u32));
        G0_HIPP_CSC_CTRL.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
        G0_HIPP_CSC_CTRL.bits.hipp_csc_en = csc_en;
        HIFB_HAL_WriteReg((HI_U32*)(HI_UINTPTR_T)addr_REG, G0_HIPP_CSC_CTRL.u32);

    }
}

HI_VOID HIFB_HAL_LAYER_CSC_SetCkGtEn(HAL_DISP_LAYER_E enLayer, HI_BOOL ck_gt_en)
{
    U_G0_HIPP_CSC_CTRL     G0_HIPP_CSC_CTRL;

    volatile HI_U32 addr_REG;

    if (enLayer >= LAYER_GFX_START && enLayer <= LAYER_GFX_END)
    {
        addr_REG = HIFB_GetAbsAddr(enLayer,(HI_UL)(HI_UINTPTR_T)&(pHifbReg->G0_HIPP_CSC_CTRL.u32));
        G0_HIPP_CSC_CTRL.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
        G0_HIPP_CSC_CTRL.bits.hipp_csc_ck_gt_en = ck_gt_en;
        HIFB_HAL_WriteReg((HI_U32*)(HI_UINTPTR_T)addr_REG, G0_HIPP_CSC_CTRL.u32);
    }
}

HI_VOID HIFB_HAL_LAYER_CSC_SetCoef(HAL_DISP_LAYER_E enLayer, VDP_CSC_COEF_S *pstCscCoef)
{
    U_G0_HIPP_CSC_COEF00   G0_HIPP_CSC_COEF00;
    U_G0_HIPP_CSC_COEF01   G0_HIPP_CSC_COEF01;
    U_G0_HIPP_CSC_COEF02   G0_HIPP_CSC_COEF02;
    U_G0_HIPP_CSC_COEF10   G0_HIPP_CSC_COEF10;
    U_G0_HIPP_CSC_COEF11   G0_HIPP_CSC_COEF11;
    U_G0_HIPP_CSC_COEF12   G0_HIPP_CSC_COEF12;
    U_G0_HIPP_CSC_COEF20   G0_HIPP_CSC_COEF20;
    U_G0_HIPP_CSC_COEF21   G0_HIPP_CSC_COEF21;
    U_G0_HIPP_CSC_COEF22   G0_HIPP_CSC_COEF22;
    volatile HI_U32 addr_REG;
    if (pstCscCoef == NULL) {
        return;
    }

    if(enLayer >= HAL_DISP_LAYER_GFX0 && enLayer <= HAL_DISP_LAYER_GFX3)
    {
        addr_REG = HIFB_GetAbsAddr(enLayer,(HI_UL)(HI_UINTPTR_T)&(pHifbReg->G0_HIPP_CSC_COEF00.u32));
        G0_HIPP_CSC_COEF00.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
        G0_HIPP_CSC_COEF00.bits.hipp_csc_coef00 = pstCscCoef->csc_coef00;
        HIFB_HAL_WriteReg((HI_U32*)(HI_UINTPTR_T)addr_REG, G0_HIPP_CSC_COEF00.u32);

        addr_REG = HIFB_GetAbsAddr(enLayer,(HI_UL)(HI_UINTPTR_T)&(pHifbReg->G0_HIPP_CSC_COEF01.u32));
        G0_HIPP_CSC_COEF01.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
        G0_HIPP_CSC_COEF01.bits.hipp_csc_coef01 = pstCscCoef->csc_coef01;
        HIFB_HAL_WriteReg((HI_U32*)(HI_UINTPTR_T)addr_REG, G0_HIPP_CSC_COEF01.u32);

        addr_REG = HIFB_GetAbsAddr(enLayer,(HI_UL)(HI_UINTPTR_T)&(pHifbReg->G0_HIPP_CSC_COEF02.u32));
        G0_HIPP_CSC_COEF02.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
        G0_HIPP_CSC_COEF02.bits.hipp_csc_coef02 = pstCscCoef->csc_coef02;
        HIFB_HAL_WriteReg((HI_U32*)(HI_UINTPTR_T)addr_REG, G0_HIPP_CSC_COEF02.u32);

        addr_REG = HIFB_GetAbsAddr(enLayer,(HI_UL)(HI_UINTPTR_T)&(pHifbReg->G0_HIPP_CSC_COEF10.u32));
        G0_HIPP_CSC_COEF10.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
        G0_HIPP_CSC_COEF10.bits.hipp_csc_coef10 = pstCscCoef->csc_coef10;
        HIFB_HAL_WriteReg((HI_U32*)(HI_UINTPTR_T)addr_REG, G0_HIPP_CSC_COEF10.u32);

        addr_REG = HIFB_GetAbsAddr(enLayer,(HI_UL)(HI_UINTPTR_T)&(pHifbReg->G0_HIPP_CSC_COEF11.u32));
        G0_HIPP_CSC_COEF11.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
        G0_HIPP_CSC_COEF11.bits.hipp_csc_coef11 = pstCscCoef->csc_coef11;
        HIFB_HAL_WriteReg((HI_U32*)(HI_UINTPTR_T)addr_REG, G0_HIPP_CSC_COEF11.u32);

        addr_REG = HIFB_GetAbsAddr(enLayer,(HI_UL)(HI_UINTPTR_T)&(pHifbReg->G0_HIPP_CSC_COEF12.u32));
        G0_HIPP_CSC_COEF12.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
        G0_HIPP_CSC_COEF12.bits.hipp_csc_coef12 = pstCscCoef->csc_coef12;
        HIFB_HAL_WriteReg((HI_U32*)(HI_UINTPTR_T)addr_REG, G0_HIPP_CSC_COEF12.u32);

        addr_REG = HIFB_GetAbsAddr(enLayer,(HI_UL)(HI_UINTPTR_T)&(pHifbReg->G0_HIPP_CSC_COEF20.u32));
        G0_HIPP_CSC_COEF20.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
        G0_HIPP_CSC_COEF20.bits.hipp_csc_coef20 = pstCscCoef->csc_coef20;
        HIFB_HAL_WriteReg((HI_U32*)(HI_UINTPTR_T)addr_REG, G0_HIPP_CSC_COEF20.u32);

        addr_REG = HIFB_GetAbsAddr(enLayer,(HI_UL)(HI_UINTPTR_T)&(pHifbReg->G0_HIPP_CSC_COEF21.u32));
        G0_HIPP_CSC_COEF21.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
        G0_HIPP_CSC_COEF21.bits.hipp_csc_coef21 = pstCscCoef->csc_coef21;
        HIFB_HAL_WriteReg((HI_U32*)(HI_UINTPTR_T)addr_REG, G0_HIPP_CSC_COEF21.u32);

        addr_REG = HIFB_GetAbsAddr(enLayer,(HI_UL)(HI_UINTPTR_T)&(pHifbReg->G0_HIPP_CSC_COEF22.u32));
        G0_HIPP_CSC_COEF22.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
        G0_HIPP_CSC_COEF22.bits.hipp_csc_coef22 = pstCscCoef->csc_coef22;
        HIFB_HAL_WriteReg((HI_U32*)(HI_UINTPTR_T)addr_REG, G0_HIPP_CSC_COEF22.u32);
    }
    else
    {
        HAL_PRINT("Error layer id found in %s,%s,%d\n",__FILE__, __FUNCTION__, __LINE__);
    }

    return ;
}


HI_VOID HIFB_HAL_LAYER_CSC_SetDcCoef(HAL_DISP_LAYER_E enLayer, VDP_CSC_DC_COEF_S *pstCscDcCoef)
{
    U_G0_HIPP_CSC_IDC0 G0_HIPP_CSC_IDC0;
    U_G0_HIPP_CSC_IDC1 G0_HIPP_CSC_IDC1;
    U_G0_HIPP_CSC_IDC2 G0_HIPP_CSC_IDC2;
    U_G0_HIPP_CSC_ODC0 G0_HIPP_CSC_ODC0;
    U_G0_HIPP_CSC_ODC1 G0_HIPP_CSC_ODC1;
    U_G0_HIPP_CSC_ODC2 G0_HIPP_CSC_ODC2;
    volatile HI_U32 addr_REG;
    if (pstCscDcCoef == NULL) {
        return;
    }

    if(enLayer == HAL_DISP_LAYER_GFX0)
    {
        addr_REG = HIFB_GetAbsAddr(enLayer,(HI_UL)(HI_UINTPTR_T)&(pHifbReg->G0_HIPP_CSC_IDC0.u32));
        G0_HIPP_CSC_IDC0.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
        G0_HIPP_CSC_IDC0.bits.hipp_csc_idc0 = pstCscDcCoef->csc_in_dc0;
        HIFB_HAL_WriteReg((HI_U32*)(HI_UINTPTR_T)addr_REG, G0_HIPP_CSC_IDC0.u32);

        addr_REG = HIFB_GetAbsAddr(enLayer,(HI_UL)(HI_UINTPTR_T)&(pHifbReg->G0_HIPP_CSC_IDC1.u32));
        G0_HIPP_CSC_IDC1.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
        G0_HIPP_CSC_IDC1.bits.hipp_csc_idc1 = pstCscDcCoef->csc_in_dc1;
        HIFB_HAL_WriteReg((HI_U32*)(HI_UINTPTR_T)addr_REG, G0_HIPP_CSC_IDC1.u32);

        addr_REG = HIFB_GetAbsAddr(enLayer,(HI_UL)(HI_UINTPTR_T)&(pHifbReg->G0_HIPP_CSC_IDC2.u32));
        G0_HIPP_CSC_IDC2.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
        G0_HIPP_CSC_IDC2.bits.hipp_csc_idc2 = pstCscDcCoef->csc_in_dc2;
        HIFB_HAL_WriteReg((HI_U32*)(HI_UINTPTR_T)addr_REG, G0_HIPP_CSC_IDC2.u32);

        addr_REG = HIFB_GetAbsAddr(enLayer,(HI_UL)(HI_UINTPTR_T)&(pHifbReg->G0_HIPP_CSC_ODC0.u32));
        G0_HIPP_CSC_ODC0.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
        G0_HIPP_CSC_ODC0.bits.hipp_csc_odc0 = pstCscDcCoef->csc_out_dc0;
        HIFB_HAL_WriteReg((HI_U32*)(HI_UINTPTR_T)addr_REG, G0_HIPP_CSC_ODC0.u32);

        addr_REG = HIFB_GetAbsAddr(enLayer,(HI_UL)(HI_UINTPTR_T)&(pHifbReg->G0_HIPP_CSC_ODC1.u32));
        G0_HIPP_CSC_ODC1.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
        G0_HIPP_CSC_ODC1.bits.hipp_csc_odc1 = pstCscDcCoef->csc_out_dc1;
        HIFB_HAL_WriteReg((HI_U32*)(HI_UINTPTR_T)addr_REG, G0_HIPP_CSC_ODC1.u32);

        addr_REG = HIFB_GetAbsAddr(enLayer,(HI_UL)(HI_UINTPTR_T)&(pHifbReg->G0_HIPP_CSC_ODC2.u32));
        G0_HIPP_CSC_ODC2.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
        G0_HIPP_CSC_ODC2.bits.hipp_csc_odc2 = pstCscDcCoef->csc_out_dc2;
        HIFB_HAL_WriteReg((HI_U32*)(HI_UINTPTR_T)addr_REG, G0_HIPP_CSC_ODC2.u32);
    }
    else
    {
        HAL_PRINT("Error layer id found in %s,%s,%d\n",__FILE__, __FUNCTION__, __LINE__);
    }

}

HI_VOID HIFB_HAL_LAYER_CSC_SetParam(HAL_DISP_LAYER_E enLayer, CscCoefParam_S *pstCoefParam)
{
    U_G0_HIPP_CSC_SCALE    G0_HIPP_CSC_SCALE;
    U_G0_HIPP_CSC_MIN_Y    G0_HIPP_CSC_MIN_Y;
    U_G0_HIPP_CSC_MIN_C    G0_HIPP_CSC_MIN_C;
    U_G0_HIPP_CSC_MAX_Y    G0_HIPP_CSC_MAX_Y;
    U_G0_HIPP_CSC_MAX_C    G0_HIPP_CSC_MAX_C;
    volatile HI_U32 addr_REG;
    if (pstCoefParam == NULL) {
        return;
    }

    if (enLayer >= LAYER_GFX_START && enLayer <= LAYER_GFX_END)
    {
        addr_REG = HIFB_GetAbsAddr(enLayer,(HI_UL)(HI_UINTPTR_T)&(pHifbReg->G0_HIPP_CSC_SCALE.u32));
        G0_HIPP_CSC_SCALE.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
        G0_HIPP_CSC_SCALE.bits.hipp_csc_scale = pstCoefParam->csc_scale2p;
        HIFB_HAL_WriteReg((HI_U32*)(HI_UINTPTR_T)addr_REG, G0_HIPP_CSC_SCALE.u32);

        addr_REG = HIFB_GetAbsAddr(enLayer,(HI_UL)(HI_UINTPTR_T)&(pHifbReg->G0_HIPP_CSC_MIN_Y.u32));
        G0_HIPP_CSC_MIN_Y.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
        G0_HIPP_CSC_MIN_Y.bits.hipp_csc_min_y = pstCoefParam->csc_clip_min;
        HIFB_HAL_WriteReg((HI_U32*)(HI_UINTPTR_T)addr_REG, G0_HIPP_CSC_MIN_Y.u32);

        addr_REG = HIFB_GetAbsAddr(enLayer,(HI_UL)(HI_UINTPTR_T)&(pHifbReg->G0_HIPP_CSC_MIN_C.u32));
        G0_HIPP_CSC_MIN_C.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
        G0_HIPP_CSC_MIN_C.bits.hipp_csc_min_c = pstCoefParam->csc_clip_min;
        HIFB_HAL_WriteReg((HI_U32*)(HI_UINTPTR_T)addr_REG, G0_HIPP_CSC_MIN_C.u32);

        addr_REG = HIFB_GetAbsAddr(enLayer,(HI_UL)(HI_UINTPTR_T)&(pHifbReg->G0_HIPP_CSC_MAX_Y.u32));
        G0_HIPP_CSC_MAX_Y.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
        G0_HIPP_CSC_MAX_Y.bits.hipp_csc_max_y = pstCoefParam->csc_clip_max;
        HIFB_HAL_WriteReg((HI_U32*)(HI_UINTPTR_T)addr_REG, G0_HIPP_CSC_MAX_Y.u32);

        addr_REG = HIFB_GetAbsAddr(enLayer,(HI_UL)(HI_UINTPTR_T)&(pHifbReg->G0_HIPP_CSC_MAX_C.u32));
        G0_HIPP_CSC_MAX_C.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
        G0_HIPP_CSC_MAX_C.bits.hipp_csc_max_c = pstCoefParam->csc_clip_max;
        HIFB_HAL_WriteReg((HI_U32*)(HI_UINTPTR_T)addr_REG, G0_HIPP_CSC_MAX_C.u32);
    }
}

HI_BOOL HIFB_HAL_LAYER_SetCscCoef(HAL_DISP_LAYER_E enLayer, CscCoef_S* pstCscCoef)
{
    if (pstCscCoef == NULL) {
        return HI_FALSE;
    }

    if(enLayer < HAL_DISP_LAYER_VHD0 || enLayer > HAL_DISP_LAYER_GFX3)
    {
        HAL_PRINT("Error, Wrong layer ID!%d\n",__LINE__);
        return HI_FALSE;
    }

    HIFB_HAL_LAYER_CSC_SetDcCoef(enLayer, (VDP_CSC_DC_COEF_S *)(&pstCscCoef->csc_in_dc0));
    HIFB_HAL_LAYER_CSC_SetCoef(enLayer, (VDP_CSC_COEF_S *)(&pstCscCoef->csc_coef00));
    HIFB_HAL_LAYER_CSC_SetParam(enLayer, (CscCoefParam_S *)(&pstCscCoef->new_csc_scale2p));

    return HI_TRUE;
}

HI_BOOL HIFB_HAL_LAYER_SetCscMode(HAL_DISP_LAYER_E enLayer, HI_BOOL bIsHCModeBy709)
{
    if(enLayer < HAL_DISP_LAYER_VHD0 || enLayer > HAL_DISP_LAYER_GFX3)
    {
        HAL_PRINT("Error, Wrong layer ID!%d\n",__LINE__);
        return HI_FALSE;
    }

    return HI_TRUE;
}

HI_BOOL HIFB_HAL_LAYER_SetCscEn(HAL_DISP_LAYER_E enLayer, HI_BOOL bCscEn)
{
    if(enLayer < HAL_DISP_LAYER_VHD0 || enLayer > HAL_DISP_LAYER_GFX3)
    {
        HAL_PRINT("Error, Wrong layer ID!%d\n",__LINE__);
        return HI_FALSE;
    }

    HIFB_HAL_LAYER_CSC_SetCkGtEn(enLayer, bCscEn);
    HIFB_HAL_LAYER_CSC_SetEnable(enLayer, bCscEn);

    return HI_TRUE;
}

HI_BOOL HIFB_HAL_LAYER_SetSrcResolution(HAL_DISP_LAYER_E enLayer, HIFB_RECT *  pstRect)
{
    U_GFX_SRC_RESO GFX_SRC_RESO;
    volatile  HI_UL addr_REG;
    if (pstRect == NULL) {
        return HI_FALSE;
    }

    if(enLayer >= LAYER_GFX_START && enLayer <= LAYER_GFX_END)
    {
        addr_REG = HIFB_GetGfxAbsAddr(enLayer,(HI_UL)(HI_UINTPTR_T)&(pHifbReg->GFX_SRC_RESO.u32));
        GFX_SRC_RESO.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
        GFX_SRC_RESO.bits.src_w = pstRect->w- 1;
        GFX_SRC_RESO.bits.src_h = pstRect->h- 1;
        HIFB_HAL_WriteReg((HI_U32*)(HI_UINTPTR_T)addr_REG, GFX_SRC_RESO.u32);
    }
    else
    {
        HAL_PRINT("Error:layer id not found in %s: L%d\n",__FUNCTION__, __LINE__);
        return HI_FALSE;
    }

    return HI_TRUE;
}

HI_BOOL HIFB_HAL_LAYER_SetLayerInRect(HAL_DISP_LAYER_E enLayer, HIFB_RECT* pstRect)
{
    U_GFX_IRESO   GFX_IRESO;
    volatile  HI_UL addr_REG;
    if (pstRect == NULL) {
        return HI_FALSE;
    }
    if(enLayer >= LAYER_GFX_START && enLayer <= LAYER_GFX_END)
    {
        addr_REG = HIFB_GetGfxAbsAddr(enLayer, (HI_UL)(HI_UINTPTR_T)&(pHifbReg->GFX_IRESO.u32));
        GFX_IRESO.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
        GFX_IRESO.bits.ireso_w = pstRect->w - 1;
        GFX_IRESO.bits.ireso_h = pstRect->h - 1;
        HIFB_HAL_WriteReg((HI_U32*)(HI_UINTPTR_T)addr_REG, GFX_IRESO.u32);
    }
    else
    {
        HAL_PRINT("Error layer id found in %s,%s,%d\n",__FILE__, __FUNCTION__, __LINE__);
        return HI_FALSE;
    }

    return HI_TRUE;
}

/*
* Name : HAL_LAYER_SetLayerGAlpha
* Desc : Set video/graphic layer's global alpha
*/
HI_BOOL HIFB_HAL_LAYER_SetLayerGAlpha(HAL_DISP_LAYER_E enLayer,
                                 HI_U8 u8Alpha0)
{
    volatile U_V0_CTRL V0_CTRL;
    volatile U_G0_CTRL G0_CTRL;

    volatile  HI_UL addr_REG;

    switch (enLayer)
    {
        case HAL_DISP_LAYER_VHD0:
        case HAL_DISP_LAYER_VHD1:
        case HAL_DISP_LAYER_VHD2:
        {
            addr_REG = HIFB_GetAbsAddr(enLayer, (HI_UL)(HI_UINTPTR_T)&(pHifbReg->V0_CTRL.u32));
            V0_CTRL.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
            V0_CTRL.bits.galpha = u8Alpha0;
            HIFB_HAL_WriteReg((HI_U32*)(HI_UINTPTR_T)addr_REG, V0_CTRL.u32);
            break;
        }

        case HAL_DISP_LAYER_GFX0:
        case HAL_DISP_LAYER_GFX1:
        case HAL_DISP_LAYER_GFX3:
        {
            addr_REG = HIFB_GetAbsAddr(enLayer, (HI_UL)(HI_UINTPTR_T)&(pHifbReg->G0_CTRL.u32));
            G0_CTRL.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
            G0_CTRL.bits.galpha = u8Alpha0;
            HIFB_HAL_WriteReg((HI_U32*)(HI_UINTPTR_T)addr_REG, G0_CTRL.u32);
            break;
        }

        default:
        {
            HAL_PRINT("Error layer id %d found in %s: L%d\n", enLayer, __FUNCTION__, __LINE__);
            return HI_FALSE;
        }
    }

    return HI_TRUE;
}

HI_BOOL HIFB_HAL_LAYER_GetLayerGAlpha(HAL_DISP_LAYER_E enLayer, HI_U8* pu8Alpha0)
{
    volatile U_V0_CTRL V0_CTRL;
    volatile U_G0_CTRL G0_CTRL;
    volatile  HI_UL addr_REG;
    if (pu8Alpha0 == NULL) {
        return HI_FALSE;
    }
    switch (enLayer)
    {
        case HAL_DISP_LAYER_VHD0:
        case HAL_DISP_LAYER_VHD1:
        case HAL_DISP_LAYER_VHD2:
        {
            addr_REG = HIFB_GetAbsAddr(enLayer, (HI_UL)(HI_UINTPTR_T) & (pHifbReg->V0_CTRL.u32));
            V0_CTRL.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
            *pu8Alpha0 = V0_CTRL.bits.galpha;
            break;
        }

        case HAL_DISP_LAYER_GFX0:
        case HAL_DISP_LAYER_GFX1:
        case HAL_DISP_LAYER_GFX3:
        {
            addr_REG = HIFB_GetAbsAddr(enLayer, (HI_UL)(HI_UINTPTR_T) & (pHifbReg->G0_CTRL.u32));
            G0_CTRL.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
            *pu8Alpha0 = G0_CTRL.bits.galpha;
            break;
        }

        default:
        {
            HAL_PRINT("Error layer id found in %s: L%d\n", __FUNCTION__, __LINE__);
            return HI_FALSE;
        }
    }

    return HI_TRUE;
}

/*
* Name : HAL_LAYER_SetRegUp
* Desc : Set layer(video or graphic) register update.
*/
HI_BOOL  HIFB_HAL_LAYER_SetRegUp(HAL_DISP_LAYER_E enLayer)
{
    U_G0_UPD G0_UPD;
    volatile HI_UL addr_REG;

    switch (enLayer)
    {
        case HAL_DISP_LAYER_GFX0:
        case HAL_DISP_LAYER_GFX1:
        case HAL_DISP_LAYER_GFX3:
        {
            addr_REG = HIFB_GetAbsAddr(enLayer, (HI_UL)(HI_UINTPTR_T)&(pHifbReg->G0_UPD.u32));
            G0_UPD.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
            /* graphic layer register update */
            G0_UPD.bits.regup = 0x1;
            HIFB_HAL_WriteReg((HI_U32*)(HI_UINTPTR_T)addr_REG, G0_UPD.u32);
            break;
        }
        default:
        {
            HAL_PRINT("Error layer id found in %s: L%d\n", __FUNCTION__, __LINE__);
            return HI_FALSE;
        }
    }

    return HI_TRUE;
}

HI_BOOL HIFB_HAL_GRAPHIC_SetGfxAddr(HAL_DISP_LAYER_E enLayer, HI_U64 u64LAddr)
{
    volatile HI_UL ul_GFX_ADDR_H = 0;
    volatile HI_UL ul_GFX_ADDR_L = 0;

    if( HAL_DISP_LAYER_GFX0 == enLayer ||
        HAL_DISP_LAYER_GFX1 == enLayer ||
        HAL_DISP_LAYER_GFX3 == enLayer)
    {
        // Write low address to register.
        ul_GFX_ADDR_L   = HIFB_GetGfxAbsAddr(enLayer, (HI_UL)(HI_UINTPTR_T)&(pHifbReg->GFX_ADDR_L));
        HIFB_HAL_WriteReg((HI_U32*)(HI_UINTPTR_T)ul_GFX_ADDR_L, GetLowAddr(u64LAddr));

        // Write high address to register.
        ul_GFX_ADDR_H   = HIFB_GetGfxAbsAddr(enLayer, (HI_UL)(HI_UINTPTR_T)&(pHifbReg->GFX_ADDR_H));
        HIFB_HAL_WriteReg((HI_U32*)(HI_UINTPTR_T)ul_GFX_ADDR_H, GetHighAddr(u64LAddr));
    }
    else
    {
        HAL_PRINT("Error layer id found in %s: L%d\n",__FUNCTION__, __LINE__);
        return HI_FALSE;
    }
    return HI_TRUE;
}

HI_BOOL HIFB_HAL_GRAPHIC_GetGfxAddr(HAL_DISP_LAYER_E enLayer, HI_U64* pu64GfxAddr)
{
    volatile  HI_UL addr_REG;
    if (pu64GfxAddr == NULL) {
        return HI_FALSE;
    }

    if( HAL_DISP_LAYER_GFX0 == enLayer ||
        HAL_DISP_LAYER_GFX1 == enLayer ||
        HAL_DISP_LAYER_GFX3 == enLayer)
    {
        addr_REG = HIFB_GetGfxAbsAddr(enLayer,(HI_UL)(HI_UINTPTR_T)&(pHifbReg->GFX_ADDR_L));
    }
    else
    {
        HAL_PRINT("Error layer id found in %s: L%d\n",__FUNCTION__, __LINE__);
        return HI_FALSE;
    }

    *pu64GfxAddr = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);

    return HI_TRUE;
}

HI_BOOL HIFB_HAL_GRAPHIC_SetGfxStride(HAL_DISP_LAYER_E enLayer, HI_U16 u16pitch)
{
    volatile  U_GFX_STRIDE   GFX_STRIDE;
    volatile  HI_UL          addr_REG;

    if( HAL_DISP_LAYER_GFX0 == enLayer ||
        HAL_DISP_LAYER_GFX1 == enLayer ||
        HAL_DISP_LAYER_GFX3 == enLayer)
    {
        addr_REG = HIFB_GetGfxAbsAddr(enLayer,(HI_UL)(HI_UINTPTR_T)&(pHifbReg->GFX_STRIDE.u32));
        GFX_STRIDE.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
        GFX_STRIDE.bits.surface_stride = u16pitch;
        HIFB_HAL_WriteReg((HI_U32*)(HI_UINTPTR_T)addr_REG, GFX_STRIDE.u32);
    }
    else
    {
        HAL_PRINT("Error layer id found in %s: L%d\n",__FUNCTION__, __LINE__);
        return HI_FALSE;
    }

    return HI_TRUE;
}

HI_BOOL HIFB_HAL_GRAPHIC_GetGfxStride(HAL_DISP_LAYER_E enLayer, HI_U32* pu32GfxStride)
{
    volatile  HI_UL addr_REG;
    if (pu32GfxStride == NULL) {
        return HI_FALSE;
    }
        if( HAL_DISP_LAYER_GFX0 == enLayer ||
            HAL_DISP_LAYER_GFX1 == enLayer ||
            HAL_DISP_LAYER_GFX3 == enLayer)
        {
            addr_REG = HIFB_GetGfxAbsAddr(enLayer,(HI_UL)(HI_UINTPTR_T)&(pHifbReg->GFX_STRIDE.u32));
        }
        else
        {
            HAL_PRINT("Error layer id found in %s: L%d\n",__FUNCTION__, __LINE__);
            return HI_FALSE;
        }

        *pu32GfxStride = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);

    return HI_TRUE;
}

HI_BOOL HIFB_HAL_GRAPHIC_SetGfxExt(HAL_DISP_LAYER_E enLayer,
                              HAL_GFX_BITEXTEND_E enMode)
{
    U_GFX_OUT_CTRL GFX_OUT_CTRL;

    volatile  HI_UL addr_REG;

    if( HAL_DISP_LAYER_GFX0 == enLayer ||
        HAL_DISP_LAYER_GFX1 == enLayer ||
        HAL_DISP_LAYER_GFX3 == enLayer)
    {
        addr_REG = HIFB_GetGfxAbsAddr(enLayer,(HI_UL)(HI_UINTPTR_T)&(pHifbReg->GFX_OUT_CTRL.u32));
        GFX_OUT_CTRL.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
        GFX_OUT_CTRL.bits.bitext = enMode;
        HIFB_HAL_WriteReg((HI_U32*)(HI_UINTPTR_T)addr_REG, GFX_OUT_CTRL.u32);
    }
    else
    {
        HAL_PRINT("Error layer id found in %s: L%d\n",__FUNCTION__, __LINE__);
        return HI_FALSE;
    }
    return HI_TRUE;
}

HI_BOOL HIFB_HAL_GRAPHIC_SetGfxPreMult(HAL_DISP_LAYER_E enLayer, HI_U32 bEnable)
{
    U_GFX_OUT_CTRL GFX_OUT_CTRL;

    volatile  HI_UL addr_REG;

    if( HAL_DISP_LAYER_GFX0 == enLayer ||
        HAL_DISP_LAYER_GFX1 == enLayer ||
        HAL_DISP_LAYER_GFX3 == enLayer)
    {
        addr_REG = HIFB_GetGfxAbsAddr(enLayer,(HI_UL)(HI_UINTPTR_T)&(pHifbReg->GFX_OUT_CTRL.u32));
        GFX_OUT_CTRL.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
        GFX_OUT_CTRL.bits.premulti_en = bEnable;
        HIFB_HAL_WriteReg((HI_U32*)(HI_UINTPTR_T)addr_REG, GFX_OUT_CTRL.u32);
    }
    else
    {
        HAL_PRINT("Error layer id found in %s: L%d\n",__FUNCTION__, __LINE__);
        return HI_FALSE;
    }

    return HI_TRUE;
}

HI_BOOL HIFB_HAL_GRAPHIC_GetGfxPreMult(HAL_DISP_LAYER_E enLayer, HI_U32* pbEnable)
{
    U_GFX_OUT_CTRL GFX_OUT_CTRL;
    volatile  HI_UL addr_REG;
    if (pbEnable == NULL) {
        return HI_FALSE;
    }

    if( HAL_DISP_LAYER_GFX0 == enLayer ||
        HAL_DISP_LAYER_GFX1 == enLayer ||
        HAL_DISP_LAYER_GFX3 == enLayer)
    {
        addr_REG = HIFB_GetGfxAbsAddr(enLayer,(HI_UL)(HI_UINTPTR_T)&(pHifbReg->GFX_OUT_CTRL.u32));
        GFX_OUT_CTRL.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
        *pbEnable = GFX_OUT_CTRL.bits.premulti_en;
    }
    else
    {
        HAL_PRINT("Error layer id found in %s: L%d\n",__FUNCTION__, __LINE__);
        return HI_FALSE;
    }

    return HI_TRUE;
}

HI_BOOL HIFB_HAL_GRAPHIC_SetGfxPalpha(HAL_DISP_LAYER_E enLayer,
                                 HI_U32 bAlphaEn, HI_U32 bArange,
                                 HI_U8 u8Alpha0, HI_U8 u8Alpha1)
{
    U_GFX_OUT_CTRL GFX_OUT_CTRL;

    U_GFX_1555_ALPHA GFX_1555_ALPHA;

    volatile HI_UL addr_REG;

    if (HAL_DISP_LAYER_GFX0 == enLayer ||
        HAL_DISP_LAYER_GFX1 == enLayer ||
        HAL_DISP_LAYER_GFX3 == enLayer)
    {
        addr_REG = HIFB_GetGfxAbsAddr(enLayer, (HI_UL)(HI_UINTPTR_T)&(pHifbReg->GFX_OUT_CTRL.u32));
        GFX_OUT_CTRL.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
        GFX_OUT_CTRL.bits.palpha_en = bAlphaEn;
        HIFB_HAL_WriteReg((HI_U32*)(HI_UINTPTR_T)addr_REG, GFX_OUT_CTRL.u32);

        if (HI_TRUE == bAlphaEn)
        {
            addr_REG = HIFB_GetGfxAbsAddr(enLayer, (HI_UL)(HI_UINTPTR_T)&(pHifbReg->GFX_1555_ALPHA.u32));
            GFX_1555_ALPHA.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
            GFX_1555_ALPHA.bits.alpha_1 = u8Alpha1;
            HIFB_HAL_WriteReg((HI_U32*)(HI_UINTPTR_T)addr_REG, GFX_1555_ALPHA.u32);


            addr_REG = HIFB_GetGfxAbsAddr(enLayer, (HI_UL)(HI_UINTPTR_T)&(pHifbReg->GFX_1555_ALPHA.u32));
            GFX_1555_ALPHA.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
            GFX_1555_ALPHA.bits.alpha_0 = u8Alpha0;
            HIFB_HAL_WriteReg((HI_U32*)(HI_UINTPTR_T)addr_REG, GFX_1555_ALPHA.u32);
        }
        else
        {
            addr_REG = HIFB_GetGfxAbsAddr(enLayer, (HI_UL)(HI_UINTPTR_T)&(pHifbReg->GFX_1555_ALPHA.u32));
            GFX_1555_ALPHA.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
            GFX_1555_ALPHA.bits.alpha_1 = 0xff;
            HIFB_HAL_WriteReg((HI_U32*)(HI_UINTPTR_T)addr_REG, GFX_1555_ALPHA.u32);

            addr_REG = HIFB_GetGfxAbsAddr(enLayer, (HI_UL)(HI_UINTPTR_T)&(pHifbReg->GFX_1555_ALPHA.u32));
            GFX_1555_ALPHA.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
            GFX_1555_ALPHA.bits.alpha_0 = 0xff;
            HIFB_HAL_WriteReg((HI_U32*)(HI_UINTPTR_T)addr_REG, GFX_1555_ALPHA.u32);
        }
    }
    else
    {
        HAL_PRINT("Error layer id found in %s: L%d\n", __FUNCTION__, __LINE__);
        return HI_FALSE;
    }

    return HI_TRUE;
}

/*
* Name : HAL_GRAPHIC_SetGfxPalphaRange
* Desc : alpha range setting
*/
HI_BOOL HIFB_HAL_GRAPHIC_SetGfxPalphaRange(HAL_DISP_LAYER_E enLayer, HI_U32 bArange)
{
    U_GFX_OUT_CTRL GFX_OUT_CTRL;
    volatile  HI_UL addr_REG;
    switch(enLayer)
    {
        case HAL_DISP_LAYER_GFX0:
        case HAL_DISP_LAYER_GFX1:
        case HAL_DISP_LAYER_GFX3:
        {
            addr_REG = HIFB_GetGfxAbsAddr(enLayer,(HI_UL)(HI_UINTPTR_T)&(pHifbReg->GFX_OUT_CTRL.u32));
            GFX_OUT_CTRL.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
            GFX_OUT_CTRL.bits.palpha_range = bArange;
            HIFB_HAL_WriteReg((HI_U32*)(HI_UINTPTR_T)addr_REG, GFX_OUT_CTRL.u32);
            break;
        }

        default:
        {
            HAL_PRINT("Error layer id found in %s: L%d\n", __FUNCTION__, __LINE__);
            return HI_FALSE;
        }
    }

    return HI_TRUE;
}

HI_BOOL HIFB_HAL_GRAPHIC_SetGfxKeyEn(HAL_DISP_LAYER_E enLayer, HI_U32 u32KeyEnable)
{
    //*
    U_GFX_OUT_CTRL GFX_OUT_CTRL;

    volatile  HI_UL addr_REG;

    if( HAL_DISP_LAYER_GFX0 == enLayer ||
        HAL_DISP_LAYER_GFX1 == enLayer ||
        HAL_DISP_LAYER_GFX3 == enLayer)
    {
        addr_REG = HIFB_GetGfxAbsAddr(enLayer,(HI_UL)(HI_UINTPTR_T)&(pHifbReg->GFX_OUT_CTRL.u32));
        GFX_OUT_CTRL.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
        GFX_OUT_CTRL.bits.key_en = u32KeyEnable;
        HIFB_HAL_WriteReg((HI_U32*)(HI_UINTPTR_T)addr_REG, GFX_OUT_CTRL.u32);
    }
    else
    {
        HAL_PRINT("Error layer id %d not support colorkey in %s: L%d\n",
            (HI_S32)enLayer, __FUNCTION__, __LINE__);
        return HI_FALSE;
    }

    return HI_TRUE;
}

HI_BOOL HIFB_HAL_GRAPHIC_SetGfxKeyMode(HAL_DISP_LAYER_E enLayer, HI_U32 u32KeyOut)
{
    U_GFX_OUT_CTRL GFX_OUT_CTRL;

    volatile  HI_UL addr_REG;

    if( HAL_DISP_LAYER_GFX0 == enLayer ||
        HAL_DISP_LAYER_GFX1 == enLayer ||
        HAL_DISP_LAYER_GFX3 == enLayer)
    {
        addr_REG = HIFB_GetGfxAbsAddr(enLayer,(HI_UL)(HI_UINTPTR_T)&(pHifbReg->GFX_OUT_CTRL.u32));
        GFX_OUT_CTRL.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
        GFX_OUT_CTRL.bits.key_mode = u32KeyOut;
        HIFB_HAL_WriteReg((HI_U32*)(HI_UINTPTR_T)addr_REG, GFX_OUT_CTRL.u32);
    }
    else
    {
        HAL_PRINT("Error layer id %d not support colorkey mode in %s: L%d\n",
            (HI_S32)enLayer, __FUNCTION__, __LINE__);
        return HI_FALSE;
    }

    return HI_TRUE;
}

HI_BOOL HIFB_HAL_GRAPHIC_SetColorKeyValue(HAL_DISP_LAYER_E enLayer,
                                     HAL_GFX_KEY_MAX_S stKeyMax, HAL_GFX_KEY_MIN_S stKeyMin)
{
    U_GFX_CKEY_MAX GFX_CKEY_MAX;
    U_GFX_CKEY_MIN GFX_CKEY_MIN;

    volatile  HI_UL addr_REG;

    if( HAL_DISP_LAYER_GFX0 == enLayer ||
        HAL_DISP_LAYER_GFX1 == enLayer ||
        HAL_DISP_LAYER_GFX3 == enLayer)
    {
        addr_REG = HIFB_GetGfxAbsAddr(enLayer,(HI_UL)(HI_UINTPTR_T)&(pHifbReg->GFX_CKEY_MAX.u32));
        GFX_CKEY_MAX.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
        GFX_CKEY_MAX.bits.key_r_max = stKeyMax.u8KeyMax_R;
        GFX_CKEY_MAX.bits.key_g_max = stKeyMax.u8KeyMax_G;
        GFX_CKEY_MAX.bits.key_b_max = stKeyMax.u8KeyMax_B;
        HIFB_HAL_WriteReg((HI_U32*)(HI_UINTPTR_T)addr_REG, GFX_CKEY_MAX.u32);

        addr_REG = HIFB_GetGfxAbsAddr(enLayer,(HI_UL)(HI_UINTPTR_T)&(pHifbReg->GFX_CKEY_MIN.u32));
        GFX_CKEY_MIN.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
        GFX_CKEY_MIN.bits.key_r_min = stKeyMin.u8KeyMin_R;
        GFX_CKEY_MIN.bits.key_g_min = stKeyMin.u8KeyMin_G;
        GFX_CKEY_MIN.bits.key_b_min = stKeyMin.u8KeyMin_B;
        HIFB_HAL_WriteReg((HI_U32*)(HI_UINTPTR_T)addr_REG, GFX_CKEY_MIN.u32);
    }
    else
    {
        HAL_PRINT("Error layer id %d not support colorkey in %s: L%d\n",
            (HI_S32)enLayer, __FUNCTION__, __LINE__);
        return HI_FALSE;
    }

    return HI_TRUE;
}

HI_BOOL HIFB_HAL_GRAPHIC_SetColorKeyMask(HAL_DISP_LAYER_E enLayer, HAL_GFX_MASK_S stMsk)
{
    U_GFX_CKEY_MASK GFX_CKEY_MASK;

    volatile  HI_UL addr_REG;

    if( HAL_DISP_LAYER_GFX0 == enLayer ||
        HAL_DISP_LAYER_GFX1 == enLayer ||
        HAL_DISP_LAYER_GFX3 == enLayer)
    {
        addr_REG = HIFB_GetGfxAbsAddr(enLayer,(HI_UL)(HI_UINTPTR_T)&(pHifbReg->GFX_CKEY_MASK.u32));
        GFX_CKEY_MASK.u32 = HIFB_HAL_ReadReg((HI_U32*)(HI_UINTPTR_T)addr_REG);
        GFX_CKEY_MASK.bits.key_r_msk = stMsk.u8Mask_r;
        GFX_CKEY_MASK.bits.key_g_msk = stMsk.u8Mask_g;
        GFX_CKEY_MASK.bits.key_b_msk = stMsk.u8Mask_b;
        HIFB_HAL_WriteReg((HI_U32*)(HI_UINTPTR_T)addr_REG, GFX_CKEY_MASK.u32);
    }
    else
    {
        HAL_PRINT("Error layer id %d not support colorkey mask in %s: L%d\n",
            (HI_S32)enLayer, __FUNCTION__, __LINE__);
        return HI_FALSE;
    }

    return HI_TRUE;
}
