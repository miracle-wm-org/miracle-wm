from i3ipc import Connection, Event, TickEvent
import pytest
from subprocess import Popen, PIPE, STDOUT
import os
import time
import threading

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
    process = Popen([command, '--platform-display-libs', 'mir:virtual', '--virtual-output', '800x600'],
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

class TestSendTick:
    def test_send_tick(self, server):
        class Reply:
            def __init__(self) -> None:
                self.reply = ""

        print(f"Using server: {server}")
        
        reply = Reply()
        def wait_on_tick():
            def on_tick(i3, e: TickEvent):
                if not e.first:
                    reply.reply = e.payload
                    assert(e.payload == "ping")
                    conn1.main_quit()

            conn1 = Connection(server)
            conn1.on(Event.TICK, on_tick)
            conn1.main()
        
        t1 = threading.Thread(target=wait_on_tick)
        t1.start()
        time.sleep(1)  # A small wait time to ensure that the subscribe goes through first

        try:
            conn2 = Connection(server)
            conn2.send_tick("ping")
            t1.join()
        except Exception as e:
            print(f"Encountered error: {e}")
        
        assert reply.reply == "ping"
