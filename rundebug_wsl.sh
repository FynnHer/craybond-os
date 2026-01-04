#!/bin/bash

# Capture arguments and current directory
ARGS="$@"
DIR=$(pwd)

# Command 1: Build and Run
# 1. cd to the working directory (wt.exe starts in Windows home by default)
# 2. Run the make/qemu script
# 3. 'exec bash' keeps the terminal open if the command crashes so you can read errors
CMD_BUILD="cd \"$DIR\" && ./run debug; exec bash"

# Command 2: Debugger
# 1. cd to working directory
# 2. sleep 1 to give QEMU a moment to start (replaces 'delay 0.5')
# 3. Explicitly run with 'bash' to fix syntax errors
CMD_DEBUG="cd \"$DIR\" && sleep 1 && bash ./debug $ARGS; exec bash"

# Launch Windows Terminal (wt.exe)
# usage: wt.exe new-tab [args] ; split-pane [args]
# Note: The semicolon ';' must be escaped as '\;' so the script doesn't interpret it.

wt.exe new-tab --title "Kernel Build" wsl.exe -d "$WSL_DISTRO_NAME" bash -li -c "$CMD_BUILD" \; \
       split-pane --horizontal --title "Debugger" wsl.exe -d "$WSL_DISTRO_NAME" bash -li -c "$CMD_DEBUG"