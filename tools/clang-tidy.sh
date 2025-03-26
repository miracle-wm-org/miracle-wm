#!/bin/bash
find ./src/ -regex '.*\.\(cpp\|h\)' -exec /usr/bin/clang-format -style=file -i {} \;
find ./libmiracle-wm-config/src/ -regex '.*\.\(cpp\|h\)' -exec /usr/bin/clang-format -style=file -i {} \;
find ./libmiracle-wm-config/include/ -regex '.*\.\(cpp\|h\)' -exec /usr/bin/clang-format -style=file -i {} \;
find ./tests/ -regex '.*\.\(cpp\|h\)' -exec /usr/bin/clang-format -style=file -i {} \;
find ./miraclemsg/ -regex '.*\.\(cpp\|h\)' -exec /usr/bin/clang-format -style=file -i {} \;
