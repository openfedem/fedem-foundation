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


class FFaFunctionManager
{
public:
  static double getValue(int baseID,
			 const std::vector<int>& intVars,
			 const std::vector<double>& realVars,
			 const double x, int& ierr);

  static double getValue(int baseID, int fType, int extrap,
			 const std::vector<double>& realVars,
			 const double x, int& ierr);

  static double getDerivative(int baseID, int fType, int extrap,
			      const std::vector<double>& realVars,
			      const double x, int& ierr);

  static int getSmartPoints(int funcType, int extrap,
			    const double start, const double stop,
			    const std::vector<double>& realVars,
			    std::vector<double>& xtofill,
			    std::vector<double>& ytofill);

  static int getTypeID(const std::string& functionType);

  static bool initWaveFunction(const std::string& fileName, const int nWave,
			       const int rSeed, std::vector<double>& realVars);
  static bool initWaveFunction(const int iop, const int nWave, const int nDir,
			       const int sprExp, const int rSeed,
			       std::vector<double>& realVars);
  static bool initWaveFunction(const int iop, const double g, double d,
			       std::vector<double>& realVars);
  static bool initWaveFunction (const int iop, const int nWave, const int rSeed,
				const double g, const double d,
				std::vector<int>& intVars,
				std::vector<double>& realVars);

  static double getWaveValue(const std::vector<double>& realVars,
			     const double g, const double d, const FaVec3& x,
			     const double t, int iop = 0);
  static double getWaveValue(const std::vector<int>& intVars,
  			     const std::vector<double>& realVars,
			     const double g, const double d, const FaVec3& x,
			     const double t, int iop = 0);
  static double getWaveValue(const std::vector<int>& intVars,
  		             const std::vector<double>& realVars,
			     const double g, const double d, const FaVec3& x,
			     const double t, FaVec3& v, FaVec3& a, int iop = 0);
};

#endif
