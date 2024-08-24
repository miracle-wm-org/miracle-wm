# IPC Testing
This subproject intends to test IPC interactions with miracle-wm's socket. This
is accomplished by starting a headless miracle server and sending IPC requests
to it. The goal is to have complete coverage for miracle-wm's IPC.

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
