// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include <string>
#include <cstring>
#include <map>
#include <iostream>

#include "FFaLib/FFaTypeCheck/FFaTypeCheck.H"


int FFaTypeCheck::counter = 0;


static std::map<std::string,int>& getFFaTypeCheckTypNmToIDMap()
{
  static std::map<std::string,int> ourTypeNameToID;
  return ourTypeNameToID;
}


int FFaTypeCheck::getNewTypeID(const char* typeName)
{
  if (!typeName) return counter; // Only return the number of types so far

  std::map<std::string,int>& typeNameToID = getFFaTypeCheckTypNmToIDMap();
  if (typeNameToID.insert(std::make_pair(typeName,counter+1)).second)
    ++counter;
  else
    std::cerr <<"FFaTypeCheck: A typeID for class "<< typeName
              <<" already exists."<< std::endl;

  return counter;
}


int FFaTypeCheck::getTypeIDFromName(const char* typeName)
{
  if (!typeName) return NO_TYPE_ID;

  const std::map<std::string,int>& typeNameToID = getFFaTypeCheckTypNmToIDMap();
  std::map<std::string,int>::const_iterator tit = typeNameToID.find(typeName);
  if (tit != typeNameToID.end()) return tit->second;

  std::cerr <<"FFaTypeCheck: Unknown typeName "<< typeName << std::endl;

  // Temporary fix of flawed Fc-typenames in Fedem R5.1i1 and R5.1i2.
  // This is needed only as long as model files written with these two
  // internal releases are present.
  if (!strcmp(typeName, "FcSTRAIGHTMASTER"))
    return getTypeIDFromName("FcMASTER_LINE");
  else if (!strcmp(typeName, "FcARCSEGMENTMASTER"))
    return getTypeIDFromName("FcMASTER_ARC_SEGMENT");
  else if (!strcmp(typeName, "FcELEMENTGROUPPROXY"))
    return getTypeIDFromName("FcELEMENT_GROUP");
  else if (!strcmp(typeName, "FcFILEREFERENCE"))
    return getTypeIDFromName("FcFILE_REFERENCE");
  else if (!strcmp(typeName, "FcGENERICDBOBJECT"))
    return getTypeIDFromName("FcGENERIC_DB_OBJECT");
  else if (!strcmp(typeName, "FcPIPESTRINGDATAEXPORTER"))
    return getTypeIDFromName("FcPIPE_STRING_DATA_EXPORTER");

  return NO_TYPE_ID;
}


const char* FFaTypeCheck::getTypeNameFromID(int typeID)
{
  const std::map<std::string,int>& typeNameToID = getFFaTypeCheckTypNmToIDMap();
  std::map<std::string,int>::const_iterator tit = typeNameToID.begin();
  while (tit != typeNameToID.end())
    if (tit->second == typeID)
      return tit->first.c_str();
    else
      ++tit;

  return "(unknown)";
}
