// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFA_TETRAHEDRON_GEOMETRY_H
#define FFA_TETRAHEDRON_GEOMETRY_H

#include <vector>
#include "FFaGeometryBase.H"


class FFaTetrahedronGeometry : public FFaGeometryBase
{
public:
  FFaTetrahedronGeometry() {}

  FFaTetrahedronGeometry(const FaVec3& point1,
			 const FaVec3& point2,
			 const FaVec3& point3,
			 const FaVec3& top)
  { this->setData(point1,point2,point3,top); }

  virtual ~FFaTetrahedronGeometry() {}

  void setData(const FaVec3& point1,
	       const FaVec3& point2,
	       const FaVec3& point3,
	       const FaVec3& top);

  std::vector<FaVec3> getData() const;

  bool sameSide(const FaVec3& point1, const FaVec3& point2,
		const FaVec3& a, const FaVec3& b) const;
  double getZValue() const;

  // Standard interface

  virtual FFaGeometryBase* getCopy() const;

  virtual bool isInside(const FaVec3& point, double tolerance) const;

  virtual bool isEqual(const FFaGeometryBase* geo) const;

protected:
  virtual std::ostream& writeStream(std::ostream& s) const;
  virtual std::istream& readStream(std::istream& s);

private:
  FaVec3 myPoint1;
  FaVec3 myPoint2;
  FaVec3 myPoint3;
  FaVec3 myTop;
};

#endif
