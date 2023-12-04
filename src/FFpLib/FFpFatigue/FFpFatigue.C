// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include <cmath>
#include <algorithm>
#ifdef FFP_DEBUG
#include <iostream>
#endif

#include "FFpLib/FFpFatigue/FFpFatigue.H"
#include "FFpLib/FFpFatigue/FFpSNCurve.H"
#include "FFpLib/FFpFatigue/FFpSNCurveLib.H"


//==============================================================================
// PVX methods

FFpPVXprocessor::FFpPVXprocessor(double gate)
{
  isFirstData = true;
  myGateValue = gate;
  myDeltaTP = 0.0;
  myPossibleTP = std::make_pair(0.0,0.0);
}


bool FFpPVXprocessor::process(const double* data, int nData,
                              std::vector<double>& turns, bool isLastData)
{
  int iFirst = 0;
  if (isFirstData)
  {
    isFirstData = false;
    iFirst = this->locateFirstTP(data,nData);
    if (iFirst < 0) return true; // No range larger than the gate value found

    myPossibleTP.second = data[iFirst];
    myDeltaTP = data[iFirst+1] - data[iFirst];
    turns.push_back(myPossibleTP.second);
    ++iFirst;
  }

  for (int i = iFirst; i < nData; i++)
  {
    double delta = data[i] - myPossibleTP.second;
    if (delta*myDeltaTP <= 0.0)
    {
      if (fabs(delta) > myGateValue)
        // Detected a new turning point
        turns.push_back(myPossibleTP.second);
      else
        continue; // Ignore small ranges
    }

    // Update the end point and gradient of last range
    myPossibleTP.second = data[i];
    myDeltaTP = data[i] - (turns.empty() ? 0.0 : turns.back());
  }

  if (isLastData && fabs(myDeltaTP) > myGateValue)
    turns.push_back(myPossibleTP.second);

#ifdef FFP_DEBUG
  for (size_t j = 0; j < turns.size(); j++)
    std::cout <<"\n\tTurning point "<< 1+j <<": "<< turns[j];
  std::cout << std::endl;
#endif

  return true;
}


bool FFpPVXprocessor::process(const double* times,
                              const double* data, int nData,
                              std::vector<FFpPoint>& turns, bool isLastData)
{
  int iFirst = 0;
  if (isFirstData && nData > 0)
  {
    isFirstData = false;
    iFirst = this->locateFirstTP(data,nData);
    if (iFirst < 0) // No range larger than the gate value found
    {
      myPossibleTP.first = times[-1-iFirst];
      return true;
    }

    myPossibleTP = FFpPoint(times[iFirst],data[iFirst]);
    myDeltaTP = data[iFirst+1] - data[iFirst];
    turns.push_back(myPossibleTP);
    ++iFirst;
  }

  for (int i = iFirst; i < nData; i++)
  {
    double delta = data[i] - myPossibleTP.second;
    if (delta*myDeltaTP <= 0.0)
    {
      if (fabs(delta) > myGateValue)
        // Detected a new turning point
        turns.push_back(myPossibleTP);
      else
        continue; // Ignore small ranges
    }

    // Update the end point and gradient of last range
    myPossibleTP = FFpPoint(times[i],data[i]);
    myDeltaTP    = data[i] - (turns.empty() ? 0.0 : turns.back().second);
  }

  if (isLastData && fabs(myDeltaTP) > myGateValue)
    turns.push_back(myPossibleTP);

#ifdef FFP_DEBUG
  for (size_t j = 0; j < turns.size(); j++)
    std::cout <<"\n\tTurning point "<< 1+j <<": "
              << turns[j].first <<", "<< turns[j].second;
  std::cout << std::endl;
#endif

  return true;
}


int FFpPVXprocessor::locateFirstTP(const double* data, int nData)
{
  double deltaTP = data[0];
  int iMin = 0, iMax = 0, iTP = 0;
  for (int i = 1; i < nData; i++)
    if ((data[i] - data[iTP])*deltaTP > 0.0)
      iTP = i; // Continuing with the same gradient, update last range end point
    else if (data[i-1] - data[iMin] > myGateValue)
      return iMin; // The lowest value so far is the first turning point
    else if (data[iMax] - data[i-1] > myGateValue)
      return iMax; // The highest value so far is the first turning point
    else if (data[iTP] > data[iMax])
    {
      iMax = iTP; // Update the index of the highest value found so far
      deltaTP = data[i] - data[iTP]; // Update the gradient (changed sign)
    }
    else if (data[iTP] < data[iMin])
    {
      iMin = iTP; // Update the index of the lowest value found so far
      deltaTP = data[i] - data[iTP]; // Update the gradient (changed sign)
    }
    else
    {
      iTP = i; // Update potential turning point index (end point of last range)
      deltaTP = data[i] - data[iTP]; // Update the gradient (changed sign)
    }

  // All stress ranges are within the gate value. Just update the possible
  // turning point to the maximum (or minimum) point found, in case invoked
  // for another time series data array.
  iTP = iMax > iMin ? iMax : iMin;
  myPossibleTP.second = data[iTP];
  myDeltaTP = deltaTP;
  return -1-iTP;
}


//==============================================================================
// Rainflow methods

bool FFpRainFlowCycleCounter::process(const double* turns, int nTurns,
				      FFpCycles& cycles, bool isLastData)
{
  for (int i = 0; i < nTurns; i++)
    myTPList.push_back(turns[i]);

  int nPoint = 0;
  while (this->processTPList(cycles,nPoint));

  if (isLastData)
    return this->processFinish(cycles,nPoint);

  return true;
}


bool FFpRainFlowCycleCounter::process(const std::vector<FFpPoint>& turns,
				      FFpCycles& cycles, bool isLastData)
{
  for (size_t i = 0; i < turns.size(); i++)
    myTPList.push_back(turns[i].second);

  int nPoint = 0;
  while (this->processTPList(cycles,nPoint));

  if (isLastData)
    return this->processFinish(cycles,nPoint);

  return true;
}


bool FFpRainFlowCycleCounter::processTPList(FFpCycles& cycles, int& pnt)
{
  if (myTPList.size() < 4) return false; // To be handled by a special cleanup

  int i, nRemoved = 0;
  std::list<double>::iterator tp[4];
  double range[3];

  // Initiate the 4 iterators to point at the first 4 elements
  tp[0] = myTPList.begin();
  tp[1] = tp[0]; ++tp[1];
  tp[2] = tp[1]; ++tp[2];
  tp[3] = tp[2]; ++tp[3];

  // Traverse the list while counting and removing all full cycles
  while (tp[3] != myTPList.end())
  {
    // Calculate ranges
    for (i = 0; i < 3; i++)
      range[i] = *tp[i+1] - *tp[i];

    pnt++;
    if (range[0]*range[1] > 0.0) // Point 1 is not a turning point
    {
#ifdef FFP_DEBUG
      std::cout <<"FFpRainFlowCycleCounter: Point "<< pnt
		<<" "<< *tp[1] <<" is not a turning point"<< std::endl;
#endif
      nRemoved++;
      tp[1] = myTPList.erase(tp[1]);
      ++tp[2];
      ++tp[3];
    }
    else if (range[1]*range[2] > 0.0) // Point 2 is not a turning point
    {
#ifdef FFP_DEBUG
      std::cout <<"FFpRainFlowCycleCounter: Point "<< pnt+1
		<<" "<< *tp[2] <<" is not a turning point"<< std::endl;
#endif
      nRemoved++;
      tp[2] = myTPList.erase(tp[2]);
      ++tp[3];
    }
    else if (fabs(range[0]) >= fabs(range[1]) &&
	     fabs(range[2]) >= fabs(range[1])) // A genuine cycle
    {                           
#if FFP_DEBUG > 1
      std::cout <<"FFpRainFlowCycleCounter: Points "<< pnt <<","<< pnt+1
                <<" define a cycle ["<< *tp[1] <<","<< *tp[2] <<"]"<< std::endl;
#endif
      pnt++;
      nRemoved += 2;
      if (fabs(range[1]) > myGateValue)
	cycles.push_back(FFpCycle(*tp[1],*tp[2]));

      myTPList.erase(tp[1]);
      tp[1] = myTPList.erase(tp[2]);
      if (++tp[3] != myTPList.end())
	tp[2] = tp[3]++;
    }
    else // The cycle can not be counted yet
      for (i = 0; i < 4; i++)
	++tp[i];
  }

#if FFP_DEBUG > 1
  std::cout <<"FFpRainFlowCycleCounter: "<< nRemoved
            <<" points removed"<< std::endl;
#endif
  return nRemoved > 0;
}


bool FFpRainFlowCycleCounter::processFinish(FFpCycles& cycles, int& pnt)
{
  // The remainder of the list is finished by finding max value in the list,
  // injecting a copy of this into the list, splitting the list at the max
  // values and reversing the order of the blocks so that the list now begins
  // and ends at the max value.
  // This allow a normal counting of all cycles until the list has only three
  // data elements, which then becomes the last cycle
#if FFP_DEBUG > 1
  std::cout <<"FFpRainFlowCycleCounter: "<< myTPList.size()
            <<" points left"<< std::endl;
#endif

  if (myTPList.size() > 1)
  {
    // Find largest value
    std::list<double>::iterator pos = myTPList.begin();
    std::list<double>::iterator maxPos = pos;
    for (++pos; pos != myTPList.end(); ++pos)
      if (fabs(*pos) > fabs(*maxPos))
	maxPos = pos;

    // Insert copy of largest value
    myTPList.insert(maxPos,*maxPos);

    // Move elements around so that list starts and ends on largest value
    myTPList.insert(myTPList.begin(),maxPos,myTPList.end());
    myTPList.erase(maxPos,myTPList.end());

    // Count "normal" cycles
    while (this->processTPList(cycles,pnt));

    // Now we should have 3 points left, which can be counted as last cycle
    if (myTPList.size() != 3) return false;

#if FFP_DEBUG > 1
    std::cout <<"FFpRainFlowCycleCounter: Final cycle,";
    for (pos = myTPList.begin(); pos != myTPList.end(); ++pos)
      std::cout <<" "<< *pos;
    std::cout << std::endl;
#endif
    maxPos = myTPList.begin(); pos = maxPos++;
    cycles.push_back(FFpCycle(*pos,*maxPos));
  }
  myTPList.clear();
  return true;
}


bool FFpFatigue::readSNCurves(const std::string& snCurveFile)
{
  return FFpSNCurveLib::instance()->readSNCurves(snCurveFile);
}


double FFpFatigue::calcRainFlowAndDamage(const std::vector<double>& data,
                                         std::vector<double>& ranges,
                                         double gateValueMPa, double toMPa,
                                         int snStd, int snCurve)
{
  FFpCycle::setScaleToMPa(toMPa);

  std::vector<double>   turns;
  std::vector<FFpCycle> cycles;

  FFpPVXprocessor pvx(gateValueMPa/toMPa);
  pvx.process(&data.front(),data.size(),turns,true);
  if (!turns.empty())
  {
    FFpRainFlowCycleCounter cyc(gateValueMPa/toMPa);
    cyc.process(&turns.front(),turns.size(),cycles,true);
  }
  ranges.resize(cycles.size());
  for (size_t i = 0; i < cycles.size(); i++)
    ranges[i] = cycles[i].range();

  std::sort(ranges.begin(),ranges.end());
#ifdef FFP_DEBUG
  for (size_t j = 0; j < ranges.size(); j++)
    std::cout <<"\n\tRange "<< 1+j <<": "<< ranges[j];
  std::cout <<"\n# Turning points: "<< turns.size()
            <<"\n# Stress ranges: "<< ranges.size() << std::endl;
#endif
  return snStd < 0 || snCurve < 0 ? -1.0 : getDamage(ranges,snStd,snCurve);
}


double FFpFatigue::getDamage(std::vector<double>& ranges,
                             int snStd, int snCurve)
{
  const FFpSNCurve* curve = FFpSNCurveLib::instance()->getCurve(snStd,snCurve);

  double damage = 0.0;
  if (curve && curve->isValid())
    for (size_t i = 0; i < ranges.size(); i++)
      damage += 1.0 / curve->getValue(ranges[i]);

#ifdef FFP_DEBUG
  std::cout <<"FFpFatigue::getDamage: "<< damage;
  if (!ranges.empty())
    std::cout <<" ["<< ranges.front() <<","<< ranges.back() <<"]";
  std::cout << std::endl;
#endif
  return damage;
}


double FFpFatigue::getDamage(const FFpCycles& cycles, const FFpSNCurve& snCurve)
{
  double damage = 0.0;
  for (size_t i = 0; i < cycles.size(); i++)
    damage += 1.0 / snCurve.getValue(cycles[i].range());
#ifdef FFP_DEBUG
  std::vector<double> ranges(cycles.size());
  for (size_t i = 0; i < cycles.size(); i++)
    ranges[i] = cycles[i].range();
  std::sort(ranges.begin(),ranges.end());
  for (size_t j = 0; j < ranges.size(); j++)
    std::cout <<"\n\tRange "<< 1+j <<": "<< ranges[j];
  std::cout <<"\nFFpFatigue::getDamage: "<< damage << std::endl;
#endif
  return damage;
}
