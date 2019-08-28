/*
 * uvc interface
 */

#ifndef __UVC_GADGETE_H__
#define __UVC_GADGETE_H__

int open_uvc_device(void);
int close_uvc_device(void);
int run_uvc_device(void);
int wait_forever_usb_connect(void);

#endif //__UVC_GADGETE_H__
