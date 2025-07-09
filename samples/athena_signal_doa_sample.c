// Reference from:
// https://github.com/athena-team/athena-signal/blob/master/athena_signal/dios_signal.c
// https://blog.csdn.net/weixin_49337111/article/details/134229333
//
// build & run with following cmd:
// $ gcc -Wall -O2 -o athena_signal_doa_sample athena_signal_doa_sample.c -I<header file path> -L<lib file path> -lathenasignal -lm
// $ ./install/bin/samples/athena_signal_doa_sample -h
// Usage: athena_signal_doa_sample
// --input_file, -i: input multi-channel audio file. default: 'input.wav'
// --chunk_size,  -c: audio chunk size to read every time. default: 640
// --mic_num, -m: number of mics. default: 3
// --mic_coord_file, -f: mics coordinate text file. default: mic_coord.txt
//
// $ ./athena_signal_doa_sample -i 3channels.wav -m 3 -f mic_coord.txt
//
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <getopt.h>
#include "dios_ssp_api.h"
#include "dios_ssp_return_defs.h"

#define MAX_STR_LEN 128
#define ATHENA_SIGNAL_FRAME_SIZE (128)  // athena-signal use hard-coded frame size


void show_progressbar(int progress, int total, int barWidth)
{
    float percentage = (float)progress / total;
    int filledLength = barWidth * percentage;

    printf("\r[");
    for (int i = 0; i < barWidth; i++) {
        if (i < filledLength) {
            printf("#");
        } else {
            printf(" ");
        }
    }
    printf("] %.1f%%", percentage * 100);

    fflush(stdout);
}


int athena_signal_doa_sample(char* input_file, int chunk_size, int mic_num, float* mic_coord)
{
    int ret=0;
    float angle=0.0;

    // create dios ssp param data structure
    objSSP_Param* SSP_PARAM = (objSSP_Param*)malloc(sizeof(objSSP_Param));
    memset(SSP_PARAM, 0, sizeof(objSSP_Param));

    // prepare dios ssp param
    SSP_PARAM->DOA_KEY = 1;
    SSP_PARAM->mic_num = mic_num;

    if(SSP_PARAM->DOA_KEY == 1) {
        if (SSP_PARAM->mic_num <= 1) {
            printf("DOA is turned on, mic_num must be greater than 1. \n");
            free(SSP_PARAM);
            return -1;
        }

        // assign microphone coordinate
        for (int i=0; i < SSP_PARAM->mic_num; i++) {
            SSP_PARAM->mic_coord[i].x = mic_coord[3 * i];
            SSP_PARAM->mic_coord[i].y = mic_coord[3 * i + 1];
            SSP_PARAM->mic_coord[i].z = mic_coord[3 * i + 2];
        }
    }

    if (chunk_size % ATHENA_SIGNAL_FRAME_SIZE != 0) {
        printf("WARNING: chunk_size is not multiple of Athena-signal frame size %d, which will cause process issue!\n", ATHENA_SIGNAL_FRAME_SIZE);
    }

    // init dios ssp api
    void* st;
    st = dios_ssp_init_api(SSP_PARAM);
    dios_ssp_reset_api(st, SSP_PARAM);

    // open input audio file
    FILE* fp_input = fopen(input_file, "rb");

    // calculate audio sample number
    // 16-bit (2 bytes) per data, and mic_num/channels data per sample
    fseek(fp_input, 0, SEEK_END);
    long file_len = ftell(fp_input);
    long sample_num = file_len / (2*mic_num);
    printf("sample number of input audio is %ld\n", sample_num);
    // re-direct file pointer back to head
    rewind(fp_input);

    // calculate audio chunk number based on chunk size
    long chunk_num = sample_num / chunk_size;
    printf("chunk number is %ld\n", chunk_num);

    // prepare data buffers
    short* ptr_input_data = (short*)calloc(mic_num*ATHENA_SIGNAL_FRAME_SIZE, sizeof(short));
    short* ptr_temp = (short*)calloc(mic_num*chunk_size, sizeof(short));
    short* ptr_ref_data = (short*)calloc(chunk_size, sizeof(short));
    short* ptr_output_data = (short*)calloc(ATHENA_SIGNAL_FRAME_SIZE, sizeof(short));

    // set ref signal as 0 (DOA don't need it)
    memset(ptr_ref_data, 0, chunk_size*sizeof(short));

    // signal processing here
    for(long i=0; i < chunk_num; i++) {
        // read data from input file
        ret = fread(ptr_temp, sizeof(short), mic_num*chunk_size, fp_input);
        short* ptr_tmp_input = ptr_temp;
        int loop_num = chunk_size / ATHENA_SIGNAL_FRAME_SIZE;

        for(int j=0; j < loop_num; j++) {
            // since layout of multi-channel audio data in .wav audio is interleave,
            // but athena-signal lib only handle non-interleave layout data,
            // so we need convert audio data from interleave to non-interleave
            for(int j=0; j < ATHENA_SIGNAL_FRAME_SIZE; j++) {
                for(int k=0; k < mic_num; k++) {
                    ptr_input_data[j+k*ATHENA_SIGNAL_FRAME_SIZE] = ptr_tmp_input[mic_num*j+k];
                }
            }

            // dios ssp processing
            ret = dios_ssp_process_api(st, ptr_input_data, ptr_ref_data, ptr_output_data, SSP_PARAM);
            if (ret != OK_AUDIO_PROCESS) {
                printf("dios_ssp_process_api return error %d on chunk %ld frame %d, exit process!\n", ret, i, j);
                dios_ssp_uninit_api(st, SSP_PARAM);
                fclose(fp_input);
                free(ptr_input_data);
                free(ptr_temp);
                free(ptr_output_data);
                free(ptr_ref_data);
                free(SSP_PARAM);
                return -1;
            }

            // only pick DOA angle when detect speech
            if (dios_ssp_vad_result_get_api(st, SSP_PARAM) > 0) {
                angle = dios_ssp_doa_result_get_api(st, SSP_PARAM);
            }
            // move to next frame
            ptr_tmp_input += mic_num*ATHENA_SIGNAL_FRAME_SIZE;
        }

        // show process bar
        int percentage = 100*i/chunk_num;
        show_progressbar(percentage, 100, 100);
    }
    printf("\nDOA angle: %f\n", angle);

    // uninit dios ssp api
    dios_ssp_uninit_api(st, SSP_PARAM);
    // release resources
    fclose(fp_input);
    free(ptr_input_data);
    free(ptr_temp);
    free(ptr_output_data);
    free(ptr_ref_data);
    free(SSP_PARAM);

    return (int)angle;
}


void parse_mic_coord(char* mic_coord_file, float* mic_coord, int mic_num)
{
    // parse mic array 3D coordinate (x,y,z) from txt file, one line per mic and
    // x-axis point to right, y-axis point to front and z-axis point to up.
    // the coordinate unit is meters, like follow:
    //
    // 0.0,-0.0277,0.0
    // 0.024,0.0138575,0.0
    // -0.024,0.0138575,0.0
    //
    FILE* fp_mic_coord = fopen(mic_coord_file, "rb");
    for(int i=0; i < mic_num; i++) {
        fscanf(fp_mic_coord, "%f,%f,%f", &(mic_coord[3*i]), &(mic_coord[3*i+1]), &(mic_coord[3*i+2]));
    }

    fclose(fp_mic_coord);
    return;
}


void display_usage()
{
    printf("Usage: athena_signal_doa_sample\n" \
           "--input_file, -i: input multi-channel audio file. default: 'input.wav'\n" \
           "--chunk_size,  -c: audio chunk size to read every time. default: 640\n" \
           "--mic_num, -m: number of mics. default: 3\n" \
           "--mic_coord_file, -f: mics coordinate text file. default: mic_coord.txt\n" \
           "\n");
    return;
}


int main(int argc, char** argv)
{
    char input_file[MAX_STR_LEN] = "input.wav";
    int chunk_size = 640;
    int mic_num = 3;
    char mic_coord_file[MAX_STR_LEN] = "mic_coord.txt";
    float* mic_coord;
    int ret = 0;

    int c;
    while (1) {
        static struct option long_options[] = {
            {"input_file", required_argument, NULL, 'i'},
            {"chunk_size", required_argument, NULL, 'c'},
            {"mic_num", required_argument, NULL, 'm'},
            {"mic_coord_file", required_argument, NULL, 'f'},
            {"help", no_argument, NULL, 'h'},
            {NULL, 0, NULL, 0}};

        /* getopt_long stores the option index here. */
        int option_index = 0;
        c = getopt_long(argc, argv, "c:f:hi:m:", long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1) break;

        switch (c) {
            case 'c':
                chunk_size = strtol(optarg, NULL, 10);
                break;
            case 'f':
                memset(mic_coord_file, 0, MAX_STR_LEN);
                strcpy(mic_coord_file, optarg);
                break;
            case 'i':
                memset(input_file, 0, MAX_STR_LEN);
                strcpy(input_file, optarg);
                break;
            case 'm':
                mic_num = strtol(optarg, NULL, 10);
                break;
            case 'h':
            case '?':
            default:
                /* getopt_long already printed an error message. */
                display_usage();
                exit(-1);
        }
    }

    // alloc memory & parse mic coordinate
    mic_coord = malloc(mic_num*3*sizeof(float));
    parse_mic_coord(mic_coord_file, mic_coord, mic_num);

    printf("NOTE: Athena-signal lib only support 16k sample rate, 16-bit audio data!\n");
    ret = athena_signal_doa_sample(input_file, chunk_size, mic_num, mic_coord);

    free(mic_coord);
    printf("\nProcess finished.\n");

    return ret;
}

