#!/bin/sh
# macOS poweroff script
# Customize as needed. To run without password, consider configuring sudoers.
# Default: use shutdown (may require sudo)
if command -v shutdown >/dev/null 2>&1; then
  exec sudo shutdown -h now
else
  echo "shutdown command not found" >&2
  exit 127
fi
