// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFaPyramidGeometry.H"


FFaGeometryBase* FFaPyramidGeometry::getCopy() const
{
  return new FFaPyramidGeometry(*this);
}


/*!
  Checks whether a point lies inside the pyramid.
  The function uses equally shaped triangles and easy comparison.
*/

bool FFaPyramidGeometry::isInside(const FaVec3& point, double) const
{	
  FaVec3 pVec = this->getTransMatrix().inverse() * point;
  double tmpHeight = fabs(pVec[2]);
  if (this->getHeightData() <= 0.0 || this->getHeightData() < tmpHeight)
    return false; // avoid dividing by 0

  double tmpSide = fabs(tmpHeight * this->getSideData()*0.5) / this->getHeightData();
  return (fabs(pVec[0]) <= tmpSide) || (fabs(pVec[1]) <= tmpSide);
}


std::ostream& FFaPyramidGeometry::writeStream(std::ostream& s) const
{
  s <<"PYRAMID "<< this->myAddExclude << '\n';
  s << this->mySide << ' ' << this->myHeight;
  s << this->myPosition;

  return s << std::endl;
}


std::istream& FFaPyramidGeometry::readStream(std::istream& s)
{
  s >> this->myAddExclude;
  s >> this->mySide >> this->myHeight;
  s >> this->myPosition;
  return s;
}


bool FFaPyramidGeometry::isEqual(const FFaGeometryBase* geo) const
{
  const FFaPyramidGeometry* pg = dynamic_cast<const FFaPyramidGeometry*>(geo);
  if (!pg) return false;

  return (mySide       == pg->mySide     &&
	  myHeight     == pg->myHeight   &&
	  myPosition   == pg->myPosition &&
	  myAddExclude == pg->myAddExclude);
}
