// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFaBody.H
  \brief Surface tesselation and intersection of volumetric objects.
*/

#ifndef FFA_BODY_H
#define FFA_BODY_H

#include <vector>
#include <string>

#include "FFaLib/FFaAlgebra/FFaTensor3.H"
#include "FFaLib/FFaAlgebra/FFaVec3.H"

using FaEdge = std::pair<size_t,size_t>; //!< Vertex indices of an edge
class FaFace;
class FaMat34;


/*!
  \brief Class representing the surface geometry a volumetric object.
  \details This class is used to represent the geometry of Generic Parts in
  Fedem, and is equipped with methods for calculating the intersection between
  the geometry and a plane. This can be used for buoyancy calculation of partly
  submerged rigid objects.
*/

class FFaBody
{
public:
  //! \brief Default constructor.
  FFaBody() : myVolume(0.0), startVx(0)
  { isVolumeComputed = isBBoxComputed = false; }

  //! \brief Returns the total number of vertices in the body.
  size_t getNoVertices() const { return myVertices.size(); }
  //! \brief Returns the total number of faces in the body.
  size_t getNoFaces() const { return myFaces.size(); }

  //! \brief Computes the bounding box of the body.
  bool computeBoundingBox(FaVec3& minX, FaVec3& maxX) const;
  //! \brief Computes the total volume and volume centroid of the body.
  //! \details Optionally the volume inertia is also computed.
  bool computeTotalVolume(double& Vb, FaVec3& C0b, FFaTensor3* Ib = NULL) const;
  //! \brief Computes the volume of a body portion below of given plane.
  bool computeVolumeBelow(double& Vb, double& As, FaVec3& C0b, FaVec3& C0s,
			  const FaVec3& normal, double z0,
			  double zeroTol = 1.0e-16);

  //! \brief Saves current plane intersection.
  bool saveIntersection(const FaMat34& cs);
  //! \brief Computes the intersection area increment.
  bool computeIncArea(double& dAs, FaVec3& C0s,
		      const FaVec3& normal, const FaMat34& cs);

  //! \brief Adds a face to the body definition.
  size_t addFace(int v1, int v2, int v3, int v4 = -1);
  //! \brief Adds a vertex to the body definition.
  size_t addVertex(const FaVec3& pos, double tol = -1.0);
  //! \brief Returns the coordinates of a specified vertex.
  const FaVec3& getVertex(size_t idx) const { return myVertices[idx]; }
  //! \brief Returns the global index of a face vertex.
  int getFaceVtx(size_t idx, int i) const;

  //! \brief Creates a new body by parsing the given input stream.
  static FFaBody* readFromCAD(std::istream& in, double duplTol = -1.0);

  //! \brief Exports the body to specified CAD file.
  bool writeCAD(const std::string& fileName, const FaMat34& partCS) const;

protected:
  //! \brief Creates a new body by parsing STL content from an input stream.
  static FFaBody* readSTL(std::istream& in, double duplTol);
  //! \brief Creates a new body by parsing CAD content from an input stream.
  static FFaBody* readCAD(std::istream& in);
  //! \brief Creates a new body by parsing VRML content from an input stream.
  static FFaBody* readWRL(std::istream& in, int version);

  //! \cond DO_NOT_DOCUMENT
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
  //! \endcond

public:
  static std::string prefix; //!< File prefix to use for included files

private:
  mutable bool        isBBoxComputed; //!< If \e true, \ref myBBox is defined
  mutable FaVec3      myBBox[2];      //!< Bounding box of the body
  std::vector<FaVec3> myVertices;     //!< Vertex container of the body
  std::vector<FaFace> myFaces;        //!< Face container of the body
  FaVec3              myX0s;          //!< Centroid of the plane intersection
  std::vector<FaVec3> myLoopVer;      //!< Vertices defining the intersection
  std::vector<FaEdge> myIntLoop;      //!< Edges of the intersection surface
  mutable char      isVolumeComputed; //!< If \e true, \ref myVolume is computed
  mutable double      myVolume;       //!< The volume of this body
  mutable FFaTensor3  myInertia;      //!< The volume intertia of this body
  mutable FaVec3      myCentroid;     //!< The volume centroid of this body
  size_t              startVx;        //!< Number of vertices before intersected
};


/*!
  \brief Class representing a surface segment of a FFaBody object.
  \details The class also have data and methods for calculating the intersection
  of the face with a given plane.
*/

class FaFace
{
public:
  //! \brief Default face constructor.
  FaFace();
  //! \brief Constructor defining a face spanned by three or four vertices.
  FaFace(FFaBody* body, size_t i1, size_t i2, size_t i3, int i4 = -1);

  //! \brief Calculates the intersection with a given plane.
  int intersect(const FaVec3& normal, double z0, double zeroTol = 1.0e-16);
  //! \brief Returns \e true if face is intersected by the intersection plane.
  bool isIntersected() const { return iEdge.first != iEdge.second; }
  //! \brief Returns \e true if face is totally below the intersection plane.
  bool isBelow() const { return IAmBelow; }

  //! \brief Returns the number of face vertices.
  size_t size() const { return myVertices.size(); }
  //! \brief Returns the global index of the \a i'th vertex of this face.
  size_t operator[](int i) const { return myVertices[i]; }
  //! \brief Returns the coordinates of the \a i'th vertex of this face.
  const FaVec3& vertex(int i) const { return owner->getVertex(myVertices[i]); }

  //! \brief Accumulates area of an intersection surface.
  double accumulateArea(const FaVec3& vn, const FaVec3& v0, FaVec3& X) const;
  //! \brief Accumulates volume with contribution from *this as surface segment.
  double accumulateVolume(const FaVec3& v0, FaVec3& X, FFaTensor3* I) const;

  //! \brief Returns the sub-faces of the face when it is intersected.
  const std::vector<FaFace>& getSubFaces() const { return mySubFaces; }
  //! \brief Returns the index of the intersecting edge.
  FaEdge getIntEdge() const { return iEdge; }
  //! \brief Returns the sum of the end points of the intersecting edge.
  FaVec3 getIntEdgeCoord() const;
  //! \brief Returns the \ref owner of this face.
  FFaBody* getBody() const { return owner; }

protected:
  //! \brief Divides the face into two quadrilateral sub-faces.
  int quad2Quads(const char* status, const double* dist);
  //! \brief Divides the face into one triangle and two quadrilaterals.
  int quad2QuadTria(const char* status, const double* dist, bool oneAbove);

  //! \brief Global stream operator printing the face definition.
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
