#!/usr/bin/env bash
# Rebuild the bundled curl against the installed devkitPro mbedTLS ABI.
set -euo pipefail
repo_root="$(cd "$(dirname "$0")/.." && pwd)"
export DEVKITPRO="${DEVKITPRO:-/opt/devkitpro}"
source_archive="$repo_root/curl/source/curl-7.69.1.tar.xz"
printf '%s  %s\n' 03c7d5e6697f7b7e40ada1b2256e565a555657398e6c1fcfa4cb251ccd819d4f "$source_archive" | sha256sum -c -
curl_build_dir="$(mktemp -d /tmp/moonlight-curl.XXXXXX)"
"${TAR:-tar}" -xf "$source_archive" -C "$curl_build_dir"
cd "$curl_build_dir/curl-7.69.1"
export CC="$DEVKITPRO/devkitA64/bin/aarch64-none-elf-gcc"
export AR="$DEVKITPRO/devkitA64/bin/aarch64-none-elf-ar"
export RANLIB="$DEVKITPRO/devkitA64/bin/aarch64-none-elf-ranlib"
export CFLAGS='-O2 -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE -ffunction-sections -fdata-sections -Wno-implicit-function-declaration'
export CPPFLAGS="-D__SWITCH__ -I$DEVKITPRO/portlibs/switch/include -I$DEVKITPRO/libnx/include"
export LDFLAGS="-specs=$DEVKITPRO/libnx/switch.specs -march=armv8-a+crc+crypto -mtp=soft -fPIE -L$DEVKITPRO/portlibs/switch/lib -L$DEVKITPRO/libnx/lib"
export LIBS='-lnx'
./configure --host=aarch64-none-elf --disable-shared --enable-static \
  --disable-ipv6 --disable-unix-sockets --disable-manual --disable-ntlm-wb \
  --disable-threaded-resolver --without-ssl --with-mbedtls="$DEVKITPRO/portlibs/switch" \
  --without-libidn2 --without-libpsl --without-nghttp2 --without-brotli \
  --without-ca-bundle --without-ca-path
make -C lib -j2
cp lib/.libs/libcurl.a "$repo_root/curl/lib/libcurl.a"
printf 'Rebuilt curl archive; build files: %s\n' "$curl_build_dir"
printf 'Rebuild Moonlight with make -j2.\n'
