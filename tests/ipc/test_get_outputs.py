from i3ipc import Connection
import os
import subprocess
from time import sleep

class TestGetOutputs:
    def test_one_output(self, server):
        conn = Connection(server.ipc)
        output_list = conn.get_outputs()

        assert len(output_list) == 1
        assert output_list[0].name == "Virtual-1"
        assert output_list[0].active == True
        assert output_list[0].make == "Unknown"
        assert output_list[0].model == "Unknown"
        assert output_list[0].serial == "Unknown"
        assert output_list[0].current_workspace == "1"
        assert output_list[0].rect.x == 0
        assert output_list[0].rect.y == 0
        assert output_list[0].rect.width == 800
        assert output_list[0].rect.height == 600
