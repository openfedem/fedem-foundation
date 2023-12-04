// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFA_FUNCTION_PROPERTIES_H
#define FFA_FUNCTION_PROPERTIES_H

#include <string>
#include <vector>


/*!
  These enum values must match exactly with the corresponding parameters
  defined in the Fortran file explicitFunctionsModule.f90
*/

enum FFaFunctionType {
  UNDEFINED_p           =  0,
  SINUSOIDAL_p          =  1,
  COMPL_SINUS_p         =  2,
  DELAYED_COMPL_SINUS_p =  3,
  WAVE_SINUS_p          =  4,
  LIN_VEL_VAR_p         =  5,
  SPLINE_p              =  6,
  WAVE_STOKES5_p        =  7,
  WAVE_STREAMLINE_p     =  8,
  WAVE_EMBEDDED_p       =  9,
  LIN_VAR_p             = 10,
  RAMP_p                = 11,
  STEP_p                = 12,
  SQUARE_PULS_p         = 13,
  DIRAC_PULS_p          = 14,
  CONSTANT_p            = 15,
  LIM_RAMP_p            = 16,
  SCALE_p               = 17,
  DEVICE_FUNCTION_p     = 18,
  MATH_EXPRESSION_p     = 19,
  USER_DEFINED_p        = 21,
  SMOOTH_TRAJ_p         = 22
};


namespace FFaFunctionProperties
{
  int getTypeID(const std::string& functionType);

  int getSmartPoints(int funcType, double start, double stop, int extrap,
                     const std::vector<double>& realVars,
                     std::vector<double>& x, std::vector<double>& y);

  int getValue(int baseID,
               const std::vector<int>& intVars,
               const std::vector<double>& realVars,
               double x, double& value);
}

#endif
