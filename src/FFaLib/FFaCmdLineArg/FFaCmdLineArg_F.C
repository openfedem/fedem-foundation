// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFaCmdLineArg_F.C
  \brief Fortran wrapper for the FFaCmdLineArg methods.
*/

#include <cstring>

#include "FFaLib/FFaCmdLineArg/FFaCmdLineArg.H"
#include "FFaLib/FFaOS/FFaFortran.H"

//! \cond NO_NOT_DOCUMENT
#ifdef _NCHAR_AFTER_CHARARG
#define REF(T,val,nchar) const int nchar, T& val
#define CHAR(cval,nchar) const int nchar, char* cval
#else
#define REF(T,val,nchar) T& val, const int nchar
#define CHAR(cval,nchar) char* cval, const int nchar
#endif
//! \endcond


/*!
  \brief Adds some common command-line options used by all Fortran modules.
*/

SUBROUTINE (ffa_cmdlinearg_init,FFA_CMDLINEARG_INIT) ()
{
  FFaCmdLineArg::instance()->addOption("debug",0,"Debug print switch");
  FFaCmdLineArg::instance()->addOption("terminal",6,"Console file unit number");
  FFaCmdLineArg::instance()->addOption("consolemsg",false,
                                       "Output error messages to console");
}


/*!
  \brief Prints out all specified command-line options.
*/

SUBROUTINE (ffa_cmdlinearg_list,FFA_CMDLINEARG_LIST) (const int& noDefault)
{
  FFaCmdLineArg::instance()->listOptions (noDefault);
}


/*!
  \brief Returns the actual value of the specified integer command-line option.
*/

SUBROUTINE (ffa_cmdlinearg_getint,FFA_CMDLINEARG_GETINT) (const char* id,
                                                          REF(int,val,nchar))
{
  val = 0;
  if (FFaCmdLineArg::empty()) return;
  FFaCmdLineArg::instance()->getValue (std::string(id,nchar),val);
}


/*!
  \brief Returns the actual value of the specified float command-line option.
*/

SUBROUTINE (ffa_cmdlinearg_getfloat,FFA_CMDLINEARG_GETFLOAT) (const char* id,
                                                              REF(float,val,
                                                                  nchar))
{
  val = 0.0;
  if (FFaCmdLineArg::empty()) return;
  FFaCmdLineArg::instance()->getValue (std::string(id,nchar),val);
}


/*!
  \brief Returns the actual value of the specified double command-line option.
*/

SUBROUTINE (ffa_cmdlinearg_getdouble,FFA_CMDLINEARG_GETDOUBLE) (const char* id,
                                                                REF(double,val,
                                                                    nchar))
{
  val = 0.0;
  if (FFaCmdLineArg::empty()) return;
  FFaCmdLineArg::instance()->getValue (std::string(id,nchar),val);
}


/*!
  \brief Returns the actual values of the specified integer command-line option.
*/

SUBROUTINE (ffa_cmdlinearg_getints,
            FFA_CMDLINEARG_GETINTS) (const char* id,
#ifdef _NCHAR_AFTER_CHARARG
                                     const int nchar,
                                     int* val, const int& nval
#else
                                     int* val, const int& nval,
                                     const int nchar
#endif
){
  IntVec values;
  if (!FFaCmdLineArg::empty())
    FFaCmdLineArg::instance()->getValue (std::string(id,nchar),values);
  for (int i = 0; i < nval; i++)
    val[i] = i < (int)values.size() ? values[i] : 0;
}


/*!
  \brief Returns the actual values of the specified double command-line option.
*/

SUBROUTINE (ffa_cmdlinearg_getdoubles,
            FFA_CMDLINEARG_GETDOUBLES) (const char* id,
#ifdef _NCHAR_AFTER_CHARARG
                                        const int nchar,
                                        double* val, const int& nval
#else
                                        double* val, const int& nval,
                                        const int nchar
#endif
){
  DoubleVec values;
  if (!FFaCmdLineArg::empty())
    FFaCmdLineArg::instance()->getValue (std::string(id,nchar),values);
  for (int i = 0; i < nval; i++)
    val[i] = i < (int)values.size() ? values[i] : 0.0;
}


/*!
  \brief Returns the actual value of the specified bool command-line option.
*/

SUBROUTINE (ffa_cmdlinearg_getbool,FFA_CMDLINEARG_GETBOOL) (const char* id,
                                                            REF(int,val,nchar))
{
  bool value = false;
  if (!FFaCmdLineArg::empty())
    FFaCmdLineArg::instance()->getValue (std::string(id,nchar),value);
  val = value ? 1 : 0;
}


/*!
  \brief Returns the actual value of the specified string command-line option.
*/

SUBROUTINE (ffa_cmdlinearg_getstring,FFA_CMDLINEARG_GETSTRING) (const char* id,
                                                                CHAR(val,nchar),
                                                                const int m)
{
  std::string value;
  if (!FFaCmdLineArg::empty())
    FFaCmdLineArg::instance()->getValue (std::string(id,nchar),value);
  int nval = value.length();
  if (nval > m) nval = m;
  if (nval > 0) strncpy(val,value.c_str(),nval);
  if (nval < m) memset(val+nval,' ',m-nval);
}


/*!
  \brief Checks if the option \a id is specified on the command-line.
*/

INTEGER_FUNCTION (ffa_cmdlinearg_isset,FFA_CMDLINEARG_ISSET) (const char* id,
                                                              const int nchar)
{
  if (FFaCmdLineArg::empty()) return 0;
  return FFaCmdLineArg::instance()->isOptionSetOnCmdLine(std::string(id,nchar));
}
