#!/bin/bash

dir=$1
k=$2
maxy=$3
km1=$((k - 1))
kdiv2=$((km1 / 2))
python ~/horse/experimental/hedera-ryu/util/plot_rate.py \
       -f $dir/ecmp/bwm.txt  \
       --legend ECMP \
       --maxy $maxy \
       --xlabel 'Time (s)' \
       --ylabel 'Rate (Mbps)' \
       --total \
       -i "(([0-$km1]_[0-$kdiv2]_1-eth\d*[24680])|(s1-eth.*))" \
       -o $dir/ecmp_summary.png
