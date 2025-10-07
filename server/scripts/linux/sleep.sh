#!/bin/sh
# Linux sleep script
if command -v systemctl >/dev/null 2>&1; then
  exec systemctl suspend
elif command -v pm-suspend >/dev/null 2>&1; then
  exec pm-suspend
else
  echo "No suspend command found" >&2
  exit 127
fi
