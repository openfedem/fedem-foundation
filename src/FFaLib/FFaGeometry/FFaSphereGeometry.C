// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFaSphereGeometry.H"


FFaGeometryBase* FFaSphereGeometry::getCopy() const
{
  return new FFaSphereGeometry(*this);
}


/*!
  Checks if point is inside the sphere or the "sphere-cone" if angle < 2*pi.
*/

bool FFaSphereGeometry::isInside(const FaVec3& point, double) const
{
  FaVec3 pTrans = this->getTransMatrix().inverse() * point;
  if (pTrans.sqrLength() > this->getRadius()*this->getRadius())
    return false;

  if (pTrans.angle(FaVec3(0.0,0.0,1.0)) > this->getAngleData())
    return false;

  return true;
}


std::ostream& FFaSphereGeometry::writeStream(std::ostream& s) const
{
  s <<"SPHERE "<< this->myAddExclude << '\n';
  s << this->myRadius << ' ' << this->myAngle;
  s << this->myPosition;
  return s << std::endl;
}


std::istream& FFaSphereGeometry::readStream(std::istream& s)
{
  s >> this->myAddExclude;
  s >> this->myRadius >> this->myAngle;
  s >> this->myPosition;
  return s;
}


bool FFaSphereGeometry::isEqual(const FFaGeometryBase* geo) const
{
  const FFaSphereGeometry* sg = dynamic_cast<const FFaSphereGeometry*>(geo);
  if (!sg) return false;

  return (myRadius     == sg->myRadius   &&
	  myAngle      == sg->myAngle    &&
	  myPosition   == sg->myPosition &&
	  myAddExclude == sg->myAddExclude);
}
