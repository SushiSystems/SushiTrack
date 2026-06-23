#!/bin/bash

# Get the directory of the current script
TOOLS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Add to PATH for current session
export PATH="$TOOLS_DIR:$PATH"

# Create aliases
alias ds="python3 $TOOLS_DIR/sushitrack.py"
alias sushitrack="python3 $TOOLS_DIR/sushitrack.py"

echo -e "\033[0;32mSushiTrack Tools installed in current session.\033[0m"
echo -e "\033[0;36mCommands available: ds, sushitrack\033[0m"
echo ""
echo -e "\033[0;33mTo make this permanent, add the following to your ~/.bashrc or ~/.zshrc:\033[0m"
echo -e "alias ds='python3 $TOOLS_DIR/sushitrack.py'"
echo -e "alias sushitrack='python3 $TOOLS_DIR/sushitrack.py'"
