/******************************************************************************

  Copyright (C), 2011-2021, Hisilicon Tech. Co., Ltd.

 ******************************************************************************/
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/version.h>
#include <linux/pagemap.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/poll.h>
#include <linux/sched.h>

#include "vou_exp.h"



#define VO_NOTIFY     0


void hi_user_notify_vo_event(int module_id, int vodev)
{
    switch ( vodev )
    {
        case 0 :
        {
        /* do something */
            break;
        }
        default:
        {
            break;
        }
    }
}


extern VOU_EXPORT_SYMBOL_S   g_stVouExpSymbol;

int __init hi_user_init(void)
{
#if VO_NOTIFY
    {
        VOU_EXPORT_CALLBACK_S stVoExpCallback;
        memset(&stVoExpCallback, 0, sizeof(VOU_EXPORT_CALLBACK_S));
        if(NULL != g_stVouExpSymbol.pfnVoRegisterExpCallback)
        {
            stVoExpCallback.pfnVoNotify = hi_user_notify_vo_event;
            g_stVouExpSymbol.pfnVoRegisterExpCallback(&stVoExpCallback);
        }
    }
#endif

    return 0;
}

void __exit hi_user_exit(void)
{
#if VO_NOTIFY
    {
        VOU_EXPORT_CALLBACK_S stVoExpCallback;

        if(NULL != g_stVouExpSymbol.pfnVoRegisterExpCallback)
        {
            memset(&stVoExpCallback, 0, sizeof(VOU_EXPORT_CALLBACK_S));
            g_stVouExpSymbol.pfnVoRegisterExpCallback(&stVoExpCallback);
        }
    }
#endif

    return;
}

#ifndef __HuaweiLite__
module_init(hi_user_init);
module_exit(hi_user_exit);
#endif

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Hisilicon");

