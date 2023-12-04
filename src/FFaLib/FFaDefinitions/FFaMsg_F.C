// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include <cstdio>

#include "FFaLib/FFaDefinitions/FFaMsg.H"
#include "FFaLib/FFaOS/FFaFortran.H"


SUBROUTINE (ffamsg_list,FFAMSG_LIST) (const char* text,
#ifdef _NCHAR_AFTER_CHARARG
                                      const int nchar, const int* ival = 0
#else
                                      const int* ival = 0, const int nchar = 0
#endif
){
  if (ival)
  {
    // An optional integer value was given, append that to the message
    char strv[16];
    snprintf(strv,16," %d\n",*ival);
    FFaMsg::list(std::string(text,nchar) + strv);
  }
  else
    FFaMsg::list(std::string(text,nchar) + "\n");
}
