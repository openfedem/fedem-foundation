// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFaLib/FFaGeometry/FFaPointSetGeometry.H"


FFaGeometryBase* FFaPointSetGeometry::getCopy() const
{
  return new FFaPointSetGeometry(*this);
}


/*!
  Returns true if a point lies on the boundary surface (or inside)
*/

bool FFaPointSetGeometry::isInside(const FaVec3& point, double tolerance) const
{
  if (myPoints.empty()) return false;

  if (!haveBBox)
  {
    myMin = myMax = myPoints.front();
    for (size_t i = 1; i < myPoints.size(); i++)
      for (int v = 0; v < 3; v++)
        if (myPoints[i][v] < myMin[v])
          myMin[v] = myPoints[i][v];
        else if (myPoints[i][v] > myMax[v])
          myMax[v] = myPoints[i][v];

    haveBBox = true;
  }

  // Check if the point is outside the bounding box
  for (int j = 0; j < 3; j++)
    if (point[j] < myMin[j] - tolerance ||
	point[j] > myMax[j] + tolerance)
      return false;

  // Check if the point matches any of myPoints
  for (size_t i = 0; i < myPoints.size(); i++)
    if ((point - myPoints[i]).length() < tolerance)
      return true;

  return false;
}


std::ostream& FFaPointSetGeometry::writeStream(std::ostream& s) const
{
  s <<"POINTSET "<< this->myAddExclude;
  for (size_t i = 0; i < myPoints.size(); i++)
    s << '\n' << myPoints[i];

  return s << std::endl;
}


std::istream& FFaPointSetGeometry::readStream(std::istream& s)
{
  this->clearPoints();

  s >> this->myAddExclude;
  FaVec3 point;

  while (s)
  {
    s >> point;
    if (s)
      myPoints.push_back(point);
  }
  s.clear();
  return s;
}


bool FFaPointSetGeometry::isEqual(const FFaGeometryBase* geo) const
{
  const FFaPointSetGeometry* pg = dynamic_cast<const FFaPointSetGeometry*>(geo);
  if (!pg) return false;

  if (myPoints.size() != pg->myPoints.size())
    return false;

  // Loop until end or until points are different
  for (size_t i = 0; i < myPoints.size(); i++)
    if (myPoints[i] != pg->myPoints[i])
      return false;

  return true;
}
