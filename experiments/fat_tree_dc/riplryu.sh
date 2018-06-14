#!/bin/sh

export PYTHONPATH=${PYTHONPATH}:`pwd`/ripl:`pwd`/riplryu
ryu-manager  riplryu/ryuripl.py --ripl-topo=ft,$1 --ripl-routing=$2
