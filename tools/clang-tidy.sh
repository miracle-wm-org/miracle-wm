#!/bin/bash
find ./src/ -regex '.*\.\(cpp\|h\)' -exec /usr/bin/clang-format -style=file -i {} \;
find ./miracle-wm-c/src/ -regex '.*\.\(cpp\|h\)' -exec /usr/bin/clang-format -style=file -i {} \;
find ./miracle-wm-c/include/ -regex '.*\.\(cpp\|h\)' -exec /usr/bin/clang-format -style=file -i {} \;
find ./miracle-wm-c/tests/ -regex '.*\.\(cpp\|h\)' -exec /usr/bin/clang-format -style=file -i {} \;
find ./tests/ -regex '.*\.\(cpp\|h\)' -exec /usr/bin/clang-format -style=file -i {} \;
find ./miraclemsg/ -regex '.*\.\(cpp\|h\)' -exec /usr/bin/clang-format -style=file -i {} \;
