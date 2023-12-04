// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFaLib/FFaGeometry/FFaLineGeometry.H"


FFaLineGeometry::FFaLineGeometry(const FaVec3& P0, const FaVec3& P1)
  : myXaxis(myPosition[VX]), myOrigin(myPosition[VW])
{
  myPosition.makeGlobalizedCS(P0,P1);
}


FFaGeometryBase* FFaLineGeometry::getCopy() const
{
  return new FFaLineGeometry(*this);
}


/*!
  Returns true if a point lies on the line
*/

bool FFaLineGeometry::isInside(const FaVec3& point, double tolerance) const
{
  return ((point-myOrigin) ^ myXaxis).sqrLength() <= tolerance*tolerance;
}


std::ostream& FFaLineGeometry::writeStream(std::ostream& s) const
{
  s <<"LINE "<< this->myAddExclude;
  s << this->myXaxis << ' ' << this->myOrigin;
  return s << std::endl;
}


std::istream& FFaLineGeometry::readStream(std::istream& s)
{
  s >> this->myAddExclude;
  s >> this->myXaxis >> this->myOrigin;
  return s;
}


bool FFaLineGeometry::isEqual(const FFaGeometryBase* geo) const
{
  const FFaLineGeometry* lg = dynamic_cast<const FFaLineGeometry*>(geo);
  if (!lg) return false;

  return (myXaxis      == lg->myXaxis &&
	  myOrigin     == lg->myOrigin &&
	  myAddExclude == lg->myAddExclude);
}
