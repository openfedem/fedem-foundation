// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFaLib/FFaProfiler/FFaMemoryProfiler.H"
#include "FFaLib/FFaOS/FFaFortran.H"


SUBROUTINE (ffa_getmemusage,FFA_GETMEMUSAGE) (float* usage)
{
  FFaMemoryProfiler::MemoryStruct reporter;
  FFaMemoryProfiler::getMemoryUsage(reporter);

  const float MByte = 1048576.0f;
  usage[0] = static_cast<float>(reporter.myWorkSize)     / MByte;
  usage[1] = static_cast<float>(reporter.myPageSize)     / MByte;
  usage[2] = static_cast<float>(reporter.myPeakWorkSize) / MByte;
  usage[3] = static_cast<float>(reporter.myPeakPageSize) / MByte;
}


INTEGER_FUNCTION (ffa_getphysmem,FFA_GETPHYSMEM) (const bool& wantTotal)
{
  unsigned int totalMem, availableMem;
  FFaMemoryProfiler::getGlobalMem(totalMem,availableMem);
  return static_cast<int>(wantTotal ? totalMem : availableMem);
}
