// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include <cmath>
#include <cstdlib>
#include <fstream>

#include "FFpLib/FFpFatigue/FFpSNCurve.H"
#include "FFaLib/FFaString/FFaTokenizer.H"
#include "FFaLib/FFaDefinitions/FFaMsg.H"
#include "FFpSNCurveLib.H"


FFpSNCurveLib::~FFpSNCurveLib()
{
  for (size_t i = 0; i < myCurves.size(); i++)
  {
    size_t n = myCurves[i].second.size();
    while (n--) delete myCurves[i].second[n];
  }
}


std::string FFpSNCurveLib::getCurveId(long int stdIdx, long int curveIdx) const
{
  if (stdIdx >= 0 && (size_t)stdIdx < myCurves.size())
    if (curveIdx >= 0 && (size_t)curveIdx < myCurves[stdIdx].second.size())
      return myCurves[stdIdx].second[curveIdx]->getName()
	+ " from the " + myCurves[stdIdx].first + " standard";

  return "(none)";
}


FFpSNCurve* FFpSNCurveLib::getCurve(long int stdIdx, long int curveIdx) const
{
  if (stdIdx < 0 || (size_t)stdIdx >= myCurves.size())
    ListUI <<" *** S-N curve standard index "<< (int)stdIdx
           <<" is out of range [0,"<< myCurves.size() <<">.\n";
  else if (curveIdx < 0 || (size_t)curveIdx >= myCurves[stdIdx].second.size())
    ListUI <<" *** S-N curve index "<< (int)curveIdx <<" is out of range [0,"
           << myCurves[stdIdx].second.size() <<">.\n";
  else
    return myCurves[stdIdx].second[curveIdx];

  return NULL;
}


FFpSNCurve* FFpSNCurveLib::getCurve(const std::string& snStd,
				    const std::string& snName) const
{
  for (size_t i = 0; i < myCurves.size(); i++)
    if (myCurves[i].first == snStd)
      for (size_t j = 0; j < myCurves[i].second.size(); j++)
	if (myCurves[i].second[j]->getName() == snName)
	  return myCurves[i].second[j];

  return NULL;
}


size_t FFpSNCurveLib::getNoCurves(long int stdIdx) const
{
  if (stdIdx < 0 || (size_t)stdIdx >= myCurves.size())
    return 0;

  return myCurves[stdIdx].second.size();
}


const std::string& FFpSNCurveLib::getCurveStd(long int stdIdx) const
{
  if (stdIdx >= 0 && (size_t)stdIdx < myCurves.size())
    return myCurves[stdIdx].first;

  static std::string empty;
  return empty;
}


const std::string& FFpSNCurveLib::getCurveName(long int stdIdx,
                                               long int curveIdx) const
{
  if (stdIdx >= 0 && (size_t)stdIdx < myCurves.size())
    if (curveIdx >= 0 && (size_t)curveIdx < myCurves[stdIdx].second.size())
      return myCurves[stdIdx].second[curveIdx]->getName();

  static std::string empty;
  return empty;
}


double FFpSNCurveLib::getThicknessExp(long int stdIdx, long int curveIdx) const
{
  if (stdIdx >= 0 && (size_t)stdIdx < myCurves.size())
    if (curveIdx >= 0 && (size_t)curveIdx < myCurves[stdIdx].second.size())
      return myCurves[stdIdx].second[curveIdx]->getThicknessExponent();

  return 0.0;
}


void FFpSNCurveLib::getCurveStds(std::vector<std::string>& snStds) const
{
  snStds.clear();
  for (size_t i = 0; i < myCurves.size(); i++)
    snStds.push_back(myCurves[i].first);
}


void FFpSNCurveLib::getCurveNames(std::vector<std::string>& names,
				  const std::string& snStd) const
{
  names.clear();
  for (size_t i = 0; i < myCurves.size(); i++)
    if (myCurves[i].first == snStd)
      for (size_t j = 0; j < myCurves[i].second.size(); j++)
	names.push_back(myCurves[i].second[j]->getName());
}


bool FFpSNCurveLib::readSNCurves(const std::string& filename)
{
  myCurves.clear();

  std::ifstream is(filename.c_str());
  if (!is)
  {
    ListUI <<" *** Can't open S-N curves file "<< filename <<"\n";
    return false;
  }

  // Search for the begin-character '<'
  char c;
  while (is.get(c) && !is.eof())
  {
    while (!is.eof() && isspace(c))
      is.get(c); // ignore leading white-space

    if (c == '#')
      is.ignore(BUFSIZ,'\n'); // ignore comment lines
    else if (c == '<')
    {
      // Found valid start of entry.
      // Note that we don't putback the '<' character read here, because the
      // FFaTokenizer::createTokens(istream&) method then will fail (TODO: fix)
      if (!this->read(is))
	return false;
    }
    else if (!is.eof())
    {
      ListUI <<" *** Invalid leading character \'"<< c
             <<"\' in S-N curves file "<< filename <<"\n";
      is.ignore(BUFSIZ,'\n');
    }
  }

  return true;
}


bool FFpSNCurveLib::read(std::istream& is)
{
  // tokens for standards
  FFaTokenizer stdTokens(is,'<','>');
  if (stdTokens.empty()) return false;

  const std::string& snStd = stdTokens.front();
  SNCurveVec curveVec;

  for (size_t i = 2; i < stdTokens.size(); i++)
  {
    // sub-tokens for curves
    FFaTokenizer curveTokens(stdTokens[i],'<','>');

    FFpSNCurve* curve = NULL;
    switch (atoi(stdTokens[1].c_str()))
      {
      case FFpSNCurve::NORSOK:
	if (curveTokens.size() == 3)
	  curve = new FFpSNCurveNorSok(atof(curveTokens[2].c_str()));
	else
          ListUI <<" *** Error in S-N curve file - check token definition:\n"
                 <<"     "<< snStd <<": "<< stdTokens[i] <<"\n";
	break;

      case FFpSNCurve::BRITISH:
	if (curveTokens.size() == 2)
	  curve = new FFpSNCurveBritish;
	else
          ListUI <<" *** Error in S-N curve file - check token definition:\n"
                 <<"     "<< snStd <<": "<< stdTokens[i] <<"\n";
	break;

      default:
        i = stdTokens.size();
	ListUI <<" *** Error in S-N curve file - unknown curve standard: "
               << snStd <<" - "<< stdTokens[1] <<"\n";
      }

    if (!curve) continue; // skip curve parsing if invalid standard

    bool isCurveValid = true;
    curve->setName(curveTokens.front());

    // sub-tokens for curve parameters, log(a) and m
    FFaTokenizer valueTokens(curveTokens[1],'<','>');
    if (valueTokens.size()%2 != 0)
    {
      isCurveValid = false;
      ListUI <<" *** Error in S-N curve file - check token definition:\n"
             <<"     "<< snStd <<" - "<< curve->getName()
             <<": "<< curveTokens[1] <<"\n";
    }

    double loga0 = 0.0, m0 = 0.0, logN0 = 0.0;
    for (size_t k = 0; k < valueTokens.size() && isCurveValid; k += 2)
    {
      double loga1 = atof(valueTokens[k].c_str());
      double m1 = atof(valueTokens[k+1].c_str());
      if (m1 < 0.0)
      {
        isCurveValid = false;
        ListUI <<" *** Error in S-N curve - negative m value:\n"
               <<"     "<< snStd <<" - "<< curve->getName()
               <<": m"<< k%2+1 <<" = " << valueTokens[k+1] <<"\n";
      }
      else if (k > 0 && curve->getStdId() == FFpSNCurve::NORSOK)
      {
	// Calculate intersection between the last two line segments for NorSok
	if (m1 == m0) // lines are parallel
        {
          isCurveValid = false;
          ListUI <<" *** Error in S-N curve - parallel line segments:\n"
                 <<"     "<< snStd <<" - "<< curve->getName()
                 <<": "<< curveTokens[1] <<"\n";
	}
	else if (loga1 == loga0) // lines are coincident - ignore
	  continue;
	else
	{
	  double logN1 = (m1*loga0 - m0*loga1)/(m1 - m0);
	  if (logN1 > loga1 || (k > 2 && logN1 < logN0))
	  {
            isCurveValid = false;
            ListUI <<" *** Error in S-N curve - invalid intersection"
                   <<" between segments "<< k%2 <<" and "<< k%2+1 <<":\n"
                   <<"     "<< snStd <<" - " << curve->getName()
                   <<": "<< curveTokens[1] <<"\n";
	  }
	  static_cast<FFpSNCurveNorSok*>(curve)->logN0.push_back(logN1);
	  logN0 = logN1;
	}
      }

      curve->loga.push_back(loga1);
      curve->m.push_back(m1);
      loga0 = loga1;
      m0 = m1;
    }

    if (isCurveValid)
      curveVec.push_back(curve);
    else if (curve)
      delete curve;
  }

  if (!curveVec.empty())
    myCurves.push_back(SNCurveStd(snStd,curveVec));

  return true;
}
