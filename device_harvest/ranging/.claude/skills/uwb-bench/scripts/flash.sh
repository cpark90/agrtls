#!/usr/bin/env bash
# Build + upload one UWB node by role/id/port (PlatformIO; arduino-cli forbidden).
#
# Usage: flash.sh <role> <id> <port>
#   role: anchor-sync | responder | anchor-acc | tag-acc | anchor-mesh
# Examples:
#   flash.sh anchor-sync 0 /dev/ttyUSB0
#   flash.sh responder     1 /dev/ttyUSB3
set -euo pipefail

role="${1:?role (anchor-sync|responder|anchor-acc|tag-acc|anchor-mesh)}"
id="${2:?id (integer node number)}"
port="${3:?port (e.g. /dev/ttyUSB0)}"

case "$role" in
  anchor-sync) env=anchor_dw1000_synchronous;             flag="-DANCHOR_ID=$id" ;;
  responder)     env=tag_dw1000_responder;             flag="-DTAG_ID=$id" ;;
  anchor-acc)    env=anchor_dw1000_accuracy;           flag="-DANCHOR_ID=$id" ;;
  tag-acc)       env=tag_dw1000_accuracy;              flag="-DTAG_ID=$id" ;;
  anchor-mesh)   env=anchor_dw1000_accuracy_meshagent; flag="-DANCHOR_ID=$id" ;;
  *) echo "unknown role: $role" >&2; exit 1 ;;
esac

# Run from the dir that holds platformio.ini (walk up from this script).
dir="$(cd "$(dirname "$0")" && pwd)"
while [ "$dir" != "/" ] && [ ! -f "$dir/platformio.ini" ]; do dir="$(dirname "$dir")"; done
[ -f "$dir/platformio.ini" ] || { echo "platformio.ini not found above $0" >&2; exit 1; }
cd "$dir"

echo "# flashing $env id=$id -> $port"
PLATFORMIO_BUILD_FLAGS="$flag" pio run -e "$env" -t upload --upload-port "$port"
