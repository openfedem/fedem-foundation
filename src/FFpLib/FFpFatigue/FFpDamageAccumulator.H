// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFP_DAMAGE_ACCUMULATOR_H
#define FFP_DAMAGE_ACCUMULATOR_H

#include "FFpLib/FFpFatigue/FFpDamageAccumulator.H"
#include "FFpLib/FFpFatigue/FFpFatigue.H"
#include <cmath>


class FFpDamageAccumulator
{
public:
  FFpDamageAccumulator(const FFpSNCurve* snc = NULL, double gate = 0.0);

  void setGateValue(double gate) { myGateValue = gate; }
  void addStressHistory(const double* time, const double* data, int nPt);
  void addStressHistory(const double* data, int nPt);
  void addStressValue(double time, double sigma);
  void addStressValue(double sigma);
  const std::vector<FFpCycle>& updateRainflow(bool doClose = false);
  void getRainflow(double* ranges) const;
  double updateDamage();
  double close();

  FFpPoint getTimeRange() const { return { Tmin, Tmax }; }
  FFpPoint getMaxPoint() const { return myMax; }

private:
  size_t iLast;
  double tLast;
  double Tmin;
  double Tmax;
  double myGateValue;
  double myDamage;
  std::pair<double,double> myMax;
  std::vector<FFpPoint>    myTurns;
  std::vector<FFpCycle>    myCycles;
  FFpPVXprocessor          myPvx;
  FFpRainFlowCycleCounter  myRFc;
  const FFpSNCurve*        mySNcurve;
};

#endif
