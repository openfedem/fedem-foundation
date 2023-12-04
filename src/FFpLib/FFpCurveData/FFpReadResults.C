// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFpLib/FFpCurveData/FFpReadResults.H"
#include "FFpLib/FFpCurveData/FFpGraph.H"
#include "FFaLib/FFaDefinitions/FFaResultDescription.H"
#include "FFrLib/FFrExtractor.H"


FFpVar::FFpVar (const char* n, const char* t, const char* o)
{
  if (n) name = n;
  if (t) type = t;
  if (o) oper = o;
}


bool FFp::readHistories (const char* objType,
                         const std::vector<int>& baseIds,
                         const std::vector<FFpVar>& vars,
                         FFrExtractor* extractor,
                         double& Tmin, double& Tmax, bool includeTime,
                         DoubleVectors& values, std::string& message)
{
  if ((baseIds.empty() && !objType) || vars.empty() || !extractor)
    return false;

  enum { X = 0, Y = 1 };

  FFpGraph rdbCurves(0,false);
  size_t   i, j, c;

  if (includeTime)
    rdbCurves.addCurve(new FFpCurve(1)); // read time and result value
  else
    rdbCurves.addCurve(new FFpCurve(0)); // read result value only

  size_t nItems = baseIds.empty() ? 1 : baseIds.size();
  for (i = 1; i < nItems*vars.size(); i++)
    rdbCurves.addCurve(new FFpCurve(0)); // read result value only

  // Define the physical time result (first curve only)
  FFaTimeDescription time;
  std::string None("None");
  if (includeTime)
    rdbCurves[0].initAxis(time,None,X);

  // Define the result quantities to read for each object
  std::vector<FFaResultDescription> rDescr(rdbCurves.numCurves(),
                                           FFaResultDescription(objType));
  for (i = c = 0; i < nItems; i++)
    for (j = 0; j < vars.size(); j++, c++)
    {
      if (!baseIds.empty()) rDescr[c].baseId = baseIds[i];
      rDescr[c].varRefType = vars[j].type.empty() ? "SCALAR" : vars[j].type;

      // Split up the path description into a vector of strings
      size_t iStart = 0, iEnd, nChr;
      while (iStart != std::string::npos)
      {
        iEnd = vars[j].name.find('|',iStart);
        nChr = iEnd == std::string::npos ? iEnd : iEnd-iStart;
        rDescr[c].varDescrPath.push_back(vars[j].name.substr(iStart,nChr));
        iStart = iEnd == std::string::npos ? iEnd : iEnd+1;
      }

      if (vars[j].oper.empty())
        rdbCurves[c].initAxis(rDescr[c],None,Y);
      else
        rdbCurves[c].initAxis(rDescr[c],vars[j].oper,Y);
    }

  // Now load the data from file
  rdbCurves.setTimeInterval(Tmin, Tmax < 0.0 ? 1.0e30 : Tmax+1.0e-8);
  if (!rdbCurves.loadTemporalData(extractor,message))
    return false;

  // Extract the results
  if (includeTime)
    values.push_back(rdbCurves[0].getAxisData(X));
  for (i = 0; i < rdbCurves.numCurves(); i++)
    values.push_back(rdbCurves[i].getAxisData(Y));

  rdbCurves.getTimeInterval(Tmin,Tmax);
  return true;
}
