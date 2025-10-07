#!/bin/sh
# macOS sleep script
if command -v pmset >/dev/null 2>&1; then
  exec pmset sleepnow
else
  echo "pmset command not found" >&2
  exit 127
fi
