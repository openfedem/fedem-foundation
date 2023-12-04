// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFaLib/FFaGeometry/FFaPlaneGeometry.H"


FFaPlaneGeometry::FFaPlaneGeometry(const FaVec3& P0, const FaVec3& normal)
  : myNormal(myPosition[VZ]), myOrigin(myPosition[VW])
{
  FaMat33 myCS;
  myCS.makeGlobalizedCS(P0+normal);
  myPosition = FaMat34(myCS.shift(-1),P0);
}


FFaPlaneGeometry::FFaPlaneGeometry(const FaVec3& P0,
                                   const FaVec3& P1, const FaVec3& P2)
  : myNormal(myPosition[VZ]), myOrigin(myPosition[VW])
{
  myPosition.makeGlobalizedCS(P0,P1,P2);
}


FFaGeometryBase* FFaPlaneGeometry::getCopy() const
{
  return new FFaPlaneGeometry(*this);
}


/*!
  Returns true if a point lies on or below the plane
*/

bool FFaPlaneGeometry::isInside(const FaVec3& point, double tolerance) const
{
  return (point-myOrigin) * myNormal <= tolerance;
}


std::ostream& FFaPlaneGeometry::writeStream(std::ostream& s) const
{
  s <<"PLANE "<< this->myAddExclude;
  s << this->myNormal << ' ' << this->myOrigin;
  return s << std::endl;
}


std::istream& FFaPlaneGeometry::readStream(std::istream& s)
{
  s >> this->myAddExclude;
  s >> this->myNormal >> this->myOrigin;
  return s;
}


bool FFaPlaneGeometry::isEqual(const FFaGeometryBase* geo) const
{
  const FFaPlaneGeometry* lg = dynamic_cast<const FFaPlaneGeometry*>(geo);
  if (!lg) return false;

  return (myNormal     == lg->myNormal &&
	  myOrigin     == lg->myOrigin &&
	  myAddExclude == lg->myAddExclude);
}
