// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFA_FUNCTION_MANAGER_H
#define FFA_FUNCTION_MANAGER_H

#include <vector>
#include <string>

class FaVec3;


namespace FFaFunctionManager
{
  using IntVec  = std::vector<int>;
  using RealVec = std::vector<double>;

  double getValue(int baseID, const IntVec& intVars,
                  const RealVec& realVars, const double x, int& ierr);

  double getValue(int baseID, int fType, int extrap,
                  const RealVec& realVars, const double x, int& ierr);

  double getDerivative(int baseID, int fType, int extrap,
                       const RealVec& realVars, const double x, int& ierr);

  int getSmartPoints(int funcType, int extrap,
                     const double start, const double stop,
                     const RealVec& realVars,
                     RealVec& xtofill, RealVec& ytofill);

  int getTypeID(const std::string& functionType);

  bool initWaveFunction(const std::string& fileName, const int nWave,
                        const int rSeed, RealVec& realVars);
  bool initWaveFunction(const int iop, const int nWave, const int nDir,
                        const int sprExp, const int rSeed, RealVec& realVars);
  bool initWaveFunction(const int iop, const double g, double d,
                        RealVec& realVars);
  bool initWaveFunction(const int iop, const int nWave, const int rSeed,
                        const double g, const double d,
                        IntVec& intVars, RealVec& realVars);

  double getWaveValue(const RealVec& realVars,
                      const double g, const double d, const FaVec3& x,
                      const double t, int iop = 0);
  double getWaveValue(const IntVec& intVars, const RealVec& realVars,
                      const double g, const double d, const FaVec3& x,
                      const double t, int iop = 0);
  double getWaveValue(const IntVec& intVars, const RealVec& realVars,
                      const double g, const double d, const FaVec3& x,
                      const double t, FaVec3& v, FaVec3& a, int iop = 0);
}

#endif
