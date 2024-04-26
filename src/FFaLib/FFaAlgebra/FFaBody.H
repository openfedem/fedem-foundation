// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFA_BODY_H
#define FFA_BODY_H

#include <vector>
#include <string>

#include "FFaLib/FFaAlgebra/FFaTensor3.H"
#include "FFaLib/FFaAlgebra/FFaVec3.H"

using FaEdge = std::pair<size_t,size_t>; //!< Vertex indices of an edge
class FaFace;
class FaMat34;


class FFaBody
{
public:
  FFaBody() : myVolume(0.0), startVx(0) { isVolumeComputed = isBBoxComputed = false; }

  size_t getNoVertices() const { return myVertices.size(); }
  size_t getNoFaces() const { return myFaces.size(); }

  bool computeBoundingBox(FaVec3& minX, FaVec3& maxX) const;
  bool computeTotalVolume(double& Vb, FaVec3& C0b, FFaTensor3* Ib = NULL) const;
  bool computeVolumeBelow(double& Vb, double& As, FaVec3& C0b, FaVec3& C0s,
			  const FaVec3& normal, double z0,
			  double zeroTol = 1.0e-16);

  bool saveIntersection(const FaMat34& cs);
  bool computeIncArea(double& dAs, FaVec3& C0s,
		      const FaVec3& normal, const FaMat34& cs);

  size_t addFace(int v1, int v2, int v3, int v4 = -1);
  size_t addVertex(const FaVec3& pos, double tol = -1.0);
  const FaVec3& getVertex(size_t idx) const { return myVertices[idx]; }
  int getFaceVtx(size_t idx, int i) const;

  static FFaBody* readFromCAD(std::istream& in, double duplTol = -1.0);
  bool writeCAD(const std::string& fileName, const FaMat34& partCS) const;

protected:
  static FFaBody* readSTL(std::istream& in, double duplTol);
  static FFaBody* readCAD(std::istream& in);
  static FFaBody* readWRL(std::istream& in, int version);

  static void readWRL1     (FFaBody*& body, std::istream& in);
  static void readWRL2     (FFaBody*& body, std::istream& in);
  static void readChildren (FFaBody*& body, const FaMat34& bodyCS,
                            std::istream& in);
  static void readTransform(FFaBody*& body, const FaMat34& bodyCS,
                            std::istream& in);
  static void readShape    (FFaBody*& body, const FaMat34& bodyCS,
                            std::istream& in);

  void readIndexedFaceSet(std::istream& in, const FaMat34& partCS);
  void readBody(std::istream& in, const FaMat34& partCS);
  void readCoords(std::istream& in, const FaMat34& partCS, char endChar = '}');
  void readFaces(std::istream& in, char endChar = '}');
  void readFace(std::istream& in);

public:
  static std::string prefix;

private:
  mutable bool        isBBoxComputed;
  mutable FaVec3      myBBox[2];
  std::vector<FaVec3> myVertices;
  std::vector<FaFace> myFaces;
  FaVec3              myX0s;
  std::vector<FaVec3> myLoopVer;
  std::vector<FaEdge> myIntLoop;
  mutable char        isVolumeComputed;
  mutable double      myVolume;
  mutable FFaTensor3  myInertia;
  mutable FaVec3      myCentroid;
  size_t              startVx;
};


class FaFace
{
public:
  FaFace();
  FaFace(FFaBody* body, size_t i1, size_t i2, size_t i3, int i4 = -1);

  int intersect(const FaVec3& normal, double z0, double zeroTol = 1.0e-16);
  bool isIntersected() const { return iEdge.first != iEdge.second; }
  bool isBelow() const { return IAmBelow; }

  size_t size() const { return myVertices.size(); }
  size_t operator[](int i) const { return myVertices[i]; }
  const FaVec3& vertex(int i) const { return owner->getVertex(myVertices[i]); }

  double accumulateArea(const FaVec3& vn, const FaVec3& v0, FaVec3& X) const;
  double accumulateVolume(const FaVec3& v0, FaVec3& X, FFaTensor3* I) const;

  const std::vector<FaFace>& getSubFaces() const { return mySubFaces; }
  FaEdge getIntEdge() const { return iEdge; }
  FaVec3 getIntEdgeCoord() const;
  FFaBody* getBody() const { return owner; }

protected:
  int quad2Quads(const char* status, const double* dist);
  int quad2QuadTria(const char* status, const double* dist, bool oneAbove);

  friend std::ostream& operator<<(std::ostream& s, const FaFace& f);

private:
  std::vector<size_t> myVertices; //!< Indices to the vertices defining the face
  FaEdge              iEdge;      //!< Vertices of the intersection edge, if any
  std::vector<FaFace> mySubFaces; //!< 2 or 3 sub-triangles of intersected face
  bool                IAmBelow;   //!< Is face below current intersection plane?

  FFaBody* owner; //!< Pointer to the body object containing this face
};


inline int FFaBody::getFaceVtx(size_t idx, int i) const
{
  return i >= 0 && (size_t)i < myFaces[idx].size() ? myFaces[idx][i] : -1;
}

#endif
