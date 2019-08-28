/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2013-2019. All rights reserved.
 * Description: vgs_ext.h
 * Author: Hisilicon multimedia software group
 * Create: 2013-07-24
 */

#ifndef __VGS_EXT_H__
#define __VGS_EXT_H__

#include "hi_comm_video.h"
#include "hi_comm_vgs.h"
#include "hi_errno.h"
#include "vb_ext.h"

#define HI_TRACE_VGS(level, fmt, ...)                                                                         \
    do {                                                                                                      \
        HI_TRACE(level, HI_ID_VGS, "[Func]:%s [Line]:%d [Info]:" fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
    } while (0)

#define VGS_INVALD_HANDLE (-1UL) /* invalid job handle */

#define VGS_MAX_JOB_NUM 400 /* max job number */
#define VGS_MIN_JOB_NUM 20  /* min job number */

#define VGS_MAX_TASK_NUM 800 /* max task number */
#define VGS_MIN_TASK_NUM 20  /* min task number */

#define VGS_MAX_NODE_NUM 800 /* max node number */
#define VGS_MIN_NODE_NUM 20  /* min node number */

#define VGS_MAX_WEIGHT_THRESHOLD 100 /* max weight threshold */
#define VGS_MIN_WEIGHT_THRESHOLD 1   /* min weight threshold */

#define MAX_VGS_COVER 1
#define MAX_VGS_OSD   1

#define FD_GRID_SZ  4 /* grid size used in feature detection, in this version, the image is processed with 4x4 grid */
#define FD_CELL_NUM (FD_GRID_SZ * FD_GRID_SZ) /* blk number in total */

#define VGS_MAX_STITCH_NUM 1
/* The type of job */
typedef enum hiVGS_JOB_TYPE_E {
    VGS_JOB_TYPE_BYPASS = 0, /* BYPASS job,can only contain bypass task */
    VGS_JOB_TYPE_NORMAL = 1, /* normal job,can contain any task except bypass task and lumastat task */
    VGS_JOB_TYPE_BUTT
} VGS_JOB_TYPE_E;

/* The completion status of task */
typedef enum hiVGS_TASK_FNSH_STAT_E {
    VGS_TASK_FNSH_STAT_OK = 1,       /* task has been completed correctly */
    VGS_TASK_FNSH_STAT_FAIL = 2,     /* task failed because of exception or not completed */
    VGS_TASK_FNSH_STAT_CANCELED = 3, /* task has been cancelled */
    VGS_TASK_FNSH_STAT_BUTT
} VGS_TASK_FNSH_STAT_E;

/* the priority of VGS task */
typedef enum hiVGS_JOB_PRI_E {
    VGS_JOB_PRI_HIGH = 0,   /* high priority */
    VGS_JOB_PRI_NORMAL = 1, /* normal priority */
    VGS_JOB_PRI_LOW = 2,    /* low priority */
    VGS_JOB_PRI_BUTT
} VGS_JOB_PRI_E;

/* The status after VGS cancle job */
typedef struct hiVGS_CANCEL_STAT_S {
    HI_U32 u32JobsCanceled; /* The count of cancelled job */
    HI_U32 u32JobsLeft;     /* The count of the rest job */
} VGS_CANCEL_STAT_S;

/* The completion status of job */
typedef enum hiVGS_JOB_FNSH_STAT_E {
    VGS_JOB_FNSH_STAT_OK = 1,       /* job has been completed correctly */
    VGS_JOB_FNSH_STAT_FAIL = 2,     /* job failed because of exception or not completed */
    VGS_JOB_FNSH_STAT_CANCELED = 3, /* job has been cancelled */
    VGS_JOB_FNSH_STAT_BUTT
} VGS_JOB_FNSH_STAT_E;

/* The struct of vgs job.
After complete the job,VGS call the callback function to notify the caller with this struct as an parameter.
*/
typedef struct hiVGS_JOB_DATA_S {
    HI_U64 au64PrivateData[VGS_PRIVATE_DATA_LEN]; /* private data of job */
    VGS_JOB_FNSH_STAT_E enJobFinishStat;          /* output param:job finish status(ok or nok) */
    VGS_JOB_TYPE_E enJobType;
    void (*pJobCallBack)(MOD_ID_E enCallModId, HI_U32 u32CallDevId, HI_U32 u32CallChnId,
                         struct hiVGS_JOB_DATA_S *pJobData); /* callback */
} VGS_JOB_DATA_S;

typedef struct hiGFX_SURFACE_S {
    HI_U64 PhyAddr; /* Header address of a bitmap or the Y component */

    PIXEL_FORMAT_E enColorFmt; /* Color format */

    HI_U32 u32Height; /* Bitmap height */

    HI_U32 u32Width; /* Bitmap width */

    HI_U32 u32Stride; /* Stride of a bitmap or the Y component */

    HI_BOOL bAlphaMax255; /* The maximum alpha value of a bitmap is 255 or 128. */

    HI_BOOL bAlphaExt1555; /* Whether to enable the alpha extension of an ARGB1555 bitmap. */
    HI_U8 u8Alpha0;        /* Values of alpha0 and alpha1, used as the ARGB1555 format */
    HI_U8 u8Alpha1;        /* Values of alpha0 and alpha1, used as the ARGB1555 format */
} GFX_SURFACE_S;

/* The struct of vgs task ,include input and output frame buffer,caller,and callback function .
After complete the task,VGS call the callback function to notify the caller with this struct as an parameter.
*/
typedef struct hiVGS_TASK_DATA_S {
    union {
        struct {
            VIDEO_FRAME_INFO_S stImgIn;  /* input picture */
            VIDEO_FRAME_INFO_S stImgOut; /* output picture */
        };
        struct {
            GFX_SURFACE_S stBackGround;
            RECT_S stBackGroundRect;
            GFX_SURFACE_S stForeGround;
            RECT_S stForeGroundRect;
            GFX_SURFACE_S stDst;
            RECT_S stDstRect;
        };
    };
    HI_U64 au64PrivateData[VGS_PRIVATE_DATA_LEN]; /* task's private data */
    void (*pCallBack)(MOD_ID_E enCallModId, HI_U32 u32CallDevId, HI_U32 u32CallChnId,
                      const struct hiVGS_TASK_DATA_S *pTask); /* callback */
    MOD_ID_E enCallModId;                       /* module ID */
    HI_U32 u32CallDevId;                        /* device ID */
    HI_U32 u32CallChnId;                        /* chnnel ID */
    VGS_TASK_FNSH_STAT_E
    enFinishStat; /* output param:task finish status(ok or nok) */
    HI_U32 reserved;
} VGS_TASK_DATA_S;

/* The information of OSD */
typedef struct hiVGS_OSD_S {
    /* first address of bitmap */
    HI_U64 u64PhyAddr;

    PIXEL_FORMAT_E enPixelFormat;

    HI_U32 u32Stride;

    /* Alpha bit should be extended by setting Alpha0 and Alpha1, when enPixelFormat is PIXEL_FORMAT_RGB_1555 */
    HI_BOOL bAlphaExt1555; /* whether Alpha bit should be extended */
    HI_U8 u8Alpha0;        /* u8Alpha0 will be valid where alpha bit is 0 */
    HI_U8 u8Alpha1;        /* u8Alpha1 will be valid where alpha bit is 1 */
    HI_U16 u16ColorLUT[2];
} VGS_OSD_S;

/* The struct of OSD operation */
typedef struct hiVGS_OSD_OPT_S {
    HI_BOOL bOsdEn;
    HI_U8 u32GlobalAlpha;
    VGS_OSD_S stVgsOsd;
    RECT_S stOsdRect;

    HI_BOOL bOsdRevert;
    VGS_OSD_REVERT_S stVgsOsdRevert;
} VGS_OSD_OPT_S;

typedef struct hiVGS_LUMAINFO_OPT_S {
    RECT_S stRect; /* the regions to get lumainfo */
    HI_U64 u64PhysAddrForResult;
    HI_U64 *pu64VirtAddrForResult;
    HI_U64 *pu64VirtAddrForUser;
} VGS_LUMASTAT_OPT_S;

typedef struct hiVGS_COVER_OPT_S {
    HI_BOOL bCoverEn;
    VGS_COVER_TYPE_E enCoverType;
    union {
        RECT_S stDstRect;                    /* rectangle */
        VGS_QUADRANGLE_COVER_S stQuadRangle; /* arbitrary quadrilateral */
    };
    HI_U32 u32CoverColor;
} VGS_COVER_OPT_S;

typedef struct hiVGS_CROP_OPT_S {
    RECT_S stDestRect;
} VGS_CROP_OPT_S;

typedef enum hiVGS_SCALE_COEF_MODE_E {
    VGS_SCALE_COEF_NORMAL = 0, /* normal scale coefficient */
    VGS_SCALE_COEF_TAP2 = 1,   /* scale coefficient of 2 tap for IVE */
    VGS_SCALE_COEF_TAP4 = 2,   /* scale coefficient of 4 tap for IVE */
    VGS_SCALE_COEF_TAP6 = 3,   /* scale coefficient of 6 tap for IVE */
    VGS_SCALE_COEF_TAP8 = 4,   /* scale coefficient of 8 tap for IVE */
    VGS_SCALE_COEF_BUTT
} VGS_SCALE_COEF_MODE_E;

typedef struct hiVGS_ONLINE_OPT_S {
    HI_BOOL bCrop; /* if enable crop */
    VGS_CROP_OPT_S stCropOpt;
    HI_BOOL bCover; /* if enable cover */
    VGS_COVER_OPT_S stCoverOpt[MAX_VGS_COVER];
    HI_BOOL bOsd; /* if enable osd */
    VGS_OSD_OPT_S stOsdOpt[MAX_VGS_OSD];

    HI_BOOL bMirror;     /* if enable mirror */
    HI_BOOL bFlip;       /* if enable flip */
    HI_BOOL bForceHFilt; /* Whether to force the horizontal direction filtering, it can be
                                                set while input and output pic are same size at horizontal direction */
    HI_BOOL bForceVFilt; /* Whether to force the vertical direction filtering, it can be
                                                set while input and output pic are same size at vertical direction */
    HI_BOOL bDeflicker;  /* Whether decrease flicker */
    VGS_SCALE_COEF_MODE_E enVGSSclCoefMode;

    HI_BOOL bGdc; /* The operation is belong to fisheye */
    RECT_S stGdcRect;
} VGS_ONLINE_OPT_S;

typedef struct hiVGS_OSD_QUICKCOPY_OPT_S {
    RECT_S stSrcRect;
    RECT_S stDestRect;
} VGS_OSD_QUICKCOPY_OPT_S;
typedef struct hiVGS_FILLCOLOR_S {
    PIXEL_FORMAT_E enColorFmt; /* TDE pixel format */
    HI_U32 u32FillColor;       /* Fill colors that vary according to pixel formats */
} VGS_FILLCOLOR_S;

typedef enum hiGFX_ALUCMD_E {
    ALUCMD_NONE = 0x0,  /* No alpha and raster of operation (ROP) blending */
    ALUCMD_BLEND = 0x1, /* Alpha blending */
    ALUCMD_ROP = 0x2,   /* ROP blending */
    ALUCMD_BUTT = 0x4   /* End of enumeration */
} GFX_ALUCMD_E;

typedef enum hiGFX_COLORKEY_MODE_E {
    COLORKEY_MODE_NONE = 0,   /* No colorkey */
    COLORKEY_MODE_FOREGROUND, /* When performing the colorkey operation on the foreground bitmap,
                                 you need to perform this operation before the CLUT for color extension and
                                 perform this operation after the CLUT for color correction. */
    COLORKEY_MODE_BACKGROUND, /* Perform the colorkey operation on the background bitmap */
    COLORKEY_MODE_BUTT
} GFX_COLORKEY_MODE_E;

typedef struct hiCOLORKEY_COMP_S {
    HI_U8 u8CompMin;   /* Minimum value of a component */
    HI_U8 u8CompMax;   /* Maximum value of a component */
    HI_U8 bCompOut;    /* The colorkey of a component is within or beyond the range. */
    HI_U8 bCompIgnore; /* Whether to ignore a component. */
    HI_U8 u8CompMask;  /* Component mask */
    HI_U8 u8Reserved;
    HI_U8 u8Reserved1;
    HI_U8 u8Reserved2;
} COLORKEY_COMP_S;

typedef struct hiGFX_COLORKEY_U {
    COLORKEY_COMP_S stAlpha; /* Alpha component */
    COLORKEY_COMP_S stRed;   /* Red component */
    COLORKEY_COMP_S stGreen; /* Green component */
    COLORKEY_COMP_S stBlue;  /* Blue component */

} GFX_COLORKEY_U;

typedef enum hiGFX_CLIPMODE_E {
    CLIPMODE_NONE = 0, /* No clip */
    CLIPMODE_INSIDE,   /* Clip the data within the rectangle to output and discard others */
    CLIPMODE_OUTSIDE,  /* Clip the data outside the rectangle to output and discard others */
    CLIPMODE_BUTT
} GFX_CLIPMODE_E;

typedef enum hiGFX_MIRROR_E {
    MIRROR_NONE = 0,   /* No mirror */
    MIRROR_HORIZONTAL, /* Horizontal mirror */
    MIRROR_VERTICAL,   /* Vertical mirror */
    MIRROR_BOTH,       /* Horizontal and vertical mirror */
    MIRROR_BUTT
} GFX_MIRROR_E;

typedef enum hiGFX_OUTALPHA_FROM_E {
    OUTALPHA_FROM_NORM = 0,    /* Output from the result of alpha blending or anti-flicker */
    OUTALPHA_FROM_BACKGROUND,  /* Output from the background bitmap */
    OUTALPHA_FROM_FOREGROUND,  /* Output from the foreground bitmap */
    OUTALPHA_FROM_GLOBALALPHA, /* Output from the global alpha */
    OUTALPHA_FROM_BUTT
} GFX_OUTALPHA_FROM_E;

typedef enum hiGFX_BLENDCMD_E {
    BLENDCMD_NONE = 0x0, /* fs: sa      fd: 1.0-sa */
    BLENDCMD_SRCOVER,    /* fs: 1.0     fd: 1.0-sa */
    BLENDCMD_BUTT
} GFX_BLENDCMD_E;

typedef struct hiGFX_BLEND_OPT_S {
    HI_BOOL bGlobalAlphaEnable; /* Global alpha enable */
    HI_BOOL bPixelAlphaEnable;  /* Pixel alpha enable */
    HI_BOOL bSrc1AlphaPremulti; /* Src1 alpha premultiply enable */
    HI_BOOL bSrc2AlphaPremulti; /* Src2 alpha premultiply enable */
    GFX_BLENDCMD_E eBlendCmd;   /* Alpha blending command */
} GFX_BLEND_OPT_S;

typedef struct hiGFX_BLIT_OPT_S {
    GFX_ALUCMD_E enAluCmd; /* Logical operation type */

    GFX_COLORKEY_MODE_E enColorKeyMode; /* Colorkey mode */

    GFX_COLORKEY_U unColorKeyValue; /* Colorkey value */

    GFX_CLIPMODE_E enClipMode; /* Perform the clip operation within or beyond the area */

    RECT_S stClipRect; /* Definition of the clipping area */

    HI_BOOL bResize; /* Whether to scale */

    GFX_MIRROR_E enMirror; /* Mirror type */

    HI_U8 u8GlobalAlpha; /* Global alpha value */

    GFX_OUTALPHA_FROM_E enOutAlphaFrom; /* Source of the output alpha */

    GFX_BLEND_OPT_S stBlendOpt;

    HI_BOOL bPatternFill;
    ROTATION_E enRotationAngle;
} GFX_BLIT_OPT_S;

typedef HI_S32 FN_VGS_Init(HI_VOID *pVrgs);

typedef HI_VOID FN_VGS_Exit(HI_VOID);

typedef HI_S32 FN_VGS_BeginJob(VGS_HANDLE *pVgsHanle, VGS_JOB_PRI_E enPriLevel,
                               MOD_ID_E enCallModId, HI_U32 u32CallDevId, HI_U32 u32CallChnId,
                               VGS_JOB_DATA_S *pJobData);

typedef HI_S32 FN_VGS_EndJob(VGS_HANDLE VgsHanle, HI_BOOL bSort, VGS_JOB_DATA_S *pJobData);

typedef HI_S32 FN_VGS_EndJobBlock(VGS_HANDLE VgsHanle);

typedef HI_S32 FN_VGS_CancelJob(VGS_HANDLE hHandle);

typedef HI_S32 FN_VGS_CancelJobByModDev(MOD_ID_E enCallModId, HI_U32 u32CallDevId, HI_U32 u32CallChnId,
                                        VGS_CANCEL_STAT_S *pstVgsCancelStat);

typedef HI_S32 FN_VGS_AddCoverTask(VGS_HANDLE VgsHanle, VGS_TASK_DATA_S *pTask, VGS_COVER_OPT_S *pstCoverOpt);

typedef HI_S32 FN_VGS_AddOSDTask(VGS_HANDLE VgsHanle, VGS_TASK_DATA_S *pTask, VGS_OSD_OPT_S *pstOsdOpt);

typedef HI_S32 FN_VGS_AddOnlineTask(VGS_HANDLE VgsHanle, VGS_TASK_DATA_S *pTask, VGS_ONLINE_OPT_S *pstOnlineOpt);

typedef HI_S32 FN_VGS_Add2ScaleTask(VGS_HANDLE VgsHandle, VGS_TASK_DATA_S *pTask);

typedef HI_S32 FN_VGS_AddGetLumaStatTask(VGS_HANDLE VgsHanle, VGS_TASK_DATA_S *pTask,
                                         VGS_LUMASTAT_OPT_S *pstLumaInfoOpt);

typedef HI_S32 FN_VGS_AddQuickCopyTask(VGS_HANDLE VgsHandle, VGS_TASK_DATA_S *pTask);

typedef HI_S32 FN_VGS_AddRotationTask(VGS_HANDLE VgsHanle, VGS_TASK_DATA_S *pTask, ROTATION_E enRotationAngle);

typedef HI_S32 FN_VGS_AddLDCTask(VGS_HANDLE VgsHanle, VGS_TASK_DATA_S *pTask, LDCV3_ATTR_S *pstLdcAttr);
typedef HI_S32 FN_VGS_AddLDCAndRotationTask(VGS_HANDLE VgsHanle, VGS_TASK_DATA_S *pTask, LDCV3_ATTR_S *pstLdcAttr,
                                            ROTATION_E enRotationAngle);
typedef HI_S32 FN_VGS_CheckLDCAttr(HI_U32 u32InWidth, HI_U32 u32InHeight, LDCV3_ATTR_S *pstLdcAttr);

typedef HI_S32 FN_VGS_AddBypassTask(VGS_HANDLE VgsHanle, VGS_TASK_DATA_S *pTask);

/* only for test */
typedef HI_VOID FN_VGS_GetMaxJobNum(HI_S32 *s32MaxJobNum);
typedef HI_VOID FN_VGS_GetMaxTaskNum(HI_S32 *s32MaxTaskNum);
typedef HI_S32 FN_VGS_AddGFXQuickCopyTask(VGS_HANDLE VgsHandle, VGS_TASK_DATA_S *pTask);
typedef HI_S32 FN_VGS_AddSource2FillTask(VGS_HANDLE VgsHandle, VGS_TASK_DATA_S *pTask,
                                         VGS_FILLCOLOR_S *pstFillColor);
typedef HI_S32 FN_VGS_AddDoubleSrcBlitTask(VGS_HANDLE VgsHandle, VGS_TASK_DATA_S *pTask,
                                           GFX_BLIT_OPT_S *pstBlitOpt);

typedef VGS_TASK_DATA_S *FN_VGS_GetFreeTask(HI_VOID);
typedef HI_VOID FN_VGS_PutFreeTask(VGS_TASK_DATA_S *pstTaskData);

typedef HI_S32 FN_VGS_AddSource2BlitTask(VGS_HANDLE VgsHandle, VGS_TASK_DATA_S *pTask, GFX_BLIT_OPT_S *pstBlitOpt);

typedef HI_S32 FN_VGS_AddNormFillTask(VGS_HANDLE VgsHandle, VGS_TASK_DATA_S *pTask, VGS_FILLCOLOR_S *pstFillColor,
                                      GFX_BLIT_OPT_S *pstBlitOpt);

typedef HI_S32 FN_VGS_AddSrc2FillTask(VGS_HANDLE VgsHandle, VGS_TASK_DATA_S *pTask, VGS_FILLCOLOR_S *pstFillColor,
                                      GFX_BLIT_OPT_S *pstBlitOpt);

typedef HI_S32 FN_VGS_EnableRegionDeflicker(HI_BOOL bRegionDeflicker);

typedef HI_S32 FN_VGS_WaitForJobDone(VGS_HANDLE hHandle);

typedef HI_S32 FN_VGS_WaitAllDone(HI_VOID);
typedef HI_S32 FN_VGS_SetAlphaThreshold(HI_U8 u8ThresholdValue);
typedef HI_S32 FN_VGS_GetAlphaThreshold(HI_U8 *pu8ThresholdValue);
typedef HI_S32 FN_VGS_WaitForGFXJobDone(VGS_HANDLE hHandle);

typedef struct hiVGS_EXPORT_FUNC_S {
    FN_VGS_BeginJob *pfnVgsBeginJob;
    FN_VGS_CancelJob *pfnVgsCancelJob;
    FN_VGS_CancelJobByModDev *pfnVgsCancelJobByModDev;
    FN_VGS_EndJob *pfnVgsEndJob;
    FN_VGS_AddCoverTask *pfnVgsAddCoverTask;
    FN_VGS_AddOSDTask *pfnVgsAddOSDTask;
    FN_VGS_AddBypassTask *pfnVgsAddBypassTask;
    FN_VGS_AddGetLumaStatTask *pfnVgsGetLumaStatTask;
    FN_VGS_AddOnlineTask *pfnVgsAddOnlineTask;
    /* for jpeg */
    FN_VGS_Add2ScaleTask *pfnVgsAdd2ScaleTask;
    /* for region */
    FN_VGS_AddQuickCopyTask *pfnVgsAddQuickCopyTask;

    FN_VGS_AddRotationTask *pfnVgsAddRotationTask;
    FN_VGS_AddLDCTask *pfnVgsAddLDCTask;
    FN_VGS_AddLDCAndRotationTask *pfnVgsAddLDCAndRotationTask;
    FN_VGS_CheckLDCAttr *pfnVgsCheckLDCAttr;

    /* for ive */
    //    FN_VGS_AddSp2RgbTask*       pfnVgsAddSp2RgbTask;
    FN_VGS_EndJobBlock *pfnVgsEndJobBlock;
    FN_VGS_WaitForJobDone *pfnVgsWaitForJobDone;
    FN_VGS_WaitAllDone *pfnVgsWaitAllDone;
    FN_VGS_SetAlphaThreshold *pfnVGSSetAlphaThreshold;
    FN_VGS_GetAlphaThreshold *pfnVGSGetAlphaThreshold;
    FN_VGS_WaitForGFXJobDone *pfnVgsWaitForGFXJobDone;
    /* only for test */
    FN_VGS_GetMaxJobNum *pfnVgsGetMaxJobNum;
    FN_VGS_GetMaxTaskNum *pfnVgsGetMaxTaskNum;
    FN_VGS_AddGFXQuickCopyTask *pfnVgsAddGFXQuickCopyTask;
    FN_VGS_AddSource2FillTask *pfnVgsAddSource2FillTask;
    FN_VGS_AddDoubleSrcBlitTask *pfnVgsAddDoubleSrcBlitTask;
    FN_VGS_AddSource2BlitTask *pfnVgsAddSource2BlitTask;
    FN_VGS_EnableRegionDeflicker *pfnVgsEnableRegionDeflicker;
    FN_VGS_AddNormFillTask *pfnVgsAddNormFillTask;
    FN_VGS_AddSrc2FillTask *pfnVgsAddSrc2FillTask;
    FN_VGS_GetFreeTask *pfnVgsGetFreeTask;
    FN_VGS_PutFreeTask *pfnVgsPutFreeTask;
} VGS_EXPORT_FUNC_S;

#endif /* __VGS_EXT_H__ */

