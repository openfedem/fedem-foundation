// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFaMemoryProfiler.H"
#include <cstdio>
#if defined(win32) || defined(win64)
#include <windows.h>
#ifdef FT_USE_MEMORY_PROFILER
#include <psapi.h>
#endif
#else
#include <sys/sysinfo.h>
#endif


/*!
  \file FFaMemoryProfiler.C
  \brief Memory profiling utility class.
  \details Info from http://msdn.microsoft.com

  /verbatim
typedef struct _PROCESS_MEMORY_COUNTERS {
    DWORD cb;
    DWORD PageFaultCount;
    SIZE_T PeakWorkingSetSize;
    SIZE_T WorkingSetSize;
    SIZE_T QuotaPeakPagedPoolUsage;
    SIZE_T QuotaPagedPoolUsage;
    SIZE_T QuotaPeakNonPagedPoolUsage;
    SIZE_T QuotaNonPagedPoolUsage;
    SIZE_T PagefileUsage;
    SIZE_T PeakPagefileUsage;
} PROCESS_MEMORY_COUNTERS;

typedef PROCESS_MEMORY_COUNTERS *PPROCESS_MEMORY_COUNTERS;
  /endverbatim

Members
-------
- cb                         The size of the structure, in bytes.
- PageFaultCount             The number of page faults.
- PeakWorkingSetSize         The peak working set size.
- WorkingSetSize             The current working set size.
- QuotaPeakPagedPoolUsage    The peak paged pool usage.
- QuotaPagedPoolUsage        The current paged pool usage.
- QuotaPeakNonPagedPoolUsage The peak nonpaged pool usage.
- QuotaNonPagedPoolUsage     The current nonpaged pool usage.
- PagefileUsage              The current pagefile usage.
- PeakPagefileUsage          The peak pagefile usage.
*/


namespace
{
  FFaMemoryProfiler::MemoryStruct myBaseMemoryUsage;
}


#ifdef FT_USE_MEMORY_PROFILER
void FFaMemoryProfiler::nullifyMemoryUsage(const char* id, bool useCurrent)
{
  if (useCurrent)
    myBaseMemoryUsage.fill();
  else
  {
    myBaseMemoryUsage.myWorkSize     = 0;
    myBaseMemoryUsage.myPeakWorkSize = 0;
    myBaseMemoryUsage.myPageSize     = 0;
    myBaseMemoryUsage.myPeakPageSize = 0;
  }

  printf("%s: Memory profiler baselined\n",id);
}
#else
void FFaMemoryProfiler::nullifyMemoryUsage(const char*, bool) {}
#endif


#ifdef FT_USE_MEMORY_PROFILER
void FFaMemoryProfiler::reportMemoryUsage(const char* id)
{
  MemoryStruct reporter;
  FFaMemoryProfiler::getMemoryUsage(reporter);
  if (reporter.myWorkSize + reporter.myPageSize < 512) return;

  // Lambda function converting to MBytes.
  auto toMB = [](size_t nb) { return static_cast<float>(nb)/1048576.0f; };

  printf("%-40s Tot:%10.3f, PTot:%10.3f, PWork:%10.3f, PPage:%10.3f [MB]\n", id,
         toMB(reporter.myWorkSize + reporter.myPageSize),
         toMB(reporter.myPeakWorkSize + reporter.myPeakPageSize),
         toMB(reporter.myPeakWorkSize),
         toMB(reporter.myPeakPageSize));
}
#else
void FFaMemoryProfiler::reportMemoryUsage(const char*) {}
#endif


void FFaMemoryProfiler::getMemoryUsage(MemoryStruct& reporter)
{
  reporter.fill();
  reporter.myWorkSize     -= myBaseMemoryUsage.myWorkSize;
  reporter.myPeakWorkSize -= myBaseMemoryUsage.myPeakWorkSize;
  reporter.myPageSize     -= myBaseMemoryUsage.myPageSize;
  reporter.myPeakPageSize -= myBaseMemoryUsage.myPeakPageSize;
}


void FFaMemoryProfiler::MemoryStruct::fill()
{
#ifdef FT_USE_MEMORY_PROFILER
#if defined(win32) || defined(win64)
  PROCESS_MEMORY_COUNTERS pmc;
  if (!GetProcessMemoryInfo(GetCurrentProcess(),&pmc,sizeof(pmc)))
    fprintf(stderr,"Failed GetProcessMemoryInfo\n");

  myWorkSize     = pmc.WorkingSetSize;
  myPeakWorkSize = pmc.PeakWorkingSetSize;
  myPageSize     = pmc.PagefileUsage;
  myPeakPageSize = pmc.PeakPagefileUsage;
#endif
#endif
}


void FFaMemoryProfiler::getGlobalMem(unsigned int& total, unsigned int& avail)
{
  total = avail = 0u;
#if defined(win32) || defined(win64)
  MEMORYSTATUSEX statex;
  statex.dwLength = sizeof(statex);
  if (GlobalMemoryStatusEx(&statex))
  {
    const DWORDLONG MByte = 1048576ULL;
    total = static_cast<unsigned int>(statex.ullTotalPhys/MByte);
    avail = static_cast<unsigned int>(statex.ullAvailPhys/MByte);
  }
#else
  struct sysinfo info;
  if (sysinfo(&info) == 0)
  {
    const unsigned long int MByte = 1048576ul;
    total = static_cast<unsigned int>(info.totalram*info.mem_unit/MByte);
    avail = static_cast<unsigned int>(info.freeram*info.mem_unit/MByte);
  }
#endif
}
