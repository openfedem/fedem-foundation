// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFaMathExpr/FFaMathExprFactory.H"
#include "FFaLib/FFaOS/FFaFortran.H"


////////////////////////////////////////////////////////////////////////////////
//! \brief Creates a math expression.
//!
//! \arg narg  - Number of arguments
//! \arg expr  - User-defined function expression
//! \arg expId - Associated expression id
//!
//! \note Existing expressions are replaced.
////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(ffame_create,FFAME_CREATE) (const int& narg, const char* expr,
#ifdef _NCHAR_AFTER_CHARARG
                                       const int nchar,
                                       const int& expId, int& error
#else
                                       const int& expId, int& error,
                                       const int nchar
#endif
){
  error = FFaMathExprFactory::instance()->create(expId,
						 std::string(expr,nchar),narg);
}


////////////////////////////////////////////////////////////////////////////////
//! \brief Returns a value from the expression.
//!
//! \arg exprId - Id of the expression to evaluate
//! \arg arg    - Argument to the function expression
//! \return Corresponding function value, f(arg)
////////////////////////////////////////////////////////////////////////////////

DOUBLE_FUNCTION(ffame_getvalue,FFAME_GETVALUE) (const int& exprId,
                                                const double& arg, int& error)
{
  return FFaMathExprFactory::instance()->getValue(exprId,arg,error);
}


////////////////////////////////////////////////////////////////////////////////
//! \brief Returns a value from the multi-variable expression.
//!
//! \arg exprId - Id of the expression to evaluate
//! \arg arg    - Arguments to the function expression
//! \return Corresponding function value, f(arg[0],arg[1],...)
////////////////////////////////////////////////////////////////////////////////

DOUBLE_FUNCTION(ffame_getvalue2,FFAME_GETVALUE2) (const int& exprId,
						  const double* arg, int& error)
{
  return FFaMathExprFactory::instance()->getValue(exprId,arg,error);
}


////////////////////////////////////////////////////////////////////////////////
//! \brief Returns a value from the differentiated expression.
//!
//! \arg exprId - Id of the expression to evaluate
//! \arg arg    - Argument to the function expression
//! \return Corresponding function derivative, i.e., f'(arg)
////////////////////////////////////////////////////////////////////////////////

DOUBLE_FUNCTION(ffame_getdiff,FFAME_GETDIFF) (const int& exprId,
					      const double& arg, int& error)
{
  return FFaMathExprFactory::instance()->getDiff(exprId,arg,error);
}


////////////////////////////////////////////////////////////////////////////////
//! \brief Returns a value from the differentiated multi-variable expression.
//!
//! \arg exprId - Id of the expression to evaluate
//! \arg i      - Argument index to differentiate the expression with respect to
//! \arg arg    - Arguments to the function expression
//! \return Corresponding function derivative, df/darg[i](arg[0],arg[1],...)
////////////////////////////////////////////////////////////////////////////////

DOUBLE_FUNCTION(ffame_getdiff2,FFAME_GETDIFF2) (const int& exprId, const int& i,
						const double* arg, int& error)
{
  return FFaMathExprFactory::instance()->getDiff(exprId,i,arg,error);
}
