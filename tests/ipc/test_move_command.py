from i3ipc import Connection
from time import sleep

class TestMoveCommand:
    def test_move_to_workspace_by_number(self, server):
        conn = Connection(server.ipc)
        p1 = server.open_app("gedit")

        sleep(1)
        conn.command("move window to workspace 2")
        sleep(1)
        
        root = conn.get_tree()
        output = root.nodes[0]
        workspace = output.nodes[0]
        assert workspace.num == 2
        print(workspace.nodes[0].type == "con")

    def test_move_to_workspace_next(self, server):
        conn = Connection(server.ipc)
        p1 = server.open_app("gedit")

        sleep(1)
        conn.command("move window to workspace 2")
        sleep(1)
        
        root = conn.get_tree()
        output = root.nodes[0]
        workspace = output.nodes[0]
        assert workspace.num == 2
        print(workspace.nodes[0].type == "con")