/*
 * UVC gadget test application
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "hi_common.h"
#include "uvc_gadgete.h"
#include "frame_cache.h"
#include "camera.h"
#include "histream.h"

#include "core/usb_endian.h"
#include "core/usb.h"
#include <controller/usb_device/usb_gadget.h>
#include "gadget/f_uvc.h"
#include "gadget/usbd_video.h"
#include "gadget/hicamera_control.h"

typedef struct uvc_device
{
    uvc_t fd;

    unsigned int bitrate;
    unsigned int fcc;
    unsigned int width;
    unsigned int height;

    unsigned int stream_used;
} uvc_device;

static struct uvc_device _uvc_device;
static void uvc_close(struct uvc_device *dev)
{
    uvc_close_device(dev->fd);
}

/* ---------------------------------------------------------------------------*/

static int uvc_continue_copy(uvc_t hdl, struct uvc_transfer_data *tran, void *_stream)
{
    uint32_t len, offset;
    struct frame_node_t *stream = NULL;
    struct uvc_handle *uvc = (struct uvc_handle *)hdl;

    stream = (struct frame_node_t *)_stream;
    if ((stream == NULL) || (stream->mem == NULL))
    {
        return UVC_ERROR_PTR;
    }

    len = tran->length;
    if (uvc->retransmission == 1)
    {
        stream->offset = uvc->offset;
        uvc->retransmission = 0;
    }

    uvc->offset = stream->offset;
    offset = stream->offset;

    if (offset >= stream->used)
    {
        tran->length = 0;
        return UVC_OK;
    }

    if ((len + offset) >= stream->used)
    {
        tran->last = 1;
        tran->length = len = stream->used - offset;
        stream->offset = stream->used;
    }
    else
    {
        tran->last = 0;
        stream->offset += len;
    }

    memcpy(tran->data, &(stream->mem[offset]), len);
    return UVC_OK;
}

EVENT_CB_S sample_update_event;
static unsigned int sample_do_update_camera(void *buf, unsigned int cmdtype)
{
    LOS_EventWrite(&sample_update_event, 0x01);

    return 4;
}

static unsigned int sample_do_get_camera_version(void *buf, unsigned int cmdtype)
{
    dprintf("#%s#\n", __FUNCTION__);

    return 4;
}

static unsigned int sample_do_reset_camera(void *buf, unsigned int cmdtype)
{
    dprintf("#%s#\n", __FUNCTION__);

    return 4;
}

static unsigned int sample_do_start_camera(void *buf, unsigned int cmdtype)
{
    dprintf("#%s#\n", __FUNCTION__);

    return 4;
}

struct uvc_camera_cmd sample_mappings[9] = {
    {
        .id = CMD_RESET_CAMERA,
        .name = "reset camera",
        .uvc_control_func = sample_do_reset_camera,
    },
    {
        .id = CMD_GET_CAMERA_VERSION,
        .name = "get camera version",
        .uvc_control_func = sample_do_get_camera_version,
    },
    {
        .id = CMD_START_CAMERA_UPDATE,
        .name = "start camera update",
        .uvc_control_func = sample_do_update_camera,
    },
    {
        .id = CMD_START_CAMERA,
        .name = "start camera",
        .uvc_control_func = sample_do_start_camera,
    },
};

extern EVENT_CB_S g_frame_event;
extern UINT32 hi_getmsclock(VOID);
static void uvc_video_fill_buffer(struct uvc_device *dev)
{
    switch (dev->fcc)
    {
    case SAMPLE_V4L2_PIX_FMT_MJPEG:
    case SAMPLE_V4L2_PIX_FMT_H264:
    case SAMPLE_V4L2_PIX_FMT_YUYV:
    {
        uvc_cache_t *uvc_cache = uvc_cache_get();
        frame_node_t *node = NULL;
        frame_queue_t *q = 0, *fq = 0;

        if (uvc_cache)
        {
            q  = uvc_cache->ok_queue;
            fq = uvc_cache->free_queue;
            get_node_from_queue(q, &node);
        }

        if (node != NULL)
        {
            int ret = uvc_video_tran_copy(dev->fd, uvc_continue_copy, (void *)node);
            if (ret != UVC_OK)
            {
                // dprintf("uvc_video_tran_copy faile\n");
            }
            uvc_video_stop(dev->fd);
            /* add into free queue again...*/
            node->used = 0;
            put_node_to_queue(fq, node);
        }
        else
        {
            LOS_EventRead(&g_frame_event, 0x01, LOS_WAITMODE_OR | LOS_WAITMODE_CLR, LOS_WAIT_FOREVER);
        }
    }
        break;
    default:
        break;
    }
}

static int uvc_video_process(struct uvc_device *dev)
{
    int ret = 0;
    int con = 0;

    ret = uvc_wait_host(dev->fd, UVC_WAIT_HOST_NOP, &con);
    if ((ret == UVC_OK) && (con == 0))
    {
        dprintf("USB disconnection, wait connect...\n");
        return 0;
    }
    else if (ret != UVC_OK)
    {
        return -1;
    }

    uvc_video_fill_buffer(dev);

    return 1;
}

static int uvc_video_stream(struct uvc_device *dev, int enable)
{
    if (!enable)
    {
        dprintf("Stopping video stream.\n");
        histream_shutdown();
        return 0;
    }

    return -1;
}

static void disable_uvc_video(struct uvc_device *dev)
{
    uvc_video_stream(dev, 0);
}

static void enable_uvc_video(struct uvc_device *dev)
{
    uvc_video_stream(dev, 0);
    clear_ok_queue();

    if (dev->stream_used == 0)
    {
        encoder_property p;

        p.format   = dev->fcc;
        p.width    = dev->width;
        p.height   = dev->height;
        p.compsite = 0;

        histream_set_enc_property(&p);
        histream_shutdown();
        histream_startup();

        dev->stream_used = 1;
    }
}

static PAYLOAD_TYPE_E change_to_mpp_format(uint32_t fcc)
{
    PAYLOAD_TYPE_E t;

    switch (fcc)
    {
    case SAMPLE_V4L2_PIX_FMT_MJPEG:
    case SAMPLE_V4L2_PIX_FMT_YUYV:
        t = PT_MJPEG;
        break;
    case SAMPLE_V4L2_PIX_FMT_H264:
        t = PT_H264;
        break;
    default:
        t = PT_BUTT;
        break;
    }

    return t;
}

static void uvc_events_process(struct uvc_device *dev)
{
    enum format_switch_status format_state = FORMAT_SWITCH_FINISH;

    format_state = uvc_format_status_check();
    if (format_state == FORMAT_SWITCH_PENDING)
    {
        int ret = 0;
        struct uvc_format_info info;

        ret = uvc_format_info_get(&info);
        format_state = uvc_format_status_check();
        if (ret == UVC_OK)
        {
            disable_uvc_video(dev);

            dev->fcc = info.format;
            dev->height = info.height;
            dev->width = info.width;
            dev->stream_used = 0;
            dev->bitrate = 0;

            dprintf("handle streamon event,%d w=%d,h=%d\n", change_to_mpp_format(dev->fcc), dev->width, dev->height);
            enable_uvc_video(dev);
        }
    }
}

int open_uvc_device()
{
    struct uvc_open_param param;
    uvc_t uvc_hdl;
    int ret = 0;

    param.vid_width  = 1920;
    param.vid_height = 1080;
    param.vid_format = UVC_VFF_H264;
    ret = uvc_open_device(&uvc_hdl, &param);
    if (ret != UVC_OK)
    {
        dprintf("uvc_open_device fail !!\n");
        return -1;
    }

    _uvc_device.fd = uvc_hdl;

    return 0;
}

int wait_forever_usb_connect()
{
    int ret = 0;
    int con = 0;

    dprintf("Waiting for USB connection...\n");
    ret = uvc_wait_host(_uvc_device.fd, UVC_WAIT_HOST_FOREVER, &con);
    if ((ret == UVC_OK) && (con == 0))
    {
        dprintf("USB connection Err ret=%u...\n",ret);
        uvc_close_device(_uvc_device.fd);
        return 0;
    }
    else if (ret != UVC_OK)
    {
        dprintf("USB connectionn Err ret=%u...\n",ret);
        uvc_close_device(_uvc_device.fd);
        return -1;
    }
    dprintf("USB connectionning...\n");

    return 1;
}

int close_uvc_device()
{
    disable_uvc_video(&_uvc_device);
    clear_ok_queue();
    uvc_close(&_uvc_device);

    return 0;
}

int run_uvc_device()
{
    int r = 0;

    uvc_events_process(&_uvc_device);
    r = uvc_video_process(&_uvc_device);

    return r;
}
