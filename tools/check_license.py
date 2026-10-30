#!/usr/bin/env python3

from pathlib import Path
import os
import re
import argparse
from datetime import date

LICENSE = """/**
Copyright (C) 2025  Matthew Kosarek

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/

"""

MIRACLEMSG_LICENSE = """/**
Copyright (C) 2025  Matthew Kosarek

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

Portions of this code originate from swaymsg, licensed under the MIT license.
See the LICENSE.Sway file for details.
**/

"""

parser = argparse.ArgumentParser(description="This tool checks the licenses on CPP files and fixes them if told to do so")
parser.add_argument('--fix', type=bool,
                    help="Fix the licenses if required")

args = parser.parse_args()
fix = args.fix

to_check = ["src", "miraclemsg"]
to_exclude = [
    "wlr-output-management-unstable-v1_wrapper.h",
    "wlr-output-management-unstable-v1_wrapper.cpp"
]

def license_pattern(license: str) -> re.Pattern:
    # Accept any copyright year so that new files can use the current year.
    return re.compile(re.escape(license).replace("2025", r"\d{4}"))


def with_current_year(license: str) -> str:
    return license.replace("2025", str(date.today().year))


for d in to_check:
    root_dir = Path(__file__).parent.parent / d
    error_files = []
    error_data = []
    for x in os.listdir(root_dir.as_posix()):
        file = root_dir / x
        if x in to_exclude:
            continue

        if file.as_posix().endswith(".h") or file.as_posix().endswith(".cpp"):
            with open(file, 'r') as original:
                content = original.read()
                is_error = False
                expected = MIRACLEMSG_LICENSE if d == "miraclemsg" else LICENSE
                if not license_pattern(expected).match(content):
                    is_error = True

                if is_error:
                    error_files.append(file.as_posix())
                    error_data.append(content)

    if fix is True:
        for i in range(0, len(error_files)):
            file = error_files[i]
            data = error_data[i]
            with open(file, 'w') as f:
                to_write = MIRACLEMSG_LICENSE if "miraclemsg" in file else LICENSE
                f.write(with_current_year(to_write))
                f.write(data)
    elif len(error_files) > 0:
        print("The following files are missing the GPL License at the top: ")
        print("  " + "\n  ".join(error_files))
        exit(1)
