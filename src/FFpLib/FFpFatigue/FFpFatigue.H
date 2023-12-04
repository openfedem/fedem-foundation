// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFP_FATIGUE_H
#define FFP_FATIGUE_H

#include <vector>
#include <list>

#include "FFpLib/FFpFatigue/FFpCycle.H"

typedef std::pair<double,double> FFpPoint;
typedef std::vector<FFpCycle>    FFpCycles;


class FFpPVXprocessor
{
  bool     isFirstData;
  double   myGateValue;
  double   myDeltaTP;
  FFpPoint myPossibleTP;

public:
  FFpPVXprocessor(double gate = 0.0);

  void setGateValue(double gate) { myGateValue = gate; }

  bool process(const double* data, int nData,
               std::vector<double>& turns, bool isLastData = false);

  bool process(const double* times, const double* data, int nData,
               std::vector<FFpPoint>& turns, bool isLastData = false);

private:
  int locateFirstTP(const double* data, int nData);
};


class FFpRainFlowCycleCounter
{
  double myGateValue;

  std::list<double> myTPList; // list of Turning Points (Peak Valley list)

public:
  FFpRainFlowCycleCounter(double gate = 0.0) { myGateValue = gate; }

  void setGateValue(double gate) { myGateValue = gate; }

  bool process(const double* turns, int nTurns, FFpCycles& cycles,
               bool isLastData = false);

  bool process(const std::vector<FFpPoint>& turns, FFpCycles& cycles,
               bool isLastData = false);

private:
  bool processTPList(FFpCycles& cycles, int& pnt);
  bool processFinish(FFpCycles& cycles, int& pnt);
};


class FFpSNCurve;

namespace FFpFatigue
{
  bool readSNCurves(const std::string& snCurveFile);

  double calcRainFlowAndDamage(const std::vector<double>& data,
                               std::vector<double>& ranges,
                               double gateValueMPa = 0.0, double toMPa = 1.0e-6,
                               int snStd = -1, int snCurve = -1);

  double getDamage(std::vector<double>& ranges,
                   int snStd = -1, int snCurve = -1);

  double getDamage(const FFpCycles& cycles, const FFpSNCurve& snCurve);
};

#endif
