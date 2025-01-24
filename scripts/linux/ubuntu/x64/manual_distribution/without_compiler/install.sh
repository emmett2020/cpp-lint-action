#!/bin/bash
: << 'COMMENT'
|------------------------------|------------------------------|
|         🎃 item              |        👇 explanation        |
|------------------------------|------------------------------|
|    needs root permission?    |              No              |
|------------------------------|------------------------------|
|          dependencies        |              No              |
|------------------------------|------------------------------|
|          fellows             |         uninstall.sh         |
|------------------------------|------------------------------|

Introduction of this script:
Install binaries and libraryies into local system.
COMMENT

# Exit on error, treat unset variables as an error, and fail on pipeline errors
set -euo pipefail

CUR_SCRIPT_DIR=$(cd $(dirname ${BASH_SOURCE[0]}); pwd)

BINARY_NAME="cpp-lint-action"
LIB_INSTALL_PATH="$HOME/.local/lib/${BINARY_NAME}"
BIN_INSTALL_PATH="$HOME/.local/bin"

# Refresh libraries
if [[ -d "${LIB_INSTALL_PATH}" ]]; then
  echo "Remove old ${BINARY_NAME} libraries"
  sudo rm -rf ${LIB_INSTALL_PATH}
fi
if [[ -f "${BIN_INSTALL_PATH}/${BINARY_NAME}" ]]; then
  echo "Remove old ${BINARY_NAME} binaries"
  sudo rm -rf "${BIN_INSTALL_PATH}/${BINARY_NAME}"
fi
mkdir -p ${LIB_INSTALL_PATH}
mkdir -p ${BIN_INSTALL_PATH}

mv ${CUR_SCRIPT_DIR}/lib/${BINARY_NAME}/* ${LIB_INSTALL_PATH}
mv ${CUR_SCRIPT_DIR}/bin/*                ${BIN_INSTALL_PATH}

echo "Successfully installed  ${BINARY_NAME}"
echo "Binaries install path:  ${BIN_INSTALL_PATH}"
echo "Libraries install path: ${LIB_INSTALL_PATH}"

