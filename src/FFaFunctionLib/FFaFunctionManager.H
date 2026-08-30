// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFaFunctionManager.H
  \brief Global functions direct evaluation of some explicit function types.
*/

#ifndef FFA_FUNCTION_MANAGER_H
#define FFA_FUNCTION_MANAGER_H

#include <vector>
#include <string>

class FaVec3;


namespace FFaFunctionManager //! Explicit function manager
{
  using IntVec  = std::vector<int>;    //!< Convenience type alias
  using RealVec = std::vector<double>; //!< Convenience type alias

  //! \brief Evaluates an explicit function.
  //! \param[in] baseID base ID of the function to be evaluated
  //! \param[in] intVars Integer function parameters
  //! \param[in] realVars Real function parameters
  //! \param[in] x Function argument
  //! \param[out] ierr Error flag
  double getValue(int baseID, const IntVec& intVars,
                  const RealVec& realVars, const double x, int& ierr);

  //! \brief Evaluates an explicit function.
  //! \param[in] baseID base ID of the function to be evaluated
  //! \param[in] fType Function type identifier
  //! \param[in] extrap Extrapolation flag
  //! \param[in] realVars Real function parameters
  //! \param[in] x Function argument
  //! \param[out] ierr Error flag
  double getValue(int baseID, int fType, int extrap,
                  const RealVec& realVars, const double x, int& ierr);

  //! \brief Evaluates the derivative of an explicit function.
  //! \param[in] baseID base ID of the function to be evaluated
  //! \param[in] fType Function type identifier
  //! \param[in] extrap Extrapolation flag
  //! \param[in] realVars Real function parameters
  //! \param[in] x Function argument
  //! \param[out] ierr Error flag
  double getDerivative(int baseID, int fType, int extrap,
                       const RealVec& realVars, const double x, int& ierr);

  //! \brief Finds the so-called smart points of an explicit function.
  //! \param[in] funcType Function type identifier
  //! \param[in] extrap Extrapolation flag
  //! \param[in] start Start argument value of the function domain to consider
  //! \param[in] stop Stop argument value of the function domain to consider
  //! \param[in] realVars Real function parameters
  //! \param[out] xtofill Function argument values of the smart points
  //! \param[out] ytofill Function values of the smart points
  int getSmartPoints(int funcType, int extrap,
                     const double start, const double stop,
                     const RealVec& realVars,
                     RealVec& xtofill, RealVec& ytofill);

  //! \brief Returns the function type ID for a given \a functionType name.
  int getTypeID(const std::string& functionType);

  //! \brief Initializes parameters for a wave function from given file.
  //! \param[in] fileName Name of file to read parameters from
  //! \param[in] nWave Number of wave components to read
  //! \param[in] rSeed Seed for (pseudo-)random phase shift
  //! \param[out] realVars Real wave function parameters
  bool initWaveFunction(const std::string& fileName, const int nWave,
                        const int rSeed, RealVec& realVars);
  //! \brief Initializes parameters for a wave function from a wave spectrum.
  //! \param[in] iop Wave spectrum model
  //! \param[in] nWave Number of wave components to generate
  //! \param[in] nDir Number of wave directions
  //! \param[in] sprExp Wave spreading exponent
  //! \param[in] rSeed Seed for (pseudo-)random phase shift
  //! \param[out] realVars Real wave function parameters
  bool initWaveFunction(const int iop, const int nWave, const int nDir,
                        const int sprExp, const int rSeed, RealVec& realVars);
  //! \brief Initializes parameters for a non-linear wave function.
  //! \param[in] iop Wave model option
  //! \param[in] g Gravity constant
  //! \param[in] d Water depth
  //! \param[out] realVars Real wave function parameters
  bool initWaveFunction(const int iop, const double g, double d,
                        RealVec& realVars);
  //! \brief Initializes parameters for an embedded non-linear wave function.
  //! \param[in] iop Wave model option
  //! \param[in] nWave Number of wave components
  //! \param[in] rSeed Seed for (pseudo-)random phase shift
  //! \param[in] g Gravity constant
  //! \param[in] d Water depth
  //! \param[out] intVars Integer wave function parameters
  //! \param[out] realVars Real wave function parameters
  bool initWaveFunction(const int iop, const int nWave, const int rSeed,
                        const double g, const double d,
                        IntVec& intVars, RealVec& realVars);

  //! \brief Evaluates a wave profile function.
  //! \param[in] realVars Real wave function parameters
  //! \param[in] g Gravity constant
  //! \param[in] d Water depth
  //! \param[in] x Position to evaluate wave profile at
  //! \param[in] t Time to evaluate wave profile at
  //! \param[in] iop Wave model option
  //! \return Wave elevation
  double getWaveValue(const RealVec& realVars,
                      const double g, const double d, const FaVec3& x,
                      const double t, int iop = 0);
  //! \brief Evaluates a wave profile function.
  //! \param[in] intVars Integer wave function parameters
  //! \param[in] realVars Real wave function parameters
  //! \param[in] g Gravity constant
  //! \param[in] d Water depth
  //! \param[in] x Position to evaluate wave profile at
  //! \param[in] t Time to evaluate wave profile at
  //! \param[in] iop Wave model option
  //! \return Wave elevation
  double getWaveValue(const IntVec& intVars, const RealVec& realVars,
                      const double g, const double d, const FaVec3& x,
                      const double t, int iop = 0);
  //! \brief Evaluates a wave profile including velocity and acceleration.
  //! \param[in] intVars Integer wave function parameters
  //! \param[in] realVars Real wave function parameters
  //! \param[in] g Gravity constant
  //! \param[in] d Water depth
  //! \param[in] x Position to evaluate wave profile at
  //! \param[in] t Time to evaluate wave profile at
  //! \param[out] v Water particle velocity
  //! \param[out] a Water particle acceleration
  //! \param[in] iop Wave model option
  //! \return Wave elevation
  double getWaveValue(const IntVec& intVars, const RealVec& realVars,
                      const double g, const double d, const FaVec3& x,
                      const double t, FaVec3& v, FaVec3& a, int iop = 0);
}

#endif
