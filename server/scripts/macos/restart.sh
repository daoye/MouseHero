#!/bin/sh
# macOS restart script
if command -v shutdown >/dev/null 2>&1; then
  exec sudo shutdown -r now
else
  echo "shutdown command not found" >&2
  exit 127
fi
