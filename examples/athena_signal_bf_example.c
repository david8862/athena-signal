// Reference from:
// https://github.com/athena-team/athena-signal/blob/master/athena_signal/dios_signal.c
// https://blog.csdn.net/weixin_49337111/article/details/134229333
//
// build & run with following cmd:
// $ gcc -Wall -O2 -o athena_signal_bf_example athena_signal_bf_example.c -I<header file path> -L<lib file path> -lathenasignal -lm
// $ ./athena_signal_bf_example -h
// Usage: athena_signal_test
// --input_file, -i: input multi-channel audio file. default: 'input.wav'
// --bf_type, -b: type of beamforming, 1 for MVDR and 2 for GSC. default: 1
// --mic_num, -m: number of mics. default: 3
// --loc_phi, -l: source location azimuth (in degrees). default: 90.0
// --chunk_size, -c: audio chunk size. default: 128
// --output_file, -o: output wav file for enhanced audio. default: output.wav
//
// $ ./athena_signal_bf_example -i 3channels.wav -b 2 -m 3 -o test.pcm
//
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <getopt.h>
#include "dios_ssp_api.h"
#include "dios_ssp_return_defs.h"

#define MAX_STR_LEN 128


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


int athena_signal_bf_example(char* input_file, int bf_type, int mic_num, float* mic_coord, float loc_phi, int chunk_size, char* output_file)
{
    int ret;

    // create dios ssp param data structure
    objSSP_Param* SSP_PARAM = (objSSP_Param*)malloc(sizeof(objSSP_Param));
    memset(SSP_PARAM, 0, sizeof(objSSP_Param));

    // prepare dios ssp param
    //SSP_PARAM->NS_KEY = 0;
    SSP_PARAM->BF_KEY = bf_type;
    SSP_PARAM->mic_num = mic_num;
    SSP_PARAM->loc_phi = loc_phi;

    if(SSP_PARAM->BF_KEY != 0) {
        if (SSP_PARAM->mic_num <= 1) {
            printf("Beamforming is turned on, mic_num must be greater than 1. \n");
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

    // init dios ssp api
    void* st;
    st = dios_ssp_init_api(SSP_PARAM);
    dios_ssp_reset_api(st, SSP_PARAM);

    // open input/output file
    FILE* fp_input = fopen(input_file, "rb");
    FILE* fp_output = fopen(output_file, "wb");

    // calculate audio sample number
    // 16-bit (2 bytes) per data, and mic_num/channels data per sample
    fseek(fp_input, 0, SEEK_END);
    long file_len = ftell(fp_input);
    long sample_num = file_len / (2*mic_num);
    printf("sample number of input audio is %ld\n", sample_num);
    // re-direct file pointer back to head
    rewind(fp_input);

    // calculate audio frame number based on chunk size
    long frame_num = sample_num / chunk_size;
    int tail_num = sample_num % chunk_size;
    printf("frame number is %ld\n", frame_num);

    // prepare data buffers
    short *ptr_input_data = (short*)calloc(mic_num*chunk_size, sizeof(short));
    short *ptr_temp = (short*)calloc(mic_num*chunk_size, sizeof(short));
    short *ptr_output_data = (short*)calloc(chunk_size,sizeof(short));
    short *ptr_ref_data = (short*)calloc(chunk_size, sizeof(short));

    // set ref signal as 0 (BF don't need it)
    memset(ptr_ref_data, 0, chunk_size*sizeof(short));

    // signal processing here
    for(long i=0; i < frame_num; i++) {
        // read data from input file
        fread(ptr_temp, sizeof(short), mic_num*chunk_size, fp_input);

        // since layout of multi-channel audio data in .wav audio is interleave,
        // but athena-signal lib only handle non-interleave layout data,
        // so we need convert audio data from interleave to non-interleave
        for(int j=0; j < chunk_size; j++) {
            for(int k=0; k < mic_num; k++) {
                ptr_input_data[j+k*chunk_size] = ptr_temp[mic_num*j+k];
            }
        }

        // dios ssp processing
        ret = dios_ssp_process_api(st, ptr_input_data, ptr_ref_data, ptr_output_data, SSP_PARAM) != OK_AUDIO_PROCESS;
        if (ret != OK_AUDIO_PROCESS) {
            printf("dios_ssp_process_api return error %d on frame %ld, exit process!\n", ret, i);
            fclose(fp_input);
            fclose(fp_output);
            free(ptr_input_data);
            free(ptr_temp);
            free(ptr_output_data);
            free(ptr_ref_data);
            free(SSP_PARAM);
            return -1;
        }

        // save enhanced audio to output file
        fwrite(ptr_output_data, sizeof(short), chunk_size, fp_output);

        // show process bar
        int percentage = 100*i/frame_num;
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
    free(ptr_temp);
    free(ptr_output_data);
    free(ptr_ref_data);
    free(SSP_PARAM);
    return 0;
}


void display_usage()
{
    printf("Usage: athena_signal_test\n" \
           "--input_file, -i: input multi-channel audio file. default: 'input.wav'\n" \
           "--bf_type, -b: type of beamforming, 1 for MVDR and 2 for GSC. default: 1\n" \
           "--mic_num, -m: number of mics. default: 3\n" \
           "--loc_phi, -l: source location azimuth (in degrees). default: 90.0\n" \
           "--chunk_size, -c: audio chunk size. default: 128\n" \
           "--output_file, -o: output pcm file for enhanced audio. default: output.pcm\n" \
           "\n");
    return;
}


int main(int argc, char** argv)
{
    char input_file[MAX_STR_LEN] = "input.wav";
    int bf_type = 1;
    int mic_num = 3;
    float* mic_coord;
    float loc_phi = 90.0;
    int chunk_size = 128;
    char output_file[MAX_STR_LEN] = "output.pcm";

    int c;
    while (1) {
        static struct option long_options[] = {
            {"input_file", required_argument, NULL, 'i'},
            {"bf_type", required_argument, NULL, 'b'},
            {"mic_num", required_argument, NULL, 'm'},
            {"loc_phi", required_argument, NULL, 'l'},
            {"chunk_size", required_argument, NULL, 'c'},
            {"output_file", required_argument, NULL, 'o'},
            {"help", no_argument, NULL, 'h'},
            {NULL, 0, NULL, 0}};

        /* getopt_long stores the option index here. */
        int option_index = 0;
        c = getopt_long(argc, argv, "b:c:hi:l:m:o:", long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1) break;

        switch (c) {
            case 'b':
                bf_type = strtol(optarg, NULL, 10);
                break;
            case 'c':
                chunk_size = strtol(optarg, NULL, 10);
                break;
            case 'i':
                memset(input_file, 0, MAX_STR_LEN);
                strcpy(input_file, optarg);
                break;
            case 'l':
                loc_phi = strtof(optarg, NULL);
                break;
            case 'm':
                mic_num = strtol(optarg, NULL, 10);
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

    // alloc memory for mic coordinate
    // here we just assign some fake coordinate to make both MVDR & GSC work, not sure why...
    mic_coord = malloc(mic_num*3*sizeof(float));
    if (bf_type == 1) {
        for(int i=0; i < mic_num; i++) {
            mic_coord[3*i] = 1.0;
            mic_coord[3*i+1] = 1.0;
            mic_coord[3*i+2] = 0.0;
        }
    } else if (bf_type == 2) {
        memset(mic_coord, 0, mic_num*3*sizeof(float));
    }

    printf("NOTE: Athena-signal lib only support 16k sample rate, 16-bit audio data!\n");
    athena_signal_bf_example(input_file, bf_type, mic_num, mic_coord, loc_phi, chunk_size, output_file);

    free(mic_coord);
    printf("\nProcess finished.\n");
    return 0;
}

