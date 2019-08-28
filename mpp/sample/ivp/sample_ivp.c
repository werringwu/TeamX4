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
#include <sys/prctl.h>
#include "hi_type.h"
#include "hi_common.h"
#include "sample_comm.h"
#include "hi_ivp.h"
#include "hi_securec.h"

#define VB_MAX_NUM            10
#define ONLINE_LIMIT_WIDTH    2304

typedef struct {
    hi_s32 ivp_handle;
    hi_ivp_mem_info ivp_mem_info;
} sample_ivp_param;

typedef struct {
    hi_bool low_bitrate_en;
    hi_bool iso_adaptive_en;
    hi_bool advance_isp_en;
    hi_u32 iso_threshold[HI_IVP_VENC_MAX_ISO_THRESHOLD_LEVEL];
} sample_ivp_effect_param;

typedef struct {
    DYNAMIC_RANGE_E   dynamic_range;
    PIXEL_FORMAT_E    pixel_format;
    COMPRESS_MODE_E   compress_mode[VPSS_MAX_PHY_CHN_NUM];
    SIZE_S            output_size[VPSS_MAX_PHY_CHN_NUM];
    FRAME_RATE_CTRL_S frame_rate[VPSS_MAX_PHY_CHN_NUM];
    hi_bool           is_mirror[VPSS_MAX_PHY_CHN_NUM];
    hi_bool           is_flip[VPSS_MAX_PHY_CHN_NUM];
    hi_bool           chn_enable[VPSS_MAX_PHY_CHN_NUM];
    hi_u32            big_stream_id;
    hi_u32            small_stream_id;
    VI_VPSS_MODE_E    vi_vpss_mode;
    hi_bool           wrap_en;
    hi_u32            wrap_buf_line;
} sample_vpss_chn_attr;

typedef struct {
    hi_u32            valid_num;
    HI_U64            blk_size[VB_MAX_NUM];
    hi_u32            blk_cnt[VB_MAX_NUM];
    hi_u32            supplement_config;
} sample_vb_attr;

const SAMPLE_SNS_TYPE_E SNS_TYPE = SENSOR0_TYPE;

sample_ivp_param g_ivp_param;
sample_ivp_effect_param g_ivp_effect_param;

hi_bool   g_ivp_start_flag = HI_FALSE;
pthread_t g_ivp_thread;

SAMPLE_VI_CONFIG_S g_vi_config;

static HI_VOID get_sensor_resolution(SAMPLE_SNS_TYPE_E sns_type, SIZE_S *size)
{
    hi_s32 ret;
    SIZE_S tmp_size;
    PIC_SIZE_E pic_size;

    ret = SAMPLE_COMM_VI_GetSizeBySensor(sns_type, &pic_size);
    if (ret != HI_SUCCESS) {
        SAMPLE_PRT("SAMPLE_COMM_VI_GetSizeBySensor failed!\n");
        return;
    }

    ret = SAMPLE_COMM_SYS_GetPicSize(pic_size, &tmp_size);
    if (ret != HI_SUCCESS) {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        return;
    }

    *size = tmp_size;

    return;
}

static hi_u32 get_frame_rate_from_sensor_type(SAMPLE_SNS_TYPE_E sns_type)
{
    hi_u32 frame_rate;

    SAMPLE_COMM_VI_GetFrameRateBySensor(sns_type, &frame_rate);

    return frame_rate;
}

static hi_u32 get_full_lines_std_from_sensor_type(SAMPLE_SNS_TYPE_E sns_type)
{
    hi_u32 full_lines_std = 0;

    switch (sns_type)
    {
        case SONY_IMX327_MIPI_2M_30FPS_12BIT:
        case SONY_IMX327_MIPI_2M_30FPS_12BIT_WDR2TO1:
            full_lines_std = 1125;
            break;
        case SONY_IMX307_MIPI_2M_30FPS_12BIT:
        case SONY_IMX307_MIPI_2M_30FPS_12BIT_WDR2TO1:
        case SONY_IMX307_2L_MIPI_2M_30FPS_12BIT:
        case SONY_IMX307_2L_MIPI_2M_30FPS_12BIT_WDR2TO1:
        case SMART_SC2235_DC_2M_30FPS_10BIT:
        case SMART_SC2231_MIPI_2M_30FPS_10BIT:
            full_lines_std = 1125;
            break;
        case SONY_IMX335_MIPI_5M_30FPS_12BIT:
        case SONY_IMX335_MIPI_5M_30FPS_10BIT_WDR2TO1:
            full_lines_std = 1875;
            break;
        case SONY_IMX335_MIPI_4M_30FPS_12BIT:
        case SONY_IMX335_MIPI_4M_30FPS_10BIT_WDR2TO1:
            full_lines_std = 1375;
            break;
        case SMART_SC4236_MIPI_3M_30FPS_10BIT:
        case SMART_SC4236_MIPI_3M_20FPS_10BIT:
            full_lines_std = 1600;
            break;
        default:
            SAMPLE_PRT("Error: Not support this sensor now! ==> %d\n", sns_type);
            break;
    }

    return full_lines_std;
}

static HI_VOID get_vpss_wrap_buf_line(sample_vpss_chn_attr *vpss_attr)
{
    hi_s32 ret;
    hi_u32 buf_line = 0;
    VPSS_VENC_WRAP_PARAM_S wrap_param;

    memset(&wrap_param, 0, sizeof(VPSS_VENC_WRAP_PARAM_S));
    wrap_param.bAllOnline = (vpss_attr->vi_vpss_mode == VI_ONLINE_VPSS_ONLINE) ? HI_TRUE : HI_TRUE;
    wrap_param.u32FrameRate = get_frame_rate_from_sensor_type(SNS_TYPE);
    wrap_param.u32FullLinesStd = get_full_lines_std_from_sensor_type(SNS_TYPE);
    wrap_param.stLargeStreamSize.u32Width = vpss_attr->output_size[vpss_attr->big_stream_id].u32Width;
    wrap_param.stLargeStreamSize.u32Height = vpss_attr->output_size[vpss_attr->big_stream_id].u32Height;
    wrap_param.stSmallStreamSize.u32Width = vpss_attr->output_size[vpss_attr->small_stream_id].u32Width;
    wrap_param.stSmallStreamSize.u32Height = vpss_attr->output_size[vpss_attr->small_stream_id].u32Height;

    ret = HI_MPI_SYS_GetVPSSVENCWrapBufferLine(&wrap_param, &buf_line);
    if (ret != HI_SUCCESS) {
        SAMPLE_PRT("Error:Current BigStream(%dx%d@%d fps) and SmallStream(%dx%d@%d fps) not support Ring!== return 0x%x(0x%x)\n",
            wrap_param.stLargeStreamSize.u32Width, wrap_param.stLargeStreamSize.u32Height, wrap_param.u32FrameRate,
            wrap_param.stSmallStreamSize.u32Width, wrap_param.stSmallStreamSize.u32Height, wrap_param.u32FrameRate,
            ret, HI_ERR_SYS_NOT_SUPPORT);
        buf_line = 0;
    }

    vpss_attr->wrap_buf_line = buf_line;

    return;
}

static HI_VOID sample_ivp_get_default_vpss_attr(SIZE_S size[], sample_vpss_chn_attr *vpss_attr)
{
    hi_s32 i;

    memset(vpss_attr, 0, sizeof(sample_vpss_chn_attr));

    vpss_attr->big_stream_id = 0;
    vpss_attr->small_stream_id = 1;
    vpss_attr->chn_enable[vpss_attr->big_stream_id] = HI_TRUE;
    vpss_attr->chn_enable[vpss_attr->small_stream_id] = HI_TRUE;
    for (i = 0; i < VPSS_MAX_PHY_CHN_NUM; i++) {
        if (vpss_attr->chn_enable[i] == HI_TRUE) {
            vpss_attr->compress_mode[i] = (i == vpss_attr->big_stream_id) ? COMPRESS_MODE_SEG : COMPRESS_MODE_NONE;
            vpss_attr->frame_rate[i].s32SrcFrameRate = -1;
            vpss_attr->frame_rate[i].s32DstFrameRate = -1;
            vpss_attr->is_mirror[i] = HI_FALSE;
            vpss_attr->is_flip[i] = HI_FALSE;
        }
    }
    vpss_attr->output_size[vpss_attr->big_stream_id].u32Width = size[vpss_attr->big_stream_id].u32Width;
    vpss_attr->output_size[vpss_attr->big_stream_id].u32Height = size[vpss_attr->big_stream_id].u32Height;
    vpss_attr->output_size[vpss_attr->small_stream_id].u32Width = size[vpss_attr->small_stream_id].u32Width;
    vpss_attr->output_size[vpss_attr->small_stream_id].u32Height = size[vpss_attr->small_stream_id].u32Height;
    vpss_attr->dynamic_range = DYNAMIC_RANGE_SDR8;
    vpss_attr->pixel_format  = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    if (size[vpss_attr->big_stream_id].u32Width > ONLINE_LIMIT_WIDTH) {
        vpss_attr->wrap_en = HI_FALSE;
        vpss_attr->vi_vpss_mode = VI_OFFLINE_VPSS_ONLINE;
    } else {
        vpss_attr->wrap_en = HI_TRUE;
        vpss_attr->vi_vpss_mode = VI_ONLINE_VPSS_ONLINE;
    }
}

static HI_VOID sample_ivp_get_comm_vb_attr(const sample_vpss_chn_attr *vpss_attr,
    hi_bool support_dcf, sample_vb_attr *vb_attr)
{
    if (vpss_attr->vi_vpss_mode != VI_ONLINE_VPSS_ONLINE) {
        SIZE_S size = {0};
        get_sensor_resolution(SNS_TYPE, &size);

        if (vpss_attr->vi_vpss_mode == VI_OFFLINE_VPSS_ONLINE || vpss_attr->vi_vpss_mode == VI_OFFLINE_VPSS_OFFLINE) {
            vb_attr->blk_size[vb_attr->valid_num] = VI_GetRawBufferSize(size.u32Width, size.u32Height,
                                                                      PIXEL_FORMAT_RGB_BAYER_12BPP,
                                                                      COMPRESS_MODE_NONE,
                                                                      DEFAULT_ALIGN);
            vb_attr->blk_cnt[vb_attr->valid_num]  = 3;
            vb_attr->valid_num++;
        }

        if (vpss_attr->vi_vpss_mode == VI_OFFLINE_VPSS_OFFLINE) {
            vb_attr->blk_size[vb_attr->valid_num] = COMMON_GetPicBufferSize(size.u32Width, size.u32Height,
                                                                          PIXEL_FORMAT_YVU_SEMIPLANAR_420,
                                                                          DATA_BITWIDTH_8,
                                                                          COMPRESS_MODE_NONE,
                                                                          DEFAULT_ALIGN);
            vb_attr->blk_cnt[vb_attr->valid_num]  = 2;
            vb_attr->valid_num++;
        }

        if (vpss_attr->vi_vpss_mode == VI_ONLINE_VPSS_OFFLINE) {
            vb_attr->blk_size[vb_attr->valid_num] = COMMON_GetPicBufferSize(size.u32Width, size.u32Height,
                                                                          PIXEL_FORMAT_YVU_SEMIPLANAR_420,
                                                                          DATA_BITWIDTH_8,
                                                                          COMPRESS_MODE_NONE,
                                                                          DEFAULT_ALIGN);
            vb_attr->blk_cnt[vb_attr->valid_num]  = 3;
            vb_attr->valid_num++;

        }
    }
    if(vpss_attr->wrap_en == HI_TRUE) {
        vb_attr->blk_size[vb_attr->valid_num] = VPSS_GetWrapBufferSize(vpss_attr->output_size[0].u32Width,
                                                                     vpss_attr->output_size[0].u32Height,
                                                                     vpss_attr->wrap_buf_line,
                                                                     vpss_attr->pixel_format,
                                                                     DATA_BITWIDTH_8,
                                                                     COMPRESS_MODE_NONE,
                                                                     DEFAULT_ALIGN);
        vb_attr->blk_cnt[vb_attr->valid_num]  = 1;
        vb_attr->valid_num++;
    } else {
        vb_attr->blk_size[vb_attr->valid_num] = COMMON_GetPicBufferSize(vpss_attr->output_size[0].u32Width,
                                                                      vpss_attr->output_size[0].u32Height,
                                                                      vpss_attr->pixel_format,
                                                                      DATA_BITWIDTH_8,
                                                                      vpss_attr->compress_mode[0],
                                                                      DEFAULT_ALIGN);

        if (vpss_attr->vi_vpss_mode == VI_ONLINE_VPSS_ONLINE) {
            vb_attr->blk_cnt[vb_attr->valid_num]  = 3;
        } else {
            vb_attr->blk_cnt[vb_attr->valid_num]  = 2;
        }

        vb_attr->valid_num++;
    }

    vb_attr->blk_size[vb_attr->valid_num] = COMMON_GetPicBufferSize(vpss_attr->output_size[1].u32Width,
                                                                  vpss_attr->output_size[1].u32Height,
                                                                  vpss_attr->pixel_format,
                                                                  DATA_BITWIDTH_8,
                                                                  vpss_attr->compress_mode[1],
                                                                  DEFAULT_ALIGN);

    if (vpss_attr->vi_vpss_mode == VI_ONLINE_VPSS_ONLINE) {
        vb_attr->blk_cnt[vb_attr->valid_num]  = 5;
    } else {
        vb_attr->blk_cnt[vb_attr->valid_num]  = 4;
    }
    vb_attr->valid_num++;

    //vgs dcf use
    if(support_dcf == HI_TRUE) {
        vb_attr->blk_size[vb_attr->valid_num] = COMMON_GetPicBufferSize(160, 120,
                                                                      vpss_attr->pixel_format,
                                                                      DATA_BITWIDTH_8,
                                                                      COMPRESS_MODE_NONE,
                                                                      DEFAULT_ALIGN);
        vb_attr->blk_cnt[vb_attr->valid_num]  = 1;
        vb_attr->valid_num++;
    }

}

static hi_s32 sample_ivp_sys_init(sample_vb_attr *vb_attr)
{
    hi_s32 i;
    hi_s32 ret;
    VB_CONFIG_S vb_config;

    if (vb_attr->valid_num > VB_MAX_COMM_POOLS) {
        SAMPLE_PRT("sample_ivp_sys_init validNum(%d) too large than VB_MAX_COMM_POOLS(%d)!\n",
            vb_attr->valid_num, VB_MAX_COMM_POOLS);
        return HI_FAILURE;
    }

    memset(&vb_config, 0, sizeof(VB_CONFIG_S));

    for (i = 0; i < vb_attr->valid_num; i++) {
        vb_config.astCommPool[i].u64BlkSize = vb_attr->blk_size[i];
        vb_config.astCommPool[i].u32BlkCnt  = vb_attr->blk_cnt[i];
    }

    vb_config.u32MaxPoolCnt = vb_attr->valid_num;

    if (vb_attr->supplement_config == 0) {
        ret = SAMPLE_COMM_SYS_Init(&vb_config);
    } else {
        ret = SAMPLE_COMM_SYS_InitWithVbSupplement(&vb_config, vb_attr->supplement_config);
    }
    if (ret != HI_SUCCESS) {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        return ret;
    }

    return HI_SUCCESS;
}

static hi_s32 sample_ivp_set_ctrl_param(VI_PIPE vi_pipe, SAMPLE_SNS_TYPE_E sns_type)
{
    hi_s32 ret;
    ISP_CTRL_PARAM_S isp_ctrl_param;
    hi_u32 frame_rate;

    ret = HI_MPI_ISP_GetCtrlParam(vi_pipe, &isp_ctrl_param);
    if (ret != HI_SUCCESS) {
        SAMPLE_PRT("HI_MPI_ISP_GetCtrlParam failed with %d!\n", ret);
        return ret;
    }

    SAMPLE_COMM_VI_GetFrameRateBySensor(sns_type, &frame_rate);
    isp_ctrl_param.u32StatIntvl  = frame_rate / 30;
    if (isp_ctrl_param.u32StatIntvl == 0) {
        isp_ctrl_param.u32StatIntvl = 1;
    }

    ret = HI_MPI_ISP_SetCtrlParam(vi_pipe, &isp_ctrl_param);
    if (ret != HI_SUCCESS) {
        SAMPLE_PRT("HI_MPI_ISP_SetCtrlParam failed with %d!\n", ret);
        return ret;
    }

    return HI_SUCCESS;
}

static hi_s32 sample_ivp_vi_init(VI_DEV vi_dev, VI_PIPE vi_pipe, VI_CHN vi_chn, VI_VPSS_MODE_E pipe_mode)
{
    hi_s32 ret;
    SAMPLE_VI_CONFIG_S vi_config;

    SAMPLE_COMM_VI_GetSensorInfo(&vi_config);
    vi_config.astViInfo[0].stDevInfo.ViDev = vi_dev;
    vi_config.astViInfo[0].stDevInfo.enWDRMode = WDR_MODE_NONE;
    vi_config.astViInfo[0].stPipeInfo.aPipe[0] = vi_pipe;
    vi_config.astViInfo[0].stPipeInfo.aPipe[1] = -1;
    vi_config.astViInfo[0].stPipeInfo.enMastPipeMode = pipe_mode;
    vi_config.astViInfo[0].stChnInfo.ViChn = vi_chn;
    vi_config.astViInfo[0].stChnInfo.enDynamicRange = DYNAMIC_RANGE_SDR8;
    vi_config.astViInfo[0].stChnInfo.enPixFormat = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    vi_config.astViInfo[0].stChnInfo.enVideoFormat = VIDEO_FORMAT_LINEAR;
    vi_config.astViInfo[0].stChnInfo.enCompressMode = COMPRESS_MODE_NONE;
    vi_config.as32WorkingViId[0] = 0;
    vi_config.s32WorkingViNum = 1;

    ret = SAMPLE_COMM_VI_SetParam(&vi_config);
    if (ret != HI_SUCCESS) {
        SAMPLE_PRT("SAMPLE_COMM_VI_SetParam failed with %d!\n", ret);
        return ret;
    }

    ret = sample_ivp_set_ctrl_param(vi_pipe, vi_config.astViInfo[0].stSnsInfo.enSnsType);
    if (ret != HI_SUCCESS) {
        SAMPLE_PRT("sample_ivp_set_ctrl_param failed with %d!\n", ret);
        return ret;
    }

    ret = SAMPLE_COMM_VI_StartVi(&vi_config);
    if (ret != HI_SUCCESS) {
        SAMPLE_COMM_SYS_Exit();
        SAMPLE_PRT("SAMPLE_COMM_VI_StartVi failed with %d!\n", ret);
        return ret;
    }

    hi_memcpy(&g_vi_config,sizeof(SAMPLE_VI_CONFIG_S), &vi_config, sizeof(SAMPLE_VI_CONFIG_S));

    return HI_SUCCESS;
}

static hi_s32 sample_ivp_vpss_create_grp(VPSS_GRP vpss_grp, sample_vpss_chn_attr *vpss_attr)
{
    hi_s32 ret;
    VPSS_GRP_ATTR_S grp_attr = {0};

    grp_attr.enDynamicRange          = vpss_attr->dynamic_range;
    grp_attr.enPixelFormat           = vpss_attr->pixel_format;
    grp_attr.u32MaxW                 = vpss_attr->output_size[vpss_attr->big_stream_id].u32Width;
    grp_attr.u32MaxH                 = vpss_attr->output_size[vpss_attr->big_stream_id].u32Height;
    grp_attr.bNrEn                   = HI_TRUE;
    grp_attr.stNrAttr.enNrType       = VPSS_NR_TYPE_VIDEO;
    grp_attr.stNrAttr.enNrMotionMode = NR_MOTION_MODE_NORMAL;
    grp_attr.stNrAttr.enCompressMode = COMPRESS_MODE_FRAME;
    grp_attr.stFrameRate.s32SrcFrameRate = -1;
    grp_attr.stFrameRate.s32DstFrameRate = -1;

    ret = HI_MPI_VPSS_CreateGrp(vpss_grp, &grp_attr);
    if (ret != HI_SUCCESS) {
        SAMPLE_PRT("HI_MPI_VPSS_CreateGrp(grp:%d) failed with %#x!\n", vpss_grp, ret);
        return HI_FAILURE;
    }

    return ret;
}

static hi_s32 sample_ivp_vpss_enable_chn(VPSS_GRP vpss_grp, VPSS_CHN vpss_chn, sample_vpss_chn_attr *vpss_attr)
{
    hi_s32 ret;
    VPSS_CHN_ATTR_S chn_attr;

    memset(&chn_attr, 0, sizeof(VPSS_CHN_ATTR_S));
    chn_attr.u32Width                     = vpss_attr->output_size[vpss_chn].u32Width;
    chn_attr.u32Height                    = vpss_attr->output_size[vpss_chn].u32Height;
    chn_attr.enChnMode                    = VPSS_CHN_MODE_USER;
    chn_attr.enCompressMode               = vpss_attr->compress_mode[vpss_chn];
    chn_attr.enDynamicRange               = vpss_attr->dynamic_range;
    chn_attr.enPixelFormat                = vpss_attr->pixel_format;
    chn_attr.stFrameRate.s32SrcFrameRate  = vpss_attr->frame_rate[vpss_chn].s32SrcFrameRate;
    chn_attr.stFrameRate.s32DstFrameRate  = vpss_attr->frame_rate[vpss_chn].s32DstFrameRate;
    chn_attr.u32Depth                     = (vpss_chn == 1) ? 1 : 0;
    chn_attr.bMirror                      = vpss_attr->is_mirror[vpss_chn];
    chn_attr.bFlip                        = vpss_attr->is_flip[vpss_chn];
    chn_attr.enVideoFormat                = VIDEO_FORMAT_LINEAR;
    chn_attr.stAspectRatio.enMode         = ASPECT_RATIO_NONE;

    ret = HI_MPI_VPSS_SetChnAttr(vpss_grp, vpss_chn, &chn_attr);
    if (ret != HI_SUCCESS) {
        SAMPLE_PRT("HI_MPI_VPSS_SetChnAttr chan %d failed with %#x\n", vpss_chn, ret);
        goto exit0;
    }

    /* vpss limit! just vpss chan0 support wrap */
    if ((vpss_attr->wrap_en == HI_TRUE) && (vpss_chn == 0)) {
        hi_u32 buf_line = 0;
        VPSS_VENC_WRAP_PARAM_S wrap_param;
        VPSS_CHN_BUF_WRAP_S buf_wrap;

        memset(&wrap_param, 0, sizeof(VPSS_VENC_WRAP_PARAM_S));
        wrap_param.bAllOnline      = (vpss_attr->vi_vpss_mode == VI_ONLINE_VPSS_ONLINE) ? 1 : 0;
        wrap_param.u32FrameRate    = get_frame_rate_from_sensor_type(SNS_TYPE);
        wrap_param.u32FullLinesStd = get_full_lines_std_from_sensor_type(SNS_TYPE);
        wrap_param.stLargeStreamSize.u32Width = vpss_attr->output_size[vpss_attr->big_stream_id].u32Width;
        wrap_param.stLargeStreamSize.u32Height= vpss_attr->output_size[vpss_attr->big_stream_id].u32Height;
        wrap_param.stSmallStreamSize.u32Width = vpss_attr->output_size[vpss_attr->small_stream_id].u32Width;
        wrap_param.stSmallStreamSize.u32Height= vpss_attr->output_size[vpss_attr->small_stream_id].u32Height;

        ret = HI_MPI_SYS_GetVPSSVENCWrapBufferLine(&wrap_param, &buf_line);
        if (ret == HI_SUCCESS) {
            buf_wrap.u32WrapBufferSize = VPSS_GetWrapBufferSize(wrap_param.stLargeStreamSize.u32Width,
                wrap_param.stLargeStreamSize.u32Height, buf_line, vpss_attr->pixel_format, DATA_BITWIDTH_8,
                COMPRESS_MODE_NONE, DEFAULT_ALIGN);
            buf_wrap.bEnable = 1;
            buf_wrap.u32BufLine = buf_line;
            ret = HI_MPI_VPSS_SetChnBufWrapAttr(vpss_grp, vpss_chn, &buf_wrap);
            if (ret != HI_SUCCESS) {
                SAMPLE_PRT("HI_MPI_VPSS_SetChnBufWrapAttr Chn %d failed with %#x\n", vpss_chn, ret);
                goto exit0;
            }
        } else {
            SAMPLE_PRT("Current sensor type: %d, not support BigStream(%dx%d) and SmallStream(%dx%d) Ring!!\n",
                SNS_TYPE,
                vpss_attr->output_size[vpss_attr->big_stream_id].u32Width,
                vpss_attr->output_size[vpss_attr->big_stream_id].u32Height,
                vpss_attr->output_size[vpss_attr->small_stream_id].u32Width,
                vpss_attr->output_size[vpss_attr->small_stream_id].u32Height);
        }

    }

    ret = HI_MPI_VPSS_EnableChn(vpss_grp, vpss_chn);
    if (ret != HI_SUCCESS) {
        SAMPLE_PRT("HI_MPI_VPSS_EnableChn (%d) failed with %#x\n", vpss_chn, ret);
        goto exit0;
    }

exit0:
    return ret;
}

static hi_s32 sample_ivp_vpss_init(VPSS_GRP vpss_grp, sample_vpss_chn_attr *vpss_attr)
{
    hi_s32 i, j;
    hi_s32 ret;

    ret = sample_ivp_vpss_create_grp(vpss_grp, vpss_attr);
    if (ret != HI_SUCCESS) {
        goto exit0;
    }

    for (i = 0; i < VPSS_MAX_PHY_CHN_NUM; i++) {
        if (vpss_attr->chn_enable[i] == HI_TRUE) {
            ret = sample_ivp_vpss_enable_chn(vpss_grp, i, vpss_attr);
            if (ret != HI_SUCCESS) {
                goto exit1;
            }
        }
    }

    i--; // for abnormal case 'exit1' prossess;

    ret = HI_MPI_VPSS_StartGrp(vpss_grp);
    if (ret != HI_SUCCESS) {
        goto exit1;
    }

    return ret;

exit1:
    for (j = 0; j <= i; j++) {
        if (vpss_attr->chn_enable[j] == HI_TRUE) {
            HI_MPI_VPSS_DisableChn(vpss_grp, i);
        }
    }

    HI_MPI_VPSS_DestroyGrp(vpss_grp);
exit0:
    return ret;
}

HI_VOID sample_ivp_set_param(sample_ivp_param *ivp_param)
{
    g_ivp_param.ivp_mem_info.physical_addr = ivp_param->ivp_mem_info.physical_addr;
    g_ivp_param.ivp_mem_info.virtual_addr = ivp_param->ivp_mem_info.virtual_addr;
    g_ivp_param.ivp_mem_info.memory_size = ivp_param->ivp_mem_info.memory_size;

    g_ivp_param.ivp_handle = ivp_param->ivp_handle;
}

HI_VOID sample_ivp_get_param(sample_ivp_param *ivp_param)
{
    ivp_param->ivp_mem_info.physical_addr = g_ivp_param.ivp_mem_info.physical_addr;
    ivp_param->ivp_mem_info.virtual_addr = g_ivp_param.ivp_mem_info.virtual_addr;
    ivp_param->ivp_mem_info.memory_size = g_ivp_param.ivp_mem_info.memory_size;

    ivp_param->ivp_handle = g_ivp_param.ivp_handle;
}

static hi_s32 sample_ivp_get_file_size(hi_char *file_name, hi_u32 *file_size)
{
    hi_s32 ret;
    FILE *fp = NULL;
    hi_slong tmp_file_size;

    fp = fopen(file_name, "rb");
    if (fp == NULL) {
        SAMPLE_PRT("Error: open file failed\n");
        fclose(fp);
        return HI_FAILURE;
    }

    ret = fseek(fp, 0L, SEEK_END);
    if (ret == -1) {
        SAMPLE_PRT("Error: fseek file failed!\n");
        fclose(fp);
        return HI_FAILURE;
    }

    tmp_file_size = ftell(fp);
    if (tmp_file_size <= 0) {
        SAMPLE_PRT("Error: ftell file failed!\n");
        fclose(fp);
        return HI_FAILURE;
    }

    ret = fseek(fp, 0L, SEEK_SET);
    if (ret == -1) {
        SAMPLE_PRT("Error: fseek file failed!\n");
        fclose(fp);
        return HI_FAILURE;
    }

    fclose(fp);

    *file_size = tmp_file_size;

    return HI_SUCCESS;
}

static hi_s32 sample_ivp_read_file(hi_char *file_name, hi_u8 *buf, hi_u32 size)
{
    hi_s32 ret;
    FILE *fp = NULL;

    fp = fopen(file_name, "rb");
    if (fp == NULL) {
        SAMPLE_PRT("Error: open model file failed\n");
        fclose(fp);
        return HI_FAILURE;
    }

    ret = fread(buf, size, 1, fp);
    if (ret == -1) {
        SAMPLE_PRT("Error: fread failed\n");
        fclose(fp);
        return HI_FAILURE;
    }

    fclose(fp);

    return HI_SUCCESS;
}

static hi_s32 sample_ivp_load_resource(hi_s32 *ivp_handle)
{
    hi_s32 ret;
    hi_char resource_name[256] = "res/ivp_re_im_allday_16chn_pr1_640x360_v1030.oms";
    hi_u32 file_size;
    hi_ivp_mem_info ivp_mem_info;
    sample_ivp_param ivp_param;

    ret = sample_ivp_get_file_size(resource_name, &file_size);
    if (HI_SUCCESS != ret) {
        SAMPLE_PRT("SAMPLE_IVP_GetFileSize failed with %#x!\n", ret);
        return ret;
    }

    ret = HI_MPI_SYS_MmzAlloc(&ivp_mem_info.physical_addr, (HI_VOID**)&ivp_mem_info.virtual_addr,
        "RESOURCE_FILE_MEM", NULL, file_size);
    if(HI_SUCCESS != ret) {
        SAMPLE_PRT("HI_MPI_SYS_MmzAlloc failed with %#x!\n", ret);
        return ret;
    }
    ivp_mem_info.memory_size = file_size;

    ret = sample_ivp_read_file(resource_name, (HI_U8 *)(HI_SL)ivp_mem_info.virtual_addr, file_size);
    if (HI_SUCCESS != ret) {
        SAMPLE_PRT("SAMPLE_IVP_ReadFile failed with %#x!\n", ret);
        goto MEM_FREE_EXIT;
    }

    ret = hi_ivp_load_resource_from_memory(&ivp_mem_info, ivp_handle);
    if (HI_SUCCESS != ret) {
        SAMPLE_PRT("hi_ivp_load_resource_from_memory failed with %#x!\n", ret);
        goto MEM_FREE_EXIT;
    }

    ivp_param.ivp_handle = *ivp_handle;
    ivp_param.ivp_mem_info.physical_addr = ivp_mem_info.physical_addr;
    ivp_param.ivp_mem_info.virtual_addr = ivp_mem_info.virtual_addr;
    ivp_param.ivp_mem_info.memory_size = ivp_mem_info.memory_size;
    sample_ivp_set_param(&ivp_param);

    return HI_SUCCESS;

MEM_FREE_EXIT:
    HI_MPI_SYS_MmzFree(ivp_mem_info.physical_addr, (HI_VOID*)(HI_SL)ivp_mem_info.virtual_addr);
    return ret;
}

static hi_s32 sample_ivp_draw(VIDEO_FRAME_INFO_S *frame_info, RECT_S *rect, hi_u32 color)
{
    VGS_HANDLE handle = -1;
    hi_s32 ret;
    VGS_TASK_ATTR_S task;
    VGS_ADD_COVER_S vgs_add_cover;

    ret = HI_MPI_VGS_BeginJob(&handle);
    if (ret != HI_SUCCESS) {
        printf("Vgs begin job fail,Error(%#x)\n", ret);
        return ret;
    }

    memcpy(&task.stImgIn, frame_info, sizeof(VIDEO_FRAME_INFO_S));
    memcpy(&task.stImgOut, frame_info, sizeof(VIDEO_FRAME_INFO_S));

    vgs_add_cover.enCoverType = COVER_QUAD_RANGLE;
    vgs_add_cover.u32Color = color;
    vgs_add_cover.stQuadRangle.bSolid = HI_FALSE;
    vgs_add_cover.stQuadRangle.u32Thick = 2;

    vgs_add_cover.stQuadRangle.stPoint[0].s32X = rect->s32X;
    vgs_add_cover.stQuadRangle.stPoint[0].s32Y = rect->s32Y;
    vgs_add_cover.stQuadRangle.stPoint[1].s32X = rect->s32X + rect->u32Width;
    vgs_add_cover.stQuadRangle.stPoint[1].s32Y = rect->s32Y;
    vgs_add_cover.stQuadRangle.stPoint[2].s32X = rect->s32X + rect->u32Width;
    vgs_add_cover.stQuadRangle.stPoint[2].s32Y = rect->s32Y + rect->u32Height;
    vgs_add_cover.stQuadRangle.stPoint[3].s32X = rect->s32X;
    vgs_add_cover.stQuadRangle.stPoint[3].s32Y = rect->s32Y + rect->u32Height;
    ret = HI_MPI_VGS_AddCoverTask(handle, &task, &vgs_add_cover);
    if (ret != HI_SUCCESS) {
        printf("HI_MPI_VGS_AddCoverTask fail,Error(%#x)\n", ret);
        HI_MPI_VGS_CancelJob(handle);
        return ret;
    }

    ret = HI_MPI_VGS_EndJob(handle);
    if (ret != HI_SUCCESS) {
        printf("HI_MPI_VGS_EndJob fail,Error(%#x)\n", ret);
        HI_MPI_VGS_CancelJob(handle);
        return ret;
    }

    return ret;
}

HI_VOID* sample_ivp_proc(HI_VOID* p)
{
    hi_s32 ret;
    VPSS_GRP vpss_grp = 0;
    VPSS_CHN vpss_chn = 1;
    VENC_CHN venc_chn = 1;
    hi_s32 milli_sec = 20000;
    VIDEO_FRAME_INFO_S video_frame;
    sample_ivp_param ivp_param;
    hi_bool alarm;
    hi_u32 color = 0xff0000;
    RECT_S rect = {
        .s32X = 0,
        .s32Y = 0,
        .u32Width = 640,
        .u32Height = 360
    };

    sample_ivp_get_param(&ivp_param);

    prctl(PR_SET_NAME, "SAMPLE_IVP_AIProc", 0,0,0);
    while (g_ivp_start_flag == HI_TRUE)
    {
        ret = HI_MPI_VPSS_GetChnFrame(vpss_grp, vpss_chn, &video_frame, milli_sec);
        if (ret != HI_SUCCESS) {
            continue;
        }

        ret = hi_ivp_process(ivp_param.ivp_handle, &video_frame, &alarm);
        if(ret != HI_SUCCESS) {
            SAMPLE_PRT("hi_ivp_process failed with %#x!\n", ret);
        }

        /*alarm*/
        if(alarm == HI_TRUE) {
            sample_ivp_draw(&video_frame, &rect, color);
        }

        ret = HI_MPI_VENC_SendFrame(venc_chn,&video_frame, milli_sec);
        if(ret != HI_SUCCESS)
        {
            SAMPLE_PRT("HI_MPI_VENC_SendFrame failed with %#x!\n", ret);
        }

        ret = HI_MPI_VPSS_ReleaseChnFrame(vpss_grp, vpss_chn, &video_frame);
        if(ret != HI_SUCCESS)
        {
            SAMPLE_PRT("HI_MPI_VPSS_ReleaseChnFrame failed with %#x!\n", ret);
        }
    }

    return NULL;
}

static hi_s32 sample_ivp_set_venc_low_bitrate(hi_s32 ivp_handle)
{
    hi_s32 ret;
    hi_s32 venc_chn = 0;
    hi_ivp_venc_lowlight_iso_threshold threshold;

    if (g_ivp_effect_param.low_bitrate_en == HI_TRUE) {
        ret = hi_ivp_set_venc_low_bitrate(ivp_handle, venc_chn, g_ivp_effect_param.low_bitrate_en);
        if (ret != HI_SUCCESS) {
            SAMPLE_PRT("hi_ivp_set_venc_low_bitrate failed with %#x!\n", ret);
            return ret;
        }

        if (g_ivp_effect_param.iso_adaptive_en == HI_TRUE) {
            threshold.iso_adaptive_enable = g_ivp_effect_param.iso_adaptive_en;
            threshold.iso_threshold[0] = g_ivp_effect_param.iso_threshold[0];
            threshold.iso_threshold[1] = g_ivp_effect_param.iso_threshold[1];
            threshold.iso_threshold[2] = g_ivp_effect_param.iso_threshold[2];
            ret = hi_ivp_set_venc_lowlight_iso_threshold(ivp_handle, venc_chn, &threshold);
            if (ret != HI_SUCCESS) {
                SAMPLE_PRT("hi_ivp_set_venc_lowlight_iso_threshold failed with %#x!\n", ret);
                return ret;
            }
        }
    }

    return HI_SUCCESS;
}

static hi_s32 sample_ivp_set_advance_isp(hi_s32 ivp_handle)
{
    hi_s32 ret;
    hi_s32 vi_pipe = 0;
    ISP_SMART_EXPOSURE_ATTR_S smart_exp_attr;

    if(g_ivp_effect_param.advance_isp_en == HI_TRUE) {
        ret = hi_ivp_set_advance_isp(ivp_handle, vi_pipe, g_ivp_effect_param.advance_isp_en);
        if (ret!= HI_SUCCESS) {
            SAMPLE_PRT("hi_ivp_set_advance_isp failed with %#x!\n", ret);
            return ret;
        }

        ret = HI_MPI_ISP_GetSmartExposureAttr(vi_pipe, &smart_exp_attr);
        if (ret == HI_SUCCESS) {
            smart_exp_attr.bEnable = HI_TRUE;
            ret = HI_MPI_ISP_SetSmartExposureAttr(vi_pipe, &smart_exp_attr);
            if (ret != HI_SUCCESS) {
                SAMPLE_PRT("HI_MPI_ISP_SetSmartExposureAttr failed with %#x!\n", ret);
                return ret;
            }
        }
    }

    return HI_SUCCESS;
}

static hi_s32 sample_ivp_set_roi(hi_s32 ivp_handle)
{
    hi_s32 ret;
    hi_ivp_roi_attr roi_attr;
    hi_ivp_roi_map roi_map;
    hi_u32 mb_size = 4;
    hi_s32 mb_range_x, mb_range_y;
    hi_s32 i, j, idx;

    roi_attr.enable = HI_FALSE;
    roi_attr.threshold = 256;
    ret = hi_ivp_set_roi_attr(ivp_handle, &roi_attr);
    if (ret != HI_SUCCESS) {
        printf("hi_ivp_set_roi_attr fail 0x%x\n",ret);
        return HI_FAILURE;
    }

    roi_map.roi_mb_mode = HI_IVP_ROI_MB_MODE_16X16;
    roi_map.img_width = 640;
    roi_map.img_height = 360;
    roi_map.mb_map = NULL;

    switch(roi_map.roi_mb_mode) {
        case HI_IVP_ROI_MB_MODE_4X4:
            mb_size = 4;
            break;
        case HI_IVP_ROI_MB_MODE_8X8:
            mb_size = 8;
            break;
        case HI_IVP_ROI_MB_MODE_16X16:
            mb_size = 16;
            break;
        default:
            return HI_FAILURE;
    }

    mb_range_x = DIV_UP(roi_map.img_width, mb_size);
    mb_range_y = DIV_UP(roi_map.img_height, mb_size);
    roi_map.mb_map = (hi_u8 *)malloc(mb_range_x * mb_range_y);
    if(roi_map.mb_map == NULL) {
        return HI_FAILURE;
    }

    /*****************************************************************************
    1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
    1 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
    1 1 1 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
    1 1 1 1 1 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
    1 1 1 1 1 1 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
    0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
    0 0 0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
    0 0 0 0 0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
    0 0 0 0 0 0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
    0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
    0 0 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
    0 0 0 0 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0 0 0 0 0 0
    0 0 0 0 0 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0 0 0 0 0
    0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0 0 0
    0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0
    0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 0 0 0 0 0 0
    0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 0 0 0 0 0
    0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 0 0 0
    0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 0
    0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 1 1 1 1 1 1
    0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 1 1 1 1 1
    0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 1 1 1
    0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 1
    ********************************************************************************/
    for (i = 0; i < mb_range_y; i++) {
        idx = i * mb_range_x;
        for (j = 0; j < mb_range_x; j++) {
            if ((j > mb_range_x * i / mb_range_y - mb_range_x / 5) &&
                (j < mb_range_x * i / mb_range_y + mb_range_x / 5)) {
                roi_map.mb_map[idx] = 1;
            } else {
                roi_map.mb_map[idx] = 0;
            }
            idx++;
        }
    }

    if (roi_attr.enable == HI_TRUE) {
        ret = hi_ivp_set_roi_map(ivp_handle, &roi_map);
        if (ret != HI_SUCCESS) {
            printf("hi_ivp_set_roi_map fail 0x%x\n",ret);
            free(roi_map.mb_map);
            return HI_FAILURE;
        }
    }

    free(roi_map.mb_map);
    
    return HI_SUCCESS;
}

static hi_s32 sample_ivp_set_optional_attr(hi_s32 ivp_handle)
{
    hi_s32 ret;
    hi_ivp_ctrl_attr ivp_ctrl_attr;
    
    ivp_ctrl_attr.threshold = 0.92;
    ret = hi_ivp_set_ctrl_attr(ivp_handle, &ivp_ctrl_attr);
    if (ret != HI_SUCCESS) {
        SAMPLE_PRT("hi_ivp_set_ctrl_attr failed with %#x!\n", ret);
        return ret;
    }

    ret = sample_ivp_set_venc_low_bitrate(ivp_handle);
    if (ret != HI_SUCCESS) {
        SAMPLE_PRT("sample_ivp_set_venc_low_bitrate failed with %#x!\n", ret);
        return ret;
    }

    ret = sample_ivp_set_advance_isp(ivp_handle);
    if (ret != HI_SUCCESS) {
        SAMPLE_PRT("sample_ivp_set_advance_isp failed with %#x!\n", ret);
        return ret;
    }

    ret = sample_ivp_set_roi(ivp_handle);
    if (ret != HI_SUCCESS) {
        SAMPLE_PRT("sample_ivp_set_roi failed!\n");
        return ret;
    }

    return HI_SUCCESS;
}

HI_S32 sample_ivp_smd_start(void)
{
    hi_s32 ret;
    hi_s32 ivp_handle = -1;

    ret = hi_ivp_init();
    if (HI_SUCCESS != ret) {
        SAMPLE_PRT("hi_ivp_init failed with %#x!\n", ret);
        return HI_FAILURE;
    }

    ret = sample_ivp_load_resource(&ivp_handle);
    if (ret != HI_SUCCESS) {
        SAMPLE_PRT("sample_ivp_load_resource failed with %#x!\n", ret);
        goto IVP_DEINIT_EXIT;
    }

    ret = sample_ivp_set_optional_attr(ivp_handle);
    if (ret != HI_SUCCESS) {
        SAMPLE_PRT("sample_ivp_set_optional_attr failed with %#x!\n", ret);
        goto UNLOAD_RES;
    }

    g_ivp_start_flag = HI_TRUE;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setstacksize(&attr, 0x10000);
    pthread_create(&g_ivp_thread, &attr, sample_ivp_proc, NULL);

    return HI_SUCCESS;

UNLOAD_RES:
    hi_ivp_unload_resource(ivp_handle);
IVP_DEINIT_EXIT:
    hi_ivp_deinit();

    return ret;
}

HI_S32 sample_ivp_smd_stop(void)
{
    sample_ivp_param ivp_param;

    sample_ivp_get_param(&ivp_param);
    g_ivp_start_flag = HI_FALSE;
    pthread_join(g_ivp_thread, 0);

    HI_MPI_SYS_MmzFree(ivp_param.ivp_mem_info.physical_addr, (HI_VOID*)(HI_SL)ivp_param.ivp_mem_info.virtual_addr);

    hi_ivp_unload_resource(ivp_param.ivp_handle);
    hi_ivp_deinit();

    return HI_SUCCESS;
}

HI_VOID sample_ivp_set_effect_param(void)
{
    /*lowbitrate para*/
    g_ivp_effect_param.low_bitrate_en = HI_TRUE;

    /*iso adaptive*/
    g_ivp_effect_param.iso_adaptive_en = HI_TRUE;
    g_ivp_effect_param.iso_threshold[0] = 2500;
    g_ivp_effect_param.iso_threshold[1] = 5000;
    g_ivp_effect_param.iso_threshold[2] = 10000;

    /*advance isp para*/
    g_ivp_effect_param.advance_isp_en = HI_TRUE;    
}

static hi_s32 sample_ivp_init_param(PIC_SIZE_E pic_size[], sample_vpss_chn_attr *vpss_attr, sample_vb_attr *vb_attr)
{
    hi_s32 ret;
    SIZE_S size[2];
    hi_s32 i;

    sample_ivp_set_effect_param();

    if (SNS_TYPE == SAMPLE_SNS_TYPE_BUTT) {
        SAMPLE_PRT("Not set SENSOR%d_TYPE !\n", 0);
        return HI_FAILURE;
    }

    ret = SAMPLE_COMM_VI_GetSizeBySensor(SNS_TYPE, &pic_size[0]);
    if (ret != HI_SUCCESS) {
        SAMPLE_PRT("SAMPLE_COMM_VI_GetSizeBySensor failed!\n");
        return ret;
    }

    for (i = 0; i < 2; i++) {
        ret = SAMPLE_COMM_SYS_GetPicSize(pic_size[i], &size[i]);
        if (ret != HI_SUCCESS) {
            SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
            return ret;
        }
    }

    sample_ivp_get_default_vpss_attr(size, vpss_attr);
    if (vpss_attr->wrap_en == HI_TRUE) {
        get_vpss_wrap_buf_line(vpss_attr);
        if (vpss_attr->wrap_buf_line == 0) {
            return HI_FAILURE;
        }
    }

    memset(vb_attr, 0, sizeof(sample_vb_attr));
    vb_attr->supplement_config = 0;
    sample_ivp_get_comm_vb_attr(vpss_attr, HI_FALSE, vb_attr);

    return HI_SUCCESS;
}

static void sample_ivp_config_venc_attr(VENC_CHN_ATTR_S *chn_attr, VENC_RC_PARAM_S *rc_param)
{
    switch (chn_attr->stRcAttr.enRcMode) {
        case VENC_RC_MODE_H264CVBR:
            chn_attr->stRcAttr.stH264CVbr.u32Gop = 200;
            chn_attr->stRcAttr.stH264CVbr.u32StatTime = 4;
            chn_attr->stRcAttr.stH264CVbr.u32MaxBitRate = 1024;
            chn_attr->stRcAttr.stH264CVbr.u32ShortTermStatTime = 4;
            chn_attr->stRcAttr.stH264CVbr.u32LongTermStatTime = 1440;
            chn_attr->stRcAttr.stH264CVbr.u32LongTermMaxBitrate = 1024;
            chn_attr->stRcAttr.stH264CVbr.u32LongTermMinBitrate = 0;

            rc_param->stParamH264CVbr.u32MaxQp = 51;
            rc_param->stParamH264CVbr.u32MinQp = 27;
            rc_param->stParamH264CVbr.u32MaxIQp = 51;
            rc_param->stParamH264CVbr.u32MinIQp = 25;
            rc_param->stParamH264CVbr.u32MinQpDelta = 2;
            rc_param->stParamH264CVbr.u32LongTermStatTimeUnit = 1;
            break;
        case VENC_RC_MODE_H265CVBR:
            chn_attr->stRcAttr.stH265CVbr.u32Gop = 200;
            chn_attr->stRcAttr.stH265CVbr.u32StatTime = 4;
            chn_attr->stRcAttr.stH265CVbr.u32MaxBitRate = 1024;
            chn_attr->stRcAttr.stH265CVbr.u32ShortTermStatTime = 4;
            chn_attr->stRcAttr.stH265CVbr.u32LongTermStatTime = 1440;
            chn_attr->stRcAttr.stH265CVbr.u32LongTermMaxBitrate = 1024;
            chn_attr->stRcAttr.stH265CVbr.u32LongTermMinBitrate = 0;

            rc_param->stParamH265CVbr.u32MaxQp = 51;
            rc_param->stParamH265CVbr.u32MinQp = 27;
            rc_param->stParamH265CVbr.u32MaxIQp = 51;
            rc_param->stParamH265CVbr.u32MinIQp = 25;
            rc_param->stParamH265CVbr.u32MinQpDelta = 2;
            rc_param->stParamH265CVbr.u32LongTermStatTimeUnit = 1;
            break;
        default:
            break;
    }

    chn_attr->stGopAttr.enGopMode = VENC_GOPMODE_SMARTP;
    chn_attr->stGopAttr.stSmartP.u32BgInterval = 1200;
    chn_attr->stGopAttr.stSmartP.s32BgQpDelta = 4;
    chn_attr->stGopAttr.stSmartP.s32ViQpDelta = 2;
}

static hi_s32 sample_ivp_set_venc_attr(VENC_CHN venc_chn)
{
    hi_s32 ret;
    VENC_CHN_ATTR_S chn_attr;
    VENC_RC_PARAM_S rc_param;

    ret = HI_MPI_VENC_GetChnAttr(venc_chn, &chn_attr);
    if (ret != HI_SUCCESS) {
        SAMPLE_PRT("HI_MPI_VENC_GetChnAttr failed %#x!\n", ret);
        return ret;
    }

    ret = HI_MPI_VENC_GetRcParam(venc_chn, &rc_param);
    if (ret != HI_SUCCESS) {
        SAMPLE_PRT("HI_MPI_VENC_GetRcParam failed %#x!\n", ret);
        return ret;
    }

    sample_ivp_config_venc_attr(&chn_attr, &rc_param);

    ret = HI_MPI_VENC_SetRcParam(venc_chn, &rc_param);
    if (ret != HI_SUCCESS) {
        SAMPLE_PRT("HI_MPI_VENC_SetRcParam failed %#x!\n", ret);
        return ret;
    }

    ret = HI_MPI_VENC_SetChnAttr(venc_chn, &chn_attr);
    if (ret != HI_SUCCESS) {
        SAMPLE_PRT("HI_MPI_VENC_SetChnAttr failed %#x!\n", ret);
        return ret;
    }

    return HI_SUCCESS;
}

static hi_s32 sample_ivp_start_venc(VENC_CHN venc_chn[], PIC_SIZE_E pic_size[])
{
    hi_s32 ret;
    VENC_GOP_MODE_E gop_mode[2] = {VENC_GOPMODE_SMARTP, VENC_GOPMODE_NORMALP};
    VENC_GOP_ATTR_S gop_attr[2];
    hi_u32 profile[2] = {0, 0};
    PAYLOAD_TYPE_E payload[2] = {PT_H265, PT_H265};
    SAMPLE_RC_E rc_mode[2] = {SAMPLE_RC_CVBR, SAMPLE_RC_AVBR};
    hi_bool share_buf = HI_TRUE;

    /***start h.265 bigStream encoder **/
    ret = SAMPLE_COMM_VENC_GetGopAttr(gop_mode[0], &gop_attr[0]);
    if (ret != HI_SUCCESS) {
        SAMPLE_PRT("Venc Get GopAttr for %#x!\n", ret);
        return ret;
    }
    ret = SAMPLE_COMM_VENC_Start(venc_chn[0], payload[0], pic_size[0], rc_mode[0],
        profile[0], share_buf, &gop_attr[0]);
    if (ret != HI_SUCCESS) {
        SAMPLE_PRT("Venc Start failed for %#x!\n", ret);
        return ret;
    }

    /***set h.265 bigStream attr **/
    ret = sample_ivp_set_venc_attr(venc_chn[0]);
    if (ret != HI_SUCCESS) {
        SAMPLE_PRT("Set Venc Attr failed for %#x!\n", ret);
    }

    /***start h.265 smallStream encoder **/
    ret = SAMPLE_COMM_VENC_GetGopAttr(gop_mode[1], &gop_attr[1]);
    if (ret != HI_SUCCESS) {
        SAMPLE_PRT("Venc Get GopAttr for %#x!\n", ret);
        goto EXIT_VENC_BIG_STREAM_STOP;
    }
    ret = SAMPLE_COMM_VENC_Start(venc_chn[1], payload[1], pic_size[1], rc_mode[1],
        profile[1], share_buf, &gop_attr[1]);
    if (ret != HI_SUCCESS) {
        SAMPLE_PRT("Venc Start failed for %#x!\n", ret);
        goto EXIT_VENC_BIG_STREAM_STOP;
    }
    
    return HI_SUCCESS;

EXIT_VENC_BIG_STREAM_STOP:
    SAMPLE_COMM_VENC_Stop(venc_chn[0]);
    return ret;
}

/******************************************************************************
* function:
* H265(Large Stream)   LowBitrate + AdvanceIsp
* H265(Small Stream)   SMD + AdvanceIsp
******************************************************************************/
hi_s32 sample_ivp_smd_init(void)
{
    hi_s32 ret;
    PIC_SIZE_E pic_size[2] = {PIC_1080P, PIC_640x360};
    VENC_CHN venc_chn[2] = {0, 1};
    VI_DEV          vi_dev        = 0;
    VI_PIPE         vi_pipe       = 0;
    VI_CHN          vi_chn        = 0;
    VPSS_GRP        vpss_grp      = 0;
    VPSS_CHN        vpss_chn[2]   = {0, 1};
    sample_vpss_chn_attr vpss_attr;
    sample_vb_attr vb_attr;

    /******************************************
      step 0: Initialize related parameters
    ******************************************/
    ret = sample_ivp_init_param(pic_size, &vpss_attr, &vb_attr);
    if (ret != HI_SUCCESS) {
        SAMPLE_PRT("sample_ivp_init_param err for %#x!\n", ret);
        return ret;
    }

    /******************************************
      step 1: init sys alloc common vb
    ******************************************/
    ret = sample_ivp_sys_init(&vb_attr);
    if (ret != HI_SUCCESS) {
        SAMPLE_PRT("Init SYS err for %#x!\n", ret);
        return ret;
    }

    /******************************************
      step 2: init and start vi
    ******************************************/
    ret = sample_ivp_vi_init(vi_dev, vi_pipe, vi_chn, vpss_attr.vi_vpss_mode);
    if (ret != HI_SUCCESS) {
        SAMPLE_PRT("Init VI err for %#x!\n", ret);
        goto EXIT_SYS;
    }

    /******************************************
      step 3: init and start vpss: One group with two channel(0:BigStream  1:SmallStream)
    ******************************************/
    ret = sample_ivp_vpss_init(vpss_grp, &vpss_attr);
    if (ret != HI_SUCCESS) {
        SAMPLE_PRT("Init VPSS err for %#x!\n", ret);
        goto EXIT_VI_STOP;
    }

    /******************************************
      step 4: Bind VI and VPSS
    ******************************************/
    ret = SAMPLE_COMM_VI_Bind_VPSS(vi_pipe, vi_chn, vpss_grp);
    if (ret != HI_SUCCESS) {
        SAMPLE_PRT("VI Bind VPSS err for %#x!\n", ret);
        goto EXIT_VPSS_STOP;
    }

    /******************************************
     step 5: start VENC (0: H265 BigStream; 1: H265 SmallStream)
    ******************************************/
    ret = sample_ivp_start_venc(venc_chn, pic_size);
    if (ret != HI_SUCCESS) {
        SAMPLE_PRT("sample_ivp_start_venc err for %#x!\n", ret);
        goto EXIT_VI_VPSS_UNBIND;
    }

    /******************************************
     step 6: Bind VPSS and VENC( BigStream attach Vpss chan[0], SmallStream attch Vpss chan[1])
    ******************************************/
    ret = SAMPLE_COMM_VPSS_Bind_VENC(vpss_grp, vpss_chn[0], venc_chn[0]);
    if (ret != HI_SUCCESS) {
        SAMPLE_PRT("Venc Get GopAttr failed for %#x!\n", ret);
        goto EXIT_VENC_H265_STOP;
    }

    /******************************************
     step 7: start get video stream (this thread just Get video channel stream(venc chan[0] and chan[2]))
    ******************************************/
    ret = SAMPLE_COMM_VENC_StartGetStream(venc_chn, 2);
    if (ret != HI_SUCCESS) {
        SAMPLE_PRT("Start Venc failed!\n");
        goto EXIT_VENC_H265_UnBind;
    }

    /******************************************
     step 8: start ivp
    ******************************************/
    ret = sample_ivp_smd_start();
    if (ret != HI_SUCCESS) {
        SAMPLE_PRT("Start Ivp failed!\n");
        goto EXIT_GET_STREAM_STOP;
    }

    PAUSE();

    printf("exit this sample!\n");

    sample_ivp_smd_stop();
EXIT_GET_STREAM_STOP:
    SAMPLE_COMM_VENC_StopGetStream();
EXIT_VENC_H265_UnBind:
    SAMPLE_COMM_VPSS_UnBind_VENC(vpss_grp, vpss_chn[0], venc_chn[0]);
EXIT_VENC_H265_STOP:
    SAMPLE_COMM_VENC_Stop(venc_chn[1]);
    SAMPLE_COMM_VENC_Stop(venc_chn[0]);
EXIT_VI_VPSS_UNBIND:
    SAMPLE_COMM_VI_UnBind_VPSS(vi_pipe, vi_chn, vpss_grp);
EXIT_VPSS_STOP:
    SAMPLE_COMM_VPSS_Stop(vpss_grp, vpss_attr.chn_enable);
EXIT_VI_STOP:
    SAMPLE_COMM_VI_StopVi(&g_vi_config);
EXIT_SYS:
    SAMPLE_COMM_SYS_Exit();

    return ret;
}

HI_S32 sample_ivp_smd_exit(void)
{
    VENC_CHN venc_chn[2] = {0, 1};
    VPSS_GRP vpss_grp = 0;
    VPSS_CHN vpss_chn[2] = {0,1};
    hi_bool vpss_chn_enable[VPSS_MAX_PHY_CHN_NUM] = {HI_TRUE, HI_TRUE, HI_FALSE};
    VI_PIPE vi_pipe = 0;
    VI_CHN vi_chn = 0;

    sample_ivp_smd_stop();
    SAMPLE_COMM_VENC_StopGetStream();
    SAMPLE_COMM_VPSS_UnBind_VENC(vpss_grp, vpss_chn[0], venc_chn[0]);
    SAMPLE_COMM_VENC_Stop(venc_chn[1]);
    SAMPLE_COMM_VENC_Stop(venc_chn[0]);
    SAMPLE_COMM_VI_UnBind_VPSS(vi_pipe, vi_chn, vpss_grp);
    SAMPLE_COMM_VPSS_Stop(vpss_grp, vpss_chn_enable);
    SAMPLE_COMM_VI_StopVi(&g_vi_config);
    SAMPLE_COMM_SYS_Exit();
    return HI_SUCCESS;
}

/******************************************************************************
* function : to process abnormal case
******************************************************************************/
#ifndef __HuaweiLite__
void sample_ivp_handle_sig(HI_S32 signo)
{
    signal(SIGINT, SIG_IGN);
    signal(SIGTERM, SIG_IGN);

    if (signo == SIGINT || signo == SIGTERM) {
        sample_ivp_smd_exit();
        printf("\033[0;31mprogram termination abnormally!\033[0;39m\n");
    }
    exit(-1);
}
#endif
/******************************************************************************
* function : show usage
******************************************************************************/
void sample_ivp_usage(char* prg_name)
{
    printf("Usage : %s <index> \n", prg_name);
    printf("index:\n");
    printf("\t  0) H265(Large Stream)+H265(Small Stream)+ SMD + LowBitrate + AdvanceIsp.\n");
}

/******************************************************************************
* function : ive sample
******************************************************************************/
#ifdef __HuaweiLite__
int app_main(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
    if (argc < 2) {
        sample_ivp_usage(argv[0]);
        return HI_FAILURE;
    }
#ifndef __HuaweiLite__
    signal(SIGINT, sample_ivp_handle_sig);
    signal(SIGTERM, sample_ivp_handle_sig);
#endif

    switch (*argv[1]) {
        case '0':
            sample_ivp_smd_init();
            break;
        default :
            sample_ivp_usage(argv[0]);
            break;
    }

    return HI_SUCCESS;
}

