// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFA_MEMORY_PROFILER_H
#define FFA_MEMORY_PROFILER_H

#include <string>


struct MemoryStruct
{
  size_t myWorkSize;
  size_t myPeakWorkSize;
  size_t myPageSize;
  size_t myPeakPageSize;

  MemoryStruct() : myWorkSize(0), myPeakWorkSize(0),
		   myPageSize(0), myPeakPageSize(0) {}
  void fill();
};


class FFaMemoryProfiler
{
public:
  static void getGlobalMem(unsigned int& total, unsigned int& avail);
  static void getMemoryUsage(MemoryStruct& reporter);
  static void reportMemoryUsage(const std::string& reportIdentifier);
  static void nullifyMemoryUsage(const std::string& reportIdentifier,
                                 bool useCurrent = true);

private:
  static MemoryStruct myBaseMemoryUsage;
};

#endif
