// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFaTetrahedronGeometry.H"
#include <algorithm>


FFaGeometryBase* FFaTetrahedronGeometry::getCopy() const
{
  return new FFaTetrahedronGeometry(*this);
}


void FFaTetrahedronGeometry::setData(const FaVec3& point1,
				     const FaVec3& point2,
				     const FaVec3& point3,
				     const FaVec3& top)
{
  this->myPoint1 = point1;
  this->myPoint2 = point2;
  this->myPoint3 = point3;
  this->myTop = top;
}


std::vector<FaVec3> FFaTetrahedronGeometry::getData() const
{
  std::vector<FaVec3> retVector;
  retVector.push_back(this->myPoint1);
  retVector.push_back(this->myPoint2);
  retVector.push_back(this->myPoint3);
  retVector.push_back(this->myTop);
  return retVector;
}


bool FFaTetrahedronGeometry::sameSide(const FaVec3& point1,
				      const FaVec3& point2,
				      const FaVec3& a,
				      const FaVec3& b) const
{
  FaVec3 cp1 = (b-a)^(point1-a);
  FaVec3 cp2 = (b-a)^(point2-a);
  return (cp1*cp2 >= 0.0);
}


double FFaTetrahedronGeometry::getZValue() const
{
  std::vector<FaVec3> tetraData = this->getData();
  FaMat34 transMat = this->getTransMatrix().inverse();

  tetraData[0] = transMat * tetraData[0];
  tetraData[1] = transMat * tetraData[1];
  tetraData[2] = transMat * tetraData[2];

  double point1z = tetraData[0][2];
  double point2z = tetraData[1][2];
  double point3z = tetraData[2][2];

  double maxz = std::max(std::max(point1z, point2z), point3z);
  double minz = std::min(std::min(point1z, point2z), point3z);

  return 0.5*(maxz + minz);
}


/*!	
  Checks if point lies inside a tetrahedron consisting of four FaVec3 points.
  TODO: THIS MUST BE REWRITTEN!!! NOT GOOD ENOUGH
*/

bool FFaTetrahedronGeometry::isInside(const FaVec3& point, double) const
{
  FaVec3 qTrans = this->getTransMatrix().inverse() * point;

  std::vector<FaVec3> tetraData = this->getData();
  FaVec3 point1(tetraData[0]);
  FaVec3 point2(tetraData[1]);
  FaVec3 point3(tetraData[2]);
  FaVec3 top(tetraData[3]);

  double zValue = this->getZValue();
	
  if (sameSide(point, point1, point2, point3) &&
      sameSide(point, point2, point1, point3) &&
      sameSide(point, point3, point1, point2))
    return (fabs(qTrans[2]) < fabs(zValue*1.01) &&
	    fabs(qTrans[2]) > fabs(zValue*0.99));

  return false;
}


std::ostream& FFaTetrahedronGeometry::writeStream(std::ostream& s) const
{
  s <<"TETRAHEDRON "<< this->myAddExclude << '\n';
  s << this->myPoint1 << ' ' << this->myPoint2 << ' '
    << this->myPoint3 << ' ' << this->myTop;
  s << this->myPosition;
  return s << std::endl;
}


std::istream& FFaTetrahedronGeometry::readStream(std::istream& s)
{
  s >> this->myAddExclude;
  s >> this->myPoint1 >> this->myPoint2 >> this->myPoint3 >> this->myTop;
  s >> this->myPosition;
  return s;
}


static bool isPartOf(const FaVec3& p,  const FaVec3& p1,
		     const FaVec3& p2, const FaVec3& p3)
{
  return (p == p1 || p == p2 || p == p3);
}


// NOT FINISHED!!! TODO: Finish this member function.

bool FFaTetrahedronGeometry::isEqual(const FFaGeometryBase* geo) const
{
  const FFaTetrahedronGeometry* tg = dynamic_cast<const FFaTetrahedronGeometry*>(geo);
  if (!tg) return false;

  return (myTop	== tg->myTop &&
	  isPartOf(myPoint1, tg->myPoint1, tg->myPoint2, tg->myPoint3) &&
	  isPartOf(myPoint2, tg->myPoint1, tg->myPoint2, tg->myPoint3) &&
	  isPartOf(myPoint3, tg->myPoint1, tg->myPoint2, tg->myPoint3) &&
	  myPosition   == tg->myPosition &&
	  myAddExclude == tg->myAddExclude);
}
