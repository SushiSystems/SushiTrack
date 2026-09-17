#!/bin/sh
# Registers `st` and `sushitrack` for the current shell session. Source it, do not execute it:
#   . cli/install.sh
CLI_DIR="$(cd "$(dirname "${BASH_SOURCE:-$0}")" && pwd)"

PATH="$CLI_DIR:$PATH"
export PATH
alias st="python3 $CLI_DIR/st.py"
alias sushitrack="python3 $CLI_DIR/sushitrack.py"

printf '\033[0;32mSushiTrack CLI registered for this session.\033[0m\n'
printf '\033[0;36mst build | st test | st status | st doctor | st --help\033[0m\n\n'
printf '\033[0;33mTo make it permanent, add this to ~/.bashrc or ~/.zshrc:\033[0m\n'
printf "alias st='python3 %s/st.py'\n" "$CLI_DIR"
printf "alias sushitrack='python3 %s/sushitrack.py'\n" "$CLI_DIR"
