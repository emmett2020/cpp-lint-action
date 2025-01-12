#!/bin/bash

set -euo pipefail

# ninja
wget https://github.com/ninja-build/ninja/releases/download/v${NINJA_VERSION}/ninja-linux.zip
unzip ninja-linux.zip -d ${COMPILING_TOOLS_DIR}
rm ninja-linux.zip

echo "XXXXXXXXXXXXXXXXXXXXXXXX"

# ccache
wget https://github.com/ccache/ccache/releases/download/v${CCACHE_VERSION}/ccache-${CCACHE_VERSION}-linux-x86_64.tar.xz \
     -O ccache.tar.xz
tar -xf ccache.tar.xz
mv ccache-${CCACHE_VERSION}-linux-x86_64/ccache ${COMPILING_TOOLS_DIR}
rm -rf ccache.tar.xz  ccache-${CCACHE_VERSION}-linux-x86_64/


