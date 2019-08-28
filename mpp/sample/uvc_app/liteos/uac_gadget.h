
#ifndef __UAC_H__
#define __UAC_H__

int open_uac_device(void);
int close_uac_device(void);
int run_uac_device(void);
int wait_forever_uac_connect(void);


#endif //__UAC_H__
