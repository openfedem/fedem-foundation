// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFaLib/FFaContainers/FFaByteArray.H"
#include <cctype>


/*!
  Write byte array as as Hex code.
  Each byte two letters [0-9a-f].
*/

std::ostream& operator<< (std::ostream& s, const FFaByteArray& array)
{
  for (size_t i = 0; i < array.size(); i++)
  {
    if (i > 0) s <<" ";
    s << "0123456789abcdef"[ array[i]       & 0xF]
      << "0123456789abcdef"[(array[i] >> 4) & 0xF];
  }

  return s;
}


/*!
  Read bytes as hex codes.
*/

std::istream& operator>> (std::istream& is, FFaByteArray& array)
{
  array.clear();

  char c1, c2;
  while (is)
  {
    is >> c1;
    if (is && isxdigit(c1))
    {
      is >> c2;
      if (is && isxdigit(c2))
      {
        char out = 0;
        if (isdigit(c1))
          out |= (int)c1 - (int)'0';
        else
          out |= (10 + toupper(c1) - (int)'A');

        if (isdigit(c2))
          out |= ((int)c2 - (int)'0') << 4;
        else
          out |= (10 + toupper(c2) - (int)'A') << 4;

        array.push_back(out);
      }
      else
      {
        is.putback(c2);
        break;
      }
    }
    else
    {
      is.putback(c1);
      break;
    }
  }

  is.clear();
  return is;
}
