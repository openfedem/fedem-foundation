// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFaLib/FFaAlgebra/FFaMath.H"
#include "FFaLib/FFaGeometry/FFaConeGeometry.H"


FFaConeGeometry::FFaConeGeometry()
{
  myRadius     = 1.0;
  myZStart     =-HUGE_VAL;
  myZEnd       = HUGE_VAL;
  myAngleStart = 0.0;
  myAngleEnd   = 2*M_PI;
}


FFaConeGeometry::FFaConeGeometry(double radius,
				 double zStart, double zEnd,
				 double angleStart, double angleEnd)
{
  this->setRadius(radius);
  this->setZData(zStart,zEnd);
  this->setAngleData(angleStart,angleEnd);
}


FFaGeometryBase* FFaConeGeometry::getCopy() const
{
  return new FFaConeGeometry(*this);
}


void FFaConeGeometry::setZData(double zStart, double zEnd)
{
  this->myZStart = zStart < zEnd ? zStart : zEnd;
  this->myZEnd   = zStart < zEnd ? zEnd   : zStart;
}
	

std::pair<double,double> FFaConeGeometry::getZData() const
{
  return std::make_pair(this->myZStart,this->myZEnd);
}


void FFaConeGeometry::setAngleData(double angleStart, double angleEnd)
{
  this->myAngleStart = angleStart < angleEnd ? angleStart : angleEnd;
  this->myAngleEnd   = angleStart < angleEnd ? angleEnd   : angleStart;
}


std::pair<double,double> FFaConeGeometry::getAngleData() const
{
  return std::make_pair(this->myAngleStart,this->myAngleEnd);
}


/*!
  Checks whether point is inside or outside a cone geometry.
  The function uses equally shaped triangles,
  Pythagoras and simple angle functions.
*/

bool FFaConeGeometry::isInside(const FaVec3& point, double tolerance) const
{
  double height = this->getZData().second - this->getZData().first;
  if (height <= 0) return false;

  FaVec3 pTrans = this->getTransMatrix().inverse() * point;
  double ax = pTrans[0];
  double ay = pTrans[1];
  double H  = pTrans[2] - this->getZData().first;
  if (H < -tolerance || H > height+tolerance) return false;
  double R  = (H * this->getRadius() / height) + tolerance;
  if (ax*ax + ay*ay > R*R) return false;

  FaVec3 xVec(1.0, 0.0, 0.0);
  FaVec3 pointVec(ax, ay, 0.0);
  double angle = xVec.angle(pointVec);
  if (ay < 0.0) angle = 2.0*M_PI - angle; // angle is negative if ay < 0

  double angularTol = tolerance/this->getRadius();
  if (angle < this->getAngleData().first - angularTol) return false;
  if (angle > this->getAngleData().second + angularTol) return false;

  return true;
}


std::ostream& FFaConeGeometry::writeStream(std::ostream& s) const
{
  s <<"CONE "<< this->myAddExclude << '\n';
  s << this->myRadius << ' ' << this->myZStart << ' ' << this->myZEnd << ' '
    << this->myAngleStart << ' ' << this->myAngleEnd;
  s << this->myPosition;
  return s << std::endl;
}


std::istream& FFaConeGeometry::readStream(std::istream& s)
{
  s >> this->myAddExclude;
  s >> this->myRadius >> this->myZStart >> this->myZEnd
    >> this->myAngleStart >> this->myAngleEnd;
  s >> this->myPosition;
  return s;
}


bool FFaConeGeometry::isEqual(const FFaGeometryBase* geo) const
{
  const FFaConeGeometry* cg = dynamic_cast<const FFaConeGeometry*>(geo);
  if (!cg) return false;

  return (myRadius     == cg->myRadius     &&
          myZStart     == cg->myZStart     &&
          myZEnd       == cg->myZEnd       &&
          myAngleStart == cg->myAngleStart &&
          myAngleEnd   == cg->myAngleEnd   &&
          myPosition   == cg->myPosition   &&
          myAddExclude == cg->myAddExclude);
}
