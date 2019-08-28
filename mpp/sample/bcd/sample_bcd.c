#include <stdio.h>
#include <stdlib.h>
#include "hi_audio_bcd.h"

#define CRY_TIMELIMIT    3000
#define CRY_TIMELIMITCNT 4
#define CRY_ALARMLIMIT   80

#define HI_UPVQE_MAXFRAMESIZE 4096

static int g_test_num = 0;
static int g_test_time = 0;

HI_S16 g_sin_buf[HI_UPVQE_MAXFRAMESIZE] = { 0 };
HI_S16 g_sou_buf[HI_UPVQE_MAXFRAMESIZE] = { 0 };

static void showtime(int time)
{
    int hour = 0;
    int min = 0;
    int sec = time / 1000;

    if (sec != 0) {
        min = sec / 60;
        sec = sec % 60;
    }

    if (min != 0) {
        hour = min / 60;
        min = min % 60;
    }

    printf("[%d:%02d:%02d:%d]", hour, min, sec, time % 1000);
}

static hi_s32 CryCallBack(HI_VOID *pAttr)
{
    showtime(g_test_time);
    printf("CryCallBack Now[%d]\n", g_test_num++);
    return HI_SUCCESS;
}

static hi_void CryUsage(hi_void)
{
    printf("\n\n/Usage:./bcd_sample <sampleRate> <filePath>/\n");
    printf("\tsampleRate list: 8000 16000\n");
}

#ifdef __HuaweiLite__
hi_s32 app_main(int argc, char* argv[])
#else
hi_s32 main(int argc, char* argv[])
#endif
{
    hi_s32 ret = HI_SUCCESS;
    FILE *fd_sin = NULL;
    char *cry_file_path = NULL;

    bcd_handle bcd = HI_NULL;
    hi_s32 sample_rate = 8000;
    hi_s32 frame_sample = 0;
    hi_bcd_config bcd_cfg;
    hi_bcd_process_data input_data = { 0 };
    hi_bcd_process_data output_data = { 0 };

    if (argc != 3) {
        CryUsage();
        return HI_FAILURE;
    }

    if (strcmp(argv[1], "-h") == HI_SUCCESS) {
        CryUsage();
        return HI_FAILURE;
    }

    sample_rate = atoi(argv[1]);
    if ((sample_rate != 8000) && (sample_rate != 16000)) {
        printf("%s: sample_rate = %d is wrong!\n", __FUNCTION__, sample_rate);
        ret = HI_FAILURE;
        goto ERR;
    }
    cry_file_path = argv[2];

    g_test_num = 0;
    g_test_time = 0;

    /* Open file. */
    fd_sin = fopen(cry_file_path, "rb");
    if (fd_sin == NULL) {
        printf("open sin file error!\n");
        ret = HI_FAILURE;
        goto ERR;
    }

    /* Create Cry Handle. */
    bcd_cfg.usr_mode = HI_TRUE;
    bcd_cfg.bypass = HI_FALSE;
    bcd_cfg.alarm_threshold = CRY_ALARMLIMIT;
    bcd_cfg.time_limit = CRY_TIMELIMIT;
    bcd_cfg.time_limit_threshold_count = CRY_TIMELIMITCNT;
    bcd_cfg.interval_time = 0;
    bcd_cfg.callback = (fn_bcd_callback)CryCallBack;
    ret = hi_baby_crying_detection_init(&bcd, sample_rate, &bcd_cfg);
    if (ret != HI_SUCCESS) {
        printf("init cry handle failed with %#x\n", ret);
        ret = HI_FAILURE;
        goto ERR;
    }

    /* Cry Detection Loop. */
    frame_sample = sample_rate / 100;
    input_data.data_size = frame_sample * sizeof(HI_S16);
    output_data.data_size = frame_sample * sizeof(HI_S16);
    input_data.data = g_sin_buf;
    output_data.data = g_sou_buf;
    while (1) {
        if (frame_sample != fread(input_data.data, sizeof(HI_S16), frame_sample, fd_sin)) {
            // printf("%s: remain data not enough\n", __FUNCTION__);
            break;
        }

        ret = hi_baby_crying_detection_process(bcd, &input_data, &output_data);
        if (ret != HI_SUCCESS) {
            printf("cry proc fail with %#x.\n", ret);
            ret = HI_FAILURE;
            break;
        }

        g_test_time += 10;
    }

    showtime(g_test_time);
    printf("## result ## cry detect over, CheckCount = %d.\n", g_test_num);

ERR:
    if (bcd != HI_NULL) {
        hi_baby_crying_detection_deinit(bcd);
    }

    if (fd_sin != NULL) {
        fclose(fd_sin);
    }

    return ret;
}

