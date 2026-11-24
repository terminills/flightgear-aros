#include "fgaros.h"

/* days from 1970-01-01 to the first day of the given month of the proleptic Gregorian calendar */
static long long days_from_civil(long long y, int m)
{
    long long era, yoe, doy, doe;

    y -= m <= 2;
    era = (y >= 0 ? y : y - 399) / 400;
    yoe = y - era * 400;
    doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5;
    doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + doe - 719468;
}

time_t timegm(struct tm *tm)
{
    long long year = 1900LL + tm->tm_year + tm->tm_mon / 12;
    int mon = tm->tm_mon % 12;
    long long days, t;
    time_t r;
    struct tm *n;

    if (mon < 0)
    {
        mon += 12;
        year--;
    }
    days = days_from_civil(year, mon + 1) + tm->tm_mday - 1;
    t = ((days * 24 + tm->tm_hour) * 60 + tm->tm_min) * 60 + tm->tm_sec;
    r = (time_t)t;
    /* as mktime does, hand back the normalised fields */
    if ((n = gmtime(&r)) != NULL)
        *tm = *n;
    return r;
}
