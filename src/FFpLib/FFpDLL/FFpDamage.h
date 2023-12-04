// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFP_DAMAGE_H
#define FFP_DAMAGE_H

#if defined(win32) || defined(win64)
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT
#endif


extern "C"
{
  DLL_EXPORT bool FFpDamageInit(const char* snCurveFile);

  DLL_EXPORT int FFpGetNoSNStd();
  DLL_EXPORT int FFpGetNoSNCurves(int snStd);
  DLL_EXPORT int FFpGetSNStdName(int snStd, char** stdName, int nchar);
  DLL_EXPORT int FFpGetSNCurveName(int snStd, int snCurve,
                                   char** curveName, int nchar);
  DLL_EXPORT double FFpGetSNCurveThickExp(int snStd, int snCurve);

  DLL_EXPORT int FFpAddHotSpot(int snStd, int snCurve, double gate);
  DLL_EXPORT bool FFpDeleteHotSpot(int id);

  DLL_EXPORT bool FFpAddTimeStressHistory(int id, const double* time,
                                          const double* data, int ndata);
  DLL_EXPORT bool FFpAddStressHistory(int id, const double* data, int ndata);

  DLL_EXPORT bool FFpGetTimeRange(int id, double* T0, double* T1);
  DLL_EXPORT bool FFpGetMaxPoint(int id, double* Tmax, double* Smax);

  DLL_EXPORT int FFpUpdateRainFlow(int id, bool close);
  DLL_EXPORT bool FFpGetRainFlow(int id, double* ranges);
  DLL_EXPORT double FFpUpdateDamage(int id);
  DLL_EXPORT double FFpFinalDamage(int id);
  DLL_EXPORT double FFpCalculateDamage(const double* ranges, int nrange,
                                       int snStd, int snCurve);
}

#endif
