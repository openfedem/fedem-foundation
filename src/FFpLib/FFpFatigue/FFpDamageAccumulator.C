// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFpLib/FFpFatigue/FFpDamageAccumulator.H"
#include <cmath>


FFpDamageAccumulator::FFpDamageAccumulator(const FFpSNCurve* snc, double gate)
{
  Tmin =  0.0;
  Tmax = -1.0;
  tLast = 0.0;
  iLast = 0;
  myMax = std::make_pair(0.0,0.0);
  mySNcurve = snc;
  myGateValue = gate;
  myDamage = 0.0;
}


void FFpDamageAccumulator::addStressHistory(const double* time,
                                            const double* data, int nPt)
{
  myPvx.setGateValue(myGateValue);
  myPvx.process(time,data,nPt,myTurns);
  if (myTurns.empty()) return;

  if (Tmin > Tmax)
    Tmin = myTurns.front().first;
  if (Tmax < myTurns.back().first)
    Tmax = myTurns.front().first;

  for (size_t i = iLast; i < myTurns.size(); i++)
    if (fabs(myTurns[i].second) > fabs(myMax.second))
      myMax = myTurns[i];

  iLast = myTurns.size();
  tLast = data[nPt-1];
}


void FFpDamageAccumulator::addStressHistory(const double* data, int nPt)
{
  // Generate a pseudo-time
  std::vector<double> time(nPt);
  for (int i = 0; i < nPt; i++) time[i] = tLast + double(1+i);
  this->addStressHistory(&time.front(),data,nPt);
}


void FFpDamageAccumulator::addStressValue(double time, double sigma)
{
  this->addStressHistory(&time,&sigma,1);
}


void FFpDamageAccumulator::addStressValue(double sigma)
{
  this->addStressValue(tLast+1.0,sigma);
}


const std::vector<FFpCycle>& FFpDamageAccumulator::updateRainflow(bool doClose)
{
  if (doClose)
  {
    myPvx.process(NULL,NULL,0,myTurns,true);
    if (!myTurns.empty() && Tmax < myTurns.back().first)
      Tmax = myTurns.front().first;

    for (size_t i = iLast; i < myTurns.size(); i++)
      if (fabs(myTurns[i].second) > fabs(myMax.second))
        myMax = myTurns[i];
  }

  myRFc.setGateValue(myGateValue);
  myRFc.process(myTurns,myCycles,doClose);

  iLast = 0;
  myTurns.clear(); // Erase the turning points already counted

  return myCycles;
}


void FFpDamageAccumulator::getRainflow(double* ranges) const
{
  for (size_t i = 0; i < myCycles.size(); i++)
    ranges[i] = myCycles[i].range();
}


double FFpDamageAccumulator::updateDamage()
{
  if (mySNcurve)
  {
    myDamage += FFpFatigue::getDamage(myCycles,*mySNcurve);
    myCycles.clear(); // Erase the cycles already accounted for
  }
  return myDamage;
}


double FFpDamageAccumulator::close()
{
  myPvx.process(NULL,NULL,0,myTurns,true);
  if (!myTurns.empty() && Tmax < myTurns.back().first)
    Tmax = myTurns.front().first;

  for (size_t i = iLast; i < myTurns.size(); i++)
    if (fabs(myTurns[i].second) > fabs(myMax.second))
      myMax = myTurns[i];

  myRFc.process(myTurns,myCycles,true);

  iLast = 0;
  myTurns.clear(); // Erase the turning points already counted

  if (mySNcurve)
  {
    myDamage += FFpFatigue::getDamage(myCycles,*mySNcurve);
    myCycles.clear(); // Erase the cycles already accounted for
  }
  return myDamage;
}
