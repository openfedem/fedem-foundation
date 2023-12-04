// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFpLib/FFpDLL/FFpDamage.h"
#include "FFpLib/FFpFatigue/FFpDamageAccumulator.H"
#include "FFpLib/FFpFatigue/FFpSNCurveLib.H"
#include <cstring>

static std::vector<FFpDamageAccumulator*> ourAccs;
static std::vector<double>                ourRanges;

#define CHECK_ID(Id) \
  if (Id < 0 || Id >= (int)ourAccs.size() || !ourAccs[Id]) return false;


bool FFpDamageInit (const char* snCurveFile)
{
  return FFpSNCurveLib::instance()->readSNCurves(snCurveFile);
}


int FFpGetNoSNStd ()
{
  return FFpSNCurveLib::instance()->getNoCurveStds();
}


int FFpGetNoSNCurves (int snStd)
{
  return FFpSNCurveLib::instance()->getNoCurves(snStd-1);
}


int FFpGetSNStdName (int snStd, char** stdName, int nchar)
{
  const std::string& s = FFpSNCurveLib::instance()->getCurveStd(snStd-1);
  if (s.empty() || nchar < 1) return 0;

  if (s.size() < (size_t)(nchar-1))
    nchar = s.size();
  else
    --nchar;

  memcpy(*stdName,s.c_str(),nchar);
  (*stdName)[nchar] = '\0';

  return nchar;
}


int FFpGetSNCurveName (int snStd, int snCurve, char** curveName, int nchar)
{
  const std::string& s = FFpSNCurveLib::instance()->getCurveName(snStd-1,
                                                                 snCurve-1);
  if (s.empty() || nchar < 1) return 0;

  if (s.size() < (size_t)(nchar-1))
    nchar = s.size();
  else
    --nchar;

  memcpy(*curveName,s.c_str(),nchar);
  (*curveName)[nchar] = '\0';

  return nchar;
}


double FFpGetSNCurveThickExp (int snStd, int snCurve)
{
  return FFpSNCurveLib::instance()->getThicknessExp(snStd-1,snCurve-1);
}


int FFpAddHotSpot (int snStd, int snCurve, double gate)
{
  const FFpSNCurve* curve = NULL;
  if (snStd > 0 && snCurve > 0)
    curve = FFpSNCurveLib::instance()->getCurve(snStd-1,snCurve-1);

  for (size_t i = 0; i < ourAccs.size(); i++)
    if (!ourAccs[i])
    {
      ourAccs[i] = new FFpDamageAccumulator(curve,gate);
      return i;
    }

  ourAccs.push_back(new FFpDamageAccumulator(curve,gate));
  return ourAccs.size()-1;
}


bool FFpDeleteHotSpot (int id)
{
  CHECK_ID(id);

  delete ourAccs[id];
  ourAccs[id] = NULL;
  return true;
}


bool FFpAddTimeStressHistory (int id, const double* time,
                              const double* data, int ndata)
{
  CHECK_ID(id);

  ourAccs[id]->addStressHistory(time,data,ndata);
  return true;
}


bool FFpAddStressHistory (int id, const double* data, int ndata)
{
  CHECK_ID(id);

  ourAccs[id]->addStressHistory(data,ndata);
  return true;
}


bool FFpAddTimeStressValue (int id, double time, double sigma)
{
  CHECK_ID(id);

  ourAccs[id]->addStressValue(time,sigma);
  return true;
}


bool FFpAddStressValue (int id, double sigma)
{
  CHECK_ID(id);

  ourAccs[id]->addStressValue(sigma);
  return true;
}


bool FFpGetTimeRange (int id, double* T0, double* T1)
{
  CHECK_ID(id);

  FFpPoint Trange = ourAccs[id]->getTimeRange();

  *T0 = Trange.first;
  *T1 = Trange.second;
  return true;
}


bool FFpGetMaxPoint (int id, double* Tmax, double* Smax)
{
  CHECK_ID(id);

  FFpPoint maxPt = ourAccs[id]->getMaxPoint();

  *Tmax = maxPt.first;
  *Smax = maxPt.second;
  return true;
}


int FFpUpdateRainFlow (int id, bool close)
{
  CHECK_ID(id);

  return ourAccs[id]->updateRainflow(close).size();
}


bool FFpGetRainFlow (int id, double* ranges)
{
  CHECK_ID(id);

  ourAccs[id]->getRainflow(ranges);
  return true;
}


double FFpUpdateDamage (int id)
{
  CHECK_ID(id);

  return ourAccs[id]->updateDamage();
}


double FFpFinalDamage (int id)
{
  CHECK_ID(id);

  return ourAccs[id]->close();
}


double FFpCalculateDamage (const double* ranges, int nrange,
                           int snStd, int snCurve)
{
  std::vector<double> rangeValues(ranges,ranges+nrange);
  return FFpFatigue::getDamage(rangeValues,snStd,snCurve);
}
