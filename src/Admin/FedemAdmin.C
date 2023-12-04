// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FedemAdmin.H"
#include <string.h>
#include <stdio.h>
#include <time.h>


static const char* fedem_version =
#include "version.h"
;

static const char* build_date =
#include "build_date.h"
;

static int build_no =
#include "build_no.h"
;


const char* FedemAdmin::getVersion ()
{
  static char full_version[32];
  snprintf(full_version,32,"%s (build %d)",fedem_version,build_no);
  return full_version;
}

const char* FedemAdmin::getBuildDate ()
{
  return build_date;
}


const char* FedemAdmin::getBuildYear ()
{
  static char year[5]; year[4] = '\0';
  return strncpy(year,build_date+7,4);
}


const char* FedemAdmin::getCopyrightString ()
{
  static char copyright[30] = "Copyright 2016 - 2000  SAP SE";
  strncpy(copyright+17,build_date+7,4);
  return copyright;
}


/*!
  Count the number of days since the program was built.
*/

int FedemAdmin::getDaysSinceBuilt ()
{
#include "build_date_C.h"
  // Workaround: PowerShell get-date -uformat %j does not return the
  // leading zero(es) for less than 100 days in the way unix `date +"%j"` does
  if (build_day < 100)
    build_day += 990;
  else if (build_day < 1000)
    build_day += 900;

  time_t currTime = time((time_t*)0);
  struct tm* curr = localtime(&currTime);
  return curr->tm_yday + 1001-build_day + 365*(curr->tm_year-build_year);
}


int FedemAdmin::getExpireAfter ()
{
#ifdef FT_EXPIRE
  return FT_EXPIRE;
#else
  return -1;
#endif
}


bool FedemAdmin::is64bit ()
{
#ifdef FT_IS64BIT
  return true;
#else
  return false;
#endif
}
