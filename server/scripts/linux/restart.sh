#!/bin/sh
# Linux restart script
if command -v reboot >/dev/null 2>&1; then
  exec reboot
elif command -v shutdown >/dev/null 2>&1; then
  exec shutdown -r now
else
  echo "reboot/shutdown command not found" >&2
  exit 127
fi
