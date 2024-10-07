from i3ipc import Connection, Event, TickEvent
import os
import subprocess
from time import sleep

class TestSendTick:
    def test_empty_tree(self, server):
        conn = Connection(server.ipc)
        container = conn.get_tree()
        assert container.type == "root"

        output = container.nodes[0]
        assert output.type == "output"

        workspace = output.nodes[0]
        assert workspace.type == "workspace"

        assert len(workspace.nodes) == 0

    def test_regular_container(self, server):
        # Open gedit first
        my_env = os.environ.copy()
        my_env['WAYLAND_DISPLAY'] = server.wayland
        p = subprocess.Popen(['gedit'], env=my_env)
        sleep(1)  # Give gedit some time to settle down and open

        conn = Connection(server.ipc)
        container = conn.get_tree()
        output = container.nodes[0]
        workspace = output.nodes[0]
        
        assert len(workspace.nodes) == 1
