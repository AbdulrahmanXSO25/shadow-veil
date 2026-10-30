#!/usr/bin/env bash
# Contact sheet of a clip for review: sheet.sh <clip.mp4> <out.png> [cols] [rows]
c=${3:-4}; r=${4:-2}
d=$(ffprobe -v error -show_entries format=duration -of csv=p=0 "$1")
ffmpeg -v error -y -i "$1" -vf "fps=$(echo "$c*$r/$d" | bc -l),scale=480:-1,tile=${c}x${r}" -frames:v 1 "$2"
