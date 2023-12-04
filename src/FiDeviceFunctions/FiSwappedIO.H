// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FI_SWAPPED_IO_H
#define FI_SWAPPED_IO_H

#include "FFaLib/FFaOS/FFaIO.H"


namespace Fi
{
  template<class T> size_t readSwapped(char* q, T&, FT_FILE fd)
  {
    static char p[8];
    int nChar = sizeof(T);
    size_t nBytes = FT_read(p,nChar,1,fd);
    for (int i = nChar-1; i > 0; i--, q++)
      *q = p[i];
    *q = p[0];

    return nBytes;
  }

  template<class T> size_t writeSwapped(const T val, FT_FILE fd)
  {
    T swappedValue;
    int nChar = sizeof(T);
    char* p = (char*)&val;
    char* q = (char*)(&swappedValue);
    for (int i = nChar-1; i > 0; i--, q++)
      *q = p[i];
    *q = p[0];

    return FT_write(&swappedValue,nChar,1,fd);
  }
}

#endif
