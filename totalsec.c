#ifdef __WIN32__

#include <time.h>

/*------------------------------------------------------------------------*/

int _isDST (unsigned hour, unsigned yday, unsigned month, unsigned year);

extern char _Days[12];
extern int _YDays[13];

/*------------------------------------------------------------------------*
Name            _totalsec - convert a date to seconds since 1970

Usage           #include <_io.h>
                unsigned long _totalsec
                    (int year, int month, int day, int hour, int min, int sec);

Protype         _io.h

Description     Converts a broken-down date to the number of seconds elapsed
                since year 70 (1970) in calendar time.  All arguments are
                zero based, except for the year, which is the number of years
                since 1900.

Return value    The time in seconds since 1/1/1970 GMT.

*---------------------------------------------------------------------------*/

unsigned long _totalsec
    (int year, int month, int day, int hour, int min, int sec)
{
    int leaps;
    time_t days, secs;

    if (year < 70 || year > 138)
        return ((time_t) -1);

    min += sec / 60;
    sec %= 60;              /* Seconds are normalized */
    hour += min / 60;
    min %= 60;              /* Minutes are normalized */
    day += hour / 24;
    hour %= 24;             /* Hours are normalized   */

    year += month / 12;     /* Normalize month (not necessarily final) */
    month %= 12;

    while (day >= _Days[month])
    {
        if (!(year & 3) && (month ==1))
        {
            if (day > 28)
            {
                day -= 29;
                month++;
            }
            else
                break;
        }
        else
        {
            day -= _Days[month];
            month++;
        }

        if (month >= 12)    /* Normalize month */
        {
            month -= 12;
            year++;
        }
    }

    year -= 70;
    leaps = (year + 2) / 4;

    if (!((year+70) & 3) && (month < 2))
        --leaps;

    days = year*365L + leaps + _YDays[month] + day;

    secs = days*86400L + hour*3600L + min*60L + sec + _timezone;

    if (_daylight && _isDST(hour, day, month+1, year))
        secs -= 3600;

    return(secs > 0 ? secs : (time_t) -1);
}
/*------------------------------------------------------------------------*/
#endif
