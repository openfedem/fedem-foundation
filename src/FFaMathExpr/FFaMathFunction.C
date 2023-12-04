// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFaMathExpr.H"
#include "FFaMathOps.H"
#include "FFaMathString.H"
#include "FFaMathVar.H"
#include <cstdio>


/*!
  \fn FFaMathFunction::FFaMathFunction()
  \brief An object of type FFaMathFunction is created. Constructor.
*/
FFaMathFunction::FFaMathFunction()
{
  type = -1;
  name = new char[1];
  name[0] = 0;
  nvars = 0;
  ppvar = NULL;
  pfuncval = NULL;
  op = FFaMathOps::ErrVal;
  buf = NULL;
}

/*!
  \fn FFaMathFunction::FFaMathFunction(double(*)(double))
  \brief An object of type FFaMathFunction is created. Constructor.
*/
FFaMathFunction::FFaMathFunction(double (*pfuncvalp)(double))
{
  type = 0;
  pfuncval = pfuncvalp;
  name = new char[1];
  name[0] = 0;
  nvars = 1;
  ppvar = NULL;
  op = FFaMathOps::ErrVal;
  buf = NULL;
}

/*!
  \fn FFaMathFunction::FFaMathFunction(const FFaMathExpr&,FFaMathVar*)
  \brief An object of type FFaMathFunction is created. Constructor.
  \param[in] opp Object of type FFaMathExpr
  \param pvarp Variable of type FFaMathVar
*/
FFaMathFunction::FFaMathFunction(const FFaMathExpr& opp,
				 FFaMathVar* pvarp) : op(opp)
{
  type = 1;
  pfuncval = NULL;
  name = new char[1];
  name[0] = 0;
  nvars = 1;
  ppvar = new FFaMathVar*[1];
  ppvar[0] = pvarp;
  buf = new double[1];
}

/*!
  \fn FFaMathFunction::FFaMathFunction(const FFaMathExpr&,int,FFaMathVar**)
  \brief An object of type FFaMathFunction is created. Constructor.
  \param[in] opp Object of type FFaMathExpr
  \param[in] nvarsp varibale index
  \param ppvarp Variable of type FFaMathVar
*/
FFaMathFunction::FFaMathFunction(const FFaMathExpr& opp, int nvarsp,
				 FFaMathVar** ppvarp) : op(opp)
{
  type = 1;
  pfuncval = NULL;
  name = new char[1];
  name[0] = 0;
  nvars = nvarsp;

  if(nvars) {
    ppvar = new FFaMathVar*[nvars];
    for(int i = 0; i < nvars; i++)
      ppvar[i] = ppvarp[i];
    buf = new double[nvars];
  } else {
    ppvar = NULL;
    buf = NULL;
  }
}

/*!
  \fn FFaMathFunction::~FFaMathFunction()
  \brief Destructor.
*/
FFaMathFunction::~FFaMathFunction()
{
  if(name != NULL)
    delete[]name;

  if(ppvar != NULL)
    delete[]ppvar;

  if(buf != NULL)
    delete[]buf;
}

/*!
  \fn FFaMathFunction::SetName(const char*)
  \brief Sets the name of the function.
  \param[in] s The function name
*/
void FFaMathFunction::SetName(const char*s)
{
  if(name != NULL)
    delete[]name;

  name = FFaMathString::CopyStr(s);
}

/*!
  \fn FFaMathFunction::Val(double*) const
  \brief Returns the function value.
  \param pv Value of the function argument
*/
double FFaMathFunction::Val(double*pv) const
{
  if(type == -1)
    return FFaMathOps::ErrVal;

  if(type == 0)
    return (*pfuncval)(*pv);

  int i;
  for(i = 0; i < nvars; i++){
    buf[i] = *ppvar[i];
    // Warning : could cause trouble if this value is used in a parallel process
    *ppvar[i] = pv[i];
  }

  double y = op.Val();

  for(i = 0; i < nvars; i++)
    *ppvar[i] = buf[i];

  return y;
}


FFaMathExpr FFaMathFunction::operator()(const FFaMathExpr& anExpr)
{
  FFaMathExpr op2;
  op2.op = FFaMathExpr::Fun;
  op2.pfunc = this;
  op2.mmb2 = new FFaMathExpr(anExpr);
  return op2;
}

/*!
  \fn FFaMathFunction::operator==(const FFaMathFunction&) const
  \brief Overloaded equal-to operator.
  \param[in] f2 The FFaMathFunction object to compare with
*/
bool FFaMathFunction::operator== (const FFaMathFunction& f2) const
{
  if(this->type != f2.type)
    return false;

  if(this->type == -1)
    return true; // Nonfunction==nonfunction

  if(this->type == 0)
    return (this->pfuncval == f2.pfuncval && FFaMathString::EqStr(this->name, f2.name));

  if(this->op != f2.op)
    return false;

  if(!FFaMathString::EqStr(this->name, f2.name))
    return false;

  if(this->nvars != f2.nvars)
    return false;

  for(int i = 0; i < this->nvars; i++)
    if(!(*this->ppvar[i] == *f2.ppvar[i]))
      return false;

  return true;
}
