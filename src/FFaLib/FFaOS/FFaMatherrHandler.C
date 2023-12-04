// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <string>
#include <cmath>

/*!
  \file FFaMatherrHandler.C

  \brief Re-implementation of matherr handling on Windows.

  \details Handles several math errors caused by passing a negative argument
  to log or log10 (_DOMAIN errors). When this happens, _matherr
  returns the natural or base-10 logarithm of the absolute value
  of the argument and suppresses the usual error message.

  When an error occurs in a math routine, _matherr is called with a
  pointer to an _exception type structure (defined in math.h) as an argument.
  The _exception structure contains the following elements:

  - int  type  Exception type.
  - char *name Name of function where error occurred.
  - double arg1, arg2 First and second (if any) arguments to function.
  - double retval Value to be returned by function.

  The type specifies the type of math error.
  It is one of the following values, defined in math.h:

  - _DOMAIN    Argument domain error.
  - _SING      Argument singularity.
  - _OVERFLOW  Overflow range error.
  - _PLOSS     Partial loss of significance.
  - _TLOSS     Total loss of significance.
  - _UNDERFLOW The result is too small to be represented.
               (This condition is not currently supported.)
*/

extern "C" void _stdcall
MATHERRQQ (char* name, const int nchar, int& len, void* typeStruct, int& retval)
{
  struct _exception* except = (_exception*)typeStruct;

  std::cerr << "MATHERR: ";
  if ( except->type == _DOMAIN )
    std::cerr << "Argument domain error (_DOMAIN)";
  else if( except->type == _SING )
    std::cerr << "Argument singularity error (_SING)";
  else if( except->type == _OVERFLOW )
    std::cerr << "Argument overflow range error (_OVERFLOW)";
  else if( except->type == _PLOSS )
    std::cerr << "Argument partial loss of significance error (_PLOSS)";
  else if( except->type == _TLOSS )
    std::cerr << "Argument total loss of significance error (_TLOSS)";
  else if( except->type == _UNDERFLOW )
    std::cerr << "Argument underflow error (_UNDERFLOW)";

  std::cerr <<" in "<< std::string(name,nchar) << std::endl;
  retval = 1;
}
