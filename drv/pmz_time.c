#include <stdio.h>
#include <string.h>

#include "peripheral/rtcc/plib_rtcc.h"

#include "drv_mcp7940x.h"
#include "log.h"
#include "pmz_time.h"

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Constants                                                         */
/* ************************************************************************** */
/* ************************************************************************** */

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Data types                                                        */
/* ************************************************************************** */
/* ************************************************************************** */

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: File Scope or Global Data                                         */
/* ************************************************************************** */
/* ************************************************************************** */
/* struct tm info */
// Member	Type	Meaning	Range //
// tm_sec	int     seconds after the minute    0-61*
// tm_min	int     minutes after the hour      0-59
// tm_hour	int     hours since midnight        0-23
// tm_mday	int     day of the month            1-31
// tm_mon	int     months since January        0-11
// tm_year	int     years since 1900	
// tm_wday	int     days since Sunday           0-6
// tm_yday	int     days since January 1        0-365
// tm_isdst	int     Daylight Saving Time flag	
// __tm_gmtoff  long            //Aparece en el XC32 4.0
// __tm_zone    const char *    //Aparece en el XC32 4.0
static struct tm utc_t;
static time_t utc_ts;
static bool sysTimeIsReliable = false; //By defult system time NOT is reliable

/* ************************************************************************** */
/* ************************************************************************** */
// Section: Local Functions                                                   */
/* ************************************************************************** */

/* ************************************************************************** */
static bool isDst (struct tm *ts) //dayligth saving time
{
  int daysRemainigUntilNextSunday, remainingDaysOfMonth, st;
  bool dst;

  //January, february, november and december are out.
  if (ts->tm_mon < 2 || ts->tm_mon > 9)
    return false;
  //April to September are in
  if (ts->tm_mon > 2 && ts->tm_mon < 9)
    return true;

  //March or October
  daysRemainigUntilNextSunday = (7 - ts->tm_wday);
  remainingDaysOfMonth = 31 - ts->tm_mday; //March & October have 31 days
  dst = false;
  if (daysRemainigUntilNextSunday > remainingDaysOfMonth)
    {
      st = (ts->tm_hour * 60) + ts->tm_min; //For last Sunday test
      if ((ts->tm_wday != 0) || ((ts->tm_wday == 0) && (st >= 60))) //Just last Sunday. On 01:00 UTC
        dst = true;
    }
  //March
  if ((ts->tm_mon == 2) && dst)
    return true;
  //October  
  if ((ts->tm_mon == 9) && !dst)
    return true;

  return false;
}

static void update_external_rtcc (void)
{
  if (mcp7940x_set_ts (pmz_time_get_s ()) == 0)
    {
      _l ("(PMZ TIME) External RTCC updating...\n");
    }
  else
    {
      _l ("(PMZ TIME) External RTCC updating error!\n");
    }
}

/* ************************************************************************** */
/* ************************************************************************** */
// Section: Interface Functions                                               */
/* ************************************************************************** */

/* ************************************************************************** */

void pmz_time_set_s (struct tm *t)
{
  RTCC_TimeSet (t);
  update_external_rtcc ();
}

void pmz_time_set (time_t *ts, bool updateExternalRTCC)
{
  _l ("(PMZ TIME) Current time: %s", ctime (pmz_time_get ()));
  _l ("(PMZ TIME) Setting time: %s\n", ctime (ts));
  gmtime_r (ts, &utc_t);
  RTCC_TimeSet (&utc_t);
  if (updateExternalRTCC) update_external_rtcc ();
  sysTimeIsReliable = true; //If I set the time, I consider it reliable
}

struct tm *pmz_time_get_s (void)
{

  RTCC_TimeGet (&utc_t);

  return &utc_t;
}

time_t *pmz_time_get (void)
{

  RTCC_TimeGet (&utc_t);
  utc_ts = mktime (&utc_t);

  return &utc_ts;
}

bool pmz_time_is_reliable (void)
{
  return sysTimeIsReliable;
}

//Function to convert utc to local time

time_t pmz_time_utc_to_local (time_t utc)
{
  if (isDst (gmtime (&utc)))
    return (utc + 7200); //Apply (GMT +1) + (DST) => GMT+2

  else
    return (utc + 3600); //Apply GMT +1
}

time_t pmz_time_local_to_utc (time_t local)
{
  if (isDst (gmtime (&local)))
    return (local - 7200); //Apply (GMT +1) + (DST) => GMT+2
  else
    return (local - 3600); //Apply GMT +1
}

/* ************************************************************** End of File */