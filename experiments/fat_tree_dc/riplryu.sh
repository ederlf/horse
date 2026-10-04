#!/bin/sh

export PYTHONPATH="${PYTHONPATH:-}:$PWD/ripl:$PWD/riplryu"
ryu-manager riplryu/ryuripl.py --ripl-topo=ft,"$1" --ripl-routing="$2"
