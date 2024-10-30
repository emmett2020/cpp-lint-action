#!/bin/bash
: << 'COMMENT'
|------------------------------|------------------------------|
|         🎃 item              |        👇 explanation        |
|------------------------------|------------------------------|
|    needs root permission?    |              Yes             |
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

# This file needs sudo permission.
# Install interpreter into desired directory.
BINARY_NAME="cpp-lint-action"
LIB_INSTALL_PATH="/usr/local/lib/${BINARY_NAME}"
BIN_INSTALL_PATH="/usr/local/bin/"

# Refresh libraries
if [[ -d "${LIB_INSTALL_PATH}" ]]; then
  echo "Remove old ${BINARY_NAME} libraries"
  sudo rm -rf ${LIB_INSTALL_PATH}
fi
if [[ -d "${BIN_INSTALL_PATH}" ]]; then
  echo "Remove old ${BINARY_NAME} binaries"
  sudo rm -rf ${BIN_INSTALL_PATH}
fi
sudo mkdir -p ${LIB_INSTALL_PATH}
sudo mkdir -p ${BIN_INSTALL_PATH}

sudo mv ${CUR_SCRIPT_DIR}/lib/${BINARY_NAME}/* ${LIB_INSTALL_PATH}
sudo mv ${CUR_SCRIPT_DIR}/bin/*                ${BIN_INSTALL_PATH}

echo "Successfully installed  ${BINARY_NAME}"
echo "Binaries install path:  ${BIN_INSTALL_PATH}"
echo "Libraries install path: ${LIB_INSTALL_PATH}"



