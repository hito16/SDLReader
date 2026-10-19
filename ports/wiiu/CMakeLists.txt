# SDLReader for Wii U (wut).
#
#   ./build_mupdf.sh                         # once: stock MuPDF -> deps/prefix
#   powerpc-eabi-cmake -S . -B build && cmake --build build
#
# Produces build/SDLReader.rpx and build/SDLReader.wuhb.
cmake_minimum_required(VERSION 3.13)
project(SDLReader C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SDLREADER_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")
set(MUPDF_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/deps/prefix" CACHE PATH "Install prefix of build_mupdf.sh")
if(NOT EXISTS "${MUPDF_PREFIX}/lib/libmupdf.a")
   message(FATAL_ERROR "MuPDF not found in ${MUPDF_PREFIX}; run ports/wiiu/build_mupdf.sh first")
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL REQUIRED IMPORTED_TARGET sdl2 SDL2_ttf)
# MuPDF was built against these portlibs (USE_SYSTEM_*), shared with SDL2_ttf
pkg_check_modules(MUPDF_DEPS REQUIRED IMPORTED_TARGET freetype2 harfbuzz libjpeg zlib)

file(GLOB CORE_SOURCES "${SDLREADER_ROOT}/src/*.cpp")

add_executable(SDLReader main_wiiu.cpp ${CORE_SOURCES})
target_include_directories(SDLReader PRIVATE
   "${SDLREADER_ROOT}/include"
   "${MUPDF_PREFIX}/include")
target_link_libraries(SDLReader PRIVATE
   "${MUPDF_PREFIX}/lib/libmupdf.a"
   "${MUPDF_PREFIX}/lib/libmupdf-third.a"
   "${MUPDF_PREFIX}/lib/libwiiu_compat.a"
   PkgConfig::SDL
   PkgConfig::MUPDF_DEPS)

wut_create_rpx(SDLReader)

# Files bundled into the .wuhb, readable at fs:/vol/content/
set(CONTENT_DIR "${CMAKE_CURRENT_BINARY_DIR}/content")
file(MAKE_DIRECTORY "${CONTENT_DIR}")
configure_file("${SDLREADER_ROOT}/res/Roboto-Regular.ttf" "${CONTENT_DIR}/Roboto-Regular.ttf" COPYONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/content/sample.pdf" "${CONTENT_DIR}/sample.pdf" COPYONLY)

wut_create_wuhb(SDLReader
   CONTENT "${CONTENT_DIR}"
   NAME "SDLReader"
   SHORTNAME "SDLReader"
   AUTHOR "hito16")
