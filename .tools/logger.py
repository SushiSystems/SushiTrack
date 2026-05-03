import sys
import os

YELLOW = "\033[93m"
RED    = "\033[91m"
GREEN  = "\033[92m"
BLUE   = "\033[94m"
RESET  = "\033[0m"

def set_color_mode():
    """@brief Enables ANSI color output on Windows via the system 'color' command."""
    if sys.platform == "win32":
        os.system("color")

set_color_mode()

def log_info(message):
    """
    @brief Prints an informational message in blue.
    @param message The text to display.
    """
    print(f"{BLUE}[INFO]{RESET} {message}")

def log_warning(message):
    """
    @brief Prints a warning message in yellow.
    @param message The text to display.
    """
    print(f"{YELLOW}[WARNING]{RESET} {message}")

def log_error(message):
    """
    @brief Prints an error message in red.
    @param message The text to display.
    """
    print(f"{RED}[ERROR]{RESET} {message}")

def log_success(message):
    """
    @brief Prints a success message in green.
    @param message The text to display.
    """
    print(f"{GREEN}[SUCCESS]{RESET} {message}")
