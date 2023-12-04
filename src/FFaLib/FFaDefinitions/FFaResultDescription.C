// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFaLib/FFaDefinitions/FFaResultDescription.H"
#include "FFaLib/FFaString/FFaTokenizer.H"
#include "FFaLib/FFaString/FFaStringExt.H"
#include <cstdlib>


FFaTimeDescription::FFaTimeDescription()
{
  varDescrPath = { "Physical time" };
  varRefType = "SCALAR";
}


void FFaResultDescription::clear()
{
  OGType.erase();
  baseId = userId = 0;
  varDescrPath.clear();
  varRefType.erase();
}


void FFaResultDescription::copyResult(const FFaResultDescription& other)
{
  varDescrPath = other.varDescrPath;
  varRefType = other.varRefType;
}


std::string FFaResultDescription::getText() const
{
  std::string txt = OGType;
  if (userId > 0)
    txt += FFaNumStr(" [%d]",userId);
  else if (baseId > 0)
    txt += FFaNumStr(" {%d}",baseId);

  if (varDescrPath.empty())
    return txt;
  else if (txt.empty())
    txt = varDescrPath.front();
  else
    txt += ", " + varDescrPath.front();

  for (size_t i = 1; i < varDescrPath.size(); i++)
    txt += ", " + varDescrPath[i];

  return txt;
}


bool FFaResultDescription::isTime() const
{
  if (baseId > 0 || userId > 0 || !OGType.empty()) return false;
  return varDescrPath.size() == 1 && varDescrPath.front() == "Physical time";
}


bool FFaResultDescription::isBeamSectionResult() const
{
  if ((baseId < 1 && userId < 1) || OGType != "Beam") return false;
  if (varDescrPath.size() != 1) return false;

  return (varDescrPath.front().find("Sectional") == 0 &&
	  varDescrPath.front().find(", end") != std::string::npos);
}


bool FFaResultDescription::operator==(const FFaResultDescription& entry) const
{
  if (OGType       != entry.OGType)       return false;
  if (baseId       != entry.baseId)       return false;
  if (varDescrPath != entry.varDescrPath) return false;
  return true;
}


std::ostream& operator<<(std::ostream& s, const FFaResultDescription& descr)
{
  s <<'<';
  if (!descr.OGType.empty() || descr.baseId > 0 || descr.userId > 0)
    s <<'"'<< descr.OGType <<'"'<<','<< descr.baseId <<','<< descr.userId <<',';

  s <<'"'<< descr.varRefType <<'"';
  for (const std::string& path : descr.varDescrPath)
    s <<','<<'"'<< path <<'"';

  s <<'>';

  return s;
}


std::istream& operator>>(std::istream& s, FFaResultDescription& descr)
{
  descr.clear();

  int c = ' ';
  while ((c = s.get()) && isspace(c));
  if (c != '<')
  {
    std::cerr <<"operator>>(std::istream&,FFaResultDescription&):"
              <<" Invalid result description - it does not start with '<'."
              << std::endl;
    return s;
  }

  FFaTokenizer line(s,'<','>',',');
  // Check for old (pre R6.0) format always with the value "0"
  // as its first entry, the new format is more compact.
  size_t nwmin = 1;
  size_t nword = line.size();
  if (nword > 0 && line.front() == "0")
    nwmin = 7;
  else if (nword > 1 && isdigit(line[1][0]))
    nwmin = 4;

  if (nword < nwmin)
  {
    std::cerr <<"operator>>(std::istream&,FFaResultDescription&):"
              <<" Empty result description";
    if (nwmin > 1)
      std::cerr <<" - it should have at least "<< nwmin <<" entries";
    std::cerr <<"."<< std::endl;
    return s;
  }

  if (nwmin > 1)
  {
    descr.OGType = line[nwmin > 4 ? 1 : 0];
    descr.baseId = atoi(line[nwmin > 4 ? 3 : 1].c_str());
    descr.userId = atoi(line[nwmin > 4 ? 5 : 2].c_str());
  }
  descr.varRefType = line[nwmin-1];
  if (nword > nwmin)
    descr.varDescrPath.insert(descr.varDescrPath.end(),
                              line.begin()+nwmin,line.end());

  return s;
}
