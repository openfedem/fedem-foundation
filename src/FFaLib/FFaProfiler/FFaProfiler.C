// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFaLib/FFaProfiler/FFaProfiler.H"
#include <cstring>
#include <cstdio>
#include <time.h>
#if defined(win32) || defined(win64)
#include <sys/types.h>
#include <sys/timeb.h>
#else
#include <sys/time.h>
#endif


/*!
  \class FFaProfiler FFaProfiler.H
  \brief Small profiler class

  Use this class to time stuff. The management overhead is approx 0.015 ms
  (wall time on a SGI O2) for the start and stop operations.
  Multiple stop watches can be active in the same profiler object.

  \code
  FFaProfiler aProf("Test profiler");
  aProf.startTimer("foo()");
  foo();
  aProf.stopTimer("foo()");
  aProf.report();
  \endcode

  \author Jens Lien
*/


namespace
{
  unsigned long int WallTime()
  {
#if defined(win32) || defined(win64)
    struct _timeb timebuffer;
    _ftime(&timebuffer);
    return 1000000ul*(unsigned long int)timebuffer.time
      +       1000ul*(unsigned long int)timebuffer.millitm;
#else
    struct timeval currenttime;
    gettimeofday(&currenttime,NULL);
    return 1000000ul*currenttime.tv_sec + currenttime.tv_usec;
#endif
  }

  unsigned long int CPUTime()
  {
    return (unsigned long int)clock();
  }
}


/*!
  Start a named timer. No other initialization is necessary.
*/

void FFaProfiler::startTimer(const std::string& name)
{
  ProfileStruct& prof = myTimers[name];
  if (prof.iAmRunning) return;

  prof.iAmRunning = true;
  prof.myWallTime.myLastStartTime = WallTime();
  prof.myCPUTime.myLastStartTime  = CPUTime();
}


/*!
  Stop the named timer.
*/

void FFaProfiler::stopTimer(const std::string& name)
{
  unsigned long int tmpCPUSlice = CPUTime();
  unsigned long int tmpSlice    = WallTime();

  TimerMap::iterator it = myTimers.find(name);
  if (it == myTimers.end())
  {
    fprintf(stderr,"Mo matching timer for %s\n",name.c_str());
    return;
  }

  ProfileStruct& prof = it->second;
  if (!prof.iAmRunning) return;

  prof.iAmRunning = false;
  prof.myInvocations++;

  // Wall time calculations
  unsigned long int timeSlice = tmpSlice - prof.myWallTime.myLastStartTime;
  if (prof.myInvocations == 1)
  {
    prof.myWallTime.totalTime = timeSlice;
    prof.myWallTime.minTime = timeSlice;
    prof.myWallTime.maxTime = timeSlice;
    prof.myWallTime.minInvNo = 1;
    prof.myWallTime.maxInvNo = 1;
  }
  else
  {
    prof.myWallTime.totalTime += timeSlice;
    if (timeSlice < prof.myWallTime.minTime)
    {
      prof.myWallTime.minTime = timeSlice;
      prof.myWallTime.minInvNo = prof.myInvocations;
    }
    if (timeSlice > prof.myWallTime.maxTime)
    {
      prof.myWallTime.maxTime = timeSlice;
      prof.myWallTime.maxInvNo = prof.myInvocations;
    }
  }

  // CPU time calculations
  unsigned long int CPUtimeSlice = tmpCPUSlice - prof.myCPUTime.myLastStartTime;
  if (prof.myInvocations == 1)
  {
    prof.myCPUTime.totalTime = CPUtimeSlice;
    prof.myCPUTime.minTime = CPUtimeSlice;
    prof.myCPUTime.maxTime = CPUtimeSlice;
    prof.myCPUTime.minInvNo = 1;
    prof.myCPUTime.maxInvNo = 1;
  }
  else
  {
    prof.myCPUTime.totalTime += CPUtimeSlice;
    if (CPUtimeSlice < prof.myCPUTime.minTime)
    {
      prof.myCPUTime.minTime = CPUtimeSlice;
      prof.myCPUTime.minInvNo = prof.myInvocations;
    }
    if (CPUtimeSlice > prof.myCPUTime.maxTime)
    {
      prof.myCPUTime.maxTime = CPUtimeSlice;
      prof.myCPUTime.maxInvNo = prof.myInvocations;
    }
  }
}


/*!
  Writes a formatted report to stdout.
*/

void FFaProfiler::report()
{
  if (myTimers.empty()) return;

  printf("\n%-40s\n", myName.c_str());
  printf("--------------------------------------------------+"
	 "-------------------+-------------------+------------------------\n");
  printf("%-20s%5s%12s%12s |%12s %5s |%12s %5s |%12s%12s\n",
	 "Description",
	 "Inv.",
	 "Total [s]",
	 "Avg. [ms]",
	 "Min [ms]",
	 "Inv#",
	 "Max [ms]",
	 "Inv#",
	 "CPUtot [s]",
	 "CPUavg [ms]");

  printf("--------------------------------------------------+"
	 "-------------------+-------------------+------------------------\n");
  char name[21]; name[20] = '\0';
  for (const TimerMap::value_type& timer : myTimers)
    if (timer.second.myInvocations > 0)
    {
      const ProfileStruct& prof = timer.second;
      strncpy(name,timer.first.c_str(),20);
      printf("%-20s%5ld%12.3f%12.3f |%12.3f%6ld |%12.3f%6ld |%12.3f%12.3f\n",
             name, prof.myInvocations,
	     (float)prof.myWallTime.totalTime/1000000.0f,
	     (float)prof.myWallTime.totalTime/(float)(prof.myInvocations*1000),
	     (float)prof.myWallTime.minTime/1000.0f,
	     prof.myWallTime.minInvNo,
	     (float)prof.myWallTime.maxTime/1000.0f,
	     prof.myWallTime.maxInvNo,
	     (float)prof.myCPUTime.totalTime/(float)CLOCKS_PER_SEC,
	     (float)prof.myCPUTime.totalTime/(float)(prof.myInvocations*CLOCKS_PER_SEC/1000));
    }

  printf("--------------------------------------------------+"
	 "-------------------+-------------------+------------------------\n");
}
