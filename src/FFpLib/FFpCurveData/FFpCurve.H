// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFP_CURVE_H
#define FFP_CURVE_H

#include <vector>
#include <string>

#include "FFpLib/FFpFatigue/FFpCycle.H"
#include "FFaLib/FFaOperation/FFaOperation.H"
#include "FFaLib/FFaString/FFaEnum.H"

class FFrExtractor;
class FFrVariableReference;
class FFaResultDescription;
class FFpSNCurve;
struct DFTparams;

struct RFprm
{
  double start;
  double stop;
  double gateValue;
  RFprm(double g = 0.0) : start(0.0), stop(-1.0), gateValue(g) {}
  RFprm(double t0, double t1, double g) : start(t0), stop(t1), gateValue(g) {}
};


class FFpCurve
{
  enum { X = 0, Y = 1, N_AXES = 2 };

public:
  enum TimeOp { None, Min, Max, AMax, Mean, RMS };

  FFaEnumMapping(TimeOp) {
    FFaEnumEntry(None, "None");
    FFaEnumEntry(Min,  "Min");
    FFaEnumEntry(Max,  "Max");
    FFaEnumEntry(AMax, "Absolute Max");
    FFaEnumEntry(Mean, "Mean");
    FFaEnumEntry(RMS,  "RMS");
    FFaEnumEntryEnd;
  };

  FFpCurve (size_t nSpatialPoints = 1) { this->resize(nSpatialPoints); }
  FFpCurve (const FFpCurve& curve);
  ~FFpCurve () { this->resizeReader(X); this->resizeReader(Y); }

  FFpCurve& operator= (const FFpCurve&) = delete;

  void clear ();
  void resize (size_t nSpatialPoints);
  bool unref (bool clearReadOp = false);
  bool initAxis (const FFaResultDescription& desc,
		 const std::string& oper, int axis);
  bool initAxes (const std::vector<FFaResultDescription>& xdesc,
		 const std::vector<FFaResultDescription>& ydesc,
		 const std::string& xOper, const std::string& yOper,
		 const std::pair<double,double>& tRange,
		 const std::string& tOper, short int end1);
  int getSpatialXaxisObject (size_t i) const;

  const std::pair<double,double>& getTimeRange () const { return timeRange; }

  bool notReadThisFar (double& lastStep) const;
  bool findVarRefsAndOpers (FFrExtractor* extr, std::string& errMsg);
  void printPosition (std::ostream& os) const;

  bool loadTemporalData (double currentTime);
  bool loadSpatialData (double currentTime, const double epsT = 0.0);
  bool loadCurrentSpatialX ();
  void finalizeTimeOp ();
  bool usingInitialXaxis () const { return useInitialXaxis; }

  bool loadFileData (const std::string& filePath, const std::string& channel,
		     std::string& errMsg, double minX = 0.0, double maxX =-1.0);

  bool combineData (int ID, const std::string& expression,
		    const std::vector<FFpCurve*>& compCurves,
		    const char** compNames, bool clipXdomain,
		    std::string& message);

  bool replaceByScaledShifted (const DFTparams& dft);
  bool replaceByDerivative ();
  bool replaceByIntegral ();
  bool replaceByDFT (const DFTparams& dft, const std::string& cId,
		     std::string& errMsg);
  bool replaceByRainflow (const RFprm& rf, double toMPa, bool doPVXonly,
			  const std::string& cId, std::string& errMsg);

  bool getCurveStatistics (bool entire, double start, double stop,
			   bool useScaledShifted, const DFTparams& dft,
			   double& rms, double& avg, double& stdDev,
			   double& integral, double& min, double& max,
			   std::string& errMsg) const;

  double getDamage (const RFprm& rf, double toMPa, const FFpSNCurve& sn);

  bool empty () const { return points[X].empty() && points[Y].empty(); }

  bool hasDataChanged () const { return dataChanged; }
  void onDataPlotted () { dataChanged = false; }
  void setDataChanged ();

  std::vector<double>& operator[] (int axis) { return points[axis]; }
  const std::vector<double>& getAxisData (int ax) const { return points[ax]; }

  double getValue (double x, bool& monotonX) const;

  double getXrange () const { return Xrange; }
  bool inDomain (double x) const;
  bool clipX (double Xmin, double Xmax);

  bool checkAxesSize ();

private:
  int numberOfSamples (double delta, double start, double stop) const;

  bool sample (double start, double stop, double shift, double scale,
	       bool subMean, size_t n, std::vector<double>& yOut,
	       std::string& errMsg) const;

  bool performRainflowCalc (const RFprm& rf, bool doPVXonly = false);

  bool reversePoints ();

  void resizeReader (int axis, size_t n = 0);

  struct PointData
  {
    FFaResultDescription* rDescr = NULL;
    FFrVariableReference* varRef = NULL;
    FFaOperation<double>* readOp = NULL;
  };

  friend std::ostream& operator<< (std::ostream& os, const PointData&);

  std::vector<PointData>   reader[N_AXES];
  std::string*             rdOper[N_AXES];
  std::vector<double>      points[N_AXES];
  std::vector<FFpCycle>    cycles;
  RFprm                    lastRF;
  double                   lastKey;
  double                   Xrange;
  mutable size_t           lastX;
  mutable size_t           timeSamples;
  std::pair<double,double> timeRange;
  TimeOp                   timeOper;
  bool                     useInitialXaxis;
  short int                beamEndFlag;
  bool                     dataChanged;
  bool                     needRainflow;
};

#endif
