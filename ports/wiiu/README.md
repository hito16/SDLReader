# SDLReader for Wii U

Minimal Wii U port of SDLReader, built with devkitPPC + wut against an
**unmodified** upstream MuPDF release.

## Status

* Builds with stock MuPDF 1.25.1 (same version as Debian 13's `libmupdf-dev`).
  1.28.5 (latest at the time of writing) also builds and runs; see below.
* Runs in Cemu: opens a PDF, renders it, page/scale overlay via SDL2_ttf.
* Not yet tested on real hardware.
* GamePad: D-pad left/right = page, up/down = zoom, sticks = scroll,
  A = reset view, HOME = exit.

## Building

Needs a devkitPro container with `wut`, `wiiu-sdl2`, `wiiu-sdl2_ttf` and the
ppc portlibs (`ppc-freetype`, `ppc-harfbuzz`, `ppc-libjpeg-turbo`, `ppc-zlib`),
e.g. `devkitpro/devkitppc` plus `cmake`.

```sh
cd ports/wiiu
./build_mupdf.sh                              # once: downloads + builds MuPDF into deps/prefix
powerpc-eabi-cmake -S . -B build -G Ninja
cmake --build build                           # -> build/SDLReader.rpx, build/SDLReader.wuhb
```

Use the `.wuhb`: it carries the font and a sample PDF in `/vol/content`.

## Where it looks for PDFs

The first folder that contains a `.pdf` wins (alphabetically first file):

1. `sd:/wiiu/apps/SDLReader/`
2. `sd:/pdf/`
3. the bundled `content/sample.pdf`

## How MuPDF is built without patching it

`build_mupdf.sh` only passes variables to MuPDF's own Makefile:

| Setting | Why |
|---|---|
| `OS=wiiu` | Any OS other than Linux/MACOS/OpenBSD skips MuPDF's host probing (host `pkg-config`, objcopy, pthread). |
| `USE_SYSTEM_{FREETYPE,HARFBUZZ,LIBJPEG,ZLIB}=yes` | Use the portlibs copies, which SDL2_ttf also uses. One copy of each library means no duplicate symbols. This is what Linux distros do. |
| `SYS_*_CFLAGS/LIBS` from `powerpc-eabi-pkg-config` | Point MuPDF at the Wii U portlibs. |
| `HAVE_OBJCOPY=no` | Fonts are embedded as generated C, not via the host `ld -b binary`. |
| `threading=no`, `HAVE_X11/GLUT/LIBCRYPTO=no` | Not available on the Wii U. |
| `XCFLAGS="-mcpu=750 ... -include compat/wiiu_compat.h"` | Espresso CPU flags, plus declarations for libc functions newlib lacks. |

The remaining bundled libraries (openjpeg, jbig2dec, lcms2mt, mujs, gumbo,
extract) go into `libmupdf-third.a`; nothing else in the app contains them.

### Compatibility shims (`compat/`)

`compat/wiiu_compat.h` is force-included into every MuPDF source file, and
`compat/*.c` builds into `deps/prefix/lib/libwiiu_compat.a`:

| Problem | Fix |
|---|---|
| newlib has no `timegm` | declared in the header, implemented in `timegm.c` |
| newlib's `<sys/types.h>` has `#define quad quad_t` (BSD compat), which renames `fz_stext_char.quad` in files that include it after the MuPDF headers. This hits `output-csv.c` in MuPDF 1.26+. | the header includes `<sys/types.h>` first, then `#undef quad` |
| newlib's `getentropy()` needs `_getentropy_r`, which wut doesn't provide. MuPDF 1.26+ calls it. | `getentropy.c` returns `ENOSYS`, so MuPDF falls back to its time-seeded generator (used only when writing or encrypting PDFs) |

### Using a different MuPDF version

```sh
MUPDF_VERSION=1.28.5 \
MUPDF_SHA256=98a5c10cda20c3992cdf76ff6b2a1149c32bd79cc796d3f703230b1185b7e934 \
MUPDF_PREFIX=$PWD/deps/prefix-1.28.5 ./build_mupdf.sh
powerpc-eabi-cmake -S . -B build-1.28.5 -DMUPDF_PREFIX=$PWD/deps/prefix-1.28.5
```

## MuPDF smoke test

`mupdf_smoketest/` is a tiny wut app with no SDL. It renders the embedded
sample PDF and logs the result through WHBLog. Use it to check a new MuPDF
build independently of the UI.

## Cemu: use `--force-interpreter`

Cemu 2.6's PPC recompiler crashes some builds of this app inside MuPDF's PDF
object code (`pdf_dict_get_val`, bad pointer `0x00800000`). Whether it
happens depends on the exact machine code, so unrelated changes can make it
appear or disappear. Every build that crashed ran correctly with
`CEMU_EXTRA_ARGS=--force-interpreter` (rendering is roughly 2x slower). If a
build crashes in Cemu, try the interpreter before debugging the app.

## Logging

stdout/stderr go to WHBLog: `OSReport` (Cemu `log.txt`) and UDP (run
`udplogserver` on your PC when using real hardware).

## Binary size

The `.rpx` is about 30 MB, mostly MuPDF's built-in fonts. You can pass extra
flags to `build_mupdf.sh` to shrink it, e.g. `./build_mupdf.sh XCFLAGS="... -DTOFU_CJK"`.
The cost is that PDFs that don't embed their own CJK fonts will show missing
glyphs. (`XCFLAGS` replaces the script's value, so repeat the CPU and compat flags.)
