#!/usr/bin/env bash
# Build stock (unmodified) MuPDF for the Wii U using devkitPPC/wut.
#
# Run inside a devkitPPC container that has wut + wiiu portlibs installed.
# Installs headers and static libs to ports/wiiu/deps/prefix.
#
# Everything is configured on the make command line; the MuPDF source is
# never patched:
#   OS=wiiu            any OS other than Linux/MACOS/OpenBSD skips MuPDF's
#                      host probing (host pkg-config, objcopy, pthread)
#   USE_SYSTEM_*=yes   use the portlibs freetype/harfbuzz/libjpeg/zlib, so
#                      SDL2_ttf and MuPDF share one copy (no duplicate symbols)
#   SYS_*_CFLAGS/LIBS  taken from powerpc-eabi-pkg-config
#   HAVE_OBJCOPY=no    embed fonts as generated C (hexdump.sh), not host ld -b binary
#   threading=no       no pthreads on the Wii U
set -euo pipefail

# Pinned release. To try another, set both MUPDF_VERSION and MUPDF_SHA256.
MUPDF_VERSION="${MUPDF_VERSION:-1.25.1}"
MUPDF_SHA256="${MUPDF_SHA256:-81aa1361252418cc45347b4ac075532096957a7ab772e20e046f3bb418d7263c}"
MUPDF_URL=https://mupdf.com/downloads/archive/mupdf-${MUPDF_VERSION}-source.tar.gz

HERE="$(cd "$(dirname "$0")" && pwd)"
DEPS="$HERE/deps"
PREFIX="${MUPDF_PREFIX:-$DEPS/prefix}"
TARBALL="$DEPS/mupdf-${MUPDF_VERSION}-source.tar.gz"
SRC="$DEPS/mupdf-${MUPDF_VERSION}-source"

: "${DEVKITPRO:?DEVKITPRO is not set}"
PKGCONF="$DEVKITPRO/portlibs/wiiu/bin/powerpc-eabi-pkg-config"
MACHDEP="-DESPRESSO -mcpu=750 -meabi -mhard-float"

mkdir -p "$DEPS"
[[ -f "$TARBALL" ]] || curl -fL -o "$TARBALL" "$MUPDF_URL"
echo "$MUPDF_SHA256  $TARBALL" | sha256sum -c -
[[ -d "$SRC" ]] || tar -C "$DEPS" -xzf "$TARBALL"

pc() { "$PKGCONF" "$@"; }

# libc functions newlib lacks (e.g. timegm): declared via a force-included
# header, implemented in libwiiu_compat.a which is installed next to MuPDF.
COMPAT="$HERE/compat"
mkdir -p "$PREFIX/lib" "$DEPS/compat-build"
for c in "$COMPAT"/*.c; do
    powerpc-eabi-gcc $MACHDEP -O2 -ffunction-sections -fdata-sections \
        -c "$c" -o "$DEPS/compat-build/$(basename "${c%.c}").o"
done
rm -f "$PREFIX/lib/libwiiu_compat.a"
powerpc-eabi-ar rcs "$PREFIX/lib/libwiiu_compat.a" "$DEPS"/compat-build/*.o

make -C "$SRC" -j"$(nproc)" \
    OS=wiiu \
    build=release \
    shared=no \
    CC=powerpc-eabi-gcc \
    CXX=powerpc-eabi-g++ \
    AR=powerpc-eabi-ar \
    XCFLAGS="$MACHDEP -include $COMPAT/wiiu_compat.h" \
    HAVE_X11=no \
    HAVE_GLUT=no \
    HAVE_OBJCOPY=no \
    HAVE_LIBCRYPTO=no \
    threading=no \
    USE_SYSTEM_FREETYPE=yes \
    SYS_FREETYPE_CFLAGS="$(pc --cflags freetype2)" \
    SYS_FREETYPE_LIBS="$(pc --libs freetype2)" \
    USE_SYSTEM_HARFBUZZ=yes \
    SYS_HARFBUZZ_CFLAGS="$(pc --cflags harfbuzz)" \
    SYS_HARFBUZZ_LIBS="$(pc --libs harfbuzz)" \
    USE_SYSTEM_LIBJPEG=yes \
    SYS_LIBJPEG_CFLAGS="$(pc --cflags libjpeg)" \
    SYS_LIBJPEG_LIBS="$(pc --libs libjpeg)" \
    USE_SYSTEM_ZLIB=yes \
    SYS_ZLIB_CFLAGS="$(pc --cflags zlib)" \
    SYS_ZLIB_LIBS="$(pc --libs zlib)" \
    prefix="$PREFIX" \
    "$@" \
    install-libs

ls -l "$PREFIX/lib"
