// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFA_PROFILER_H
#define FFA_PROFILER_H

#include <string>
#include <map>

#include "FFaMemoryProfiler.H"


struct TimeStruct
{
  unsigned long myLastStartTime;
  unsigned long minTime, maxTime, totalTime;
  unsigned long maxInvNo, minInvNo;
};

struct ProfileStruct
{
  bool          iAmRunning;
  TimeStruct    myWallTime;
  TimeStruct    myCPUTime;
  unsigned long myInvocations;

  ProfileStruct() : iAmRunning(false), myInvocations(0) {}
};

typedef std::map<std::string,ProfileStruct> TimerMap;
typedef TimerMap::iterator                  TimerMapIter;


class FFaProfiler
{
public:
  FFaProfiler(const std::string& profilerName, bool reportOnDestruct = false);
  virtual ~FFaProfiler();

  void startTimer(const std::string& timerName);
  void stopTimer(const std::string& timerName);

  void report();

protected:
  static unsigned long WallTime();
  static unsigned long CPUTime();

private:
  TimerMap    myTimers;
  std::string myProfilerName;
  bool        reportOnDestruct;
};

#endif
