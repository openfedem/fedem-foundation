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
  if (&a == &b) return true;

  if (a.nodes == b.nodes)
    if (a.elements == b.elements)
      if (a.properties == b.properties)
	return true;

  return false;
}


std::ostream&
operator<< (std::ostream& s, const FFlConnectorItems& connector)
{
  if (!connector.nodes.empty()) {
    s <<"\nNODES";
    for (int n : connector.nodes) s <<' '<< n;
  }
  if (!connector.elements.empty()) {
    s <<"\nELEMENTS";
    for (int e : connector.elements) s <<' '<< e;
  }
  if (!connector.properties.empty()) {
    s <<"\nPROPERTIES";
    for (int p : connector.properties) s <<' '<< p;
  }
  s << std::endl <<"END";
  return s;
}


std::istream&
operator>> (std::istream& s, FFlConnectorItems& connector)
{
  connector.clear();
  std::string keyWord;
  while (s.good()) {
    s >> keyWord;
    bool isAlpha = true;
    for (size_t i = 0; i < keyWord.size() && isAlpha; i++)
      if (!isalpha(keyWord[i]))
        isAlpha = false;

    if (isAlpha) {
      if (keyWord == "NODES")
	FFlConnectorItems::readStream(s,connector.nodes);
      else if (keyWord == "ELEMENTS")
	FFlConnectorItems::readStream(s,connector.elements);
      else if (keyWord == "PROPERTIES")
	FFlConnectorItems::readStream(s,connector.properties);
      else if (keyWord == "END")
	break;
    }
  }
  return s;
}


void FFlConnectorItems::readStream(std::istream& is, std::vector<int>& data)
{
  while (is) {
    char c = ' ';
    while (isspace(c)) {
      is >> c;
      if (!is) return;
    }
    is.putback(c);
    if (!isdigit(c)) return; // read until next non-digit character

    int i;
    is >> i;
    if (is)
      data.push_back(i);
  }
}
