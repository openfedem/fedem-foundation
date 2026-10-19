// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFaTypeCheck.C
  \brief Type checking control.
*/

#include <map>
#include <string>
#include <iostream>

#include "FFaLib/FFaTypeCheck/FFaTypeCheck.H"


namespace
{
  int counter = 0;  //!< Running type ID counter

  using TypeIdMap = std::map<std::string,int>; //!< Convenience type alias

  //! \brief Helper to create a static type name to ID container.
  TypeIdMap& getTypeIdMap()
  {
    static TypeIdMap ourMap;
    return ourMap;
  }
}


int FFaTypeCheck::getNewTypeID(const char* typeName)
{
  if (!typeName) return counter; // Only return the number of types so far

  TypeIdMap& typeNameToID = getTypeIdMap();
  if (typeNameToID.emplace(typeName,counter+1).second)
    ++counter;
  else
    std::cerr <<"FFaTypeCheck: A typeID for class "<< typeName
              <<" already exists."<< std::endl;

  return counter;
}


int FFaTypeCheck::getTypeIDFromName(const char* typeName)
{
  if (typeName)
  {
    const TypeIdMap& typeNameToID = getTypeIdMap();
    if (TypeIdMap::const_iterator tit = typeNameToID.find(typeName);
        tit != typeNameToID.end())
      return tit->second;
    std::cerr <<"FFaTypeCheck: Unknown type name "<< typeName << std::endl;
  }

  return NO_TYPE_ID;
}


const char* FFaTypeCheck::getTypeNameFromID(int typeID)
{
  const TypeIdMap& typeNameToID = getTypeIdMap();
  for (const TypeIdMap::value_type& tid : typeNameToID)
    if (tid.second == typeID)
      return tid.first.c_str();

  return "(unknown)";
}
