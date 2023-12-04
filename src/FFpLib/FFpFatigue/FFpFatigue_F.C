// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include <map>
#include <fstream>
#include <algorithm>

#include "FFpFatigue.H"
#include "FFpSNCurveLib.H"
#include "FFpSNCurve.H"
#include "FFaLib/FFaCmdLineArg/FFaCmdLineArg.H"
#include "FFaLib/FFaOS/FFaFortran.H"


struct FFpHistory
{
  std::vector<double>   times;
  std::vector<double>   data;
  std::vector<FFpCycle> cycles;
};

static std::map<int,FFpHistory> hist;


SUBROUTINE(ffp_initfatigue,FFP_INITFATIGUE) (int& ierr)
{
  std::string fileName;
  FFaCmdLineArg::instance()->getValue("SNfile",fileName);
  ierr = FFpSNCurveLib::instance()->readSNCurves(fileName) ? 0 : -1;
}


SUBROUTINE(ffp_addpoint,FFP_ADDPOINT) (int& handle,
				       const double& time, const double& value)
{
  if (handle == 0) handle = hist.size()+1;

  hist[handle].times.push_back(time);
  hist[handle].data.push_back(value);
}


SUBROUTINE(ffp_releasedata,FFP_RELEASEDATA) (const int& handle)
{
  hist.erase(handle);
}


SUBROUTINE(ffp_calcdamage,FFP_CALCDAMAGE) (const int& handle,
					   const int* snCurve,
					   const double& gateValue,
					   double& damage, int& ierr)
{
  ierr = -1;
  std::map<int,FFpHistory>::iterator it = hist.find(handle);
  if (it == hist.end()) return;

  FFpSNCurve* snC = FFpSNCurveLib::instance()->getCurve(snCurve[0],snCurve[1]);
  if (!snC) return;
  if (!snC->isValid()) return;

  FFpPVXprocessor pvx(gateValue);
  std::vector<FFpPoint> turns;
  if (!pvx.process(&it->second.times.front(),
		   &it->second.data.front(),
		   it->second.times.size(), turns, true)) return;

  it->second.cycles.clear();
  FFpRainFlowCycleCounter cyc(gateValue);
  if (!cyc.process(turns, it->second.cycles, true)) return;

  damage = FFpFatigue::getDamage(it->second.cycles,*snC);
  ierr = 0;
}


DOUBLE_FUNCTION(ffp_getdamage,FFP_GETDAMAGE) (const int& handle,
					      const double& gateValue,
					      const double* curveData,
					      const bool printCycles = false)
{
  std::map<int,FFpHistory>::iterator it = hist.find(handle);
  if (it == hist.end()) return -1.0;

  FFpPVXprocessor pvx(gateValue);
  std::vector<FFpPoint> turns;
  pvx.process(&it->second.times.front(),
	      &it->second.data.front(),
	      it->second.times.size(), turns, true);

  it->second.cycles.clear();
  FFpRainFlowCycleCounter cyc(gateValue);
  cyc.process(turns, it->second.cycles, true);

  // Sort the cycles in increasing range order
  sort(it->second.cycles.begin(),it->second.cycles.end());

  if (printCycles)
  {
    size_t i;
    char fname[32];

    sprintf(fname,"pvx_%d.asc",handle);
    std::ofstream pvxFile(fname);
    pvxFile.setf(std::ios::scientific);
    for (i = 0; i < turns.size(); i++)
      pvxFile << turns[i].first <<" "<< turns[i].second << std::endl;

    sprintf(fname,"cycles_%d.asc",handle);
    std::ofstream cyclesFile(fname);
    cyclesFile.setf(std::ios::scientific);
    for (i = 0; i < it->second.cycles.size(); i++)
      cyclesFile << "cycle: "<< it->second.cycles[i]
		 <<" mean = "<< it->second.cycles[i].mean()
		 <<" range = "<< it->second.cycles[i].range() << std::endl;
  }

  FFpSNCurveNorSok snCurve(curveData[0],curveData[1],curveData[2],curveData[3]);
  return FFpFatigue::getDamage(it->second.cycles,snCurve);
}


INTEGER_FUNCTION(ffp_getnumcycles,FFP_GETNUMCYCLES) (const int& handle,
						     const double& low,
						     const double& high)
{
  std::map<int,FFpHistory>::const_iterator it = hist.find(handle);
  if (it == hist.end()) return -1;
  if (it->second.cycles.empty()) return -1;

  std::vector<FFpCycle>::const_iterator ilow, ihigh;
  ilow = lower_bound(it->second.cycles.begin(),it->second.cycles.end(),low);
  if (ilow == it->second.cycles.end()) return -1;

  ihigh = lower_bound(ilow,it->second.cycles.end(),high);
  return ihigh - ilow;
}
