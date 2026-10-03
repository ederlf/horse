#!/usr/bin/env python3
"""Run an OpenFlow application using the pinned OS-Ken controller runtime."""
import argparse
import logging
from pathlib import Path

from os_ken import cfg
from os_ken.base import app_manager
from os_ken.controller import controller  # registers the OpenFlow CLI options

parser = argparse.ArgumentParser()
parser.add_argument('--port', type=int, default=6653)
parser.add_argument('--app', default=str(
    Path(__file__).resolve().parents[1] / 'horse/apps/simple_switch_13.py'))
args = parser.parse_args()
logging.basicConfig(level=logging.INFO)
cfg.CONF(['--ofp-listen-host', '127.0.0.1',
          '--ofp-tcp-listen-port', str(args.port)], project='horse-controller')
app_manager.AppManager.run_apps([args.app])
