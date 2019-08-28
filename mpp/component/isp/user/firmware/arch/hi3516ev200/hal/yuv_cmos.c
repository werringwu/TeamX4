/******************************************************************************

  Copyright (C), 2016, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : yuv_cmos.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2013/05/06
  Description   :
  History       :
  1.Date        : 2013/05/06
    Author      :
    Modification: Created file

******************************************************************************/

#include "isp_sensor.h"
#include "yuv_cmos_ex.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

HI_S32 ISP_GetYUVDefault(ISP_CMOS_DEFAULT_S *pstSnsDft)
{
    ISP_CHECK_POINTER(pstSnsDft);

    pstSnsDft->unKey.bit1Ca       = 0;
    pstSnsDft->pstCa              = &gyuv_stIspCA;
    pstSnsDft->unKey.bit1Wdr      = 0;
    pstSnsDft->pstWdr             = NULL;
    pstSnsDft->unKey.bit1Dpc      = 0;
    pstSnsDft->pstDpc             = NULL;

    pstSnsDft->unKey.bit1Lsc      = 0;
    pstSnsDft->pstLsc             = NULL;

    pstSnsDft->unKey.bit1Demosaic       = 0;
    pstSnsDft->pstDemosaic              = NULL;
    pstSnsDft->unKey.bit1Drc            = 0;
    pstSnsDft->pstDrc                   = NULL;
    pstSnsDft->unKey.bit1Gamma          = 0;
    pstSnsDft->pstGamma                 = NULL;
    pstSnsDft->unKey.bit1BayerNr        = 0;
    pstSnsDft->pstBayerNr               =  NULL;
    pstSnsDft->unKey.bit1Sharpen        = 0;
    pstSnsDft->pstSharpen               = &gyuv_stIspYuvSharpen;
    pstSnsDft->unKey.bit1Ge             = 0;
    pstSnsDft->pstGe                    = NULL;
    pstSnsDft->unKey.bit1AntiFalseColor = 0;
    pstSnsDft->pstAntiFalseColor        = NULL;
    pstSnsDft->unKey.bit1Ldci           = 0;
    pstSnsDft->pstLdci                  = &gyuv_stIspLdci;

    pstSnsDft->stSensorMaxResolution.u32MaxWidth  = 3072;
    pstSnsDft->stSensorMaxResolution.u32MaxHeight = 3072;
    pstSnsDft->stSensorMode.u32SensorID = 307;
    pstSnsDft->stSensorMode.u8SensorMode = WDR_MODE_NONE;

    memcpy(&pstSnsDft->stDngColorParam, &gyuv_stDngColorParam, sizeof(ISP_CMOS_DNG_COLORPARAM_S));

    pstSnsDft->stSensorMode.stDngRawFormat.u8BitsPerSample = 12;
    pstSnsDft->stSensorMode.stDngRawFormat.u32WhiteLevel = 4095;

    pstSnsDft->stSensorMode.stDngRawFormat.stDefaultScale.stDefaultScaleH.u32Denominator = 1;
    pstSnsDft->stSensorMode.stDngRawFormat.stDefaultScale.stDefaultScaleH.u32Numerator = 1;
    pstSnsDft->stSensorMode.stDngRawFormat.stDefaultScale.stDefaultScaleV.u32Denominator = 1;
    pstSnsDft->stSensorMode.stDngRawFormat.stDefaultScale.stDefaultScaleV.u32Numerator = 1;
    pstSnsDft->stSensorMode.stDngRawFormat.stCfaRepeatPatternDim.u16RepeatPatternDimRows = 2;
    pstSnsDft->stSensorMode.stDngRawFormat.stCfaRepeatPatternDim.u16RepeatPatternDimCols = 2;
    pstSnsDft->stSensorMode.stDngRawFormat.stBlcRepeatDim.u16BlcRepeatRows = 2;
    pstSnsDft->stSensorMode.stDngRawFormat.stBlcRepeatDim.u16BlcRepeatCols = 2;
    pstSnsDft->stSensorMode.stDngRawFormat.enCfaLayout = CFALAYOUT_TYPE_RECTANGULAR;
    pstSnsDft->stSensorMode.stDngRawFormat.au8CfaPlaneColor[0] = 0;
    pstSnsDft->stSensorMode.stDngRawFormat.au8CfaPlaneColor[1] = 1;
    pstSnsDft->stSensorMode.stDngRawFormat.au8CfaPlaneColor[2] = 2;
    pstSnsDft->stSensorMode.stDngRawFormat.au8CfaPattern[0] = 0;
    pstSnsDft->stSensorMode.stDngRawFormat.au8CfaPattern[1] = 1;
    pstSnsDft->stSensorMode.stDngRawFormat.au8CfaPattern[2] = 1;
    pstSnsDft->stSensorMode.stDngRawFormat.au8CfaPattern[3] = 2;
    pstSnsDft->stSensorMode.bValidDngRawFormat = HI_TRUE;
    return HI_SUCCESS;
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
