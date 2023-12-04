// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFP_READ_RESULTS_H
#define FFP_READ_RESULTS_H

#include <vector>
#include <string>

class FFrExtractor;

typedef std::vector< std::vector<double> > DoubleVectors;


struct FFpVar
{
  std::string name;
  std::string type;
  std::string oper;
  FFpVar(const char* n = NULL, const char* t = NULL, const char* o = NULL);
};


namespace FFp
{
  bool readHistories(const char* objType,
		     const std::vector<int>& baseIds,
		     const std::vector<FFpVar>& vars,
		     FFrExtractor* extractor,
		     double& Tmin, double& Tmax, bool includeTime,
		     DoubleVectors& values, std::string& message);
}

#endif
