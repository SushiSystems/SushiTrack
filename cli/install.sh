#!/bin/sh
# Registers `ds` for the current shell session. Source it, do not execute it:
#   . cli/install.sh
CLI_DIR="$(cd "$(dirname "${BASH_SOURCE:-$0}")" && pwd)"

PATH="$CLI_DIR:$PATH"
export PATH
alias ds="python3 $CLI_DIR/ds.py"

printf '\033[0;32mSushiTrack CLI registered for this session.\033[0m\n'
printf '\033[0;36mds build | ds test | ds status | ds doctor | ds --help\033[0m\n\n'
printf '\033[0;33mTo make it permanent, add this to ~/.bashrc or ~/.zshrc:\033[0m\n'
printf "alias ds='python3 %s/ds.py'\n" "$CLI_DIR"
