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


class FFaProfiler
{
  struct TimeStruct
  {
    unsigned long int myLastStartTime = 0;
    unsigned long int minTime = 0;
    unsigned long int maxTime = 0;
    unsigned long int totalTime = 0;
    unsigned long int maxInvNo = 0;
    unsigned long int minInvNo = 0;
  };

  struct ProfileStruct
  {
    TimeStruct myWallTime;
    TimeStruct myCPUTime;
    bool iAmRunning = false;
    unsigned long int myInvocations = 0;
  };

  using TimerMap = std::map<std::string,ProfileStruct>;

  std::string myName;
  TimerMap    myTimers;

public:
  FFaProfiler(const std::string& name) : myName(name) {}

  void startTimer(const std::string& name);
  void stopTimer(const std::string& name);

  void report();
};

#endif
