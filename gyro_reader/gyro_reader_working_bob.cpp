//
// Created by outdu-gram on 7/2/23.
//
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include <math.h>
#include <vmf/video_source.h>
#include "fw.h"

#include "SharedCompactMemory/shared_compact_memory.h"
#define GYRO_READER_MODULE_NAME	"gyro_reader"
static SCM_BUFFER_T g_tScmBuf;		//! SCM buffer structure.

static float g_motion_threshold=1.0;
static int g_num_samples=200;
static int g_no_motion_detect_num=60;
static int g_motion_detect_num=5;
static int g_debug=1;
static int g_ir=1;//This service run when IRCUT OPEN => IR ON; 0 OFF, 1 ON, 2 PULSE

static SCM_HANDLE_T *g_ptScmHandle = NULL;	//! The pointer to SRB handle.
static int g_bTerminate = 0;

void print_msg(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "[%s] ", GYRO_READER_MODULE_NAME);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}

static void sig_kill(int signo)
{
    print_msg("[%s] receive SIGNAL: %d\n",__func__, signo);
    g_bTerminate = 1;
    SCM_WakeupReader(g_ptScmHandle);
}

static int init_output_scm(const char *name)
{
    g_ptScmHandle = SCM_InitReader(name);
    if (!g_ptScmHandle) {
        return -1;
    }
    g_tScmBuf = SCM_BUFFER_T_DEFAULT;
    if(SCM_ReturnReaderBuff(g_ptScmHandle, &g_tScmBuf)) {
        printf("Init SCM_ReturnReaderBuff failed.\n");
        return -1;
    }
    return 0;
}

static void release_gyro() {

    SCM_Release(g_ptScmHandle);
}

/* show usage */
static void usage(void)
{
    printf("Usage:\n\t gyro_reader [-m <0.01,0.05,0.1 etc>] [-n <50,100,200, default=50>] [-t <100,200>] [-e <5,10>] [-d <0/1>] [-h help] \n");
    printf("\t -m: motion_threshold, default=%f\n", g_motion_threshold);
    printf("\t -n: num_samples, default=%d\n", g_num_samples);
    printf("\t -t: no_motion_detect_threshold, default=%d\n", g_no_motion_detect_num);
    printf("\t -e: motion_detect_threshold, default=%d\n", g_motion_detect_num);
    printf("\t -d: debug, default=%d\n", g_debug);
}

static int is_ircut_filter_on() {
    enum ON_OFF ir_cutfilter;
    if(get_ir_cutfilter(&ir_cutfilter) < 0) {
        return 0;
    }
    if(ir_cutfilter==OFF) {
        return 0;
    }
    return 1;
}

int main(int argc __attribute__((unused)), char* argv[] __attribute__((unused)))
{
    printf("gyro reader main\n");
    int ch;
    int ret = -1;

    signal(SIGTERM, sig_kill);
    signal(SIGINT, sig_kill);

    while ((ch = getopt(argc, argv, "m:n:p:t:e:d:h")) != -1) {
        switch (ch) {
            case 'm':
                g_motion_threshold = atof(optarg);
                break;
            case 'n':
                g_num_samples =  atoi(optarg);
                break;
            case 't':
                g_no_motion_detect_num = atoi(optarg);
                break;
            case 'e':
                g_motion_detect_num = atoi(optarg);
                break;
            case 'd':
                g_debug= atoi(optarg);
                break;
            case 'h':
            default:
                usage();
                exit(0);
        }
    }

    VMF_GYRO_DATA_T* ptGyroData;

    if(init_output_scm(GYRO_BUFFER_PIN)<0) {
        printf("init gyro_reader failed!\n");
        exit(0);
    }

    float a_x_prev=0,a_y_prev=0,a_z_prev=0;
    //while set prev values
    while (!g_bTerminate) {
        ret = SCM_ReturnReceiveReaderBuff(g_ptScmHandle, &g_tScmBuf);
        if (ret)
            continue;
        ptGyroData = (VMF_GYRO_DATA_T*)g_tScmBuf.pbyVirtAddr;
        int gyroNumber = (int)ptGyroData->dwDataContinuousSize;

        for (int i = 0; i < gyroNumber; i++) {
            a_x_prev = ptGyroData->afAccel[0];
            a_y_prev = ptGyroData->afAccel[1];
            a_z_prev = ptGyroData->afAccel[2];

            ptGyroData++;
        }
        break;
    }

    int totalNum=0, no_motion_detect_num=0, motion_detect_num=0;
    float total_d_x=0,total_d_y=0,total_d_z=0;
    while (!g_bTerminate) {
//        enum ON_OFF ir_cutfilter;
//        if(get_ir_cutfilter(&ir_cutfilter) < 0){
//            printf("error reading ircut status\n");
//            sleep(1);
//            continue;
//        }
//        if(ir_cutfilter==OFF) {
//            printf("ircut filter is closed\n");
//            sleep(60);
//            continue;
//        }

        ret = SCM_ReturnReceiveReaderBuff(g_ptScmHandle, &g_tScmBuf);
        if (ret)
            continue;

        ptGyroData = (VMF_GYRO_DATA_T*)g_tScmBuf.pbyVirtAddr;
//        int gyroNumber = g_tScmBuf.dwOccupiedSpace / sizeof(VMF_GYRO_DATA_T);
        int gyroNumber = (int)ptGyroData->dwDataContinuousSize;

        for (int i = 0; i < gyroNumber; i++) {
            if(g_debug>=3) {
                printf("%f,%f,%f\n", ptGyroData->afAccel[0],ptGyroData->afAccel[1],ptGyroData->afAccel[2]);
            }

            float a_x = ptGyroData->afAccel[0];
            float a_y = ptGyroData->afAccel[1];
            float a_z = ptGyroData->afAccel[2];

            float d_x = fabs(a_x-a_x_prev);
            float d_y = fabs(a_y-a_y_prev);
            float d_z = fabs(a_z-a_z_prev);

            total_d_x+=d_x;
            total_d_y+=d_y;
            total_d_z+=d_z;

            totalNum++;
            a_x_prev = a_x;
            a_y_prev = a_y;
            a_z_prev = a_z;
            ptGyroData++;
        }
        if(g_debug>=3){
            printf("DONE dwOccupiedSpace = %u, gyroNumber=%d\n", g_tScmBuf.dwOccupiedSpace, gyroNumber);
        }
//        struct timespec tp;
//        clock_gettime(CLOCK_REALTIME, &tp);
        if(totalNum>=g_num_samples) {
            float avg_d_x=total_d_x/totalNum;
            float avg_d_y=total_d_y/totalNum;
            float avg_d_z=total_d_z/totalNum;
            if(g_debug>=2){
                printf("avg %f,%f,%f\n", avg_d_x, avg_d_y, avg_d_z);
            }
            if(avg_d_x>g_motion_threshold || avg_d_y>g_motion_threshold || avg_d_z>g_motion_threshold) {
                if(g_debug>=1){
                    printf("###########MOTION##########################\n");
                }
                motion_detect_num++;
                if(motion_detect_num > g_motion_detect_num) {
                    no_motion_detect_num=0;
                    if(g_ir!=1) {
                        if(is_ircut_filter_on()){
                            if(g_debug>=1){
                                printf("###########IR ON##########################\n");
                            }
                            set_ir_led_brightness(1);
                        } else {
                            if(g_debug>=1){
                                printf("###########IR ON BUT IRCUT IS OFF##########################\n");
                            }
                        }
                        g_ir=1;//IR ON
                    }
                }
            }else {
                if(g_debug>=1){
                    printf("###########NO_MOTION##########################\n");
                }
                no_motion_detect_num++;
                motion_detect_num=0;
            }
            if(no_motion_detect_num > g_no_motion_detect_num) {
                if(g_ir==2) {
                    if(is_ircut_filter_on()){
                        if(g_debug>=1){
                            printf("###########IR OFF##########################\n");
                        }
                        set_ir_led_brightness(0);
                    } else {
                        if(g_debug>=1){
                            printf("###########IR OFF BUT IRCUT IS OFF##########################\n");
                        }
                    }
                    g_ir=0;
                } else {
                    if(is_ircut_filter_on()){
                        if(g_debug>=1){
                            printf("###########IR PULSE##########################\n");
                        }
                        set_ir_led_brightness(2);
                    } else {
                        if(g_debug>=1){
                            printf("###########IR PULSE BUT IRCUT IS OFF##########################\n");
                        }
                    }
                    g_ir=2;
                }
                no_motion_detect_num=0;
            }

            total_d_x=0;
            total_d_y=0;
            total_d_z=0;
            totalNum=0;
        }
    }

    release_gyro();
    print_msg("terminated successfully!\n");
    return 0;
}