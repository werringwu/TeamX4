#ifndef __SAMPLE_VO_H__
#define __SAMPLE_VO_H__

#include "hi_common.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#ifndef SAMPLE_PRT
#define SAMPLE_PRT(fmt...)                           \
    do {                                             \
        printf("[%s]-%d: ", __FUNCTION__, __LINE__); \
        printf(fmt);                                 \
    } while (0)
#endif

#ifndef PAUSE
#define PAUSE()                                                             \
    do {                                                                    \
        printf("---------------press Enter key to exit!---------------\n"); \
        getchar();                                                          \
    } while (0)
#endif

void SAMPLE_VIO_HandleSig(HI_S32 signo);
HI_S32 SAMPLE_VO_RGBLCD_8BIT(HI_VOID);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef __SAMPLE_VO_H__ */
