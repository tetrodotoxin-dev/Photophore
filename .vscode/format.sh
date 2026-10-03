#!/usr/bin/env bash
# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

set -euo pipefail

if (($# == 0)); then
  echo 'Pass the exact C or C++ files to format.' >&2
  exit 1
fi

repository="$(git -C "$(dirname "${BASH_SOURCE[0]}")" rev-parse --show-toplevel)"
cd "$repository"
clang-format -i -- "$@"
