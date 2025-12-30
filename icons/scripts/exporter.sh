#!/bin/bash

iconToExport="$1"
iconName="${iconToExport%.*}"

sizes=(16 22 24 32 48 64 96 128 192 256 512 1024)

echo "Started exporting ${iconToExport}."

for _size in "${sizes[@]}" 
do
	echo "Exporting ${iconToExport} at size ${_size}px."
	inkscape --export-type="png" --export-filename="${iconName}_${_size}px" --export-width=${_size}  ${iconToExport}
done

echo "Completed exporting ${iconToExport}."
