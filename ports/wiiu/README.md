# SDLReader for Wii U

Minimal Wii U port of SDLReader, built with devkitPPC + wut against an
**unmodified** upstream MuPDF release.

## Status

* Builds with stock MuPDF 1.25.1 and 
  1.28.5 (latest at the time of writing)

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

## Where WiiU app looks for PDFs

The first folder that contains a `.pdf` wins (alphabetically first file):

1. `sd:/wiiu/apps/SDLReader/`
2. `sd:/pdf/`
3. the bundled `content/sample.pdf`

### Compatibility shims (`compat/`)

`compat/wiiu_compat.h` is force-included into every MuPDF source file, and
`compat/*.c` builds into `deps/prefix/lib/libwiiu_compat.a`:
