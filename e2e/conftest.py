import pytest
import subprocess
import threading
from subprocess import Popen, PIPE, STDOUT
import os
from pathlib import Path

SCREENSHOT_DIR = Path(os.environ.get("MIRACLE_SCREENSHOT_DIR", "/tmp/miracle-screenshots"))


def _drain_pipe(pipe):
    try:
        while pipe.readline():
            pass
    except Exception:
        pass


class Server:
    def __init__(self, ipc: str, wayland: str) -> None:
        self.ipc = ipc
        self.wayland = wayland

    def open_app(self, command: str):
        path = os.environ.get("PATH", "") + ":/usr/games"
        env = {**os.environ, "WAYLAND_DISPLAY": self.wayland, "PATH": path}
        return subprocess.Popen([command], env=env)

    def screenshot(self, name: str) -> Path:
        SCREENSHOT_DIR.mkdir(parents=True, exist_ok=True)
        path = SCREENSHOT_DIR / f"{name}.png"
        env = {**os.environ, "WAYLAND_DISPLAY": self.wayland}
        result = subprocess.run(["grim", str(path)], env=env, capture_output=True, timeout=10)
        if result.returncode != 0:
            raise RuntimeError(f"grim failed: {result.stderr.decode()}")
        return path


@pytest.fixture(scope="function")
def server():
    binary = os.environ.get("MIRACLE_VISUAL_TEST_BIN", "miracle-wm")
    env = {**os.environ, "WAYLAND_DISPLAY": "wayland-98"}
    cmd = [binary, "--no-config", "1", "--platform-display-libs=mir:virtual", "--virtual-output=800x600"]
    process = Popen(
        cmd,
        env=env, stdout=PIPE, stderr=STDOUT,
    )

    socket = ""
    marker = "Listening to IPC socket on path: "
    startup_output = []

    for line in iter(process.stdout.readline, b""):
        data = line.decode("utf-8").strip()
        startup_output.append(data)
        if marker in data:
            i = data.index(marker) + len(marker)
            socket = data[i:].strip()
            break

    if not socket:
        process.wait()
        output = "\n".join(startup_output)
        pytest.fail(f"miracle-wm failed to start (exit {process.returncode}).\n{output}")

    threading.Thread(target=_drain_pipe, args=(process.stdout,), daemon=True).start()

    yield Server(socket, env["WAYLAND_DISPLAY"])

    process.terminate()
    process.stdout.close()
