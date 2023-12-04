// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFaLib/FFaProfiler/FFaProfiler.H"
#include "FFaLib/FFaOS/FFaFortran.H"


static FFaProfiler* myProfiler = NULL;


SUBROUTINE (ffa_newprofiler,FFA_NEWPROFILER) (const char* name, const int n)
{
  if (!myProfiler) myProfiler = new FFaProfiler(std::string(name,n));
}


SUBROUTINE (ffa_starttimer,FFA_STARTTIMER) (const char* prog, const int n)
{
  if (myProfiler) myProfiler->startTimer(std::string(prog,n));
}


SUBROUTINE (ffa_stoptimer,FFA_STOPTIMER) (const char* prog, const int n)
{
  if (myProfiler) myProfiler->stopTimer(std::string(prog,n));
}


SUBROUTINE (ffa_reporttimer,FFA_REPORTTIMER) ()
{
  if (myProfiler)
  {
    myProfiler->report();
    delete myProfiler;
  }
  myProfiler = NULL;
}


SUBROUTINE (ffa_getmemusage,FFA_GETMEMUSAGE) (float* usage)
{
  MemoryStruct reporter;
  FFaMemoryProfiler::getMemoryUsage(reporter);

  usage[0] = reporter.myWorkSize     / 1048576.0f;
  usage[1] = reporter.myPageSize     / 1048576.0f;
  usage[2] = reporter.myPeakWorkSize / 1048576.0f;
  usage[3] = reporter.myPeakPageSize / 1048576.0f;
}


INTEGER_FUNCTION (ffa_getphysmem,FFA_GETPHYSMEM) (const bool& wantTotal)
{
  unsigned int totalMem, availableMem;
  FFaMemoryProfiler::getGlobalMem(totalMem,availableMem);
  return wantTotal ? totalMem : availableMem;
}
