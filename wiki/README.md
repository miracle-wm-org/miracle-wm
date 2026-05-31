# miracle-wm-wiki
This subproject contains documentation for [miracle-wm](https://github.com/mattkae/miracle-wm).
There will be a new release of this documentation every time that `miracle-wm` has a release.

The docs can be found at: https://wiki.miracle-wm.org/latest/.

## Setup
```sh
python -m venv venv
source venv/bin/activate
pip install -r requirements.txt
```

## Testing
```sh
mkdocs serve
```

Then navigate to `localhost:8000`

## Releasing
- Simply tag a new release as `vX.Y.Z`
- CI will handle publishing the the latest version
