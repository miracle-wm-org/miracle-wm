import time
from PIL import Image
from i3ipc import Connection


def test_compositor_starts_and_renders(visual_server):
    p = visual_server.open_app("gedit")
    time.sleep(2.0)  # allow surface to be composited on llvmpipe

    conn = Connection(visual_server.ipc)
    tree = conn.get_tree()
    assert len(tree.nodes) > 0, "IPC tree has no outputs"

    shot = visual_server.screenshot("compositor_renders")
    img = Image.open(shot).convert("RGB")
    w, h = img.size
    samples = {
        img.getpixel((w // 4, h // 4)),
        img.getpixel((w // 2, h // 2)),
        img.getpixel((3 * w // 4, 3 * h // 4)),
    }
    assert samples != {(0, 0, 0)}, "Screenshot is entirely black — compositor may not have rendered"

    p.terminate()
