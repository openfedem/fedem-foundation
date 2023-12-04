// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFA_FUNCTION_H
#define FFA_FUNCTION_H

#if defined(win32) || defined(win64)
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT
#endif


extern "C"
{
  DLL_EXPORT int FFaFunctionInit(const double* data, int ndata,
                                 int funcId, int funcType,
                                 const char* strval = 0);

  DLL_EXPORT int FFaFunctionGetData(double* data, int ndata);

  DLL_EXPORT double FFaFunctionEvaluate(double x, const double* data, int ndata,
                                        int funcId, int funcType,
                                        int extrapType);
}

#endif
