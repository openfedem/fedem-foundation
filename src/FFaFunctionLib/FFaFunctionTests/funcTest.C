// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFaFunctionLib/FFaFunctionManager.H"
#include "FFaLib/FFaAlgebra/FFaVec3.H"
#include <cstdlib>


int main (int argc, const char** argv)
{
  if (argc < 3) return 1;
  int n = atoi(argv[2]);

  std::vector<double> rvars;
  if (!FFaFunctionManager::initWaveFunction(argv[1],n,0,rvars))
    return 2;

  FaVec3 X;
  for (int i = 3; i < argc && i-3 < 3; i++)
    X[i-3] = atof(argv[i]);
  double t = argc > 6 ? atof(argv[6]) : 0.0;
  double h = FFaFunctionManager::getWaveValue(rvars,9.81,100.0,X,t);
  std::cout <<"     Wave heigth at X={"<< X <<"} t="<< t <<": "<< h << std::endl;

  return 0;
}
