#!/bin/bash
#
# run "cppcheck" for static analysis
# Reference doc:
# https://blog.csdn.net/zong596568821xp/article/details/127527941
#
# apt install cppcheck
# cppcheck ../ --enable=all --xml 2> cppcheck_results.xml
# cppcheck-htmlreport --file=cppcheck_results.xml --report-dir=cppcheck_report --source-dir=.
# (open "cppcheck_report/index.html" with web brower to check)
#
#
# NOTE: This stress test script will be installed in and should be run at "build/install" dir!
#
if [[ "$#" -ne 1 ]]; then
    echo "Usage: $0 <loop_num>"
    exit 1
fi

LOOP_NUM=$1

SCRIPT_PATH=$(dirname $(readlink -f "$0"))
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$SCRIPT_PATH/../lib/

pushd $SCRIPT_PATH/../
mkdir output

for((i=1;i<=$LOOP_NUM;i++));
do
    rm -rf output/*

    # run MVDR beamforming test
    ./bin/samples/athena_signal_bf_sample -i testing/data/beamforming_1nan_3m_sound_0_noise_60_3channels.wav -b 1 -m 3 -f testing/data/beamforming_mic_coord.txt -l 90.0 -o output/beamforming_1nan_3m_sound_0_noise_60_mvdr_output.pcm
    # check if MVDR beamforming output match with the right result
    # due to potential calculation deviation, output may not strictly same as the right result
    MVDR_RESULT_BYTES=$(du -b testing/data/beamforming_1nan_3m_sound_0_noise_60_mvdr_output.pcm | awk '{print $1}')
    MVDR_DIFF_THRESHOLD=$(echo "$MVDR_RESULT_BYTES*0.01" | bc)
    MVDR_DIFF_BYTES=$(cmp -l testing/data/beamforming_1nan_3m_sound_0_noise_60_mvdr_output.pcm output/beamforming_1nan_3m_sound_0_noise_60_mvdr_output.pcm | wc -l | bc)
    MVDR_DIFF_RESULT=$(echo "$MVDR_DIFF_BYTES > $MVDR_DIFF_THRESHOLD" | bc)
    if [ $MVDR_DIFF_RESULT -eq 1 ]; then
        echo "MVDR beamforming test failed at loop "$i
        exit
    fi

    # run GSC beamforming test
    ./bin/samples/athena_signal_bf_sample -i testing/data/beamforming_1nan_3m_sound_0_noise_60_3channels.wav -b 2 -m 3 -f testing/data/beamforming_mic_coord.txt -l 90.0 -o output/beamforming_1nan_3m_sound_0_noise_60_gsc_output.pcm
    # check if GSC beamforming output match with the right result
    # due to potential calculation deviation, output may not strictly same as the right result
    GSC_RESULT_BYTES=$(du -b testing/data/beamforming_1nan_3m_sound_0_noise_60_gsc_output.pcm | awk '{print $1}')
    GSC_DIFF_THRESHOLD=$(echo "$GSC_RESULT_BYTES*0.06" | bc)
    GSC_DIFF_BYTES=$(cmp -l testing/data/beamforming_1nan_3m_sound_0_noise_60_gsc_output.pcm output/beamforming_1nan_3m_sound_0_noise_60_gsc_output.pcm | wc -l | bc)
    GSC_DIFF_RESULT=$(echo "$GSC_DIFF_BYTES > $GSC_DIFF_THRESHOLD" | bc)
    if [ $GSC_DIFF_RESULT -eq 1 ]; then
        echo "GSC beamforming test failed at loop "$i
        exit
    fi

    # run AEC test
    ./bin/samples/athena_signal_aec_sample -i testing/data/aec_input.wav -r testing/data/aec_ref.wav -o output/aec_output.pcm
    # check if AEC output match with the right result
    # due to potential calculation deviation, output may not strictly same as the right result
    AEC_RESULT_BYTES=$(du -b testing/data/aec_output.pcm | awk '{print $1}')
    AEC_DIFF_THRESHOLD=$(echo "$AEC_RESULT_BYTES*0.08" | bc)
    AEC_DIFF_BYTES=$(cmp -l testing/data/aec_output.pcm output/aec_output.pcm | wc -l | bc)
    AEC_DIFF_RESULT=$(echo "$AEC_DIFF_BYTES > $AEC_DIFF_THRESHOLD" | bc)
    if [ $AEC_DIFF_RESULT -eq 1 ]; then
        echo "AEC test failed at loop "$i
        exit
    fi

    # run AGC test
    ./bin/samples/athena_signal_agc_sample -i testing/data/agc_input.wav -o output/agc_output.pcm
    # check if AGC output match with the right result
    # due to potential calculation deviation, output may not strictly same as the right result
    AGC_RESULT_BYTES=$(du -b testing/data/agc_output.pcm | awk '{print $1}')
    AGC_DIFF_THRESHOLD=$(echo "$AGC_RESULT_BYTES*0.01" | bc)
    AGC_DIFF_BYTES=$(cmp -l testing/data/agc_output.pcm output/agc_output.pcm | wc -l | bc)
    AGC_DIFF_RESULT=$(echo "$AGC_DIFF_BYTES > $AGC_DIFF_THRESHOLD" | bc)
    if [ $AGC_DIFF_RESULT -eq 1 ]; then
        echo "AGC test failed at loop "$i
        exit
    fi

    # run HPF test
    ./bin/samples/athena_signal_hpf_sample -i testing/data/hpf_input.wav -o output/hpf_output.pcm
    # check if HPF output match with the right result
    # due to potential calculation deviation, output may not strictly same as the right result
    HPF_RESULT_BYTES=$(du -b testing/data/hpf_output.pcm | awk '{print $1}')
    HPF_DIFF_THRESHOLD=$(echo "$HPF_RESULT_BYTES*0.01" | bc)
    HPF_DIFF_BYTES=$(cmp -l testing/data/hpf_output.pcm output/hpf_output.pcm | wc -l | bc)
    HPF_DIFF_RESULT=$(echo "$HPF_DIFF_BYTES > $HPF_DIFF_THRESHOLD" | bc)
    if [ $HPF_DIFF_RESULT -eq 1 ]; then
        echo "HPF test failed at loop "$i
        exit
    fi

    # run DOA test
    #./bin/samples/athena_signal_doa_sample -i testing/data/beamforming_1nan_3m_sound_0_noise_60_3channels.wav -m 3 -f testing/data/beamforming_mic_coord.txt
    # check if DOA result is the right angle (0 degree)
    #if [ $? != 0 ]; then
        #echo "DOA test failed at loop "$i
        #exit
    #fi

done

popd
echo "Done! All the test cases has been passed!"

