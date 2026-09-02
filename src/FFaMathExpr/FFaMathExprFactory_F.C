// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFaMathExprFactory_F.C
  \brief Fortran wrapper for the FFaMathExprFactory methods.
  \details This file contains the implementation of the following
  Fortran wrappers of the FFaMathExprFactory module:

  - ffamathexprinterface::ffame_create
  - ffamathexprinterface::ffame_getvalue
  - ffamathexprinterface::ffame_getvalue2
  - ffamathexprinterface::ffame_getdiff
  - ffamathexprinterface::ffame_getdiff2

  No further documentation is provided here.
  The wrappers are documented in the FFaMathExprInterface.f90 file.
*/

#include "FFaMathExpr/FFaMathExprFactory.H"
#include "FFaLib/FFaOS/FFaFortran.H"


////////////////////////////////////////////////////////////////////////////////
//! \brief Creates a math expression.

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

DOUBLE_FUNCTION(ffame_getvalue,FFAME_GETVALUE) (const int& exprId,
                                                const double& arg, int& error)
{
  return FFaMathExprFactory::instance()->getValue(exprId,arg,error);
}


////////////////////////////////////////////////////////////////////////////////
//! \brief Returns a value from the multi-variable expression.

DOUBLE_FUNCTION(ffame_getvalue2,FFAME_GETVALUE2) (const int& exprId,
						  const double* arg, int& error)
{
  return FFaMathExprFactory::instance()->getValue(exprId,arg,error);
}


////////////////////////////////////////////////////////////////////////////////
//! \brief Returns a value from the differentiated expression.

DOUBLE_FUNCTION(ffame_getdiff,FFAME_GETDIFF) (const int& exprId,
					      const double& arg, int& error)
{
  return FFaMathExprFactory::instance()->getDiff(exprId,arg,error);
}


////////////////////////////////////////////////////////////////////////////////
//! \brief Returns a value from the differentiated multi-variable expression.

DOUBLE_FUNCTION(ffame_getdiff2,FFAME_GETDIFF2) (const int& exprId, const int& i,
						const double* arg, int& error)
{
  return FFaMathExprFactory::instance()->getDiff(exprId,i,arg,error);
}
