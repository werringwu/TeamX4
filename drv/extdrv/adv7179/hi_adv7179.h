/******************************************************************************

  Copyright (C), 2015, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : hi_adv7179.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2015/06/17
  Description   : Common Def Of mmz
  History       :
  1.Date        : 2015/06/17
    Author      : h00293269
    Modification: Created file
******************************************************************************/

#ifndef __HI_SIL9024_H__
#define __HI_SIL9024_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

typedef struct hiADV7179_MODULE_PARAMS_S
{
    int Norm_mode; /* display timing mode: 0:PAL,1:NTSC */
    int i2c_num;
}ADV7179_MODULE_PARAMS_S;


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif



