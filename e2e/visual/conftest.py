import pytest
import subprocess
from subprocess import Popen, PIPE, STDOUT
import os
from pathlib import Path

SCREENSHOT_DIR = Path(os.environ.get("MIRACLE_SCREENSHOT_DIR", "/tmp/miracle-screenshots"))


class Server:
    def __init__(self, ipc: str, wayland: str) -> None:
        self.ipc = ipc
        self.wayland = wayland

    def open_app(self, command: str):
        env = {**os.environ, "WAYLAND_DISPLAY": self.wayland}
        return subprocess.Popen([command], env=env)

    def screenshot(self, name: str) -> Path:
        SCREENSHOT_DIR.mkdir(parents=True, exist_ok=True)
        path = SCREENSHOT_DIR / f"{name}.png"
        env = {**os.environ, "WAYLAND_DISPLAY": self.wayland}
        result = subprocess.run(["grim", str(path)], env=env, capture_output=True, timeout=10)
        if result.returncode != 0:
            raise RuntimeError(f"grim failed: {result.stderr.decode()}")
        return path


def _create_server(args):
    binary = os.environ.get("MIRACLE_VISUAL_TEST_BIN", "miracle-wm")
    env = {**os.environ, "WAYLAND_DISPLAY": "wayland-99"}
    process = Popen([binary] + args, env=env, stdout=PIPE, stderr=STDOUT)
    return process, env


@pytest.fixture(scope="function")
def visual_server():
    process, env = _create_server([
        "--platform-display-libs", "mir:gbm-kms",
        "--no-config", "1",
    ])
    socket = ""
    marker = "Listening to IPC socket on path: "
    with process.stdout:
        for line in iter(process.stdout.readline, b""):
            data = line.decode("utf-8").strip()
            if marker in data:
                i = data.index(marker) + len(marker)
                socket = data[i:].strip()
                break

        yield Server(socket, env["WAYLAND_DISPLAY"])

        process.terminate()
        return
