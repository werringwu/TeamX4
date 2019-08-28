/******************************************************************************
Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.
******************************************************************************
File Name     : hi_inner_vo.h
Version       : Initial Draft
Author        : MPP Team
Created       : 2014/08/02
Last Modified :
Description   : The mpp inner common defination, not release to customer
Function List :
History       :
 1.Date        : 2014/08/02
   Author      : sdk
   Modification: Created file

******************************************************************************/

#ifndef __HI_INNER_VO_H__
#define __HI_INNER_VO_H__


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

typedef enum hiVO_SCREEN_HFILTER_E
{
    VO_SCREEN_HFILTER_DEF    = 0,
    VO_SCREEN_HFILTER_8M,
    VO_SCREEN_HFILTER_6M,
    VO_SCREEN_HFILTER_5M,
    VO_SCREEN_HFILTER_4M,
    VO_SCREEN_HFILTER_3M,
    VO_SCREEN_HFILTER_2M,
    VO_SCREEN_HFILTER_BUTT

} VO_SCREEN_HFILTER_E;

typedef enum hiVO_SCREEN_VFILTER_E
{
    VO_SCREEN_VFILTER_DEF    = 0,
    VO_SCREEN_VFILTER_8M,
    VO_SCREEN_VFILTER_6M,
    VO_SCREEN_VFILTER_5M,
    VO_SCREEN_VFILTER_4M,
    VO_SCREEN_VFILTER_3M,
    VO_SCREEN_VFILTER_2M,
    VO_SCREEN_VFILTER_BUTT

} VO_SCREEN_VFILTER_E;

typedef struct hiVO_SCALE_FILTER_S
{
    VO_SCREEN_HFILTER_E enHFilter;
    VO_SCREEN_VFILTER_E enVFilter;

} VO_SCREEN_FILTER_S;

typedef struct hiVO_GRAPHIC_ATTR_S
{
    DYNAMIC_RANGE_E     enDstDynamicRange;   /* Destination dynamic range type */
}VO_GRAPHIC_ATTR_S;

///////////////////////
// Cut from "hi_comm_vo.h"
///////////////////////

typedef enum hiVO_DISPLAY_FIELD_E
{
    VO_FIELD_TOP,                 /* Top field*/
    VO_FIELD_BOTTOM,              /* Bottom field*/
    VO_FIELD_BOTH,                /* Top and bottom field*/
    VO_FIELD_BUTT
} VO_DISPLAY_FIELD_E;


//Cut from "mkp_vo.h"
typedef struct hiVO_REGION_LUMA_S
{
    VO_REGION_INFO_S stRegionInfo;     /*Information of the region*/
    HI_S32           s32MilliSec;      /*time parameter.less than 0 means waiting until get the luma data,
                                             equal to 0 means get the luma data no matter whether it can or not,
                                             more than 0 means waiting how long the time parameter it is*/

    HI_U64* ATTRIBUTE pu64LumaData;     /*Luma data of the region*/
}VO_REGION_LUMA_S;

///////////////////////
// [End] Cut from "hi_comm_vo.h"
///////////////////////


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __HI_INNER_VO_H__ */
