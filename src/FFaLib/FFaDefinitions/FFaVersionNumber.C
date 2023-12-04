// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFaLib/FFaDefinitions/FFaVersionNumber.H"
#include "FFaLib/FFaString/FFaStringExt.H"
#include <cstdlib>
#include <cctype>


FFaVersionNumber::FFaVersionNumber(int n1, int n2, int n3, int n4)
{
  this->setVersion(n1,n2,n3,n4);
}


void FFaVersionNumber::setVersion(int n1, int n2, int n3, int n4)
{
  d1 = n1;
  d2 = n2;
  d3 = n3;
  build = n4;

  version = FFaNumStr(d1) + FFaNumStr(".%d", d2 > 0 ? d2 : 0);

  if (d3 > 0)
    version += FFaNumStr(".%d",d3);

  if (build > 0)
    version += FFaNumStr(d1*10+d2 < 75 ? "-i%d" : " (build %d)", build);
}


std::string FFaVersionNumber::getInterpretedString() const
{
  std::string ver = FFaNumStr(d1) + FFaNumStr(".%d",d2) + FFaNumStr(".%d",d3);
  if (build >= 0) ver += FFaNumStr(".%d",build);

  return ver;
}


void FFaVersionNumber::parseLine(const std::string& line, char skipUntil)
{
  // First rip the version number out
  size_t i = 0, len = line.length();

  // Skip everything until the first 'skipUntil' character
  if (skipUntil)
    while (i < len && line[i++] != skipUntil);

  // Skip initial spaces, if any
  while (i < len && isspace(line[i])) i++;

  // Record the version string until next space
  version = "";
  while (i < len && !isspace(line[i]))
    version += line[i++];

  // Skip spaces again
  while (i < len && isspace(line[i])) i++;

  // If the next word starts with something else than a number...
  if (i < len && isalpha(line[i]) && line.substr(i,5) != "ASCII")
  {
    // Record the version string until next space
    version += " ";
    while (i < len && !isspace(line[i]))
      version += line[i++];
  }
  else if (i+7 < len && line.substr(i,7) == "(build ") // check build number
  {
    version += " " + line.substr(i,7);
    i += 7;
    // Record the version string until next space
    while (i < len && !isspace(line[i]))
      version += line[i++];
  }

  // Now parse the version string
  bool hasInternalNumber = false;
  if (version.find(" i") != std::string::npos ||
      version.find("-i") != std::string::npos ||
      version.find("-ea") != std::string::npos ||
      version.find("-alpha") != std::string::npos ||
      version.find("-beta") != std::string::npos ||
      version.find("(build") != std::string::npos)
    hasInternalNumber = true;

  len = version.length();
  int numVDigits = 0;
  int ver[4] = { 0, 0, 0, -1};

  i = 0;
  while (i < len && numVDigits < 4 && version[i] != '+')
  {
    std::string digitText;
    while (i < len && !isdigit(version[i])) i++;
    while (i < len &&  isdigit(version[i]))
      digitText += version[i++];

    if (!digitText.empty())
      ver[numVDigits++] = atoi(digitText.c_str());
  }

  if (hasInternalNumber)
  {
    if (numVDigits > 1)
      build = ver[--numVDigits];
    else
      build = numVDigits = 0;
  }
  else
    build = -1;

  d1 = numVDigits > 0 ? ver[0] : 0;
  d2 = numVDigits > 1 ? ver[1] : 0;
  d3 = numVDigits > 2 ? ver[2] : 0;
}


void FFaVersionNumber::set(int i, int n)
{
  switch (i) {
  case 1: d1 = n; break;
  case 2: d2 = n; break;
  case 3: d3 = n; break;
  case 4: build = n; break;
  }
}


int FFaVersionNumber::get(int i) const
{
  switch (i) {
  case 1: return d1;
  case 2: return d2;
  case 3: return d3;
  case 4: return build;
  }
  return 0;
}


bool FFaVersionNumber::operator > (const FFaVersionNumber& v) const
{
  if (d1 > v.d1) return true;
  if (d1 < v.d1) return false;

  if (d2 > v.d2) return true;
  if (d2 < v.d2) return false;

  if (d3 > v.d3) return true;
  if (d3 < v.d3) return false;

  if (d1*10+d2 >= 75) // New versioning system (non-zero build number)
    return build > v.build;

  // Old versioning system (internal, alpha, beta, ...)
  if (build > v.build && v.build >= 0) return true;
  if (build < v.build && build >= 0) return false;
  if (build == v.build || v.build < 0) return false;

  return true;
}


bool FFaVersionNumber::operator == (const FFaVersionNumber& v) const
{
  return d1 == v.d1 && d2 == v.d2 && d3 == v.d3 && build == v.build;
}


bool FFaVersionNumber::operator >= (const FFaVersionNumber& v) const
{
  return (*this == v) || (*this > v);
}


bool FFaVersionNumber::operator < (const FFaVersionNumber& v) const
{
  return !(*this >= v);
}


bool FFaVersionNumber::operator <= (const FFaVersionNumber& v) const
{
  return !(*this > v);
}
