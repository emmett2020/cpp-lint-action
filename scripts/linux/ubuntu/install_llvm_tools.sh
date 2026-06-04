#!/usr/bin/env bash

set -euo pipefail

if (($# < 1)); then
  echo "usage: $0 <llvm-major-version> [apt-package ...]" >&2
  exit 2
fi

llvm_version="$1"
shift

if (($# == 0)); then
  exit 0
fi

if [[ ! -r /etc/os-release ]]; then
  echo "Cannot detect Ubuntu codename: /etc/os-release is missing" >&2
  exit 1
fi

# shellcheck disable=SC1091
. /etc/os-release

if [[ "${ID:-}" != "ubuntu" ]]; then
  echo "LLVM apt bootstrap currently supports Ubuntu only, got: ${ID:-unknown}" >&2
  exit 1
fi

codename="${VERSION_CODENAME:-}"
if [[ -z "${codename}" ]]; then
  echo "Cannot detect Ubuntu VERSION_CODENAME from /etc/os-release" >&2
  exit 1
fi

as_root=()
if ((EUID != 0)); then
  as_root=(sudo)
fi

keyring="/etc/apt/keyrings/apt.llvm.org.gpg"
source_file="/etc/apt/sources.list.d/llvm-toolchain-${llvm_version}.list"
repo_url="https://apt.llvm.org/${codename}/"
repo_suite="llvm-toolchain-${codename}-${llvm_version}"

"${as_root[@]}" apt-get update
"${as_root[@]}" apt-get install -y ca-certificates gnupg wget
"${as_root[@]}" install -d -m 0755 /etc/apt/keyrings

tmp_key="$(mktemp)"
trap 'rm -f "${tmp_key}"' EXIT
wget -qO- https://apt.llvm.org/llvm-snapshot.gpg.key | gpg --dearmor > "${tmp_key}"
"${as_root[@]}" install -m 0644 "${tmp_key}" "${keyring}"

echo "deb [signed-by=${keyring}] ${repo_url} ${repo_suite} main" \
  | "${as_root[@]}" tee "${source_file}" >/dev/null

"${as_root[@]}" apt-get update
"${as_root[@]}" apt-get install -y "$@"
