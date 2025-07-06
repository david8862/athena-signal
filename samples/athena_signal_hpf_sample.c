// Reference from:
// https://github.com/athena-team/athena-signal/blob/master/athena_signal/dios_signal.c
// https://blog.csdn.net/weixin_49337111/article/details/134229333
//
// build & run with following cmd:
// $ gcc -Wall -O2 -o athena_signal_hpf_sample athena_signal_hpf_sample.c -I<header file path> -L<lib file path> -lathenasignal -lm
// $ ./athena_signal_hpf_sample -h
// Usage: athena_signal_hpf_sample
// --input_file, -i: input raw audio file. default: 'input.wav'
// --chunk_size,  -c: audio chunk size to read every time. default: 640
// --output_file, -o: output pcm file for HPF processed audio. default: output.pcm
//
// $ ./athena_signal_hpf_sample -i hpf_input.wav -o hpf_output.pcm
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


int athena_signal_hpf_sample(char* input_file, int chunk_size, char* output_file)
{
    int ret;

    // create dios ssp param data structure
    objSSP_Param* SSP_PARAM = (objSSP_Param*)malloc(sizeof(objSSP_Param));
    memset(SSP_PARAM, 0, sizeof(objSSP_Param));

    // prepare dios ssp param
    //SSP_PARAM->NS_KEY = 1;
    SSP_PARAM->HPF_KEY = 1;
    SSP_PARAM->mic_num = 1;

    if (chunk_size % ATHENA_SIGNAL_FRAME_SIZE != 0) {
        printf("WARNING: chunk_size is not multiple of Athena-signal frame size %d, which will cause process issue!\n", ATHENA_SIGNAL_FRAME_SIZE);
    }

    // init dios ssp api
    void* st;
    st = dios_ssp_init_api(SSP_PARAM);
    dios_ssp_reset_api(st, SSP_PARAM);

    // open input/output file
    FILE* fp_input = fopen(input_file, "rb");
    FILE* fp_output = fopen(output_file, "wb");

    // calculate audio sample number
    // 16-bit (2 bytes) per sample
    fseek(fp_input, 0, SEEK_END);
    long file_len = ftell(fp_input);
    long sample_num = file_len / 2;
    printf("sample number of input audio is %ld\n", sample_num);
    // re-direct file pointer back to head
    rewind(fp_input);

    // calculate audio chunk number based on chunk size
    long chunk_num = sample_num / chunk_size;
    int tail_num = sample_num % chunk_size;
    printf("chunk number is %ld\n", chunk_num);

    // prepare data buffers
    short* ptr_input_data = (short*)calloc(chunk_size, sizeof(short));
    short* ptr_ref_data = (short*)calloc(chunk_size, sizeof(short));
    short* ptr_output_data = (short*)calloc(ATHENA_SIGNAL_FRAME_SIZE, sizeof(short));

    // set ref signal as 0 (HPF don't need it)
    memset(ptr_ref_data, 0, chunk_size*sizeof(short));

    // signal processing here
    for(long i=0; i < chunk_num; i++) {
        // read raw audio data from input file
        ret = fread(ptr_input_data, sizeof(short), chunk_size, fp_input);

        short* ptr_tmp_input = ptr_input_data;
        short* ptr_tmp_ref = ptr_ref_data;
        int loop_num = chunk_size / ATHENA_SIGNAL_FRAME_SIZE;

        for(int j=0; j < loop_num; j++) {
            // dios ssp processing
            ret = dios_ssp_process_api(st, ptr_tmp_input, ptr_tmp_ref, ptr_output_data, SSP_PARAM);
            //ret = dios_ssp_process_api(st, ptr_input_data, ptr_ref_data, ptr_output_data, SSP_PARAM);
            if (ret != OK_AUDIO_PROCESS) {
                printf("dios_ssp_process_api return error %d on chunk %ld frame %d, exit process!\n", ret, i, j);
                dios_ssp_uninit_api(st, SSP_PARAM);
                fclose(fp_input);
                fclose(fp_output);
                free(ptr_input_data);
                free(ptr_ref_data);
                free(ptr_output_data);
                free(SSP_PARAM);
                return -1;
            }

            // save enhanced audio to output file
            fwrite(ptr_output_data, sizeof(short), ATHENA_SIGNAL_FRAME_SIZE, fp_output);
            // move to next frame
            ptr_tmp_input += ATHENA_SIGNAL_FRAME_SIZE;
            ptr_tmp_ref += ATHENA_SIGNAL_FRAME_SIZE;
        }

        // show process bar
        int percentage = 100*i/chunk_num;
        show_progressbar(percentage, 100, 100);
    }
    // just save some data as tail, to align output length with input
    fwrite(ptr_output_data, sizeof(short), tail_num, fp_output);

    // uninit dios ssp api
    dios_ssp_uninit_api(st, SSP_PARAM);
    // release resources
    fclose(fp_input);
    fclose(fp_output);
    free(ptr_input_data);
    free(ptr_ref_data);
    free(ptr_output_data);
    free(SSP_PARAM);
    return 0;
}


void display_usage()
{
    printf("Usage: athena_signal_hpf_sample\n" \
           "--input_file, -i: input raw audio file. default: 'input.wav'\n" \
           "--chunk_size,  -c: audio chunk size to read every time. default: 640\n" \
           "--output_file, -o: output pcm file for HPF processed audio. default: output.pcm\n" \
           "\n");
    return;
}

int main(int argc, char** argv)
{
    char input_file[MAX_STR_LEN] = "input.wav";
    int chunk_size = 640;
    char output_file[MAX_STR_LEN] = "output.pcm";

    int c;
    while (1) {
        static struct option long_options[] = {
            {"input_file", required_argument, NULL, 'i'},
            {"chunk_size", required_argument, NULL, 'c'},
            {"output_file", required_argument, NULL, 'o'},
            {"help", no_argument, NULL, 'h'},
            {NULL, 0, NULL, 0}};

        /* getopt_long stores the option index here. */
        int option_index = 0;
        c = getopt_long(argc, argv, "c:hi:o:", long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1) break;

        switch (c) {
            case 'c':
                chunk_size = strtol(optarg, NULL, 10);
                break;
            case 'i':
                memset(input_file, 0, MAX_STR_LEN);
                strcpy(input_file, optarg);
                break;
            case 'o':
                memset(output_file, 0, MAX_STR_LEN);
                strcpy(output_file, optarg);
                break;
            case 'h':
            case '?':
            default:
                /* getopt_long already printed an error message. */
                display_usage();
                exit(-1);
        }
    }

    printf("NOTE: Athena-signal lib only support 16k sample rate, 16-bit audio data!\n");
    athena_signal_hpf_sample(input_file, chunk_size, output_file);

    printf("\nProcess finished.\n");
    return 0;
}

