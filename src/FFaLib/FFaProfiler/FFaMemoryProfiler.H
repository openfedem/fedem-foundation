// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFA_MEMORY_PROFILER_H
#define FFA_MEMORY_PROFILER_H

#include <cstddef>


namespace FFaMemoryProfiler
{
  struct MemoryStruct
  {
    size_t myWorkSize     = 0;
    size_t myPeakWorkSize = 0;
    size_t myPageSize     = 0;
    size_t myPeakPageSize = 0;

    void fill();
  };

  void getMemoryUsage(MemoryStruct& reporter);
  void nullifyMemoryUsage(const char* id, bool useCurrent = true);
  void reportMemoryUsage(const char* id);

  //! \brief Returns total and available memory in MBytes for current process.
  void getGlobalMem(unsigned int& total, unsigned int& avail);
}

#endif
