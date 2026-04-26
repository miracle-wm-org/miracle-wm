# End-to-End Testing
This suite tests miracle-wm end-to-end by spawning a real compositor instance
using the `mir:virtual` platform and exercising it via IPC and visual
screenshot comparisons.

- `ipc/` — tests for miracle-wm's IPC socket (workspace, layout, tree, etc.)
- `visual/` — tests that capture screenshots and verify rendered output

## Installation
```sh
python -m venv venv
source venv/bin/activate
pip install -r requirements.txt
```

## Running
```sh
source venv/bin/activate
pytest
```

To run a specific suite:
```sh
pytest ipc/
pytest visual/
```

### Environment variables
- `MIRACLE_VISUAL_TEST_BIN`: path to the `miracle-wm` binary. Defaults to `miracle-wm`.
- `MIRACLE_SCREENSHOT_DIR`: directory where screenshots are saved. Defaults to `/tmp/miracle-screenshots`.
