"""Run existing SDN examples against a real OS-Ken learning switch.

Usage: python3 tests/integration_smoke.py --controller-python /path/to/venv/bin/python
Requires built Horse bindings and os-ken==4.2.0 in the controller environment.
"""
import argparse
import os
from pathlib import Path
import socket
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--controller-python', required=True)
    args = parser.parse_args()
    env = dict(os.environ, PYTHONPATH=str(ROOT))
    with socket.socket() as check:
        try:
            check.bind(('127.0.0.1', 6653))
        except OSError as error:
            raise RuntimeError('port 6653 must be free') from error
    with tempfile.TemporaryDirectory(prefix='horse-integration-') as directory:
        log = Path(directory) / 'controller.log'
        with log.open('w+') as output:
            controller = subprocess.Popen(
                [args.controller_python, str(ROOT / 'tools/run_controller.py')],
                cwd=directory, stdout=output, stderr=subprocess.STDOUT)
            try:
                deadline = time.monotonic() + 10
                while time.monotonic() < deadline:
                    if controller.poll() is not None:
                        raise RuntimeError(log.read_text())
                    try:
                        with socket.create_connection(('127.0.0.1', 6653), timeout=.2):
                            break
                    except OSError:
                        time.sleep(.1)
                else:
                    raise RuntimeError('controller did not start')
                for module, arguments, replies in (
                    ('horse.linear', ['2'], 2),
                    ('horse.topos.single', [], 1),
                    ('experiments.fat_tree_dc.hedera',
                     ['--horse', '--bw', '100', '-k', '2', '--time', '1',
                      '--iperf', '/usr/bin/iperf', '--dir', str(Path(directory) / 'fattree')], 0),
                ):
                    result = subprocess.run(
                        [sys.executable, '-m', module, *arguments], cwd=directory,
                        env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                        text=True, timeout=40)
                    print(result.stdout)
                    if result.returncode or result.stdout.count('ECHO_REPLY') < replies:
                        raise RuntimeError(f'{module} failed: {result.returncode}')
                    if not replies and (result.stdout.count('Received UDP flow') < 2 or
                                        'End of the Simulation' not in result.stdout):
                        raise RuntimeError(f'{module} failed: {result.returncode}')
                    print(f'PASS {module}: ' + (f'{replies} ping replies' if replies else '2 UDP deliveries'))
            finally:
                controller.terminate()
                try:
                    controller.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    controller.kill()
                    controller.wait()
                print(log.read_text())


if __name__ == '__main__':
    main()
