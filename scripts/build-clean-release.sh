#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
export DEVKITPRO="${DEVKITPRO:-/opt/devkitpro}"
if [ -z "${TAR:-}" ]; then
    if command -v bsdtar >/dev/null; then export TAR=bsdtar; else export TAR=tar; fi
fi
export PATH="$DEVKITPRO/devkitA64/bin:$DEVKITPRO/tools/bin:$PATH"
for command in cmake make python3 g++ sha256sum; do
    command -v "$command" >/dev/null || { printf "Missing build tool: %s (use Ubuntu/Linux with devkitPro).\n" "$command" >&2; exit 1; }
done
test -x "$DEVKITPRO/devkitA64/bin/aarch64-none-elf-g++"
cd "$root"
# No previous application/Core/curl objects or private Git history; FFmpeg uses verified upstream archives.
python3 scripts/check-repository.py
bash scripts/test-host.sh
mkdir -p src/zerotier/lib curl/lib
make -f zerotier-core/zt-core.mk OUTDIR="$root/.build-core" -j2
cp .build-core/libztcore.a src/zerotier/lib/libztcore.a
bash scripts/rebuild-curl-switch.sh
make -j2
mkdir -p release/switch/Moonlight-Switch
cp moonlight.nro release/switch/Moonlight-Switch/Moonlight-Switch.nro
(cd release && sha256sum switch/Moonlight-Switch/Moonlight-Switch.nro > SHA256SUMS)
python3 scripts/check-release.py release
printf 'Build complete. Hardware testing is still required.\n'
