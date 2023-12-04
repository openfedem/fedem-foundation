// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFA_MATH_VAR_H
#define FFA_MATH_VAR_H

#include <cstddef>


class FFaMathVar
{
  double pval;

public:
  char* name;

  FFaMathVar() : pval(0.0), name(NULL) {}
  FFaMathVar(const FFaMathVar&) = delete;
  FFaMathVar(const char*);
  ~FFaMathVar();

  void rename(const char*);
  bool operator==(const FFaMathVar&) const;
  FFaMathVar& operator=(double x) { pval = x; return *this; }
  FFaMathVar& operator=(const FFaMathVar&) = delete;
  operator double() const { return pval; }
  const double* ptr() const { return &pval; }
};

#endif
