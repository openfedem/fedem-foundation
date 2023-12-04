// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFA_MATH_STRING_H
#define FFA_MATH_STRING_H

#include "FFaMathExpr.H"


namespace FFaMathString
{
  char* CopyStr(const char* s, int i1 = 0, int i2 = 0);
  char* SimplifyStr(char* s);
  char* SupprSpaces(char* s);
  char* InsStr(const char* s, int n, char c);
  bool EqStr(const char* s1, const char* s2);
  bool CompStr(const char* s, int n, const char* s2);
  bool IsNumeric(const char* s);
  int SearchCorOpenbracket(const char* s, int n);
  int SearchCorClosebracket(const char* s, int n);
  int SearchOperator(const char* s, FFaMathExpr::ROperator op);
  int GetFunction(const char* s, int n, FFaMathExpr::ROperator& fn);
  int IsVar(const char* s, int n, int nvar, FFaMathVar** ppvar);
  int IsPi(const char* s, int n = 0);
  int IsFunction(const char* s, int n, int nfunc, FFaMathFunction** ppfunc);
  char* IsolateVars(char* s, int nvar, FFaMathVar** ppvar);
  char* IsolateNumbers(char* s, int nvar, FFaMathVar** ppvar,
                       int nfunc, FFaMathFunction** ppfunc);
}

#endif
