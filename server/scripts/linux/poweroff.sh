#!/bin/sh
# Linux poweroff script
if command -v shutdown >/dev/null 2>&1; then
  exec shutdown -h now
else
  echo "shutdown command not found" >&2
  exit 127
fi
