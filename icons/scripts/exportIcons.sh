#!/bin/bash

script_dir=$(dirname "$0")
exporter="$script_dir/exporter.sh"
icon_folder="$script_dir/.."

echo "Starting batch export."

$exporter "$icon_folder/color/color.svg"
$exporter "$icon_folder/clear/clear.svg"
$exporter "$icon_folder/simplified/simplified.svg"
$exporter "$icon_folder/simplifiedColor/simplifiedColor.svg"
$exporter "$icon_folder/withoutBG/withoutBG.svg"
$exporter "$icon_folder/clearStars/clearStars.svg"

echo "Batch export complete."
