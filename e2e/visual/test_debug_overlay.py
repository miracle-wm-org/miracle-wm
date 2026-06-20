import time
from PIL import Image, ImageChops
from i3ipc import Connection


def _changed_pixels(a: Image.Image, b: Image.Image) -> int:
    """Number of pixels that differ between two same-sized RGB images."""
    diff = ImageChops.difference(a, b)
    return sum(1 for px in diff.getdata() if px != (0, 0, 0))


def test_debug_overlay_renders(server):
    # Give the overlay something to draw boxes/labels around.
    app = server.open_app("gedit")
    time.sleep(2.0)  # allow surface to be composited on llvmpipe

    conn = Connection(server.ipc)
    assert len(conn.get_tree().nodes) > 0, "IPC tree has no outputs"

    before = Image.open(server.screenshot("debug_overlay_before")).convert("RGB")

    # Toggle the debug overlay on. This launches miracle-wm-debug-overlay, which
    # draws window geometry, the cursor and a HUD on top of everything.
    replies = conn.command("debug")
    assert all(r.success for r in replies), "'debug' command was not accepted"

    time.sleep(2.5)  # allow the overlay client to launch, map and draw

    after = Image.open(server.screenshot("debug_overlay_after")).convert("RGB")
    assert before.size == after.size

    # The overlay must visibly change the output (outlines + opaque HUD panel).
    # A non-trivial threshold guards against incidental single-pixel noise.
    changed = _changed_pixels(before, after)
    assert changed > 500, f"debug overlay did not visibly render (changed={changed} px)"

    # Toggling it off again must be accepted (and terminates the client).
    replies = conn.command("debug")
    assert all(r.success for r in replies), "'debug' toggle-off was not accepted"

    app.terminate()
