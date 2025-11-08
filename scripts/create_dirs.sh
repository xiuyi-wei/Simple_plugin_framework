#!/usr/bin/env bash
# 用法: create_dirs.sh ABS_DIR1 ABS_DIR2 FILE_NAME
set -euo pipefail
d1="$1"; d2="$2"; f3="$3"
mkdir -p "$d1"
mkdir -p "$d2"
touch "$d2/$f3"
