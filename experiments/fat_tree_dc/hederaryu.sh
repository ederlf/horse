#!/bin/sh

export PYTHONPATH="${PYTHONPATH:-}:$PWD/ripl:$PWD/riplryu"
ryu-manager riplryu/hederaryu.py --hedera-topo=ft,"$1" --hedera-routing=hashed --hedera-bw="$2"
