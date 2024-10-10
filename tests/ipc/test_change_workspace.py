from i3ipc import Connection
from time import sleep

class TestChangeWorkspace:
    def check_workspace_number(conn, num, index):
        root = conn.get_tree()
        output = root.nodes[0]
        workspace = output.nodes[index]
        assert workspace.num == num

    def test_change_workspace(self, server):
        conn = Connection(server.ipc)
        conn.command("workspace 4")
        TestChangeWorkspace.check_workspace_number(conn, 4, 0)

    def test_next_workspace(self, server):
        conn = Connection(server.ipc)

        p1 = server.open_app("gedit")
        sleep(1)

        conn.command("workspace 4")

        p2 = server.open_app("gnome-chess")
        sleep(1)

        conn.command("workspace next")
        TestChangeWorkspace.check_workspace_number(conn, 1, 0)

        conn.command("workspace next")
        TestChangeWorkspace.check_workspace_number(conn, 4, 1)

    def test_prev_workspace(self, server):
        conn = Connection(server.ipc)

        p1 = server.open_app("gedit")
        sleep(1)

        conn.command("workspace 4")
        p2 = server.open_app("gnome-chess")
        sleep(1)

        conn.command("workspace prev")
        TestChangeWorkspace.check_workspace_number(conn, 1, 0)

        conn.command("workspace prev")
        TestChangeWorkspace.check_workspace_number(conn, 4, 1)

    def test_next_workspace_on_output(self, server):
        conn = Connection(server.ipc)

        p1 = server.open_app("gedit")
        sleep(1)

        conn.command("workspace 4")

        p2 = server.open_app("gnome-chess")
        sleep(1)

        conn.command("workspace next_on_output")
        TestChangeWorkspace.check_workspace_number(conn, 1, 0)

        conn.command("workspace next_on_output")
        TestChangeWorkspace.check_workspace_number(conn, 4, 1)

    def test_prev_workspace_on_output(self, server):
        conn = Connection(server.ipc)

        p1 = server.open_app("gedit")
        sleep(1)

        conn.command("workspace 4")
        p2 = server.open_app("gnome-chess")
        sleep(1)

        conn.command("workspace prev_on_output")
        TestChangeWorkspace.check_workspace_number(conn, 1, 0)

        conn.command("workspace prev_on_output")
        TestChangeWorkspace.check_workspace_number(conn, 4, 1)