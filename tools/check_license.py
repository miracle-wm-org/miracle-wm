#!/usr/bin/env python3

from pathlib import Path
import os
import argparse

LICENSE = """/**
Copyright (C) 2024  Matthew Kosarek

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

parser = argparse.ArgumentParser(description="This tool checks the licenses on CPP files and fixes them if told to do so")
parser.add_argument('--fix', type=bool,
                    help="Fix the licenses if required")

args = parser.parse_args()
fix = args.fix

root_dir = Path(__file__).parent.parent / "src"
error_files = []
error_data = []
for x in os.listdir(root_dir.as_posix()):
    file = root_dir / x
    if file.as_posix().endswith(".h") or file.as_posix().endswith(".cpp"):
        with open(file, 'r') as original:
            content = original.read()
            if not content.startswith(LICENSE):
                error_files.append(file.as_posix())
                error_data.append(content)

if fix is True:
    for i in range(0, len(error_files)):
        file = error_files[i]
        data = error_data[i]
        with open(file, 'w') as f:
            f.write(LICENSE)
            f.write(data)
elif len(error_files) > 0:
    print("The following files are missing the GPL License at the top: ")
    print("  " + "\n  ".join(error_files))
    exit(1)
