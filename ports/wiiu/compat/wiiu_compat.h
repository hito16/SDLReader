/*
 * Declarations for libc functions MuPDF expects but newlib (devkitPPC)
 * does not provide. Force-included into the MuPDF build with
 * XCFLAGS="-include .../wiiu_compat.h"; implementations live in
 * libwiiu_compat.a. The MuPDF source itself is not modified.
 */
#ifndef WIIU_COMPAT_H
#define WIIU_COMPAT_H

#include <time.h>

/*
 * newlib's <sys/types.h> does "#define quad quad_t" (BSD compat), which
 * renames MuPDF's fz_stext_char.quad member in any file that includes
 * sys/types.h (e.g. via zlib.h) after the MuPDF headers. Include it first,
 * then drop the macro; the include guard keeps it from coming back.
 */
#include <sys/types.h>
#undef quad

#ifdef __cplusplus
extern "C" {
#endif

time_t timegm(struct tm *tm);

#ifdef __cplusplus
}
#endif

#endif /* WIIU_COMPAT_H */
