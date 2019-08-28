#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/mman.h>
#include <sys/prctl.h>

#ifdef __HuaweiLite__
#include <asm/io.h>
#endif

#include "hi_type.h"
#include "hi_scene.h"
#include "hi_scene_loadparam.h"
#include "sample_comm.h"
#include "hi_scene_prev.h"

#define SAMPLE_MIPI_PHYCFG_MODE_REG  (0x11240818)

#define DEFAULT_VIDEO_MODE_IDX (0)
#define IR_VIDEO_MODE_IDX (1)
#define QUICK_THREAD_STACK_SIZE (0x20000)
#define ISP_MAX_DEV_NUM     2

#ifdef __HuaweiLite__
#define himm(address, value) writel(value, address)
#define himd(address)        readl(address)
#endif

static SAMPLE_VI_CONFIG_S stViConfig;
static HI_SCENE_PARAM_S g_stSceneParam;
static HI_SCENE_VIDEO_MODE_S stVideoMode;
static HI_BOOL g_bIrEnable = HI_FALSE;
static HI_S32 g_s32SnsExp = 0;
static HI_S32 g_s32SnsAgain = 0;
static HI_S32 g_s32SnsDgain = 0;
static HI_S32 g_s32Ae = 0;
static HI_S32 g_s32LinesPer500ms = 0;
static HI_S32 g_s32Iso = 0;

static HI_CHAR *g_pszdirname = NULL;
static SAMPLE_SNS_TYPE_E g_sensorType = SMART_SC2231_MIPI_2M_30FPS_10BIT;
static VI_PIPE g_quickViPipe = 0;
static pthread_t    g_QuickIspPid[ISP_MAX_DEV_NUM] = {0};

#define VI_PIPE_IDX (0)
#define VI_DEV_IDX (0)

#ifndef __HuaweiLite__
HI_S32  s_s32MemDev = 0;
HI_S32 memopen( void )
{
    if (s_s32MemDev <= 0)
    {
        s_s32MemDev = open ("/dev/mem", O_CREAT|O_RDWR|O_SYNC);
        if (s_s32MemDev <= 0)
        {
            return -1;
        }
    }
    return 0;
}
HI_VOID memclose(void)
{
    close(s_s32MemDev);
}

void * memmap(HI_U32 u32PhyAddr, HI_U32 u32Size )
{
    HI_U32 u32Diff;
    HI_U32 u32PagePhy;
    HI_U32 u32PageSize;
    HI_U8 * pPageAddr;

    u32PagePhy = u32PhyAddr & 0xfffff000;
    u32Diff    = u32PhyAddr - u32PagePhy;

    u32PageSize = ((u32Size + u32Diff - 1) & 0xfffff000) + 0x1000;
    pPageAddr   = mmap ((void *)0, u32PageSize, PROT_READ|PROT_WRITE,
                                    MAP_SHARED, s_s32MemDev, u32PagePhy);
    if (MAP_FAILED == pPageAddr )
    {
        perror("mmap error\n");
        return NULL;
    }
    //printf("%08x, %08x\n", (HI_U32)pPageAddr, u32PageSize);
    return (void *) (pPageAddr + u32Diff);
}

HI_S32 memunmap(HI_VOID* pVirAddr, HI_U32 u32Size )
{
    HI_U32 u32PageAddr;
    HI_U32 u32PageSize;
    HI_U32 u32Diff;

    u32PageAddr = (((HI_U32)pVirAddr) & 0xfffff000);
    u32Diff     = (HI_U32)pVirAddr - u32PageAddr;
    u32PageSize = ((u32Size + u32Diff - 1) & 0xfffff000) + 0x1000;

    return munmap((HI_VOID*)u32PageAddr, u32PageSize);
}

static HI_S32 Linux_ReadReg(HI_U32 u32Addr)
{
    HI_S32 s32Value = 0;
    HI_U32 *pPortVirAddr;
    pPortVirAddr =   memmap(u32Addr,sizeof(u32Addr) );
    s32Value = *pPortVirAddr;
    memunmap(pPortVirAddr,sizeof(u32Addr));
    return s32Value;
}

static HI_VOID Linux_SetReg(HI_U32 u32Addr,HI_S32 s32Value)
{
    HI_U32 *pPortVirAddr;
    pPortVirAddr =   memmap(u32Addr,sizeof(u32Addr) );
    *pPortVirAddr = s32Value;
    memunmap(pPortVirAddr,sizeof(u32Addr));
}

#define himm(address, value) Linux_SetReg(address, value)
#define himd(address)        Linux_ReadReg(address)

#endif


HI_U32 sc2231_ReadSnsExp(HI_VOID);
HI_VOID sc2231_ReadSnsGain(HI_U32* pu32SnsAgain, HI_U32* pu32SnsDgain);
void sc2231_sensor_init(VI_PIPE ViPipe, HI_S32 s32SnsExp, HI_S32 s32SnsAgain, HI_S32 s32SnsDgain);
void sc2231_sensor_deinit(VI_PIPE ViPipe);


HI_U32 gc2053_ReadSnsExp(HI_VOID);
HI_VOID gc2053_ReadSnsGain(HI_U32* pu32SnsAgain, HI_U32* pu32SnsDgain);
void gc2053_sensor_init(VI_PIPE ViPipe, HI_S32 s32SnsExp, HI_S32 s32SnsAgain, HI_S32 s32SnsDgain);
void gc2053_sensor_deinit(VI_PIPE ViPipe);

HI_U32 SAMPLE_COMM_VI_GetMipiLaneDivideMode(SAMPLE_VI_CONFIG_S *pstViConfig);
HI_S32 SAMPLE_COMM_VI_SetMipiHsMode(lane_divide_mode_t enHsMode);
HI_S32 SAMPLE_COMM_VI_EnableMipiClock(SAMPLE_VI_CONFIG_S *pstViConfig);
HI_S32 SAMPLE_COMM_VI_ResetMipi(SAMPLE_VI_CONFIG_S *pstViConfig);
HI_S32 SAMPLE_COMM_VI_UnresetMipi(SAMPLE_VI_CONFIG_S *pstViConfig);
HI_S32 SAMPLE_COMM_VI_GetPipeAttrBySns(SAMPLE_SNS_TYPE_E enSnsType, VI_PIPE_ATTR_S *pstPipeAttr);
HI_S32 SAMPLE_COMM_VI_CreateVi(SAMPLE_VI_CONFIG_S *pstViConfig);
HI_S32 SAMPLE_COMM_VI_DestroyVi(SAMPLE_VI_CONFIG_S *pstViConfig);
HI_S32 SAMPLE_COMM_VI_StopMIPI(SAMPLE_VI_CONFIG_S *pstViConfig);

static HI_VOID sensor_i2c0_enable(void)
{
    HI_S32 s32Value = himd(0x120100F0);
    s32Value &= (~0x2);
    s32Value |= 0x1;
    himm(0x120100F0, s32Value);
}

HI_VOID QuickStart_Sensor_ReadExpGain(HI_S32* ps32SnsExp, HI_S32* ps32SnsAgain,
    HI_S32* ps32SnsDgain,  HI_S32* ps32Ae, HI_S32* ps32LinesPer500ms, HI_S32* ps32Iso)
{
    if (g_sensorType == SMART_SC2231_MIPI_2M_30FPS_10BIT)
    {
        *ps32SnsExp = sc2231_ReadSnsExp();
        sc2231_ReadSnsGain((HI_U32*)ps32SnsAgain, (HI_U32*)ps32SnsDgain);
    }
    else if (g_sensorType == GALAXYCORE_GC2053_MIPI_2M_30FPS_10BIT)
    {
        *ps32SnsExp = gc2053_ReadSnsExp();
        gc2053_ReadSnsGain((HI_U32*)ps32SnsAgain, (HI_U32*)ps32SnsDgain);
    }
    else
    {
        printf("error should define sensor type first");
    }

    ISP_EXP_INFO_S stExpInfo;
    memset(&stExpInfo, 0x00, sizeof(stExpInfo));
    HI_S32 s32Ret = HI_SUCCESS;
    s32Ret = HI_MPI_ISP_QueryExposureInfo(g_quickViPipe, &stExpInfo);
    if(s32Ret != HI_SUCCESS)
    {
        printf("HI_MPI_ISP_QueryExposureInfo failed\n");
    }
    *ps32Ae = (s32Ret != HI_SUCCESS) ? 0 : stExpInfo.u32Exposure;
    *ps32LinesPer500ms = (s32Ret != HI_SUCCESS) ? 0 : stExpInfo.u32LinesPer500ms;
    *ps32Iso = (s32Ret != HI_SUCCESS) ? 0 : stExpInfo.u32ISO;
}

HI_VOID QuickStart_Sensor_Init(HI_S32 s32SnsExp, HI_S32 s32SnsAgain, HI_S32 s32SnsDgain)
{
    sensor_i2c0_enable();

    if (g_sensorType == SMART_SC2231_MIPI_2M_30FPS_10BIT)
    {
        sc2231_sensor_init(VI_PIPE_IDX, s32SnsExp, s32SnsAgain, s32SnsDgain);
    }
    else if (g_sensorType == GALAXYCORE_GC2053_MIPI_2M_30FPS_10BIT)
    {
        gc2053_sensor_init(VI_PIPE_IDX, s32SnsExp, s32SnsAgain, s32SnsDgain);
    }
    else
    {
        printf("error should define sensor type first");
    }
}

HI_VOID QuickStart_Sensor_DeInit(HI_VOID)
{
    if (g_sensorType == SMART_SC2231_MIPI_2M_30FPS_10BIT)
    {
        sc2231_sensor_deinit(VI_PIPE_IDX);
    }
    else if (g_sensorType == GALAXYCORE_GC2053_MIPI_2M_30FPS_10BIT)
    {
        gc2053_sensor_deinit(VI_PIPE_IDX);
    }
    else
    {
        printf("error should define sensor type first");
    }
}

static HI_VOID QuickStart_SetMIPIReg(HI_VOID)
{
    HI_S32 s32Value = himd(SAMPLE_MIPI_PHYCFG_MODE_REG);
    s32Value &= ~0x7;
    s32Value |= 0x03;
    himm(SAMPLE_MIPI_PHYCFG_MODE_REG, s32Value);
}

static HI_S32 QuickStart_SetISPQuickStart(HI_VOID)
{
    HI_S32 s32Ret;
    ISP_MOD_PARAM_S stIspModPara;
    memset(&stIspModPara, 0x00, sizeof(stIspModPara));

    s32Ret = HI_MPI_ISP_GetModParam(&stIspModPara);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("HI_MPI_ISP_GetModParam failed!\n");
        return HI_FAILURE;
    }

    stIspModPara.u32QuickStart = HI_TRUE;
    stIspModPara.u32IntBotHalf = HI_TRUE;
    s32Ret = HI_MPI_ISP_SetModParam(&stIspModPara);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("HI_MPI_ISP_SetModParam failed!\n");
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

HI_S32 QuickStart_StartMIPI(SAMPLE_VI_CONFIG_S *pstViConfig)
{
    HI_S32 s32Ret = HI_SUCCESS;
    lane_divide_mode_t lane_divide_mode = LANE_DIVIDE_MODE_0;

    if (!pstViConfig)
    {
        SAMPLE_PRT("%s: null ptr\n", __FUNCTION__);
        return HI_FAILURE;
    }

    lane_divide_mode = SAMPLE_COMM_VI_GetMipiLaneDivideMode(pstViConfig);

    s32Ret = SAMPLE_COMM_VI_SetMipiHsMode(lane_divide_mode);

    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_SetMipiHsMode failed!\n");
        return HI_FAILURE;
    }

    s32Ret = SAMPLE_COMM_VI_EnableMipiClock(pstViConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_EnableMipiClock failed!\n");

        return HI_FAILURE;
    }

    s32Ret = SAMPLE_COMM_VI_ResetMipi(pstViConfig);

    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_ResetMipi failed!\n");

        return HI_FAILURE;
    }

    s32Ret = SAMPLE_COMM_VI_SetMipiAttr(pstViConfig);

    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_SetMipiAttr failed!\n");

        return HI_FAILURE;
    }

    s32Ret = SAMPLE_COMM_VI_UnresetMipi(pstViConfig);

    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_UnresetMipi failed!\n");

        return HI_FAILURE;
    }

    return HI_SUCCESS;
}


HI_S32 QuickStart_SetIspInitAttr(SAMPLE_VI_CONFIG_S *pstViConfig)
{
    HI_S32 s32Ret = HI_SUCCESS;
    ISP_SNS_OBJ_S* pstSnsObj = NULL;
    ISP_INIT_ATTR_S stInitAttr;
    memset(&stInitAttr, 0x00, sizeof(stInitAttr));
    stInitAttr.u32Exposure = g_s32Ae;

    if (g_sensorType == SMART_SC2231_MIPI_2M_30FPS_10BIT)
    {
        pstSnsObj = &stSnsSc2231Obj;
    }
    else if (g_sensorType == GALAXYCORE_GC2053_MIPI_2M_30FPS_10BIT)
    {
        pstSnsObj = &stSnsGc2053Obj;
    }
    else
    {
        printf("error should define sensor type first");
        return HI_FAILURE;
    }

    s32Ret = pstSnsObj->pfnSetInit(pstViConfig->astViInfo[0].stPipeInfo.aPipe[0], &stInitAttr);
    if (HI_SUCCESS != s32Ret)
    {
        printf("pstSnsObj->pfnSetInit failed!\n");
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

static void *QuickStart_ISP_Thread(void *param)
{
    HI_S32 s32Ret;
    ISP_DEV IspDev;
    HI_CHAR szThreadName[20];

    IspDev = (ISP_DEV)param;

    snprintf(szThreadName, 20, "ISP%d_RUN", IspDev);
    prctl(PR_SET_NAME, szThreadName, 0, 0, 0);

    SAMPLE_PRT("ISP Dev %d running !\n", IspDev);
    s32Ret = HI_MPI_ISP_Run(IspDev);

    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_ISP_Run failed with %#x!\n", s32Ret);
    }

    return NULL;
}

HI_S32 QuickStart_ISP_Run(ISP_DEV IspDev)
{
    HI_S32 s32Ret = 0;
#ifdef __HuaweiLite__
    pthread_attr_t stThreadAttr;
    pthread_attr_init(&stThreadAttr);

    //on liteos, use higer priority 9 instead of default 10 to ensure isp run
    //before auto scene thread
    stThreadAttr.inheritsched = PTHREAD_EXPLICIT_SCHED;
    stThreadAttr.schedparam.sched_priority = 9;

    s32Ret = pthread_create(&g_QuickIspPid[IspDev], &stThreadAttr, QuickStart_ISP_Thread, (HI_VOID *)IspDev);
#else
    s32Ret = pthread_create(&g_QuickIspPid[IspDev], NULL, QuickStart_ISP_Thread, (HI_VOID *)IspDev);
#endif
    if (0 != s32Ret)
    {
        SAMPLE_PRT("create isp running thread failed!, error: %d, %s\r\n", s32Ret, strerror(s32Ret));
        goto out;
    }


out:
#ifdef __HuaweiLite__
    pthread_attr_destroy(&stThreadAttr);
#endif
    return s32Ret;
}

HI_S32 QuickStart_StartIsp(SAMPLE_VI_INFO_S *pstViInfo)
{
    HI_S32              i;
    HI_BOOL             bNeedPipe;
    HI_S32              s32Ret = HI_SUCCESS;
    VI_PIPE             ViPipe;
    HI_U32              u32SnsId;
    ISP_PUB_ATTR_S      stPubAttr;
    VI_PIPE_ATTR_S      stPipeAttr;

    SAMPLE_COMM_VI_GetPipeAttrBySns(pstViInfo->stSnsInfo.enSnsType, &stPipeAttr);
    if (VI_PIPE_BYPASS_BE == stPipeAttr.enPipeBypassMode)
    {
        return HI_SUCCESS;
    }

    for (i = 0; i < WDR_MAX_PIPE_NUM; i++)
    {
        if (pstViInfo->stPipeInfo.aPipe[i] >= 0  && pstViInfo->stPipeInfo.aPipe[i] < VI_MAX_PIPE_NUM)
        {
            ViPipe      = pstViInfo->stPipeInfo.aPipe[i];
            u32SnsId    = pstViInfo->stSnsInfo.s32SnsId;

            SAMPLE_COMM_ISP_GetIspAttrBySns(pstViInfo->stSnsInfo.enSnsType, &stPubAttr);
            stPubAttr.enWDRMode = pstViInfo->stDevInfo.enWDRMode;

            if (WDR_MODE_NONE == pstViInfo->stDevInfo.enWDRMode)
            {
                bNeedPipe = HI_TRUE;
            }
            else
            {
                bNeedPipe = (i > 0) ? HI_FALSE : HI_TRUE;
            }

            if (HI_TRUE != bNeedPipe)
            {
                continue;
            }

            s32Ret = SAMPLE_COMM_ISP_Sensor_Regiter_callback(ViPipe, u32SnsId);

            if (HI_SUCCESS != s32Ret)
            {
                SAMPLE_PRT("register sensor %d to ISP %d failed\n", u32SnsId, ViPipe);
                SAMPLE_COMM_ISP_Stop(ViPipe);
                return HI_FAILURE;
            }

            if (((pstViInfo->stSnapInfo.bDoublePipe) && (pstViInfo->stSnapInfo.SnapPipe == ViPipe))
                || (pstViInfo->stPipeInfo.bMultiPipe && i > 0))
            {
                s32Ret = SAMPLE_COMM_ISP_BindSns(ViPipe, u32SnsId, pstViInfo->stSnsInfo.enSnsType, -1);

                if (HI_SUCCESS != s32Ret)
                {
                    SAMPLE_PRT("register sensor %d bus id %d failed\n", u32SnsId, pstViInfo->stSnsInfo.s32BusId);
                    SAMPLE_COMM_ISP_Stop(ViPipe);
                    return HI_FAILURE;
                }
            }
            else
            {
                s32Ret = SAMPLE_COMM_ISP_BindSns(ViPipe, u32SnsId, pstViInfo->stSnsInfo.enSnsType, pstViInfo->stSnsInfo.s32BusId);

                if (HI_SUCCESS != s32Ret)
                {
                    SAMPLE_PRT("register sensor %d bus id %d failed\n", u32SnsId, pstViInfo->stSnsInfo.s32BusId);
                    SAMPLE_COMM_ISP_Stop(ViPipe);
                    return HI_FAILURE;
                }
            }
            s32Ret = SAMPLE_COMM_ISP_Aelib_Callback(ViPipe);

            if (HI_SUCCESS != s32Ret)
            {
                SAMPLE_PRT("SAMPLE_COMM_ISP_Aelib_Callback failed\n");
                SAMPLE_COMM_ISP_Stop(ViPipe);
                return HI_FAILURE;
            }

            s32Ret = SAMPLE_COMM_ISP_Awblib_Callback(ViPipe);

            if (HI_SUCCESS != s32Ret)
            {
                SAMPLE_PRT("SAMPLE_COMM_ISP_Awblib_Callback failed\n");
                SAMPLE_COMM_ISP_Stop(ViPipe);
                return HI_FAILURE;
            }

            s32Ret = HI_MPI_ISP_MemInit(ViPipe);

            if (s32Ret != HI_SUCCESS)
            {
                SAMPLE_PRT("Init Ext memory failed with %#x!\n", s32Ret);
                SAMPLE_COMM_ISP_Stop(ViPipe);
                return HI_FAILURE;
            }

            s32Ret = HI_MPI_ISP_SetPubAttr(ViPipe, &stPubAttr);

            if (s32Ret != HI_SUCCESS)
            {
                SAMPLE_PRT("SetPubAttr failed with %#x!\n", s32Ret);
                SAMPLE_COMM_ISP_Stop(ViPipe);
                return HI_FAILURE;
            }

            s32Ret = HI_MPI_ISP_Init(ViPipe);

            if (s32Ret != HI_SUCCESS)
            {
                SAMPLE_PRT("ISP Init failed with %#x!\n", s32Ret);
                SAMPLE_COMM_ISP_Stop(ViPipe);
                return HI_FAILURE;
            }

            s32Ret = QuickStart_ISP_Run(ViPipe);
            if (s32Ret != HI_SUCCESS)
            {
                SAMPLE_PRT("ISP Run failed with %#x!\n", s32Ret);
                SAMPLE_COMM_ISP_Stop(ViPipe);
                return HI_FAILURE;
            }
        }
    }

    return s32Ret;
}

HI_S32 QuickStart_CreateIsp(SAMPLE_VI_CONFIG_S *pstViConfig)
{
    HI_S32              i;
    HI_S32              s32ViNum;
    HI_S32              s32Ret = HI_SUCCESS;

    SAMPLE_VI_INFO_S   *pstViInfo = HI_NULL;

    if (!pstViConfig)
    {
        SAMPLE_PRT("%s: null ptr\n", __FUNCTION__);
        return HI_FAILURE;
    }

    for (i = 0; i < pstViConfig->s32WorkingViNum; i++)
    {
        s32ViNum  = pstViConfig->as32WorkingViId[i];
        pstViInfo = &pstViConfig->astViInfo[s32ViNum];

        s32Ret = QuickStart_StartIsp(pstViInfo);

        if (s32Ret != HI_SUCCESS)
        {
            SAMPLE_PRT("SAMPLE_COMM_VI_StartIsp failed !\n");
            return HI_FAILURE;
        }
    }

    return HI_SUCCESS;
}

HI_S32 QuickStart_StartVi(SAMPLE_VI_CONFIG_S *pstViConfig)
{
    HI_S32 s32Ret = HI_SUCCESS;

    if (!pstViConfig)
    {
        SAMPLE_PRT("%s: null ptr\n", __FUNCTION__);
        return HI_FAILURE;
    }

    s32Ret = QuickStart_StartMIPI(pstViConfig);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_StartMIPI failed!\n");
        return HI_FAILURE;
    }
    QuickStart_SetMIPIReg();

    s32Ret = SAMPLE_COMM_VI_SetParam(pstViConfig);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_SetParam failed!\n");
        return HI_FAILURE;
    }

    s32Ret = SAMPLE_COMM_VI_CreateVi(pstViConfig);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_CreateVi failed!\n");
        return HI_FAILURE;
    }

    s32Ret = QuickStart_SetISPQuickStart();
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_VCAP_SetISPQuickStart failed!\n");
        return HI_FAILURE;
    }

    s32Ret = QuickStart_SetIspInitAttr(pstViConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("QuickStart_SetIspInitAttr failed!\n");
        return HI_FAILURE;
    }

    s32Ret = QuickStart_CreateIsp(pstViConfig);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_COMM_VI_DestroyVi(pstViConfig);
        SAMPLE_PRT("SAMPLE_COMM_VI_CreateIsp failed!\n");
        return HI_FAILURE;
    }


    return s32Ret;
}

HI_S32 Quickstart_VPSS_Start(VPSS_GRP VpssGrp, HI_BOOL* pabChnEnable, VPSS_GRP_ATTR_S* pstVpssGrpAttr, VPSS_CHN_ATTR_S* pastVpssChnAttr)
{
    VPSS_CHN VpssChn;
    HI_S32 s32Ret;
    HI_S32 j;
    VPSS_MOD_PARAM_S stModParam;

    s32Ret = HI_MPI_VPSS_GetModParam(&stModParam);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("HI_MPI_VPSS_GetModParam(grp:%d) failed with %#x!\n", VpssGrp, s32Ret);
        return HI_FAILURE;
    }
    stModParam.bNrQuickStart = HI_TRUE;
    s32Ret = HI_MPI_VPSS_SetModParam(&stModParam);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("HI_MPI_VPSS_SetModParam(grp:%d) failed with %#x!\n", VpssGrp, s32Ret);
        return HI_FAILURE;
    }

    s32Ret = HI_MPI_VPSS_CreateGrp(VpssGrp, pstVpssGrpAttr);

    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("HI_MPI_VPSS_CreateGrp(grp:%d) failed with %#x!\n", VpssGrp, s32Ret);
        return HI_FAILURE;
    }

    for (j = 0; j < VPSS_MAX_PHY_CHN_NUM; j++)
    {
        if(HI_TRUE == pabChnEnable[j])
        {
            VpssChn = j;
            s32Ret = HI_MPI_VPSS_SetChnAttr(VpssGrp, VpssChn, &pastVpssChnAttr[VpssChn]);

            if (s32Ret != HI_SUCCESS)
            {
                SAMPLE_PRT("HI_MPI_VPSS_SetChnAttr failed with %#x\n", s32Ret);
                return HI_FAILURE;
            }

            if((VpssChn == 0))
            {
            #define VPSS_BUF_WRAP_LINE (480)
                VPSS_CHN_BUF_WRAP_S stBufWap;
                stBufWap.bEnable = HI_TRUE;
                stBufWap.u32BufLine = VPSS_BUF_WRAP_LINE;
                stBufWap.u32WrapBufferSize = VPSS_GetWrapBufferSize(1920,\
        1080, VPSS_BUF_WRAP_LINE, PIXEL_FORMAT_YVU_SEMIPLANAR_420, DATA_BITWIDTH_8, COMPRESS_MODE_NONE, DEFAULT_ALIGN);;
                s32Ret = HI_MPI_VPSS_SetChnBufWrapAttr(VpssGrp, VpssChn, &stBufWap);
                if (s32Ret != HI_SUCCESS)
                {
                    SAMPLE_PRT("HI_MPI_VPSS_SetChnBufWrapAttr failed with %#x\n", s32Ret);
                    return HI_FAILURE;
                }
            }

            s32Ret = HI_MPI_VPSS_EnableChn(VpssGrp, VpssChn);

            if (s32Ret != HI_SUCCESS)
            {
                SAMPLE_PRT("HI_MPI_VPSS_EnableChn failed with %#x\n", s32Ret);
                return HI_FAILURE;
            }
        }
    }

    s32Ret = HI_MPI_VPSS_StartGrp(VpssGrp);

    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("HI_MPI_VPSS_StartGrp failed with %#x\n", s32Ret);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

static HI_S32 Quickstart_StartMedia(HI_VOID)
{
    HI_S32             s32Ret = HI_SUCCESS;

    HI_S32             s32ViCnt       = 1;
    VI_DEV             ViDev          = 0;
    VI_CHN             ViChn          = 0;
    HI_S32             s32WorkSnsId   = 0;

    SIZE_S             stSize;
    VB_CONFIG_S        stVbConf;
    PIC_SIZE_E         enPicSize;
    HI_U32             u32BlkSize;

    WDR_MODE_E         enWDRMode      = WDR_MODE_NONE;
    DYNAMIC_RANGE_E    enDynamicRange = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E     enPixFormat    = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    VIDEO_FORMAT_E     enVideoFormat  = VIDEO_FORMAT_LINEAR;
    COMPRESS_MODE_E    enCompressMode = COMPRESS_MODE_NONE;
    VI_VPSS_MODE_E     enMastPipeMode = VI_ONLINE_VPSS_ONLINE;

    VPSS_GRP           VpssGrp        = 0;
    VPSS_GRP_ATTR_S    stVpssGrpAttr;
    VPSS_CHN           VpssChn        = VPSS_CHN0;
    HI_BOOL            abChnEnable[VPSS_MAX_PHY_CHN_NUM] = {0};
    VPSS_CHN_ATTR_S    astVpssChnAttr[VPSS_MAX_PHY_CHN_NUM];

    VENC_CHN           VencChn[1]  = {0};
    PAYLOAD_TYPE_E     enType      = PT_H265;
    SAMPLE_RC_E        enRcMode    = SAMPLE_RC_CBR;
    HI_U32             u32Profile  = 0;
    HI_BOOL            bRcnRefShareBuf = HI_TRUE;
    VENC_GOP_ATTR_S    stGopAttr;

    memset(&stViConfig, 0x00, sizeof(stViConfig));

    /*config vi*/
    SAMPLE_COMM_VI_GetSensorInfo(&stViConfig);

    stViConfig.s32WorkingViNum                                   = s32ViCnt;
    stViConfig.as32WorkingViId[0]                                = 0;
    stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.MipiDev         = ViDev;
    stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.s32BusId        = 0;
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.ViDev           = ViDev;
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.enWDRMode       = enWDRMode;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.enMastPipeMode = enMastPipeMode;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[0]       = g_quickViPipe;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[1]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.ViChn           = ViChn;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enPixFormat     = enPixFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enDynamicRange  = enDynamicRange;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enVideoFormat   = enVideoFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enCompressMode  = enCompressMode;

    /*get picture size*/
    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType, &enPicSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size by sensor failed!\n");
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enPicSize, &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size failed!\n");
        return s32Ret;
    }

    /*config vb*/
    hi_memset(&stVbConf, sizeof(VB_CONFIG_S), 0, sizeof(VB_CONFIG_S));
    stVbConf.u32MaxPoolCnt              = 1;

    u32BlkSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8, COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[0].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt   = 5;

    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        return s32Ret;
    }

    /*start vi*/
    s32Ret = QuickStart_StartVi(&stViConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed.s32Ret:0x%x !\n", s32Ret);
        goto EXIT;
    }

    printf("vi start ok\n");
    /*config vpss*/
    hi_memset(&stVpssGrpAttr, sizeof(VPSS_GRP_ATTR_S), 0, sizeof(VPSS_GRP_ATTR_S));
    stVpssGrpAttr.stFrameRate.s32SrcFrameRate    = -1;
    stVpssGrpAttr.stFrameRate.s32DstFrameRate    = -1;
    stVpssGrpAttr.enDynamicRange                 = DYNAMIC_RANGE_SDR8;
    stVpssGrpAttr.enPixelFormat                  = enPixFormat;
    stVpssGrpAttr.u32MaxW                        = stSize.u32Width;
    stVpssGrpAttr.u32MaxH                        = stSize.u32Height;
    stVpssGrpAttr.bNrEn                          = HI_TRUE;
    stVpssGrpAttr.stNrAttr.enCompressMode        = COMPRESS_MODE_NONE;
    stVpssGrpAttr.stNrAttr.enNrMotionMode        = NR_MOTION_MODE_NORMAL;

    astVpssChnAttr[VpssChn].u32Width                    = stSize.u32Width;
    astVpssChnAttr[VpssChn].u32Height                   = stSize.u32Height;
    astVpssChnAttr[VpssChn].enChnMode                   = VPSS_CHN_MODE_USER;
    astVpssChnAttr[VpssChn].enCompressMode              = COMPRESS_MODE_NONE;
    astVpssChnAttr[VpssChn].enDynamicRange              = enDynamicRange;
    astVpssChnAttr[VpssChn].enVideoFormat               = enVideoFormat;
    astVpssChnAttr[VpssChn].enPixelFormat               = enPixFormat;
    astVpssChnAttr[VpssChn].stFrameRate.s32SrcFrameRate = 30;
    astVpssChnAttr[VpssChn].stFrameRate.s32DstFrameRate = 30;
    astVpssChnAttr[VpssChn].u32Depth                    = 0;
    astVpssChnAttr[VpssChn].bMirror                     = HI_FALSE;
    astVpssChnAttr[VpssChn].bFlip                       = HI_FALSE;
    astVpssChnAttr[VpssChn].stAspectRatio.enMode        = ASPECT_RATIO_NONE;

    astVpssChnAttr[VPSS_CHN1].u32Width                    = 720;
    astVpssChnAttr[VPSS_CHN1].u32Height                   = 576;
    astVpssChnAttr[VPSS_CHN1].enChnMode                   = VPSS_CHN_MODE_USER;
    astVpssChnAttr[VPSS_CHN1].enCompressMode              = enCompressMode;
    astVpssChnAttr[VPSS_CHN1].enDynamicRange              = enDynamicRange;
    astVpssChnAttr[VPSS_CHN1].enVideoFormat               = enVideoFormat;
    astVpssChnAttr[VPSS_CHN1].enPixelFormat               = enPixFormat;
    astVpssChnAttr[VPSS_CHN1].stFrameRate.s32SrcFrameRate = 30;
    astVpssChnAttr[VPSS_CHN1].stFrameRate.s32DstFrameRate = 30;
    astVpssChnAttr[VPSS_CHN1].u32Depth                    = 0;
    astVpssChnAttr[VPSS_CHN1].bMirror                     = HI_FALSE;
    astVpssChnAttr[VPSS_CHN1].bFlip                       = HI_FALSE;
    astVpssChnAttr[VPSS_CHN1].stAspectRatio.enMode        = ASPECT_RATIO_NONE;
    /*start vpss*/
    abChnEnable[0] = HI_TRUE;
    abChnEnable[1] = HI_TRUE;

    s32Ret = Quickstart_VPSS_Start(VpssGrp, abChnEnable, &stVpssGrpAttr, astVpssChnAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vpss group failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT1;
    }

    //run auto scene after isp run
    HI_U32 u32Idx = g_bIrEnable ? IR_VIDEO_MODE_IDX : DEFAULT_VIDEO_MODE_IDX;
    s32Ret = HI_SCENE_SetSceneMode(&(stVideoMode.astVideoMode[u32Idx]));
    if (HI_SUCCESS != s32Ret)
    {
        printf("HI_SCENE_SetSceneMode failed\n");
        SAMPLE_COMM_ISP_Stop(g_quickViPipe);
        return HI_FAILURE;
    }

    /*config venc */
    stGopAttr.enGopMode  = VENC_GOPMODE_NORMALP; //VENC_GOPMODE_SMARTP
    stGopAttr.stNormalP.s32IPQpDelta = 2;
    s32Ret = SAMPLE_COMM_VENC_Start(VencChn[0], enType, enPicSize, enRcMode, u32Profile,bRcnRefShareBuf, &stGopAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start venc failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT2;
    }

    s32Ret = SAMPLE_COMM_VPSS_Bind_VENC(VpssGrp, VpssChn, VencChn[0]);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Venc bind Vpss failed. s32Ret: 0x%x !n", s32Ret);
        goto EXIT3;
    }

    s32Ret = SAMPLE_COMM_VENC_StartGetStream(VencChn, sizeof(VencChn)/sizeof(VENC_CHN));
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Get venc stream failed!\n");
        goto EXIT4;
    }

    return HI_SUCCESS;

EXIT4:
    SAMPLE_COMM_VPSS_UnBind_VENC(VpssGrp, VpssChn, VencChn[0]);
EXIT3:
    SAMPLE_COMM_VENC_Stop(VencChn[0]);
EXIT2:
    SAMPLE_COMM_VPSS_Stop(VpssGrp, abChnEnable);
EXIT1:
    SAMPLE_COMM_VI_StopVi(&stViConfig);
EXIT:
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}

HI_VOID Quickstart_ISP_Stop(ISP_DEV IspDev)
{
    if (g_QuickIspPid[IspDev])
    {
        HI_MPI_ISP_Exit(IspDev);
        pthread_join(g_QuickIspPid[IspDev], NULL);
        SAMPLE_COMM_ISP_Awblib_UnCallback(IspDev);
        SAMPLE_COMM_ISP_Aelib_UnCallback(IspDev);
        SAMPLE_COMM_ISP_Sensor_UnRegiter_callback(IspDev);
        g_QuickIspPid[IspDev] = 0;
    }

    return;
}

HI_S32 Quickstart_StopIsp(SAMPLE_VI_INFO_S *pstViInfo)
{
    HI_S32  i;
    HI_BOOL bNeedPipe;
    VI_PIPE ViPipe;

    for (i = 0; i < WDR_MAX_PIPE_NUM; i++)
    {
        if (pstViInfo->stPipeInfo.aPipe[i] >= 0  && pstViInfo->stPipeInfo.aPipe[i] < VI_MAX_PIPE_NUM)
        {
            ViPipe    = pstViInfo->stPipeInfo.aPipe[i];

            if (WDR_MODE_NONE == pstViInfo->stDevInfo.enWDRMode)
            {
                bNeedPipe = HI_TRUE;
            }
            else
            {
                bNeedPipe = (i > 0) ? HI_FALSE : HI_TRUE;
            }

            if (HI_TRUE != bNeedPipe)
            {
                continue;
            }

            Quickstart_ISP_Stop(ViPipe);
        }
    }

    return HI_SUCCESS;
}

HI_S32 Quickstart_DestroyIsp(SAMPLE_VI_CONFIG_S *pstViConfig)
{
    HI_S32              i;
    HI_S32              s32ViNum;
    HI_S32              s32Ret = HI_SUCCESS;
    SAMPLE_VI_INFO_S   *pstViInfo = HI_NULL;

    if (!pstViConfig)
    {
        SAMPLE_PRT("%s: null ptr\n", __FUNCTION__);
        return HI_FAILURE;
    }

    for (i = 0; i < pstViConfig->s32WorkingViNum; i++)
    {
        s32ViNum  = pstViConfig->as32WorkingViId[i];
        pstViInfo = &pstViConfig->astViInfo[s32ViNum];
        s32Ret = Quickstart_StopIsp(pstViInfo);
        if (s32Ret != HI_SUCCESS)
        {
            SAMPLE_PRT("SAMPLE_COMM_VI_StopIsp failed !\n");
            return HI_FAILURE;
        }
    }

    return HI_SUCCESS;
}

HI_S32 Quickstart_StopVi(SAMPLE_VI_CONFIG_S *pstViConfig)
{
    HI_S32 s32Ret = HI_SUCCESS;

    s32Ret = Quickstart_DestroyIsp(pstViConfig);

    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_DestroyIsp failed !\n");
        return HI_FAILURE;
    }

    s32Ret = SAMPLE_COMM_VI_DestroyVi(pstViConfig);

    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_DestroyVi failed !\n");
        return HI_FAILURE;
    }

    s32Ret = SAMPLE_COMM_VI_StopMIPI(pstViConfig);

    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_StopMIPI failed !\n");
        return HI_FAILURE;
    }

    return s32Ret;
}

HI_S32 SAMPLE_StopMedia(HI_VOID)
{
    VENC_CHN           VencChn[1]  = {0};
    HI_BOOL            abChnEnable[VPSS_MAX_PHY_CHN_NUM] = {HI_TRUE, HI_TRUE, HI_FALSE};
    VPSS_GRP           VpssGrp        = 0;
    VPSS_CHN           VpssChn        = VPSS_CHN0;

    SAMPLE_COMM_VENC_StopGetStream();

    SAMPLE_COMM_VPSS_UnBind_VENC(VpssGrp, VpssChn, VencChn[0]);
    SAMPLE_COMM_VENC_Stop(VencChn[0]);
    SAMPLE_COMM_VPSS_Stop(VpssGrp, abChnEnable);
    Quickstart_StopVi(&stViConfig);
    SAMPLE_COMM_SYS_Exit();

    return HI_SUCCESS;
}

HI_VOID SAMPLE_SCENE_HandleSig(HI_S32 signo)
{
    HI_S32 s32Ret;

    if (SIGINT == signo || SIGTERM == signo)
    {
        s32Ret = HI_SCENE_Deinit();
        if (s32Ret != 0)
        {
            printf("HI_SCENE_DeInit failed\n");
            exit(-1);
        }
    }
    exit(-1);
}


static HI_S64 QuickStart_GetSceneExp(HI_VOID)
{
    HI_S64 s64ExpVal = 0;
    HI_U32 u32ExpTimeUs = 0;

    //expTimePerImg = expLinesPerImg * 500,000Us / expLinesPer500ms
    u32ExpTimeUs = (((HI_U32)g_s32SnsExp) * 500000) / ((HI_U32)g_s32LinesPer500ms);
    s64ExpVal = (((HI_S64)g_s32Iso)* u32ExpTimeUs) / 100;

    return s64ExpVal;
}

static HI_VOID* QuickStart_Service_Thread(HI_VOID* args)
{
    HI_S32 s32Ret = HI_SUCCESS;

    s32Ret = HI_SCENE_CreateParam(g_pszdirname,&g_stSceneParam, &stVideoMode);  //stSceneParam & stVideoMode get value
    if (HI_SUCCESS != s32Ret)
    {
        printf("SCENETOOL_CreateParam failed\n");
        return NULL;
    }

    s32Ret = HI_SCENE_Init(&g_stSceneParam);
    if (HI_SUCCESS != s32Ret)
    {
        printf("HI_SCENE_Init failed\n");
        return NULL;
    }

    s32Ret = HI_SCENE_SetSceneInitExp(g_s32Iso, QuickStart_GetSceneExp());
    if (HI_SUCCESS != s32Ret)
    {
        printf("HI_SCENE_Init failed\n");
        return NULL;
    }

    QuickStart_Sensor_Init(g_s32SnsExp, g_s32SnsAgain, g_s32SnsDgain);

    //skip sensor first and second black frame
    usleep(36*1000);

    s32Ret = Quickstart_StartMedia();
    if (HI_SUCCESS != s32Ret)
    {
        printf("Quickstart_StartMedia failed\n");
        return NULL;
    }

    //printf("input any key to exit the sample\n");
    // int count = 0;
    // while(1)
    // {
    //     sleep(1);
    //     count++;
    //     if(count ==20)
    //         break;
    // }

    printf("input any key to exit the sample\n");

    HI_CHAR aszinput[10] = {0};
    fgets(aszinput, 10, stdin);

    HI_S32 s32OutSnsExp = 0;
    HI_S32 s32OutSnsAgain = 0;
    HI_S32 s32OutSnsDgain = 0;
    HI_S32 s32OutAe = 0;
    HI_S32 s32LinesPer500ms = 0;
    HI_S32 s32Iso = 0;
    QuickStart_Sensor_ReadExpGain(&s32OutSnsExp, &s32OutSnsAgain, &s32OutSnsDgain, &s32OutAe, &s32LinesPer500ms, &s32Iso);

    printf("output s32SnsExp: 0x%x s32SnsAgain: 0x%x s32SnsDgain: 0x%x s32Ae: %d s32LInesPer500ms:%d s32Iso:%d\n",
        s32OutSnsExp, s32OutSnsAgain, s32OutSnsDgain, s32OutAe, s32LinesPer500ms, s32Iso);

    s32Ret = HI_SCENE_Deinit();
    if (HI_SUCCESS != s32Ret)
    {
        printf("HI_SCENE_Deinit failed\n");
    }
    printf("The scene sample is end.\n");

    SAMPLE_StopMedia();

    QuickStart_Sensor_DeInit();
    return NULL;
}

#ifdef __HuaweiLite__
HI_S32 app_main(HI_S32 argc, HI_CHAR *argv[])
#else
HI_S32 main(HI_S32 argc, HI_CHAR *argv[])
#endif
{
    HI_S32 s32Ret = HI_SUCCESS;

    if (argc < 9)
    {
        printf("Usage : %s <inidir>\n\t\tfor example :./sample_quick /param/sensor_2231 s32SnsExp s32SnsAgain s32SnsDgain s32Ae s32LinesPer500ms s32Iso s32IRSwitch\n", argv[0]);
        return HI_SUCCESS;
    }

    g_sensorType = SENSOR0_TYPE;
    if(g_sensorType >= SAMPLE_SNS_TYPE_BUTT)
    {
        printf("sensor type error\n");
        return HI_FAILURE;
    }

    g_pszdirname = argv[1];
    g_s32SnsExp = strtol(argv[2], NULL, 0);
    g_s32SnsAgain = strtol(argv[3], NULL, 0);
    g_s32SnsDgain = strtol(argv[4], NULL, 0);
    g_s32Ae = strtol(argv[5], NULL, 0);
    g_s32LinesPer500ms = strtol(argv[6], NULL, 0);
    g_s32Iso = strtol(argv[7], NULL, 0);

    HI_S32 s32IRSwitch = strtol(argv[8], NULL, 0);
    g_bIrEnable = s32IRSwitch ? HI_TRUE : HI_FALSE;

    printf("input s32SnsExp: %d s32SnsAgain: %d s32SnsDgain: %d s32Ae: %d s32LinesPer500ms: %d  g_s32Iso: %d bIrEnable: %d \n",
        g_s32SnsExp, g_s32SnsAgain, g_s32SnsDgain, g_s32Ae, g_s32LinesPer500ms, g_s32Iso, g_bIrEnable);


#ifndef __HuaweiLite__
    signal(SIGINT, SAMPLE_SCENE_HandleSig);
    signal(SIGTERM, SAMPLE_SCENE_HandleSig);
#endif

#ifdef __HuaweiLite__
    pthread_attr_t stThrdAttr;
    pthread_attr_init(&stThrdAttr);
    pthread_attr_setdetachstate(&stThrdAttr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setstacksize(&stThrdAttr, QUICK_THREAD_STACK_SIZE);

    pthread_t threadID = -1;
    //iniparser use large stack size, just use thread on liteos
    s32Ret = pthread_create(&threadID, &stThrdAttr, QuickStart_Service_Thread, NULL);
    if (HI_SUCCESS != s32Ret)
    {
        printf("pthread_create failed\n");
        return HI_FAILURE;
    }
#else
    s32Ret = memopen();
    if (HI_SUCCESS != s32Ret)
    {
        printf("memopen failed\n");
        return HI_FAILURE;
    }
    QuickStart_Service_Thread(NULL);
    memclose();
#endif

    return HI_SUCCESS;
}
