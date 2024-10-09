from i3ipc import Connection
from time import sleep

class TestChangeWorkspace:
    def check_workspace_number(num, conn):
        root = conn.get_tree()
        output = root.nodes[0]
        workspace = output.nodes[0]
        assert workspace.num == num

    def test_change_workspace(self, server):
        conn = Connection(server.ipc)
        conn.command("workspace 4")

        root = conn.get_tree()
        output = root.nodes[0]
        workspace = output.nodes[0]
        assert workspace.num == 4

    def test_next_workspace(self, server):
        conn = Connection(server.ipc)

        p1 = server.open_app("gedit")
        sleep(1)

        conn.command("workspace 4")
        root = conn.get_tree()
        output = root.nodes[0]
        workspace = output.nodes[0]
        assert workspace.num == 4
        sleep (1)

        p2 = server.open_app("gnome-chess")
        sleep(1)

        conn.command("workspace next")
        sleep (1)

        sleep(1)
        conn.command("workspace next")
        