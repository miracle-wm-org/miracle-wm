#!/bin/bash
find ./src/ -regex '.*\.\(cpp\|h\)' -exec /usr/bin/clang-format -style=file -i {} \;
find ./miracle-wm-config/src/ -regex '.*\.\(cpp\|h\)' -exec /usr/bin/clang-format -style=file -i {} \;
find ./miracle-wm-config/include/ -regex '.*\.\(cpp\|h\)' -exec /usr/bin/clang-format -style=file -i {} \;
find ./miracle-wm-config/tests/ -regex '.*\.\(cpp\|h\)' -exec /usr/bin/clang-format -style=file -i {} \;
find ./tests/ -regex '.*\.\(cpp\|h\)' -exec /usr/bin/clang-format -style=file -i {} \;
find ./miraclemsg/ -regex '.*\.\(cpp\|h\)' -exec /usr/bin/clang-format -style=file -i {} \;
