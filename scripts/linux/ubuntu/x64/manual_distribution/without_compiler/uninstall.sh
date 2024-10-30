#!/bin/bash
: << 'COMMENT'
|------------------------------|------------------------------|
|         🎃 item              |        👇 explanation        |
|------------------------------|------------------------------|
|    needs root permission?    |              No              |
|------------------------------|------------------------------|
|          dependencies        |              No              |
|------------------------------|------------------------------|
|          fellows             |           install.sh         |
|------------------------------|------------------------------|

Introduction of this script:
Uninstall binaries and libraries.
COMMENT

# Exit on error, treat unset variables as an error, and fail on pipeline errors
set -euo pipefail

BINARY_NAME="cpp-lint-action"
LIB_INSTALL_PATH="$HOME/.local/lib/${BINARY_NAME}/"
BIN_INSTALL_PATH="$HOME/.local/bin/"

# Refresh libraries
if [[ -d "${LIB_INSTALL_PATH}" ]]; then
  echo "Remove old ${BINARY_NAME} libraries"
  sudo rm -rf ${LIB_INSTALL_PATH}
fi
if [[ -d "${BIN_INSTALL_PATH}" ]]; then
  echo "Remove old ${BINARY_NAME} binaries"
  sudo rm -rf ${BIN_INSTALL_PATH}
fi
echo "Successfully uninstalled ${BINARY_NAME}"

