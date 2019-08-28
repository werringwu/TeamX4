#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "sample_comm.h"
#include "sample_vo.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define FILENAME_MAX_LEN 256
#define ALIGN_BACK(x, a) ((a) * (((x) + (a)-1) / (a)))

#ifdef __HuaweiLite__
#define YUV_PATH         "/sharefs/res/320_240_420.yuv"
#else
#define YUV_PATH         "./res/320_240_420.yuv"
#endif

typedef struct hiSAMPLE_VO_THREADCTRL_INFO {
    HI_CHAR filename[FILENAME_MAX_LEN];
    HI_U32 u32Width;
    HI_U32 u32Height;
    PIXEL_FORMAT_E enPixelFmt;
    VIDEO_FORMAT_E enVideoFmt;
    HI_BOOL bQuit;
    HI_BOOL bDestroy;
    VO_LAYER VoLayer;
    VO_CHN VoChn;
    pthread_t tid;
} SAMPLE_VO_THREADCTRL_INFO;

typedef struct hiSAMPLE_VO_DEV_CONFIG_S {
    VO_DEV VoDev;
    VO_PUB_ATTR_S stPubAttr;
    VO_USER_INTFSYNC_INFO_S stUserInfo;
    HI_U32 u32Framerate;
} SAMPLE_VO_DEV_CONFIG_S;

typedef struct hiSAMPLE_VO_VIDEO_FRAME_ADDR {
    HI_U8 *pY;
    HI_U8 *pU;
    HI_U8 *pV;
} SAMPLE_VO_VIDEO_FRAME_ADDR;

typedef struct hiSAMPLE_VO_VIDEO_FRAME_STRIDE {
    HI_U32 yStride;
    HI_U32 uStride;
} SAMPLE_VO_VIDEO_FRAME_STRIDE;

#ifndef __HuaweiLite_
void SAMPLE_VO_HandleSig(HI_S32 signo)
{
    signal(SIGINT, SIG_IGN);
    signal(SIGTERM, SIG_IGN);

    if ((signo == SIGINT) || (signo == SIGTERM)) {
        SAMPLE_COMM_SYS_Exit();
        SAMPLE_PRT("\033[0;31mprogram termination abnormally!\033[0;39m\n");
    }
    exit(-1);
}
#endif

static HI_S32 SAMPLE_VO_SYS_Init(SIZE_S *pstSize)
{
    HI_S32 s32Ret;
    VB_CONFIG_S stVbConf;
    HI_U32 u32BlkSize;

    hi_memset(&stVbConf, sizeof(VB_CONFIG_S), 0, sizeof(VB_CONFIG_S));
    stVbConf.u32MaxPoolCnt = 1;

    u32BlkSize = COMMON_GetPicBufferSize(pstSize->u32Width, pstSize->u32Height, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8,
                                         COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[0].u64BlkSize = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt = 3;

    u32BlkSize = COMMON_GetPicBufferSize(pstSize->u32Width, pstSize->u32Height, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8,
                                         COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[1].u64BlkSize = u32BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt = 0;

    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);

    return s32Ret;
}

static HI_VOID SAMPLE_VO_GetUserDefConfig(SAMPLE_VO_DEV_CONFIG_S *pstVoDevConfig)
{
    pstVoDevConfig->stPubAttr.enIntfType = VO_INTF_LCD_8BIT;
    pstVoDevConfig->stPubAttr.enIntfSync = VO_OUTPUT_USER;
    pstVoDevConfig->stPubAttr.u32BgColor = COLOR_RGB_BLUE;

    pstVoDevConfig->stPubAttr.stSyncInfo.bSynm = 0;
    pstVoDevConfig->stPubAttr.stSyncInfo.bIop = 1;
    pstVoDevConfig->stPubAttr.stSyncInfo.u8Intfb = 1;
    pstVoDevConfig->stPubAttr.stSyncInfo.u16Vact = 240;
    pstVoDevConfig->stPubAttr.stSyncInfo.u16Vbb = 15;
    pstVoDevConfig->stPubAttr.stSyncInfo.u16Vfb = 9;
    pstVoDevConfig->stPubAttr.stSyncInfo.u16Hact = 320;
    pstVoDevConfig->stPubAttr.stSyncInfo.u16Hbb = 65;
    pstVoDevConfig->stPubAttr.stSyncInfo.u16Hfb = 7;
    pstVoDevConfig->stPubAttr.stSyncInfo.u16Hmid = 1;
    pstVoDevConfig->stPubAttr.stSyncInfo.u16Bvact = 240;
    pstVoDevConfig->stPubAttr.stSyncInfo.u16Bvbb = 14;
    pstVoDevConfig->stPubAttr.stSyncInfo.u16Bvfb = 9;
    pstVoDevConfig->stPubAttr.stSyncInfo.u16Hpw = 1;
    pstVoDevConfig->stPubAttr.stSyncInfo.u16Vpw = 1;
    pstVoDevConfig->stPubAttr.stSyncInfo.bIdv = 0;
    pstVoDevConfig->stPubAttr.stSyncInfo.bIhs = 0;
    pstVoDevConfig->stPubAttr.stSyncInfo.bIvs = 0;

    pstVoDevConfig->u32Framerate = 60;

    pstVoDevConfig->stUserInfo.stUserIntfSyncAttr.enClkSource = VO_CLK_SOURCE_LCDMCLK;
    pstVoDevConfig->stUserInfo.stUserIntfSyncAttr.u32LcdMClkDiv = 0x1c4255;
    pstVoDevConfig->stUserInfo.u32DevDiv = 4;
    pstVoDevConfig->stUserInfo.bClkReverse = HI_TRUE;

    return;
}

static HI_VOID SAMPLE_VO_GetUserLayerAttr(VO_VIDEO_LAYER_ATTR_S *pstLayerAttr, SIZE_S *pstDevSize)
{
    pstLayerAttr->bClusterMode = HI_FALSE;
    pstLayerAttr->bDoubleFrame = HI_FALSE;
    pstLayerAttr->enDstDynamicRange = DYNAMIC_RANGE_SDR8;
    pstLayerAttr->enPixFormat = PIXEL_FORMAT_YVU_SEMIPLANAR_420;

    pstLayerAttr->stDispRect.s32X = 0;
    pstLayerAttr->stDispRect.s32Y = 0;
    pstLayerAttr->stDispRect.u32Height = pstDevSize->u32Height;
    pstLayerAttr->stDispRect.u32Width = pstDevSize->u32Width;

    pstLayerAttr->stImageSize.u32Height = pstDevSize->u32Height;
    pstLayerAttr->stImageSize.u32Width = pstDevSize->u32Width;

    return;
}

static HI_VOID SAMPLE_VO_GetBaseThreadInfo(SAMPLE_VO_THREADCTRL_INFO *pstThreadInfo, SIZE_S *pstFrmSize)
{
    pstThreadInfo->bDestroy = HI_FALSE;
    pstThreadInfo->bQuit = HI_FALSE;
    pstThreadInfo->enPixelFmt = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    pstThreadInfo->enVideoFmt = VIDEO_FORMAT_LINEAR;
    pstThreadInfo->u32Width = pstFrmSize->u32Width;
    pstThreadInfo->u32Height = pstFrmSize->u32Height;

    return;
}

static HI_S32 SAMPLE_VO_StartUserDev(VO_DEV VoDev, SAMPLE_VO_DEV_CONFIG_S *pstVoDevConfig)
{
    HI_S32 s32Ret;

    s32Ret = HI_MPI_VO_SetPubAttr(VoDev, &pstVoDevConfig->stPubAttr);
    if (s32Ret != HI_SUCCESS) {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        return HI_FAILURE;
    }

    /* SET VO FRAME RATE */
    s32Ret = HI_MPI_VO_SetDevFrameRate(VoDev, pstVoDevConfig->u32Framerate);
    if (s32Ret != HI_SUCCESS) {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        return HI_FAILURE;
    }

    /* SET VO SYNC INFO OF USER INTF */
    s32Ret = HI_MPI_VO_SetUserIntfSyncInfo(VoDev, &pstVoDevConfig->stUserInfo);
    if (s32Ret != HI_SUCCESS) {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        return HI_FAILURE;
    }

    s32Ret = HI_MPI_VO_Enable(VoDev);
    if (s32Ret != HI_SUCCESS) {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        return HI_FAILURE;
    }

    return s32Ret;
}

static HI_S32 SAMPLE_VO_SetVideoLayerCsc(VO_LAYER VoLayer)
{
    HI_S32 s32Ret;
    VO_CSC_S stVoCsc;

    stVoCsc.enCscMatrix = VO_CSC_MATRIX_BT709_TO_RGB_PC;
    stVoCsc.u32Contrast = 50;
    stVoCsc.u32Hue = 50;
    stVoCsc.u32Luma = 50;
    stVoCsc.u32Satuature = 50;
    s32Ret = HI_MPI_VO_SetVideoLayerCSC(VoLayer, &stVoCsc);
    if (s32Ret != HI_SUCCESS) {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        return HI_FAILURE;
    }

    return s32Ret;
}

static HI_S32 SAMPLE_VO_ReadOneFrame(FILE *fp, SAMPLE_VO_VIDEO_FRAME_ADDR *pstVidFrmAddr, SIZE_S *pstFrameSize,
                                     SAMPLE_VO_VIDEO_FRAME_STRIDE pstStride, PIXEL_FORMAT_E enPixFrm)
{
    HI_U8 *pDst;
    HI_U32 u32UVHeight;
    HI_U32 u32Row;

    if (enPixFrm == PIXEL_FORMAT_YVU_SEMIPLANAR_422) {
        u32UVHeight = pstFrameSize->u32Height;
    } else if (enPixFrm == PIXEL_FORMAT_YVU_SEMIPLANAR_420) {
        u32UVHeight = pstFrameSize->u32Height / 2;
    } else {
        u32UVHeight = 0;
    }

    pDst = pstVidFrmAddr->pY;
    for (u32Row = 0; u32Row < pstFrameSize->u32Height; u32Row++) {
        if (fread(pDst, 1, pstFrameSize->u32Width, fp) != pstFrameSize->u32Width) {
            return HI_FAILURE;
        }
        pDst += pstStride.yStride;
    }

    pDst = pstVidFrmAddr->pU;
    for (u32Row = 0; u32Row < u32UVHeight; u32Row++) {
        if (fread(pDst, 1, pstFrameSize->u32Width / 2, fp) != pstFrameSize->u32Width / 2) {
            return HI_FAILURE;
        }
        pDst += pstStride.uStride;
    }

    pDst = pstVidFrmAddr->pV;
    for (u32Row = 0; u32Row < u32UVHeight; u32Row++) {
        if (fread(pDst, 1, pstFrameSize->u32Width / 2, fp) != pstFrameSize->u32Width / 2) {
            return HI_FAILURE;
        }
        pDst += pstStride.uStride;
    }

    return HI_SUCCESS;
}

static HI_S32 SAMPLE_VO_PlanToSemi(SAMPLE_VO_VIDEO_FRAME_ADDR *pstVidFrmAddr, SAMPLE_VO_VIDEO_FRAME_STRIDE pstStride,
                                   SIZE_S *pstFrameSize, PIXEL_FORMAT_E enPixFrm)
{
    HI_S32 i;
    HI_U8 *pTmpU, *pTmpV, *ptu, *ptv;
    HI_S32 s32HafW, s32HafH, s32Size;

    s32HafW = pstStride.uStride;

    if (enPixFrm == PIXEL_FORMAT_YVU_SEMIPLANAR_422) {
        s32HafH = pstFrameSize->u32Height;
    } else {
        s32HafH = pstFrameSize->u32Height >> 1;
    }

    s32Size = s32HafW * s32HafH;

    pTmpU = (HI_U8 *)malloc(s32Size);
    if (pTmpU == NULL) {
        SAMPLE_PRT("malloc fail !\n");
        return HI_FAILURE;
    }
    pTmpV = (HI_U8 *)malloc(s32Size);
    if (pTmpV == NULL) {
        SAMPLE_PRT("malloc fail !\n");
        free(pTmpU);
        return HI_FAILURE;
    }

    ptu = pTmpU;
    ptv = pTmpV;

    memcpy(pTmpU, pstVidFrmAddr->pU, s32Size);
    memcpy(pTmpV, pstVidFrmAddr->pV, s32Size);

    for (i = 0; i < (s32Size >> 1); i++) {
        *pstVidFrmAddr->pU++ = *pTmpV++;
        *pstVidFrmAddr->pU++ = *pTmpU++;
    }
    for (i = 0; i < (s32Size >> 1); i++) {
        *pstVidFrmAddr->pV++ = *pTmpV++;
        *pstVidFrmAddr->pV++ = *pTmpU++;
    }

    free(ptu);
    free(ptv);

    return HI_SUCCESS;
}

static HI_VOID SAMPLE_VO_SetVideoFrm(SIZE_S pstSrcSize, SAMPLE_VO_THREADCTRL_INFO *pInfo,
                                     VIDEO_FRAME_INFO_S *pstUserFrame)
{
    pstUserFrame->stVFrame.enField = VIDEO_FIELD_INTERLACED;
    pstUserFrame->stVFrame.enCompressMode = COMPRESS_MODE_NONE;
    pstUserFrame->stVFrame.enPixelFormat = pInfo->enPixelFmt;
    pstUserFrame->stVFrame.enVideoFormat = pInfo->enVideoFmt;
    pstUserFrame->stVFrame.enColorGamut = COLOR_GAMUT_BT709;
    pstUserFrame->stVFrame.u32Width = pstSrcSize.u32Width;
    pstUserFrame->stVFrame.u32Height = pstSrcSize.u32Height;
    pstUserFrame->stVFrame.u32Stride[0] = ALIGN_BACK(pstSrcSize.u32Width, 16);
    pstUserFrame->stVFrame.u32Stride[1] = ALIGN_BACK(pstSrcSize.u32Width, 16);
    pstUserFrame->stVFrame.u32Stride[2] = ALIGN_BACK(pstSrcSize.u32Width, 16);
    pstUserFrame->stVFrame.u32TimeRef = 0;
    pstUserFrame->stVFrame.u64PTS = 0;
    pstUserFrame->stVFrame.enDynamicRange = DYNAMIC_RANGE_SDR8;
}

static HI_VOID SAMPLE_VO_SetUserFrmParam(SIZE_S pstSrcSize, VB_BLK hBlkHdl, VIDEO_FRAME_INFO_S *pstUserFrame,
                                         HI_U32 u32Size, SAMPLE_VO_THREADCTRL_INFO *pInfo)
{
    HI_U32 u32LumaSize, u32ChromaSize;

    u32LumaSize = pstUserFrame->stVFrame.u32Stride[0] * pstSrcSize.u32Height;
    if (pInfo->enPixelFmt == PIXEL_FORMAT_YVU_SEMIPLANAR_422) {
        u32ChromaSize = pstUserFrame->stVFrame.u32Stride[0] * pstSrcSize.u32Height / 2;
    } else if ((pInfo->enPixelFmt == PIXEL_FORMAT_YVU_SEMIPLANAR_420)) {
        u32ChromaSize = pstUserFrame->stVFrame.u32Stride[0] * pstSrcSize.u32Height / 4;
    } else {
        u32ChromaSize = 0;
    }

    pstUserFrame->u32PoolId = HI_MPI_VB_Handle2PoolId(hBlkHdl);
    pstUserFrame->stVFrame.u64PhyAddr[0] = HI_MPI_VB_Handle2PhysAddr(hBlkHdl);
    pstUserFrame->stVFrame.u64PhyAddr[1] = pstUserFrame->stVFrame.u64PhyAddr[0] + u32LumaSize;
    pstUserFrame->stVFrame.u64PhyAddr[2] = pstUserFrame->stVFrame.u64PhyAddr[1] + u32ChromaSize;

    pstUserFrame->stVFrame.u64VirAddr[0] = (HI_UL)HI_MPI_SYS_Mmap(pstUserFrame->stVFrame.u64PhyAddr[0], u32Size);
    pstUserFrame->stVFrame.u64VirAddr[1] = (HI_UL)(pstUserFrame->stVFrame.u64VirAddr[0]) + u32LumaSize;
    pstUserFrame->stVFrame.u64VirAddr[2] = (HI_UL)(pstUserFrame->stVFrame.u64VirAddr[1]) + u32ChromaSize;
}

static HI_VOID SAMPLE_VO_SendOneFrame(FILE *pfd, VIDEO_FRAME_INFO_S *pstUserFrame, SAMPLE_VO_THREADCTRL_INFO *pInfo)
{
    HI_S32 s32Ret;
    VO_LAYER VoLayer;
    VO_CHN VoChn;
    SAMPLE_VO_VIDEO_FRAME_ADDR stVidFrmAddr;
    SAMPLE_VO_VIDEO_FRAME_STRIDE stStride;
    SIZE_S stFrameSize;

    VoLayer = pInfo->VoLayer;
    VoChn = pInfo->VoChn;

    stVidFrmAddr.pY = (HI_U8 *)(HI_UL)pstUserFrame->stVFrame.u64VirAddr[0];
    stVidFrmAddr.pU = (HI_U8 *)(HI_UL)pstUserFrame->stVFrame.u64VirAddr[1];
    stVidFrmAddr.pV = (HI_U8 *)(HI_UL)pstUserFrame->stVFrame.u64VirAddr[2];

    stFrameSize.u32Width = pstUserFrame->stVFrame.u32Width;
    stFrameSize.u32Height = pstUserFrame->stVFrame.u32Height;

    stStride.yStride = pstUserFrame->stVFrame.u32Stride[0];
    stStride.uStride = pstUserFrame->stVFrame.u32Stride[1] >> 1;

    s32Ret = SAMPLE_VO_ReadOneFrame(pfd, &stVidFrmAddr, &stFrameSize, stStride, pstUserFrame->stVFrame.enPixelFormat);
    if (s32Ret == HI_SUCCESS) {
        if (pInfo->enPixelFmt != PIXEL_FORMAT_YUV_400) {
            s32Ret = SAMPLE_VO_PlanToSemi(&stVidFrmAddr, stStride, &stFrameSize, pstUserFrame->stVFrame.enPixelFormat);
            if (s32Ret != HI_SUCCESS) {
                SAMPLE_PRT("SAMPLE_VO_PlanToSemi failed !\n");
                return;
            }
        }
    } else {
        return;
    }

    pstUserFrame->stVFrame.u64PTS += 40000;
    pstUserFrame->stVFrame.u32TimeRef += 40000;

    s32Ret = HI_MPI_VO_SendFrame(VoLayer, VoChn, pstUserFrame, 0);
    if (s32Ret != HI_SUCCESS) {
        SAMPLE_PRT("HI_MPI_VO_SendFrame failed with %#x!\n", s32Ret);
    }

    return;
}

static HI_VOID *SAMPLE_VO_FileToVO(HI_VOID *pData)
{
    FILE *pfd;
    VB_BLK hBlkHdl;
    SIZE_S stSrcSize;
    VB_POOL Pool;
    VIDEO_FRAME_INFO_S stUserFrame;
    VB_POOL_CONFIG_S stVbPoolCfg;
    HI_U32 u32Size;

    SAMPLE_VO_THREADCTRL_INFO *pInfo = (SAMPLE_VO_THREADCTRL_INFO *)pData;

    memset(&stUserFrame, 0, sizeof(VIDEO_FRAME_INFO_S));

    stSrcSize.u32Width = pInfo->u32Width;
    stSrcSize.u32Height = pInfo->u32Height;

    pfd = fopen(pInfo->filename, "rb");
    if (pfd == HI_NULL) {
        SAMPLE_PRT("open file %s fail \n", pInfo->filename);
        return HI_NULL;
    } else {
        SAMPLE_PRT("open file %s success!\n", pInfo->filename);
    }

    fflush(stdout);

    u32Size = stSrcSize.u32Width * stSrcSize.u32Height * 2;
    memset(&stVbPoolCfg, 0, sizeof(VB_POOL_CONFIG_S));
    stVbPoolCfg.u64BlkSize = u32Size;
    stVbPoolCfg.u32BlkCnt = 6;
    stVbPoolCfg.enRemapMode = VB_REMAP_MODE_NONE;
    Pool = HI_MPI_VB_CreatePool(&stVbPoolCfg);
    if (Pool == VB_INVALID_POOLID) {
        SAMPLE_PRT("HI_MPI_VB_CreatePool fail\n");
        return HI_NULL;
    }

    SAMPLE_VO_SetVideoFrm(stSrcSize, pInfo, &stUserFrame);

    do {
        if (feof(pfd) != 0) {
            fseek(pfd, 0, SEEK_SET);
        }

        hBlkHdl = HI_MPI_VB_GetBlock(Pool, u32Size, NULL);
        if (hBlkHdl == VB_INVALID_HANDLE) {
            SAMPLE_PRT("get vb fail!!!\n");
            continue;
        }

        SAMPLE_VO_SetUserFrmParam(stSrcSize, hBlkHdl, &stUserFrame, u32Size, pInfo);

        SAMPLE_VO_SendOneFrame(pfd, &stUserFrame, pInfo);

        HI_MPI_VB_ReleaseBlock(hBlkHdl);
        HI_MPI_SYS_Munmap ((HI_VOID *)(HI_UL)stUserFrame.stVFrame.u64VirAddr[0], u32Size);

    } while (pInfo->bQuit == HI_FALSE);

    while (pInfo->bDestroy == HI_FALSE) {}

    fclose(pfd);

    return HI_NULL;
}

static HI_VOID SAMPLE_VO_StartUserThd(SAMPLE_VO_THREADCTRL_INFO *pstThreadInfo, VO_LAYER VoLayer,
                                      VO_CHN VoChn, HI_CHAR filename[FILENAME_MAX_LEN], SIZE_S *pstFrmSize)
{
    /* CREATE USER THREAD */
    SAMPLE_VO_GetBaseThreadInfo(pstThreadInfo, pstFrmSize);

    pstThreadInfo->VoLayer = VoLayer;
    pstThreadInfo->VoChn = VoChn;

    strncpy(pstThreadInfo->filename, filename, sizeof(pstThreadInfo->filename) - 1);
    pthread_create(&pstThreadInfo->tid, NULL, SAMPLE_VO_FileToVO, (HI_VOID *)pstThreadInfo);

    return;
}

static HI_VOID SAMPLE_VO_StopUserThd(SAMPLE_VO_THREADCTRL_INFO *pstThdInfo)
{
    pstThdInfo->bQuit = HI_TRUE;
    pstThdInfo->bDestroy = HI_TRUE;
    pthread_join(pstThdInfo->tid, HI_NULL);

    return;
}

HI_S32 SAMPLE_VO_RGBLCD_8BIT(HI_VOID)
{
    HI_S32 s32Ret;
    VO_DEV VoDev = 0;
    VO_LAYER VoLayer = 0;
    VO_CHN VoChn = 0;
    VO_VIDEO_LAYER_ATTR_S stLayerAttr;
    SAMPLE_VO_THREADCTRL_INFO stThreadInfo;
    SAMPLE_VO_DEV_CONFIG_S stVoDevConfig;

    HI_CHAR filename[FILENAME_MAX_LEN] = { YUV_PATH };
    SIZE_S stFrameSize = { 320, 240 };

    s32Ret = SAMPLE_VO_SYS_Init(&stFrameSize);
    if (s32Ret != HI_SUCCESS) {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        return s32Ret;
    }

    /* SET VO PUB ATTR OF USER TYPE */
    SAMPLE_VO_GetUserDefConfig(&stVoDevConfig);

    /* START VO DEV */
    s32Ret = SAMPLE_VO_StartUserDev(VoDev, &stVoDevConfig);
    if (s32Ret != HI_SUCCESS) {
        SAMPLE_PRT("start vo dev failed !\n");
        goto EXIT0;
    }

    /* SET CSC FOR LCD */
    s32Ret = SAMPLE_VO_SetVideoLayerCsc(VoLayer);
    if (s32Ret != HI_SUCCESS) {
        SAMPLE_PRT("set video layer csc failed !\n");
        goto EXIT1;
    }

    SAMPLE_VO_GetUserLayerAttr(&stLayerAttr, &stFrameSize);
    stLayerAttr.u32DispFrmRt = stVoDevConfig.u32Framerate;

    /* START VIDEO LAYER */
    SAMPLE_COMM_VO_StartLayer(VoLayer, &stLayerAttr);
    if (s32Ret != HI_SUCCESS) {
        SAMPLE_PRT("start video layer failed!\n");
        goto EXIT1;
    }

    /* START VO CHANNEL */
    SAMPLE_COMM_VO_StartChn(VoLayer, VoChn);
    if (s32Ret != HI_SUCCESS) {
        SAMPLE_PRT("start vo channel failed!\n");
        goto EXIT2;
    }

    /* START USER THREAD */
    SAMPLE_VO_StartUserThd(&stThreadInfo, VoLayer, VoChn, filename, &stFrameSize);

    PAUSE();

    /* STOP USER THREAD */
    SAMPLE_VO_StopUserThd(&stThreadInfo);

    SAMPLE_COMM_VO_StopChn(VoLayer, VoChn);
EXIT2:
    SAMPLE_COMM_VO_StopLayer(VoLayer);
EXIT1:
    SAMPLE_COMM_VO_StopDev(VoDev);
EXIT0:
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}

/******************************************************************************
* function : show usage
******************************************************************************/
void SAMPLE_VO_Usage(char *sPrgNm)
{
    printf("Usage : %s <index>\n", sPrgNm);
    printf("index:\n");
    printf("\t 0)USERPIC - VO - 8BITLCD.\n");

    return;
}

/******************************************************************************
* function    : main()
* Description : main
******************************************************************************/
#ifdef __HuaweiLite__
int app_main(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
    HI_S32 s32Ret = HI_FAILURE;
    HI_S32 s32Index;

    if (argc < 2 || argc > 2) {
        SAMPLE_VO_Usage(argv[0]);
        return HI_FAILURE;
    }

    if (!strncmp(argv[1], "-h", 2)) {
        SAMPLE_VO_Usage(argv[0]);
        return HI_SUCCESS;
    }

#ifdef __HuaweiLite__
#else
    signal(SIGINT, SAMPLE_VO_HandleSig);
    signal(SIGTERM, SAMPLE_VO_HandleSig);
#endif

    s32Index = atoi(argv[1]);
    switch (s32Index) {
        case 0:
            s32Ret = SAMPLE_VO_RGBLCD_8BIT();
            break;
        default:
            SAMPLE_PRT("the index %d is invaild!\n", s32Index);
            SAMPLE_VO_Usage(argv[0]);
            return HI_FAILURE;
    }

    if (s32Ret == HI_SUCCESS) {
        SAMPLE_PRT("sample_vo exit success!\n");
    } else {
        SAMPLE_PRT("sample_vo exit abnormally!\n");
    }

    return s32Ret;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
