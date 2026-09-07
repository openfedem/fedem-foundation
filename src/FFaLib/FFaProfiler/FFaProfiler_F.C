// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFaProfiler_F.C
  \brief Fortran wrapper for the FFaProfiler methods.
  \details This file contains the implementation of the following
  Fortran wrappers of the FFaProfiler module:

  - ffaprofilerinterface::ffa_newprofiler
  - ffaprofilerinterface::ffa_starttimer
  - ffaprofilerinterface::ffa_stoptimer
  - ffaprofilerinterface::ffa_reporttimer

  No further documentation is provided here.
  The wrappers are documented in the FFaProfilerInterface.f90 file.
*/

#include "FFaLib/FFaProfiler/FFaProfiler.H"
#include "FFaLib/FFaOS/FFaFortran.H"


namespace
{
  FFaProfiler* myProfiler = NULL;
}


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
