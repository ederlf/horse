#!/bin/bash

# Exit on any failure
set -e

# Check for uninitialized variables
set -o nounset

ctrlc() {
    killall -9 python
    mn -c
    exit
}

trap ctrlc SIGINT


#For new kernel To disable DCTCP:
#sysctl -w net.ipv4.tcp_dctcp_enable=0
#sysctl -w net.ipv4.tcp_ecn=1

#For new kernel To disable MPTCP:
#sysctl -w net.mptcp.mptcp_enabled=0

k=$1
traffic=$2
flowsPerHost=$3
seed=$4
trial=$5
duration=$6
rootdir=results-bgp-horse/expt-$k-$traffic-$flowsPerHost-$seed-$trial
mkdir -p $rootdir
bw=1000
numhosts=$((k * k * k / 4))
maxbw=$((numhosts * bw))

# bandwidth of the whole topology is about 4Gb. For k=4 we have 48 links. so 10 each

# Note: you need to make sure you report the results
# for the correct port!
# In this example, we are assuming that each
# client is connected to port 2 on its switch
#iperf=~/iperf-patched/src/iperf
iperf=/usr/bin/iperf
# dir=$rootdir/hedera
# mn -c
# python hedera.py --bw $bw \
#         --dir $dir \
#         -k $k \
#         --iperf $iperf \
#         --traffic $traffic \
#         --seed $seed \
#         --time $duration \
#         --controller "./hederaryu.sh $k $bw" \
#         --flowsPerHost $flowsPerHost

dir=$rootdir/ecmp
# mn -c
python hedera.py --bw $bw \
        --dir $dir \
        -k $k \
        --iperf $iperf \
        --traffic $traffic \
        --seed $seed \
        --time $duration \
        --horse \
        --bgp \
        --flowsPerHost $flowsPerHost

# dir=$rootdir/random
# mn -c
# python hedera.py --bw $bw \
#         --dir $dir \
#         -k $k \
#         --iperf $iperf \
#         --traffic $traffic \
#         --seed $seed \
#         --time $duration \
#         --controller "./riplryu.sh $k random" \
#         --flowsPerHost $flowsPerHost

# dir=$rootdir/st
# mn -c
# python hedera.py --bw $bw \
#         --dir $dir \
#         -k $k \
#         --iperf $iperf \
#         --traffic $traffic \
#         --seed $seed \
#         --time $duration \
#         --controller "./riplryu.sh $k st" \
#         --flowsPerHost $flowsPerHost

# ./plot_run_ecmp.sh "$rootdir" "$k" "$maxbw"
