import pytest
from subprocess import Popen, PIPE, STDOUT
import os

@pytest.fixture()
def server():
    if "MIRACLE_IPC_TEST_USE_ENV" in os.environ:
        yield os.environ["SWAYSOCK"]
        return
    
    command = "miracle-wm"
    if "MIRACLE_IPC_TEST_BIN" in os.environ:
        command = os.environ["MIRACLE_IPC_TEST_BIN"]
    
    env = os.environ.copy()
    env['WAYLAND_DISPLAY'] = 'wayland-98'
    process = Popen([command, '--platform-display-libs', 'mir:virtual', '--virtual-output', '800x600', '--no-config', '1'],
                    env=env, stdout=PIPE, stderr=STDOUT)
    
    socket = ""
    to_find = "Listening to IPC socket on path: "
    with process.stdout:
        for line in iter(process.stdout.readline, b''):
            data = line.decode("utf-8").strip()
            if to_find in data:
                i = data.index(to_find)
                i = i + len(to_find)
                socket = data[i:].strip()
                break

        yield socket
        process.terminate()
        return