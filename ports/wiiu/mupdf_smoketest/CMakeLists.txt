# Minimal Wii U check of the stock MuPDF build (no SDL).
# Build MuPDF first: ports/wiiu/build_mupdf.sh
cmake_minimum_required(VERSION 3.13)
project(mupdf_smoketest C)

set(MUPDF_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/../deps/prefix" CACHE PATH "Install prefix of build_mupdf.sh")

find_package(PkgConfig REQUIRED)
pkg_check_modules(MUPDF_DEPS REQUIRED IMPORTED_TARGET freetype2 harfbuzz libjpeg zlib)

dkp_add_embedded_binary_library(sample_pdf ../content/sample.pdf)

add_executable(mupdf_smoketest source/main.c)
target_include_directories(mupdf_smoketest PRIVATE "${MUPDF_PREFIX}/include")
target_link_libraries(mupdf_smoketest PRIVATE
   sample_pdf
   "${MUPDF_PREFIX}/lib/libmupdf.a"
   "${MUPDF_PREFIX}/lib/libmupdf-third.a"
   "${MUPDF_PREFIX}/lib/libwiiu_compat.a"
   PkgConfig::MUPDF_DEPS)

wut_create_rpx(mupdf_smoketest)
