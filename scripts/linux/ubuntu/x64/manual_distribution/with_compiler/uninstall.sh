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
LIB_INSTALL_PATH="/usr/local/lib/${BINARY_NAME}/"
BIN_INSTALL_PATH="/usr/local/bin"

# Refresh libraries
if [[ -d "${LIB_INSTALL_PATH}" ]]; then
  echo "Remove old ${BINARY_NAME} libraries"
  sudo rm -rf ${LIB_INSTALL_PATH}
fi
if [[ -f "${BIN_INSTALL_PATH}/${BINARY_NAME}" ]]; then
  echo "Remove old ${BINARY_NAME} binaries"
  sudo rm -rf "${BIN_INSTALL_PATH}/${BINARY_NAME}"
fi

echo "Successfully uninstalled ${BINARY_NAME}"



