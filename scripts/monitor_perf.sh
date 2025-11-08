#!/usr/bin/env bash
# 用法: monitor_perf.sh PROCESS_NAME
set -euo pipefail
proc="$1"
# 这里随便写点示例逻辑
echo "monitoring $proc..." >> data/perf.log
