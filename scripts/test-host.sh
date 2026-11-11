#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
cd "$root"
export PYTHONDONTWRITEBYTECODE=1
make -C tests/zerotier config-test
make -C tests/zerotier -j2
make -C tests/zerotier enet-test -j2
make -C tests/modern gestures http-test resume-test
python3 tests/modern/test_profile.py
