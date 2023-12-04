// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include <cstdio>
#include <climits>

#include "FFaLib/FFaContainers/FFaFieldBase.H"


bool operator== (const FFaFieldBase& lhs, const std::string& rhs)
{
  return lhs.equalTo(rhs);
}

bool operator!= (const FFaFieldBase& lhs, const std::string& rhs)
{
  return !lhs.equalTo(rhs);
}

bool operator>= (const FFaFieldBase& lhs, const std::string& rhs)
{
  return !lhs.lessThan(rhs);
}

bool operator<= (const FFaFieldBase& lhs, const std::string& rhs)
{
  return !lhs.greaterThan(rhs);
}

bool operator> (const FFaFieldBase& lhs, const std::string& rhs)
{
  return lhs.greaterThan(rhs);
}

bool operator< (const FFaFieldBase& lhs, const std::string& rhs)
{
  return lhs.lessThan(rhs);
}


std::ostream& operator<< (std::ostream& os, const FFaFieldBase& field)
{
  field.write(os);
  return os;
}


std::istream& operator>> (std::istream& is, FFaFieldBase& field)
{
  field.read(is);
  return is;
}


void FFaFieldBase::writeString (std::ostream& os, const std::string& str,
                                const char* pfx)
{
  if (pfx) os << pfx;
  os <<'\"'<< str <<'\"';
}


void FFaFieldBase::readString (std::istream& is, std::string& str)
{
  char c, tmp[BUFSIZ];
  while (is.get(c) && (c != '\"'));

  do
  {
    if (c != '\"') is.putback(c);
    is.get(tmp,BUFSIZ-1,'\"');
    str += tmp;
  }
  while (is.get(c) && (c != '\"'));
}


void FFaFieldBase::readStrings (std::istream& is, Strings& str)
{
  str.clear();

  char c;
  std::string tmp;
  bool reading = false;
  size_t realSize = 0;

  while (is.get(c))
    if (c == '\"') // A "-character means that we either are about to
    {
      if (reading) // read into the string, or have just finished one
      {
        str.push_back(tmp);
        if (!tmp.empty())
          realSize = str.size(); // we read a non-empty string

        tmp.clear();
        reading = false;
      }
      else
        reading = true;
    }
    else if (reading)
      tmp.append(1,c); // we are reading the string

    else if (!isspace(c))
      break; // encountered a non-space character between strings, bail out

  is.putback(c);

  str.resize(realSize); // removing trailing empty strings (if any)
}


bool FFaFieldBase::readInt (std::istream& is, int& val, bool silence)
{
  // Catch integer overflow by reading into a long long int
  // and check against INT_MAX
  long long int lval = 0;
  is >> lval;

  if (!is)
  {
    if (!silence)
      std::cerr <<" *** FFaFieldBase::readInt: Read failure "<< lval
                << std::endl;
    val = 0;
    return false;
  }
  else if (lval > INT_MAX || -lval > INT_MAX)
  {
    std::cerr <<" *** FFaFieldBase::readInt: Integer overflow ("<< lval;
    val = lval > 0 ? INT_MAX : -INT_MAX;
    std::cerr <<").\n     The largest value allowed is INT_MAX ("<< val
              <<")."<< std::endl;
    return false;
  }

  val = (int)lval;
  return true;
}
