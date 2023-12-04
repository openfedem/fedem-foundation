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


MemoryStruct FFaMemoryProfiler::myBaseMemoryUsage;


#ifdef FT_USE_MEMORY_PROFILER
void FFaMemoryProfiler::nullifyMemoryUsage(const std::string& reportIdentifier,
#else
void FFaMemoryProfiler::nullifyMemoryUsage(const std::string&,
#endif
					   bool useCurrent)
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

#ifdef FT_USE_MEMORY_PROFILER
  printf("%s: Memory profiler baselined\n",reportIdentifier.c_str());
#endif
}


#ifdef FT_USE_MEMORY_PROFILER
void FFaMemoryProfiler::reportMemoryUsage(const std::string& reportIdentifier)
{
  MemoryStruct reporter;
  FFaMemoryProfiler::getMemoryUsage(reporter);

  printf("%-40s Tot:%10.3f, PTot:%10.3f, PWork:%10.3f, PPage:%10.3f [MB]\n",
	 reportIdentifier.c_str(),
	 (reporter.myWorkSize + reporter.myPageSize) / 1048576.0f,
	 (reporter.myPeakWorkSize + reporter.myPeakPageSize) / 1048576.0f,
	 (reporter.myPeakWorkSize) / 1048576.0f,
	 (reporter.myPeakPageSize) / 1048576.0f);
}
#else
void FFaMemoryProfiler::reportMemoryUsage(const std::string&) {}
#endif


void FFaMemoryProfiler::getMemoryUsage(MemoryStruct& reporter)
{
  reporter.fill();
  reporter.myWorkSize     -= myBaseMemoryUsage.myWorkSize;
  reporter.myPeakWorkSize -= myBaseMemoryUsage.myPeakWorkSize;
  reporter.myPageSize     -= myBaseMemoryUsage.myPageSize;
  reporter.myPeakPageSize -= myBaseMemoryUsage.myPeakPageSize;
}


void MemoryStruct::fill()
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


/*!
  Returns the total and available memory in MBytes for this process.
*/

void FFaMemoryProfiler::getGlobalMem(unsigned int& total, unsigned int& avail)
{
  total = avail = 0;
#if _MSC_VER > 1300
  MEMORYSTATUSEX statex;
  DWORDLONG MByte = 1048576;
  statex.dwLength = sizeof(statex);
  GlobalMemoryStatusEx(&statex);
  total = (unsigned int)(statex.ullTotalPhys/MByte);
  avail = (unsigned int)(statex.ullAvailPhys/MByte);
#endif
}
