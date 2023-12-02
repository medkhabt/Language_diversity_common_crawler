/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"
#include "tools.h"
#include "times.h"

typedef struct _dayn
{
  char *name;
  int   dayn;
} dayn;

static dayn dnames[] =
{
  { "Sun", 0 },
  { "Mon", 1 },
  { "Tue", 2 },
  { "Wed", 3 },
  { "Thu", 4 },
  { "Fri", 5 },
  { "Sat", 6 },
  { "Sunday", 0 },
  { "Monday", 1 },
  { "Tuesday", 2 },
  { "Wednesday", 3 },
  { "Thursday", 4 },
  { "Friday", 5 },
  { "Saturday", 6 },
};

static dayn mnames[] =
{
  { "Jan", 0 },
  { "Feb", 1 },
  { "Mar", 2 },
  { "Apr", 3 },
  { "May", 4 },
  { "Jun", 5 },
  { "Jul", 6 },
  { "Aug", 7 },
  { "Sep", 8 },
  { "Oct", 9 },
  { "Nov", 10 },
  { "Dec", 11 },
  { "January", 0 },
  { "February", 1 },
  { "March", 2 },
  { "April", 3 },
  { "May", 4 },
  { "Jun", 5 },
  { "July", 6 },
  { "August", 7 },
  { "September", 8 },
  { "October", 9 },
  { "November", 10 },
  { "December", 11 },
};

/**************************/
/* make copy of struct tm */
/**************************/
struct tm *new_tm(const struct tm *t)
{
  struct tm *tn;

  if (!t)
    return NULL;

  tn = _malloc(sizeof(*tn));
  memcpy((void *)tn, (void *)t, sizeof(*tn));
  return tn;
}


/*
 * [i_a] from the UNIX man page, e.g.
 *
 * http://www.scit.wlv.ac.uk/cgi-bin/mansec?3C+mktime
 *
 * The tm_year member must be for year 1901 or later.  Calendar
 * times  before  20:45:52  UTC,  December  13,  1901  or after
 * 03:14:07 UTC,  January 19, 2038 cannot be represented. Port-
 * able  applications  should  not  try  to create dates before
 * 00:00:00 UTC, January 1, 1970 or after 00:00:00 UTC, January
 * 1, 2038.
 *
 * ==>
 *
 * Sanitize the date/time stamp before parsing it into a time_t value
 * By limiting it to the 1/1/1970 .. 31/12/2037 range.
 *
 * WARNING:
 *
 * This code assumes the caller is using tl_mktime() and/or
 * tl_mkgmtime() to process the 'tm' time stamp.
 */
static void sanitize_and_limit_tm(struct tm *tm)
{
  if (tm->tm_year >= 2038)
  {
    tm->tm_year = 2037;
  }
  if (tm->tm_year < 1970)
  {
    tm->tm_year = 1970;
  }
  /* make sure tl_mktime() doesn't barf on arbitrary DST settings: */
  tm->tm_isdst = 0;
#if defined (HAVE_GMTOFF)
  tm->tm_gmtoff = 0;
#endif
}

/************************/
/* scan GMT time string */
/************************/
time_t scntime(const char *timestr, bool_t sanitize)
{
  char *pom = tl_strdup(timestr);
  int i;
  char *p;
  struct tm otm = { 0 };
  time_t rv;
  int ilen;
  bool_t last = 1;

  p = pom;
  p += strspn(p, ", -:");
  ilen = strcspn(p, ", -:");
  if (*(p + ilen))
    *(p + ilen) = 0;
  else
    last = 0;

  if (! * p)
    return 0L;

  for (i = 0; i < NUM_ELEM(dnames); i++)
  {
    if (!strcasecmp(p, dnames[i].name))
    {
      otm.tm_wday = dnames[i].dayn;
      break;
    }
  }
  if (!(i < NUM_ELEM(dnames)))
    return 0L;

#define NEXT_TOKEN                              \
  p += ilen + last;                             \
  p += strspn(p, ", -:");                       \
  ilen = strcspn(p, ", -:");                    \
  if (*(p + ilen))                              \
  {                                             \
    *(p + ilen) = 0;                            \
  }                                             \
  else                                          \
  {                                             \
    last = 0;                                   \
  }

  NEXT_TOKEN;
  otm.tm_mday = _atoi(p);
  if (errno == ERANGE)
  {
    for (i = 0; i < NUM_ELEM(mnames); i++)
    {
      if (!strcasecmp(p, mnames[i].name))
      {
        otm.tm_mon = mnames[i].dayn;
        break;
      }
    }
    if (!(i < NUM_ELEM(mnames)))
      return 0L;

    NEXT_TOKEN;
    otm.tm_mday = _atoi(p);
    if (errno == ERANGE)
      return 0L;

    NEXT_TOKEN;
    otm.tm_hour = _atoi(p);
    if (errno == ERANGE)
      return 0L;

    NEXT_TOKEN;
    otm.tm_min = _atoi(p);
    if (errno == ERANGE)
      return 0L;

    NEXT_TOKEN;
    otm.tm_sec = _atoi(p);
    if (errno == ERANGE)
      return 0L;

    NEXT_TOKEN;
    otm.tm_year = _atoi(p);
    if (errno == ERANGE)
      return 0L;

    otm.tm_year = (otm.tm_year > 1900) ? otm.tm_year - 1900 : otm.tm_year;
  }
  else
  {
    NEXT_TOKEN;
    for (i = 0; i < NUM_ELEM(mnames); i++)
    {
      if (!strcasecmp(p, mnames[i].name))
      {
        otm.tm_mon = mnames[i].dayn;
        break;
      }
    }
    if (!(i < NUM_ELEM(mnames)))
      return 0L;

    NEXT_TOKEN;
    otm.tm_year = _atoi(p);
    if (errno == ERANGE)
      return 0L;

    otm.tm_year = (otm.tm_year > 1900) ? otm.tm_year - 1900 : otm.tm_year;

    NEXT_TOKEN;
    otm.tm_hour = _atoi(p);
    if (errno == ERANGE)
      return 0L;

    NEXT_TOKEN;
    otm.tm_min = _atoi(p);
    if (errno == ERANGE)
      return 0L;

    NEXT_TOKEN;
    otm.tm_sec = _atoi(p);
    if (errno == ERANGE)
      return 0L;
  }

  rv = tl_mkgmtime(&otm);
  if (rv == (time_t)-1 && sanitize)
  {
    sanitize_and_limit_tm(&otm);
    rv = tl_mkgmtime(&otm);
    ASSERT(rv != (time_t)-1);
  }

  _free(pom);
  return rv;
}

/*
 * parse GMT time string in format YYYYMMDDHHmmss
 */
time_t time_ftp_scn(const char *timestr, bool_t sanitize)
{
  char pom[10];
  struct tm ftm = { 0 };
  time_t rv;

  strncpy(pom, timestr, 4);
  *(pom + 4) = 0;
  ftm.tm_year = _atoi(pom) - 1900;

  strncpy(pom, timestr + 4, 2);
  *(pom + 2) = 0;
  ftm.tm_mon = _atoi(pom) - 1;

  strncpy(pom, timestr + 6, 2);
  *(pom + 2) = 0;
  ftm.tm_mday = _atoi(pom);

  strncpy(pom, timestr + 8, 2);
  *(pom + 2) = 0;
  ftm.tm_hour = _atoi(pom);

  strncpy(pom, timestr + 10, 2);
  *(pom + 2) = 0;
  ftm.tm_min = _atoi(pom);

  strncpy(pom, timestr + 12, 2);
  *(pom + 2) = 0;
  ftm.tm_sec = _atoi(pom);

  rv = tl_mkgmtime(&ftm);
  if (rv == (time_t)-1 && sanitize)
  {
    sanitize_and_limit_tm(&ftm);
    rv = tl_mkgmtime(&ftm);
    ASSERT(rv != (time_t)-1);
  }

  return rv;
}

/*
 * Convert the YYYY.MM.DD.HH:MM string to local time.
 */
time_t time_scn_cmd(const char *timestr)
{
  time_t t = 0;
  struct tm ftm = { 0 };

  if (sscanf(timestr, "%d.%d.%d.%d:%d", &ftm.tm_year, &ftm.tm_mon, &ftm.tm_mday, &ftm.tm_hour,
             &ftm.tm_min) == 5)
  {
    ftm.tm_year = ftm.tm_year - 1900;
    ftm.tm_mon -= 1;
    ftm.tm_isdst = -1; /* Autodetect Daylight Saving Time */
    ftm.tm_sec = 0;

    t = tl_mktime(&ftm);
  }

  return t;
}


/*
 * tl_mkgmtime()
 *
 * [i_a] This implementation of a UTC returning mktime() which is
 * tolerant on input values was ripped from the Apple Open Source code
 * at URL:
 *
 * http://www.opensource.apple.com/darwinsource/Current/zip-6/zip/zip/mktime.c
 *
 * Some very minor modifications have been applied.
 */

/*
 * free mktime function
 * Copyright 1988, 1989 by David MacKenzie <djm@ai.mit.edu>
 * and Michael Haertel <mike@ai.mit.edu>
 * Unlimited distribution permitted provided this copyright notice is
 * retained and any functional modifications are prominently identified.
 */

/*
 * Revised 1997 by Christian Spieler:
 * The code was changed to get more conformance with ANSI's (resp. modern
 * UNIX releases) definition for mktime():
 * - Added adjustment for out-of-range values in the fields of struct tm.
 * - Added iterations to get the correct UTC result for input values at
 *   the gaps when daylight saving time is switched on or off.
 * - Allow forcing of DST "on" or DST "off" by setting `tm_isdst' field in
 *   the tm struct to positive number resp. zero. The `tm_isdst' field must
 *   be negative on entrance of mktime() to enable automatic determination
 *   if DST is in effect for the requested local time.
 * - Added optional check for overflowing the time_t range.
 */

/*
 * Note: This version of mktime is ignorant of the tzfile.
 * When the tm structure passed to mktime represents a local time that
 * is valid both as DST time and as standard time (= time value in the
 * gap when switching from DST back to standard time), the behaviour
 * for `tm_isdst < 0' depends on the current timezone: TZ east of GMT
 * assumes winter time, TZ west of GMT assumes summer time.
 * Although mktime() (resp. mkgmtime()) tries to adjust for invalid values
 * of struct tm members, this may fail for input values that are far away
 * from the valid ranges. The adjustment process does not check for overflows
 * or wrap arounds in the struct tm components.
 */


/*
 * Return the equivalent in seconds past 12:00:00 a.m. Jan 1, 1970 GMT
 * of the local time and date in the exploded time structure `tm',
 * adjust out of range fields in `tm' and set `tm->tm_yday', `tm->tm_wday'.
 * If `tm->tm_isdst < 0' was passed to mktime(), the correct setting of
 * tm_isdst is determined and returned. Otherwise, mktime() assumes this
 * field as valid; its information is used when converting local time
 * to UTC.
 *
 * Return -1 if time in `tm' cannot be represented as time_t value.
 */
time_t tl_mktime(struct tm *tm)
{
  struct tm *ltm;               /* Local time. */
  time_t loctime;               /* The time_t value of local time. */
  time_t then;                  /* The time to return. */
  long tzoffset_adj = 0;        /* timezone-adjustment `remainder' */
  int bailout_cnt;              /* counter of tries for tz correction */
  int save_isdst;               /* Copy of the tm->isdst input value */

  save_isdst = tm->tm_isdst;
  loctime = tl_mkgmtime(tm);
  if (loctime == -1)
  {
    tm->tm_isdst = save_isdst;
    return (time_t)-1;
  }

  /*
   * Correct for the timezone and any daylight savings time.
   * The correction is verified and repeated when not correct, to
   * take into account the rare case that a change to or from daylight
   * savings time occurs between when it is the time in `tm' locally
   * and when it is that time in Greenwich. After the second correction,
   * the "timezone & daylight" offset should be correct in all cases. To
   * be sure, we allow a third try, but then the loop is stopped.
   */
  bailout_cnt = 3;
  then = loctime;

  LOCK_TIME;

  do
  {
    ltm = localtime(&then);
    if (ltm == (struct tm *)NULL
        || (tzoffset_adj = loctime - tl_mkgmtime(ltm)) == 0L)
      break;
    then += tzoffset_adj;
  } while (--bailout_cnt > 0);

  if (ltm == (struct tm *)NULL || tzoffset_adj != 0L)
  {
    /* Signal failure if timezone adjustment did not converge. */
    tm->tm_isdst = save_isdst;
    UNLOCK_TIME;
    return (time_t)-1;
  }

  if (save_isdst >= 0)
  {
    if (ltm->tm_isdst && !save_isdst)
    {
      if (then + 3600 < then)
        then = (time_t)-1;
      else
        then += 3600;
    }
    else if (!ltm->tm_isdst && save_isdst)
    {
      if (then - 3600 > then)
        then = (time_t)-1;
      else
        then -= 3600;
    }
    ltm->tm_isdst = save_isdst;
  }

  if (tm != ltm) /* `tm' may already point to localtime's internal storage */
    *tm = *ltm;

  UNLOCK_TIME;

  return then;
}


/*
 * Provide default values for the upper limit of the time_t range.
 * These are the result of the decomposition into a `struct tm' for
 * the time value 0xFFFFFFFEL ( = (time_t)-2 ).
 *
 * Note: `(time_t)-1' is reserved for "invalid time"!
 */
#define TL_TM_YEAR_MAX         2106
#define TL_TM_MON_MAX          1       /* February */
#define TL_TM_MDAY_MAX         7
#define TL_TM_HOUR_MAX         6
#define TL_TM_MIN_MAX          28
#define TL_TM_SEC_MAX          14

/* Adjusts out-of-range values for `tm' field `tm_member'. */
#define TL_ADJUST_TM(tm_member, tm_carry, modulus)               \
  if ((tm_member) < 0)                                           \
  {                                                              \
    tm_carry -= (1 - ((tm_member) + 1) / (modulus));             \
    tm_member = (modulus - 1) + (((tm_member) + 1) % (modulus)); \
  }                                                              \
  else if ((tm_member) >= (modulus))                             \
  {                                                              \
    tm_carry += (tm_member) / (modulus);                         \
    tm_member = (tm_member) % (modulus);                         \
  }

/* Nonzero if `y' is a leap year, else zero. */
#define tl_leap(y)                                               \
  (((y) % 4 == 0 && (y) % 100 != 0) || (y) % 400 == 0)

/* Number of leap years from 1970 to `y' (not including `y' itself). */
#define tl_nleap(y)                                              \
  (((y) - 1969) / 4 - ((y) - 1901) / 100 + ((y) - 1601) / 400)

/* Additional leapday in February of leap years. */
#define tl_leapday(m, y)                                         \
  ((m) == 1 && tl_leap(y))

/* Length of month `m' (0 .. 11) */
#define tl_monthlen(m, y)                                        \
  (tl_ydays[(m) + 1] - tl_ydays[m] + tl_leapday(m, y))

/* Accumulated number of days from 01-Jan up to start of current month. */
static const short tl_ydays[] =
{
  0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365
};


/*
 * Return the equivalent in seconds past 12:00:00 a.m. Jan 1, 1970 GMT
 * of the Greenwich Mean time and date in the exploded time structure `tm'.
 * This function does always put back normalized values into the `tm' struct,
 * parameter, including the calculated numbers for `tm->tm_yday',
 * `tm->tm_wday', and `tm->tm_isdst'.
 *
 * Returns -1 if the time in the `tm' parameter cannot be represented
 * as valid `time_t' number.
 */
time_t tl_mkgmtime(struct tm *tm)
{
  int years, months, days, hours, minutes, seconds;

  years = tm->tm_year + 1900;   /* year - 1900 -> year */
  months = tm->tm_mon;          /* 0..11 */
  days = tm->tm_mday - 1;       /* 1..31 -> 0..30 */
  hours = tm->tm_hour;          /* 0..23 */
  minutes = tm->tm_min;         /* 0..59 */
  seconds = tm->tm_sec;         /* 0..61 in ANSI C. */

  TL_ADJUST_TM(seconds, minutes, 60);
  TL_ADJUST_TM(minutes, hours, 60);
  TL_ADJUST_TM(hours, days, 24);
  TL_ADJUST_TM(months, years, 12);
  ASSERT(months >= 0);
  ASSERT(months < 12);
  if (days < 0)
  {
    do
    {
      if (--months < 0)
      {
        --years;
        months = 11;
      }
      days += tl_monthlen(months, years);
    } while (days < 0);
  }
  else
  {
    while (days >= tl_monthlen(months, years))
    {
      days -= tl_monthlen(months, years);
      if (++months >= 12)
      {
        ++years;
        months = 0;
      }
    }
  }

  ASSERT(days >= 0);
  ASSERT(days < 31);
  ASSERT(months >= 0);
  ASSERT(months < 12);
  ASSERT(hours >= 0);
  ASSERT(hours < 24);
  ASSERT(minutes >= 0);
  ASSERT(minutes < 60);
  ASSERT(seconds >= 0);
  ASSERT(seconds < 60); /* we don't care about the 'leap second' in this code! */

  /* Restore adjusted values in tm structure */
  tm->tm_year = years - 1900;
  tm->tm_mon = months;
  tm->tm_mday = days + 1;
  tm->tm_hour = hours;
  tm->tm_min = minutes;
  tm->tm_sec = seconds;

  /* Set `days' to the number of days into the year. */
  days += tl_ydays[months] + (months > 1 && tl_leap(years));
  tm->tm_yday = days;

  /* Now calculate `days' to the number of days since Jan 1, 1970. */
  days = (unsigned)days + 365 * (unsigned)(years - 1970) +
         (unsigned)(tl_nleap(years));
  tm->tm_wday = ((unsigned)days + 4) % 7; /* Jan 1, 1970 was Thursday. */
  tm->tm_isdst = 0;

  if (years < 1970)
    return (time_t)-1;

  if (years > TL_TM_YEAR_MAX
      || (years == TL_TM_YEAR_MAX
          && (tm->tm_yday > tl_ydays[TL_TM_MON_MAX] + (TL_TM_MDAY_MAX - 1) +
              (TL_TM_MON_MAX > 1 && tl_leap(TL_TM_YEAR_MAX))
              || (tm->tm_yday == tl_ydays[TL_TM_MON_MAX] + (TL_TM_MDAY_MAX - 1) +
                  (TL_TM_MON_MAX > 1 && tl_leap(TL_TM_YEAR_MAX))
                  && (hours > TL_TM_HOUR_MAX
                      || (hours == TL_TM_HOUR_MAX
                          && (minutes > TL_TM_MIN_MAX
                              || (minutes == TL_TM_MIN_MAX && seconds > TL_TM_SEC_MAX))))))))
  {
    return (time_t)-1;
  }

  return (time_t)(86400L * (unsigned long)(unsigned)days
                  + 3600L * (unsigned long)hours
                  + (unsigned long)(60 * minutes + seconds));
}

