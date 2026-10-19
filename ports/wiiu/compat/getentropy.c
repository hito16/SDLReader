#include <errno.h>
#include <stddef.h>
#include <unistd.h>

/*
 * newlib declares getentropy() but its syscall hook (_getentropy_r) is not
 * provided by wut, so linking fails. Report "not supported" instead; MuPDF
 * (1.26+) then falls back to seeding from gettimeofday(). Only PDF
 * writing/encryption uses this randomness.
 */
int getentropy(void *buf, size_t len)
{
    (void)buf;
    (void)len;
    errno = ENOSYS;
    return -1;
}
