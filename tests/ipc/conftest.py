import pytest
import subprocess
from subprocess import Popen, PIPE, STDOUT
import os

class Server:
    def __init__(self, ipc: str, wayland: str) -> None:
        self.ipc = ipc
        self.wayland = wayland

    def open_app(self, command: str):
        my_env = os.environ.copy()
        my_env['WAYLAND_DISPLAY'] = self.wayland
        return subprocess.Popen([command], env=my_env)

@pytest.fixture()
def server():
    if "MIRACLE_IPC_TEST_USE_ENV" in os.environ:
        yield Server(os.environ["SWAYSOCK"], os.environ["WAYLAND_DISPLAY"])
        return
    
    command = "miracle-wm"
    if "MIRACLE_IPC_TEST_BIN" in os.environ:
        command = os.environ["MIRACLE_IPC_TEST_BIN"]
    
    env = os.environ.copy()
    env['WAYLAND_DISPLAY'] = 'wayland-98'
    process = Popen([command, '--platform-display-libs', 'mir:virtual', '--platform-rendering-libs', 'mir:stub-graphics', '--virtual-output', '800x600', '--no-config', '1'],
                    env=env, stdout=PIPE, stderr=STDOUT)
    
    socket = ""
    to_find = "Listening to IPC socket on path: "
    with process.stdout:
        for line in iter(process.stdout.readline, b''):
            data = line.decode("utf-8").strip()
            # print(data)
            if to_find in data:
                i = data.index(to_find)
                i = i + len(to_find)
                socket = data[i:].strip()
                break

        yield Server(socket, env["WAYLAND_DISPLAY"])
        process.terminate()
        return