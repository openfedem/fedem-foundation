// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFA_COMPOUND_GEOMETRY_H
#define FFA_COMPOUND_GEOMETRY_H

#include <vector>

#include "FFaLib/FFaGeometry/FFaGeometryBase.H"


class FFaCompoundGeometry : public FFaGeometryBase
{
public:
  FFaCompoundGeometry(double tol = FFa::geomEpsilon) : myTolerance(tol) {}
  FFaCompoundGeometry(const FFaCompoundGeometry& g) { this->operator=(g); }
  virtual ~FFaCompoundGeometry() { this->deleteGeometry(); }

  FFaGeometryBase& addGeometry(const FFaGeometryBase& geo);
  void deleteGeometry(int index = -1);

  bool empty() const { return myGeometry.empty(); }
  size_t size() const { return myGeometry.size(); }

  const FFaGeometryBase* operator[](int i) const { return myGeometry[i]; }

  FFaCompoundGeometry& operator=(const FFaCompoundGeometry& g);
  virtual FFaGeometryBase* getCopy() const;

  virtual bool isInside(const FaVec3& point, double tolerance) const;
  bool isInside(const FaVec3& X) const { return this->isInside(X,myTolerance); }
  void setTolerance(double tol) { myTolerance = tol; }

  friend bool operator==(const FFaCompoundGeometry& a,
                         const FFaCompoundGeometry& b);

  virtual bool isEqual(const FFaGeometryBase* g) const;

protected:
  virtual std::ostream& writeStream(std::ostream& s) const;
  virtual std::istream& readStream(std::istream& s);

private:
  double                        myTolerance;
  std::vector<FFaGeometryBase*> myGeometry;
};

#endif
