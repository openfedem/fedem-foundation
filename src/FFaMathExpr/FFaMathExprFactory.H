// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFA_MATH_EXPR_FACTORY_H
#define FFA_MATH_EXPR_FACTORY_H

#include "FFaLib/FFaPatterns/FFaSingelton.H"
#include <string>
#include <vector>
#include <map>

class FFaMathExpr;
class FFaMathVar;


class FFaMathExprFactory : public FFaSingelton<FFaMathExprFactory>
{
  class FFaMathFunc
  {
  public:
    std::string               estr;
    FFaMathExpr*              expr;
    std::vector<FFaMathExpr*> diff;
    std::vector<FFaMathVar*>  args;
    FFaMathFunc() { expr = NULL; }
    FFaMathFunc(const FFaMathFunc&) = delete;
    FFaMathFunc& operator=(const FFaMathFunc&) = delete;
    ~FFaMathFunc();
  };

  typedef std::map<int,FFaMathFunc> IndexMap;
  typedef IndexMap::iterator        IMIter;

protected:
  FFaMathExprFactory() {}
  virtual ~FFaMathExprFactory() {}

public:
  static size_t countArgs(const std::string& expression, const char** vars,
			  std::vector<bool>* present = 0);

  int create(int id, const std::string& expression,
	     size_t nvar = 1, const char** vars = 0);

  double getValue(int id, double arg, int& error);
  double getValue(int id, const double* arg, int& error);

  double getDiff(int id, double arg, int& error);
  double getDiff(int id, size_t idArg, const double* arg, int& error);

private:
  IndexMap myIndexMap;

  friend class FFaSingelton<FFaMathExprFactory>;
};

#endif
