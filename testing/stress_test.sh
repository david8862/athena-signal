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

    # run beamforming test
    #./bin/samples/athena_signal_bf_sample -i testing/data/beamforming_1nan_3m_sound_0_noise_60_3channels.wav -b 2 -m 3 -f testing/data/beamforming_mic_coord.txt -l 90.0 -o output/beamforming_1nan_3m_sound_0_noise_60_output.pcm
    # check if beamforming output match with the right result
    #diff testing/data/beamforming_1nan_3m_sound_0_noise_60_output.pcm output/beamforming_1nan_3m_sound_0_noise_60_output.pcm > /dev/null
    #if [ $? != 0 ]; then
        #echo "Beamforming test failed at loop "$i
        #exit
    #fi

    # run AEC test
    ./bin/samples/athena_signal_aec_sample -i testing/data/aec_input.wav -r testing/data/aec_ref.wav -o output/aec_output.pcm
    # check if AEC output match with the right result
    diff testing/data/aec_output.pcm output/aec_output.pcm > /dev/null
    if [ $? != 0 ]; then
        echo "AEC test failed at loop "$i
        exit
    fi

    # run AGC test
    ./bin/samples/athena_signal_agc_sample -i testing/data/agc_input.wav -o output/agc_output.pcm
    # check if AGC output match with the right result
    diff testing/data/agc_output.pcm output/agc_output.pcm > /dev/null
    if [ $? != 0 ]; then
        echo "AGC test failed at loop "$i
        exit
    fi

    # run HPF test
    #./bin/samples/athena_signal_hpf_sample -i testing/data/hpf_input.wav -o output/hpf_output.pcm
    # check if HPF output match with the right result
    #diff testing/data/hpf_output.pcm output/hpf_output.pcm > /dev/null
    #if [ $? != 0 ]; then
        #echo "HPF test failed at loop "$i
        #exit
    #fi

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

