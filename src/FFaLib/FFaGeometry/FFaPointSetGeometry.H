// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFA_POINT_SET_GEOMETRY_H
#define FFA_POINT_SET_GEOMETRY_H

#include <vector>
#include "FFaGeometryBase.H"


class FFaPointSetGeometry : public FFaGeometryBase
{
public:
  FFaPointSetGeometry() { haveBBox = false; }
  virtual ~FFaPointSetGeometry() {}

  void addPoint(const FaVec3& pt) { myPoints.push_back(pt); haveBBox = false; }
  void clearPoints() { myPoints.clear(); haveBBox = false; }

  // Standard interface

  virtual FFaGeometryBase* getCopy() const;

  virtual bool isInside(const FaVec3& point, double tolerance) const;

  virtual bool isEqual(const FFaGeometryBase* geo) const;

protected:
  virtual std::ostream& writeStream(std::ostream& s) const;
  virtual std::istream& readStream(std::istream& s);

private:
  std::vector<FaVec3> myPoints;
  mutable FaVec3      myMin;
  mutable FaVec3      myMax;
  mutable bool        haveBBox;
};

#endif
