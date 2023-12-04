// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFaMathExpr/FFaMathExprFactory.H"
#include "FFaMathExpr.H"
#include "FFaMathVar.H"
#include "FFaMathOps.H"


FFaMathExprFactory::FFaMathFunc::~FFaMathFunc()
{
  if (expr) delete expr;
  for (size_t i = 0; i < args.size(); i++)
  {
    if (args[i]) delete args[i];
    if (diff[i]) delete diff[i];
  }
}


size_t FFaMathExprFactory::countArgs(const std::string& expression,
                                     const char** vars,
                                     std::vector<bool>* present)
{
  if (present) present->clear();
  if (expression.empty()) return 0;

  size_t nvar = 0;
  for (size_t i = 0; vars[i]; i++)
    if (expression.find(vars[i]) < expression.size())
    {
      if (present)
      {
        present->resize(i,false);
        present->push_back(true);
      }
      else
        nvar = 1+i;
    }

  return present ? present->size() : nvar;
}


int FFaMathExprFactory::create(int id, const std::string& expression,
                               size_t nvar, const char** vars)
{
  IMIter im = myIndexMap.find(id);
  if (im != myIndexMap.end())
  {
    if (im->second.estr == expression && nvar == im->second.args.size())
      return im->first;
    else
      myIndexMap.erase(im);
  }

  if (expression.empty()) return -1;
  if (nvar < 1) return -2;

  // Default variable names
  const char* defvar[4] = { "x", "y", "z", "t" };
  if (nvar > 4 && !vars) return -(int)nvar;

  for (size_t i = 0; i < nvar; i++)
  {
    const char* varName = vars ? vars[i] : defvar[i];
    if (i < myIndexMap[id].args.size())
      myIndexMap[id].args[i]->rename(varName);
    else
      myIndexMap[id].args.push_back(new FFaMathVar(varName));
  }
  myIndexMap[id].expr = new FFaMathExpr(expression.c_str(), nvar,
                                        &myIndexMap[id].args.front());
  myIndexMap[id].diff.resize(nvar,0);
  return myIndexMap[id].expr->HasError() ? -3 : id;
}


double FFaMathExprFactory::getValue(int id, double arg, int& error)
{
  error = -1;
  IMIter im = myIndexMap.find(id);
  if (im == myIndexMap.end()) return 0.0;

  *im->second.args.front() = arg;
  for (size_t i = 1; i < im->second.args.size(); i++)
    *im->second.args[i] = 0.0;

  error = 0;
  double val = im->second.expr->Val();
  if (val == FFaMathOps::ErrVal)
    error = 1; // error (e.g. divsion by a very small number)
  return val;
}


double FFaMathExprFactory::getValue(int id, const double* arg, int& error)
{
  error = -1;
  IMIter im = myIndexMap.find(id);
  if (im == myIndexMap.end()) return 0.0;

  for (size_t i = 0; i < im->second.args.size(); i++)
    *im->second.args[i] = arg[i];

  error = 0;
  double val = im->second.expr->Val();
  if (val == FFaMathOps::ErrVal)
    error = 1; // error (e.g. divsion by a very small number)
  return val;
}


double FFaMathExprFactory::getDiff(int id, double arg, int& error)
{
  error = -1;
  IMIter mit = myIndexMap.find(id);
  if (mit == myIndexMap.end()) return 0.0;

  FFaMathFunc& mf = mit->second;
  if (!mf.diff.front())
    mf.diff.front() = new FFaMathExpr(mf.expr->Diff(*mf.args.front()));

  *mf.args.front() = arg;
  for (size_t i = 1; i < mf.args.size(); i++)
    *mf.args[i] = 0.0;

  error = 0;
  double val = mf.diff.front()->Val();
  if (val == FFaMathOps::ErrVal)
    error = 1; // error (e.g. divsion by a very small number)
  return val;
}


double FFaMathExprFactory::getDiff(int id, size_t idArg,
                                   const double* arg, int& error)
{
  error = -1;
  IMIter mit = myIndexMap.find(id);
  if (mit == myIndexMap.end()) return 0.0;

  FFaMathFunc& mf = mit->second;
  if (idArg < 1 || idArg > mf.args.size()) return 0.0;

  if (!mf.diff[--idArg])
    mf.diff[idArg] = new FFaMathExpr(mf.expr->Diff(*mf.args.front()));

  for (size_t i = 0; i < mf.args.size(); i++)
    *mf.args[i] = arg[i];

  error = 0;
  double val = mf.diff[idArg]->Val();
  if (val == FFaMathOps::ErrVal)
    error = 1; // error (e.g. divsion by a very small number)
  return val;
}
