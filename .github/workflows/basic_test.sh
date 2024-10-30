#!/bin/bash
set -euo pipefail

# 1. Create a temporary git repository
TEMP_REPO=$(mktemp -d)
# TEMP_REPO="${HOME}/temp/ci/test_git/"
# [[ -d "${TEMP_REPO}" ]] && rm -rf ${TEMP_REPO}
# mkdir -p "${TEMP_REPO}"

pushd "${TEMP_REPO}" &> /dev/null
git config --global init.defaultBranch master
git config --global user.email "test@gmail.com"
git config --global user.name "test"
git init
echo "Create and initialize a temporary repository at: ${TEMP_REPO}"

# 2. Add tools config files
cat <<EOT >> .clang-format
BasedOnStyle: Google
AllowShortBlocksOnASingleLine: Never
EOT

cat <<EOT >> .clang-tidy
Checks: '
  -*,
  cppcoreguidelines-*,
'
WarningsAsErrors: '*'
EOT
git add .
git commit -m "First commit: Added configs"

# 3. Add test files to master branch
cat <<EOT >> main.cpp
const int n = 2;
EOT
git add .
git commit -m "Second commit: Added test files"

# 4. Rewrite test files and commit to test branch
git checkout -b test
cat <<EOT >> main.cpp
int n             = 2;
EOT
git add .
git commit -m "Third commit: Rewrite test files"

git --no-pager log

echo "DEBUG_GITHUB_WORKSPACE=${TEMP_REPO}" >> "${GITHUB_ENV}"
echo "DEBUG_GITHUB_SHA=test" >> "${GITHUB_ENV}"
echo "DEBUG_GITHUB_EVENT_NAME=pull_request" >> "${GITHUB_ENV}"
echo "DEBUG_CPP_LINT_ACTION=true" >> "${GITHUB_ENV}"

popd &> /dev/null
