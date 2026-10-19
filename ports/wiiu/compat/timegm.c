#include "wiiu_compat.h"

/* Inverse of gmtime: struct tm (UTC) -> seconds since the epoch. */
time_t timegm(struct tm *tm)
{
    long long year = tm->tm_year + 1900LL;
    long long mon = tm->tm_mon;

    /* normalise month into 0..11 */
    year += mon / 12;
    mon %= 12;
    if (mon < 0) { mon += 12; year--; }

    /* days from 1970-01-01 to year-mon-01 (Howard Hinnant's days_from_civil) */
    long long y = year - (mon < 2);
    long long era = (y >= 0 ? y : y - 399) / 400;
    long long yoe = y - era * 400;
    long long m = (mon + 10) % 12;              /* March = 0 */
    long long doy = (153 * m + 2) / 5;
    long long doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    long long days = era * 146097 + doe - 719468 + (tm->tm_mday - 1);

    return (time_t)(days * 86400 + tm->tm_hour * 3600LL + tm->tm_min * 60LL + tm->tm_sec);
}
