// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include <string>

#include "FFaLib/FFaGeometry/FFaLineGeometry.H"
#include "FFaLib/FFaGeometry/FFaPlaneGeometry.H"
#include "FFaLib/FFaGeometry/FFaPointSetGeometry.H"
#include "FFaLib/FFaGeometry/FFaCylinderGeometry.H"
#include "FFaLib/FFaGeometry/FFaConeGeometry.H"
#include "FFaLib/FFaGeometry/FFaSphereGeometry.H"
#include "FFaLib/FFaGeometry/FFaPyramidGeometry.H"
#include "FFaLib/FFaGeometry/FFaTetrahedronGeometry.H"
#include "FFaLib/FFaGeometry/FFaCompoundGeometry.H"


FFaCompoundGeometry&
FFaCompoundGeometry::operator=(const FFaCompoundGeometry& g)
{
  if (this == &g)
    return *this;

  this->deleteGeometry();

  for (FFaGeometryBase* geo : g.myGeometry)
    myGeometry.push_back(geo->getCopy());

  myPosition = g.myPosition;
  myAddExclude = g.myAddExclude;
  myTolerance = g.myTolerance;

  return *this;
}


FFaGeometryBase* FFaCompoundGeometry::getCopy() const
{
  return new FFaCompoundGeometry(*this);
}


/*!
  Checks whether a point is inside or outside the FFaCompoundGeometry.
  The method is using AND which means isInside MUST return true for every
  geometry to return true at the end.
  See the derived classes for more specific implementation.
*/

bool FFaCompoundGeometry::isInside(const FaVec3& point, double tolerance) const
{
  for (FFaGeometryBase* geo : myGeometry)
    if (geo->isInside(point,tolerance) != geo->getAddExclude())
      return false;

  return true;
}


/*!
  Puts new geometry into the FFaCompoundGeometry's geometry list.
*/

FFaGeometryBase& FFaCompoundGeometry::addGeometry(const FFaGeometryBase& geo)
{
  myGeometry.push_back(geo.getCopy());
  return *myGeometry.back();
}


/*!
  Removes geometry from FFaCompoundGeometry's myGeometryList
  if -1 is used, all entities in this compound is deleted.
*/

void FFaCompoundGeometry::deleteGeometry(int index)
{
  if (index < 0)
  {
    for (FFaGeometryBase* geo : myGeometry)
      delete geo;
    myGeometry.clear();
  }
  else if ((size_t)index < myGeometry.size())
  {
    std::vector<FFaGeometryBase*>::iterator it = myGeometry.begin() + index;
    delete *it;
    myGeometry.erase(it);
  }
}


bool FFaCompoundGeometry::isEqual(const FFaGeometryBase* g) const
{
  const FFaCompoundGeometry* comp = dynamic_cast<const FFaCompoundGeometry*>(g);
  return comp ? *this == *comp : false;
}


/*!
  Operator == needed to satisfy FFaField
*/

bool operator==(const FFaCompoundGeometry& a, const FFaCompoundGeometry& b)
{
  bool areEqual = a.myGeometry.size() == b.myGeometry.size();
  for (size_t i = 0; areEqual && i < a.myGeometry.size(); i++)
    areEqual = a.myGeometry[i]->isEqual(b.myGeometry[i]);

  return areEqual;
}


std::ostream& FFaCompoundGeometry::writeStream(std::ostream& s) const
{
  s <<"\nTOLERANCE "<< myTolerance << std::endl;

  for (FFaGeometryBase* geo : myGeometry)
    if (dynamic_cast<FFaCompoundGeometry*>(geo))
      s <<"COMPOUNDGEOMETRY"<< *geo;
    else
      s << *geo;

  return s <<"END";
}


std::istream& FFaCompoundGeometry::readStream(std::istream& s)
{
  std::string keywd;
  while (s.good())
  {
    s >> keywd;
    if (keywd.find("END") == 0)
      break;
    else if (keywd == "TOLERANCE")
      s >> myTolerance;
    else if (keywd == "LINE")
      s >> this->addGeometry(FFaLineGeometry());
    else if (keywd == "PLANE")
      s >> this->addGeometry(FFaPlaneGeometry());
    else if (keywd == "POINTSET")
      s >> this->addGeometry(FFaPointSetGeometry());
    else if (keywd == "CYLINDER")
      s >> this->addGeometry(FFaCylinderGeometry());
    else if (keywd == "CONE")
      s >> this->addGeometry(FFaConeGeometry());
    else if (keywd == "SPHERE")
      s >> this->addGeometry(FFaSphereGeometry());
    else if (keywd == "PYRAMID")
      s >> this->addGeometry(FFaPyramidGeometry());
    else if (keywd == "TETRAHEDRON")
      s >> this->addGeometry(FFaTetrahedronGeometry());
    else if (keywd == "COMPOUNDGEOMETRY")
      s >> this->addGeometry(FFaCompoundGeometry());
    else
      std::cerr <<"  ** FFaCompoundGeometry::readStream: Unknown geometry type "
                << keywd <<" (ignored)"<< std::endl;
  }

  return s;
}
