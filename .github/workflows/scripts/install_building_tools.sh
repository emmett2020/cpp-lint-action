#!/bin/bash

# Will instal tools to ${HOME}/.tools

set -euo pipefail

# ninja
wget https://github.com/ninja-build/ninja/releases/download/v${NINJA_VERSION}/ninja-linux.zip
unzip ninja-linux.zip -d ~/.tools/
rm ninja-linux.zip

# ccache
wget https://github.com/ccache/ccache/releases/download/v${CCACHE_VERSION}/ccache-${CCACHE_VERSION}-linux-x86_64.tar.xz \
     -O ccache.tar.xz
tar -xf ccache.tar.xz
mv ccache-${CCACHE_VERSION}-linux-x86_64/ccache ~/.tools/
rm -rf ccache.tar.xz  ccache-${CCACHE_VERSION}-linux-x86_64/


