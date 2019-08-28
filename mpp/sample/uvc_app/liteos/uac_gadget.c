/*
 * UVC gadget test application
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "core/usb_endian.h"
#include "core/usb.h"
#include <controller/usb_device/usb_gadget.h>
#include "gadget/usbd_audio.h"

#include "sample_comm.h"
#include "hiuac.h"
#include "uac_gadget.h"
#include "frame_cache.h"
#include "hiaudio.h"

typedef struct taguac_device
{
    int fd;
    int nbufs;
    char flag;
} uac_device;

static uac_device __uac_device;
static unsigned char uac_events_flag = 0;

static void uac_close(uac_device *dev)
{
}

/* ---------------------------------------------------------------------------
 * Video streaming
 */
extern EVENT_CB_S g_audio_event;
extern UINT32 hi_getmsclock(VOID);
static void uac_video_fill_buffer(uac_device *dev)
{
    uac_cache_t *uac_cache = uac_cache_get();
    frame_node_t *node = 0;
    frame_queue_t *q = 0, *fq = 0;

    if (uac_cache)
    {
        q  = uac_cache->ok_queue;
        fq = uac_cache->free_queue;
        get_node_from_queue(q, &node);
    }

    if (node != 0)
    {
        int ret = fuac_send_message(node->mem, node->used);
        if (ret != 0)
        {
            dprintf("fuac_send_message faile  %d\n", ret);
        }

        /* add into free queue again...*/
        node->used = 0;
        put_node_to_queue(fq, node);
    }
    else
    {
        LOS_EventRead(&g_audio_event, 0x01, LOS_WAITMODE_OR | LOS_WAITMODE_CLR, LOS_WAIT_FOREVER);
    }
}

static int uac_video_process(uac_device *dev)
{
    int ret;

    ret = uac_wait_host(UAC_WAIT_HOST_NOP);
    if (ret == UAC_ERROR_NOMATCH)
    {
        return 0;
    }
    else if ((ret != UAC_ERROR_NOMATCH) && (ret != UAC_OK))
    {
        dprintf("UAC ERROR FAIL\n");
        return -1;
    }

    uac_video_fill_buffer(dev);

    return 1;
}

/* ---------------------------------------------------------------------------
 * Request processing
 */

static int uac_audio_stream(uac_device *dev, int enable)
{
    if (!enable)
    {
        dprintf("Stopping audio stream.\n");
        hiaudio_shutdown();
        if (dev->flag == 1)
        {
            hiaudio_deinit();
        }
        return 0;
    }

    return -1;
}

static void disable_uac_audio(uac_device *dev)
{
    uac_audio_stream(dev, 0);
}

static void enable_uac_audio(uac_device *dev)
{
    uac_audio_stream(dev, 0);
    clear_ok_queue();

    if (dev->flag == 1)
    {
        hiaudio_init();
    }

    hiaudio_startup();
    dev->flag = 0;
}

extern AUDIO_SAMPLE_RATE_E g_enInSampleRate;
extern unsigned int fuac_rate_get(void);
static AUDIO_SAMPLE_RATE_E change_to_mpp_format(unsigned int fcc)
{
    AUDIO_SAMPLE_RATE_E t;

    switch (fcc)
    {
    case 8000:    /* 8K samplerate*/
    case 12000:   /* 12K samplerate*/
    case 11025:   /* 11.025K samplerate*/
    case 16000:   /* 16K samplerate*/
    case 22050:   /* 22.050K samplerate*/
    case 24000:   /* 24K samplerate*/
    case 32000:   /* 32K samplerate*/
    case 44100:   /* 44.1K samplerate*/
    case 48000:   /* 48K samplerate*/
    case 64000:   /* 64K samplerate*/
    case 96000:   /* 96K samplerate*/
        t = fcc;
        break;
    default:
        t = AUDIO_SAMPLE_RATE_BUTT;
        break;
    }

    return t;
}

static void uac_events_process(uac_device *dev)
{
    AUDIO_SAMPLE_RATE_E Rate = AUDIO_SAMPLE_RATE_BUTT;

    Rate = change_to_mpp_format(fuac_rate_get());

    if (AUDIO_SAMPLE_RATE_BUTT == Rate)
    {
        dprintf("audio rate is not support!!\n");
        return;
    }

    if (Rate != g_enInSampleRate)
    {
        g_enInSampleRate = Rate;
        dev->flag = 1;
        uac_events_flag = 1;

        dprintf("handle audio event, %d\n", g_enInSampleRate);
    }

    if (uac_events_flag != 0)
    {
        enable_uac_audio(dev);
        uac_events_flag = 0;
    }
}

/* ---------------------------------------------------------------------------
 * main
 */
int open_uac_device()
{
    memset(&__uac_device, 0, sizeof(uac_device));

    return 0;
}

int wait_forever_uac_connect()
{
    int ret = 0;

    dprintf("waiting for UAC connect!\n");

    ret = uac_wait_host(UAC_WAIT_HOST_FOREVER);
    if (ret == UAC_OK)
    {
        uac_events_flag = 1;
        dprintf("uac connectionning... %d\n", ret);
    }
    else
    {
        return -1;
    }

    return 0;
}

int close_uac_device()
{
    uac_events_flag = 0;
    disable_uac_audio(&__uac_device);
    clear_ok_queue();
    uac_close(&__uac_device);

    return 0;
}

int run_uac_device()
{
    int r;

    uac_events_process(&__uac_device);
    r = uac_video_process(&__uac_device);

    return r;
}

