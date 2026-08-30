// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFaFunctionProperties.H
  \brief Smart-point evaluation for some closed-form explicit functions.
*/

#ifndef FFA_FUNCTION_PROPERTIES_H
#define FFA_FUNCTION_PROPERTIES_H

#include <string>
#include <vector>


/*!
  \brief Function type enum values.
  \details These values must match exactly with the corresponding parameters
  defined in the Fortran file explicitFunctionsModule.f90
*/

enum FFaFunctionType {
  UNDEFINED_p           =  0, //!< Unknown or undefined function
  SINUSOIDAL_p          =  1, //!< Simple sine function
  COMPL_SINUS_p         =  2, //!< Two-wave sine function with delay
  DELAYED_COMPL_SINUS_p =  3, //!< Two-wave sine function with delay
  WAVE_SINUS_p          =  4, //!< Complex multi-component sine function
  LIN_VEL_VAR_p         =  5, //!< Servo simulator function
  SPLINE_p              =  6, //!< Spline function
  WAVE_STOKES5_p        =  7, //!< 5th order Stokes wave function
  WAVE_STREAMLINE_p     =  8, //!< Nonlinear streamline wave function
  WAVE_EMBEDDED_p       =  9, //!< Irregular waves with embedded streamlines
  LIN_VAR_p             = 10, //!< Piece-wise linear function
  RAMP_p                = 11, //!< Ramp function
  STEP_p                = 12, //!< Step function
  SQUARE_PULS_p         = 13, //!< Square pulse function
  DIRAC_PULS_p          = 14, //!< Dirac pulse function
  CONSTANT_p            = 15, //!< Constant function
  LIM_RAMP_p            = 16, //!< Extended ramp function
  SCALE_p               = 17, //!< Linear scaling function
  DEVICE_FUNCTION_p     = 18, //!< External device function
  MATH_EXPRESSION_p     = 19, //!< General match expression function
  USER_DEFINED_p        = 21, //!< User-defined function (from plug-in)
  SMOOTH_TRAJ_p         = 22  //!< Smooth trajectory function
};


namespace FFaFunctionProperties //! Explicit function properties
{
  //! \brief Returns the function type ID for a given \a functionType name.
  int getTypeID(const std::string& functionType);

  //! \brief Finds the so-called smart points of an explicit function.
  //! \param[in] funcType Function type identifier
  //! \param[in] start Start argument value of the function domain to consider
  //! \param[in] stop Stop argument value of the function domain to consider
  //! \param[in] extrap Extrapolation flag
  //! \param[in] realVars Real function parameters
  //! \param[out] x Function argument values of the smart points
  //! \param[out] y Function values of the smart points
  int getSmartPoints(int funcType, double start, double stop, int extrap,
                     const std::vector<double>& realVars,
                     std::vector<double>& x, std::vector<double>& y);

  //! \brief Evaluates an explicit function
  //! \param[in] baseID base ID of the function to be evaluated
  //! \param[in] intVars Integer function parameters
  //! \param[in] realVars Real function parameters
  //! \param[in] x Function argument
  //! \param[out] value The function value
  int getValue(int baseID,
               const std::vector<int>& intVars,
               const std::vector<double>& realVars,
               double x, double& value);
}

#endif
