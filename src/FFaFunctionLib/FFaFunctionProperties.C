// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFaFunctionProperties.H"
#include "FFaFunctionManager.H"
#include "FFaUserFuncPlugin.H"
#include "FFaMathExpr/FFaMathExprFactory.H"
#include "FiDeviceFunctions/FiDeviceFunctionFactory.H"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


static double evalFunc(int fType, double x, const std::vector<double>& realVars,
                       int extrap, int& ierr)
{
  return FFaFunctionManager::getValue(fType,fType,extrap,realVars,x,ierr);
}


static double interpolate(bool fromLeft, double x,
                          double x0, double y0, double x1, double y1)
{
  return x1 > x0 ? y0 + (x-x0)*(y1-y0)/(x1-x0) : (fromLeft ? y0 : y1);
}


int FFaFunctionProperties::getSmartPoints(int funcType,
					  double start, double stop, int extrap,
					  const std::vector<double>& realVars,
					  std::vector<double>& xvec,
					  std::vector<double>& yvec)
{
  xvec.clear();
  yvec.clear();
  if (start > stop) return -1;

  int nRvals = realVars.size();
  int ierr = 0;

  // *** Functions needing special treatment

  if (funcType == LIN_VAR_p)
  {
    if (nRvals < 2) return -3;

    if (realVars[0] > start && extrap > 0)
    {
      // Extrapolation before first point
      xvec.push_back(start);
      if (extrap == 1) // Flat extrapolation
	yvec.push_back(realVars[1]);
      else if (nRvals > 3) // Linear extrapolation
	yvec.push_back(interpolate(true,start,
				   realVars[0],realVars[1],
				   realVars[2],realVars[3]));
    }

    for (int i = 0; i < nRvals; i += 2)
      if (realVars[i] >= start && realVars[i] <= stop)
      {
	if (xvec.empty() && i > 0 && realVars[i] > start)
	{
	  // Interpolate the first point
	  xvec.push_back(start);
	  yvec.push_back(interpolate(false,start,
				     realVars[i-2],realVars[i-1],
				     realVars[i]  ,realVars[i+1]));
	}
	xvec.push_back(realVars[i]);
	yvec.push_back(realVars[i+1]);
      }
      else if (i > 0 && realVars[i] > stop)
      {
	// Interpolate the last point
	xvec.push_back(stop);
	yvec.push_back(interpolate(true,stop,
				   realVars[i-2],realVars[i-1],
				   realVars[i]  ,realVars[i+1]));
	break;
      }

    int j = 2*(nRvals/2-1);
    if (realVars[j] < stop && extrap > 0)
    {
      // Extrapolation after last point
      xvec.push_back(stop);
      if (extrap == 1) // Flat extrapolation
	yvec.push_back(realVars[j+1]);
      else if (j > 1) // Linear extrapolation
	yvec.push_back(interpolate(false,stop,
				   realVars[j-2],realVars[j-1],
				   realVars[j  ],realVars[j+1]));
    }
  }

  // *** Functions having a few defining points from parameters

  else if (funcType == SCALE_p)
  {
    if (nRvals < 1) return -3;

    xvec.push_back(start);
    yvec.push_back(realVars[0]*start);
    xvec.push_back(stop);
    yvec.push_back(realVars[0]*stop);
  }

  else if (funcType == CONSTANT_p)
  {
    if (nRvals < 1) return -3;

    xvec.push_back(start);
    yvec.push_back(realVars[0]);
    xvec.push_back(stop);
    yvec.push_back(realVars[0]);
  }

  else if (funcType == DIRAC_PULS_p)
  {
    if (nRvals < 4) return -3;

    bool add1st = false, add2nd = false;

    double startVal   = realVars[0];
    double pulseVal   = startVal + realVars[1];
    double pulseStart = realVars[3] - 0.5*realVars[2];
    double pulseStop  = pulseStart + realVars[2];

    xvec.push_back(start);
    if (start >= pulseStart && start < pulseStop)
      yvec.push_back(pulseVal);
    else
      yvec.push_back(startVal);

    if (start < pulseStart)
      if (stop < pulseStart)
	yvec.push_back(startVal);
      else if (stop < pulseStop)
	add1st = true;
      else
	add1st = add2nd = true;

    else if (start >= pulseStart && start < pulseStop)
      if (stop < pulseStop)
	yvec.push_back(pulseVal);
      else
	add2nd = true;

    else
      yvec.push_back(startVal);

    if (add1st) {
      xvec.push_back(pulseStart);
      xvec.push_back(pulseStart);
      yvec.push_back(startVal);
      yvec.push_back(pulseVal);
      if (!add2nd) yvec.push_back(pulseVal);
    }
    if (add2nd) {
      xvec.push_back(pulseStop);
      xvec.push_back(pulseStop);
      yvec.push_back(pulseVal);
      yvec.push_back(startVal);
      yvec.push_back(startVal);
    }
    xvec.push_back(stop);
  }

  else if (funcType == STEP_p)
  {
    if (nRvals < 3) return -3;

    double startVal  = realVars[0];
    double stepVal   = startVal + realVars[1];
    double stepStart = realVars[2];

    xvec.push_back(start);
    yvec.push_back(start > stepStart ? stepVal : startVal);
    if (stop < stepStart || start > stepStart)
      yvec.push_back(stop > stepStart ? stepVal : startVal);
    else {
      xvec.push_back(stepStart);
      xvec.push_back(stepStart);
      yvec.push_back(startVal);
      yvec.push_back(stepVal);
      yvec.push_back(stepVal);
    }
    xvec.push_back(stop);
  }

  else if (funcType == RAMP_p)
  {
    if (nRvals < 3) return -3;

    xvec.push_back(start);
    yvec.push_back(evalFunc(funcType,start,realVars,extrap,ierr));
    if (start < realVars[2] && stop > realVars[2]) {
      xvec.push_back(realVars[2]);
      yvec.push_back(realVars[0]);
    }
    xvec.push_back(stop);
    yvec.push_back(evalFunc(funcType,stop,realVars,extrap,ierr));
  }

  else if (funcType == LIM_RAMP_p)
  {
    if (nRvals < 4) return -3;

    bool add1st = false, add2nd = false;

    xvec.push_back(start);
    yvec.push_back(evalFunc(funcType,start,realVars,extrap,ierr));
    if (start < realVars[2])
      if (stop < realVars[2])
	yvec.push_back(realVars[0]);
      else if (stop < realVars[3])
	add1st = true;
      else
	add1st = add2nd = true;

    else if (start < realVars[3])
      if (stop < realVars[3])
	yvec.push_back(evalFunc(funcType,stop,realVars,extrap,ierr));
      else
	add2nd = true;

    else
      yvec.push_back(evalFunc(funcType,realVars[3],realVars,extrap,ierr));

    if (add1st) {
      xvec.push_back(realVars[2]);
      yvec.push_back(realVars[0]);
    }
    if (add2nd) {
      xvec.push_back(realVars[3]);
      yvec.push_back(evalFunc(funcType,stop,realVars,extrap,ierr));
    }
    xvec.push_back(stop);
    yvec.push_back(evalFunc(funcType,stop,realVars,extrap,ierr));
  }

  // *** Periodic functions needing more intelligent sampling

  else if (funcType == SINUSOIDAL_p)
  {
    if (nRvals < 5) return -3;

    double omega = realVars[1];
    double tmax  = realVars[4];
    if (tmax <= 0.0) tmax = stop;

    xvec.push_back(start);
    yvec.push_back(evalFunc(funcType,start,realVars,extrap,ierr));

    if (start >= tmax || omega == 0.0)
      yvec.push_back(evalFunc(funcType,tmax,realVars,extrap,ierr));
    else {
      double dt = M_PI*fabs(0.1/omega);
      double t = start + dt;
      int j, n = (int)floor((tmax-start)/dt);
      for (j = 1; j < n && t < stop; j++, t += dt) {
	xvec.push_back(t);
	yvec.push_back(evalFunc(funcType,t,realVars,extrap,ierr));
      }
      if (stop <= tmax)
	yvec.push_back(evalFunc(funcType,stop,realVars,extrap,ierr));
      else {
        yvec.push_back(t = evalFunc(funcType,tmax,realVars,extrap,ierr));
	xvec.push_back(tmax);
	yvec.push_back(t);
      }
    }
    xvec.push_back(stop);
  }

  else if (funcType == COMPL_SINUS_p)
  {
    if (nRvals < 8) return -3;

    double sec1 = realVars[0];
    double sec2 = realVars[1];
    double sec  = sec1 > sec2 ? sec1 : sec2;
    double tmax = realVars[7];
    if (tmax <= 0.0) tmax = stop;

    xvec.push_back(start);
    yvec.push_back(evalFunc(funcType,start,realVars,extrap,ierr));

    if (start >= tmax || sec == 0.0)
      yvec.push_back(evalFunc(funcType,tmax,realVars,extrap,ierr));
    else {
      double dt = fabs(0.05/sec);
      double t = start + dt;
      int j, n = (int)floor((tmax-start)/dt);
      for (j = 1; j < n && t < stop; j++, t += dt) {
	xvec.push_back(t);
	yvec.push_back(evalFunc(funcType,t,realVars,extrap,ierr));
      }
      if (stop <= tmax)
	yvec.push_back(evalFunc(funcType,stop,realVars,extrap,ierr));
      else {
	t = evalFunc(funcType,tmax,realVars,extrap,ierr);
        yvec.push_back(t);
	xvec.push_back(tmax);
	yvec.push_back(t);
      }
    }
    xvec.push_back(stop);
  }

  else if (funcType == DELAYED_COMPL_SINUS_p)
  {
    if (nRvals < 8) return -3;

    double sec1 = realVars[0];
    double sec2 = realVars[1];
    double sec  = sec1 > sec2 ? sec1 : sec2;
    double tmn  = realVars[7];

    xvec.push_back(start);
    yvec.push_back(evalFunc(funcType,start,realVars,extrap,ierr));
    if (stop <= tmn)
      yvec.push_back(evalFunc(funcType,tmn,realVars,extrap,ierr));
    else if (sec1 == 0.0 && sec2 == 0.0)
      yvec.push_back(realVars[6]);
    else {
      if (start < tmn) {
	xvec.push_back(tmn);
	yvec.push_back(evalFunc(funcType,tmn,realVars,extrap,ierr));
      }
      double step = fabs(0.05/sec);
      int j, nval = (int)floor((stop-tmn)/step);
      for (j = 1; j < nval && tmn+j*step < stop; j++) {
	xvec.push_back(tmn+j*step);
	yvec.push_back(evalFunc(funcType,tmn+j*step,realVars,extrap,ierr));
      }
      yvec.push_back(evalFunc(funcType,stop,realVars,extrap,ierr));
    }
    xvec.push_back(stop);
  }

  else if (funcType == SQUARE_PULS_p)
  {
    if (nRvals < 4) return -3;

    double shift  = realVars[3];
    double period = realVars[2];
    double pos    = fmod(start+shift,period);
    double bottom = realVars[0] - realVars[1];
    double top    = realVars[0] + realVars[1];
    xvec.push_back(start);
    if (pos >= 0.5*period)
      yvec.push_back(bottom);
    else {
      yvec.push_back(top);
      xvec.push_back(realVars[2]*0.5 + start-pos);
      xvec.push_back(realVars[2]*0.5 + start-pos);
      yvec.push_back(top);
      yvec.push_back(bottom);
    }

    double firstLeapUp = start+period - pos;
    int n = 2*(int)ceil((stop-start)/realVars[2]);

    for (int k = 0; k < n; k++) {
      double x = firstLeapUp + k*0.5*realVars[2];
      if (x <= stop) {
	xvec.push_back(x);
	xvec.push_back(x);
	yvec.push_back(k%2 == 0 ? bottom : top);
	yvec.push_back(k%2 == 1 ? bottom : top);
      }
      else {
	xvec.push_back(stop);
	yvec.push_back(fmod(stop+shift,period) < 0.5*period ? top : bottom);
	break;
      }
    }
  }

  // *** Other function types not having "smart points" return error

  else
    return -4;

  return ierr < 0 ? -10-ierr : 0;
}


int FFaFunctionProperties::getValue (int baseID,
                                     const std::vector<int>& intVars,
                                     const std::vector<double>& realVars,
                                     double x, double& value)
{
  int ierr = baseID;
  switch (intVars.front())
    {
    case DEVICE_FUNCTION_p:
      value = FiDeviceFunctionFactory::instance()->getValue(intVars[2],x,ierr,
                                                            intVars[3],
                                                            realVars[0],
                                                            realVars[1]);
      break;

    case MATH_EXPRESSION_p:
      value = FFaMathExprFactory::instance()->getValue(intVars[2],x,ierr);
      break;

    case USER_DEFINED_p:
      value = FFaUserFuncPlugin::instance()->getValue(baseID,intVars[2],
                                                      realVars.data(),&x,ierr);
      break;

    default:
      value = 0.0;
      break;
    }

  return ierr;
}


int FFaFunctionProperties::getTypeID (const std::string& functionType)
{
  static std::map<std::string,int> typeMap;
  if (typeMap.empty())
  {
    typeMap["DEVICE_FUNCTION"] = DEVICE_FUNCTION_p;
    typeMap["MATH_EXPRESSION"] = MATH_EXPRESSION_p;
    typeMap["USER_DEFINED"   ] = USER_DEFINED_p;
  }

  std::map<std::string,int>::const_iterator it = typeMap.find(functionType);
  return it == typeMap.end() ? -99 : it->second;
}
