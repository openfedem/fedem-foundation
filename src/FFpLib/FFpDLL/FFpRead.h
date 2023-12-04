// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFP_READ_H
#define FFP_READ_H

#if defined(win32) || defined(win64)
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT
#endif


extern "C"
{
  DLL_EXPORT bool FFpReadInit(const char* fileNames);
  DLL_EXPORT void FFpReadDone();

  DLL_EXPORT int FFpReadHistories(const char* objType, const char* baseID,
                                  const char* var,
                                  double* startTime, double* endTime,
                                  bool readTime);
  DLL_EXPORT int FFpReadHistory(const char* vars,
                                double* startTime, double* endTime);

  DLL_EXPORT bool FFpGetReadData(double* data, int* ncol, int* nrow);

}

#endif
