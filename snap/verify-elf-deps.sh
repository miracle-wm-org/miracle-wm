#!/bin/bash
#
# Verifies that the snap ships every shared library that its own binaries
# link against. Exits non-zero and prints the offending sonames otherwise.
#
# Usage: verify-elf-deps.sh <prime-dir>

set -u
cd "$1" || exit 1

# Missing = sonames needed by ELFs, minus every core26 lib on snap
missing=$(comm -23 \
    <(readelf -d usr/bin/miracle* usr/lib/*/miracle-wm/* 2>/dev/null \
      | sed -n 's/.*Shared library: \[\(.*\)\].*/\1/p' | sort -u) \
    <(
        {
            find . -printf '%f\n'
            printf '%s\n' \
                ld-linux-x86-64.so.2 \
                ld-linux-aarch64.so.1 \
                ld-linux-armhf.so.3 \
                libc.so.6 \
                libm.so.6 \
                libpthread.so.0 \
                libdl.so.2 \
                librt.so.1 \
                libresolv.so.2 \
                libutil.so.1 \
                libanl.so.1 \
                libnss_files.so.2 \
                libnss_dns.so.2 \
                libgcc_s.so.1 \
                libstdc++.so.6 \
                libglib-2.0.so.0 \
                libgio-2.0.so.0 \
                libgobject-2.0.so.0 \
                libgmodule-2.0.so.0 \
                libxkbcommon.so.0 \
                libpcre2-8.so.0
        } | sort -u
    )
)

if [ -n "$missing" ]; then
    echo "verify-elf-deps: unresolved soname(s); refusing to pack:" >&2
    sed 's/^/  /' <<<"$missing" >&2
    echo "  Update stage-packages in snap/snapcraft.yaml to the matching runtime package." >&2
    exit 1
fi
echo "verify-elf-deps: all project binaries resolve their dependencies inside the snap"
