// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include <string>
#include <cctype>

#include "FFlConnectorItems.H"


bool operator== (const FFlConnectorItems& a, const FFlConnectorItems& b)
{
  if (&a == &b)
    return true;

  if (a.nodes == b.nodes)
    if (a.elements == b.elements)
      return true;

  return false;
}


std::ostream& operator<< (std::ostream& s, const FFlConnectorItems& c)
{
  if (!c.nodes.empty())
  {
    s <<"\nNODES";
    for (int n : c.nodes)
      s <<' '<< n;
  }

  if (!c.elements.empty())
  {
    s <<"\nELEMENTS";
    for (int e : c.elements)
      s <<' '<< e;
  }

  return s <<"\nEND";
}


std::istream& operator>> (std::istream& s, FFlConnectorItems& c)
{
  // Lambda function reading a vector of integers from the input stream
  auto&& readStream = [&s](std::vector<int>& data)
  {
    while (s)
    {
      char c = ' ';
      while (s && isspace(c))
        s >> c; // read until next non-space character
      if (s)
        s.putback(c);
      if (!s || !isdigit(c))
        return; // read until next non-digit character

      int i;
      s >> i;
      if (s)
        data.push_back(i);
    }
  };

  c.clear();
  std::string keyWord;
  while (s.good())
  {
    s >> keyWord;
    bool isAlpha = true;
    for (size_t i = 0; i < keyWord.size() && isAlpha; i++)
      if (!isalpha(keyWord[i]))
        isAlpha = false;

    if (isAlpha)
    {
      if (keyWord == "NODES")
        readStream(c.nodes);
      else if (keyWord == "ELEMENTS")
        readStream(c.elements);
      else if (keyWord == "END")
        break;
    }
  }

  return s;
}
