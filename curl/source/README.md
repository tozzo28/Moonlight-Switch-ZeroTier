The source archive is curl 7.69.1 from:
https://curl.se/download/curl-7.69.1.tar.xz

SHA-256: 03c7d5e6697f7b7e40ada1b2256e565a555657398e6c1fcfa4cb251ccd819d4f

Run `scripts/rebuild-curl-switch.sh` from the repository to rebuild the static
library against the installed devkitPro headers and mbedTLS libraries. Rebuild
curl whenever the mbedTLS ABI/configuration changes. The portlibs curl currently
installed in this environment uses the libnx TLS backend, so it cannot directly
replace the bundled mbedTLS backend for this process-local socket transport.
