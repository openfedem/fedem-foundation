// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFL_VIS_FACE_H
#define FFL_VIS_FACE_H

#include "FFlLib/FFlVisualization/FFlVisEdge.H"

class FFlElementBase;
class FFlGeomUniqueTester;


/*!
  \brief Struct used in faces to store reference to the element creating it.
*/

struct FFlFaceElemRef
{
  FFlElementBase* myElement = NULL;
  char myElementFaceNumber = 0;
  char elementFaceNodeOffset = 0;
  bool elementAndFaceNormalParallel = false;
};

// Type aliases to be used with the FFlVisFace class

using VisEdgeRefVec  = std::vector<FFlVisEdgeRef>;
using FaceElemRefVec = std::vector<FFlFaceElemRef>;

using VisEdgeRefVecCIter  = VisEdgeRefVec::const_iterator;
using FaceElemRefVecCIter = FaceElemRefVec::const_iterator;


class FFlVisFace
{
public:
  FFlVisFace();

  bool setFaceVertices(const std::vector<FFlVertex*>& vertices,
                       std::vector<FFlVisEdge*>& edgeContainer,
                       FFlFaceElemRef& faceRef,
                       FFlGeomUniqueTester& tester);

  // Get face running vertex idxes

  void getElmFaceVertices(std::vector<int>& vertexIdxes)  const;
  void getElmFaceVertices(int*& vertexIdxArrayPtr) const;
  void getFaceVertices(std::vector<int>& vertexIdxes) const;
  int  getNumVertices() const { return myEdges.size(); }

  void getElmFaceTopology(std::vector<int>& topology,
                          const FaceElemRefVecCIter& elmRefIt);

  // Element face normal or normal of face
  // (Face normal "always" consistent with edge order and directions)

  bool getElmFaceNormal(FaVec3& normal);
  bool getFaceNormal   (FaVec3& normal);

  // Ref-counting from elements myRefCount = nElms using this face

  int ref()           { return ++myRefCount; }
  int unRef()         { return --myRefCount; }
  int getRefs() const { return   myRefCount; }

  // Mark face as a shell face

  void setShellFace()      { iAmShellFace = true; }
  bool isShellFace() const { return iAmShellFace; }

  // Return surface status of face

  bool isVisible() const;
  bool isSurfaceFace() const { return iAmShellFace || myRefCount == 1; }
  bool isExpandedFace() const { return IAmAnExpandedFace; }
  void setIsExpandedFace(bool isExpanded) { IAmAnExpandedFace = isExpanded; }

  // Polygon reduction traversal marking

  void setVisited(bool yesOrNo = true) { IAmVisited = yesOrNo; }
  bool isVisited() const               { return IAmVisited; }

  // Iterate over the face edges from splitEdgeIt

  bool nextEdge(VisEdgeRefVecCIter& eit,
                const VisEdgeRefVecCIter& splitEdgeIt,
                bool loopForward) const;

  // Define the geometry status of the edges of a surface

  void setEdgeGeomStatus(double angleTol);

  // Edge access

  VisEdgeRefVecCIter edgesBegin() const { return myEdges.begin(); }
  VisEdgeRefVecCIter edgesEnd()   const { return myEdges.end(); }

  // Element reference access

  FaceElemRefVecCIter elementRefsBegin() const { return myElementRefs.begin(); }
  FaceElemRefVecCIter elementRefsEnd()   const { return myElementRefs.end(); }

  // Managing element references

  void addFaceElemRef(const FFlFaceElemRef& fe) { myElementRefs.push_back(fe); }
  void deleteFaceElemRef() { FaceElemRefVec empty; myElementRefs.swap(empty); }

  // Functor object to make comparing faces possible

  struct FFlVisFaceLess
  {
    bool operator()(const FFlVisFace* a, const FFlVisFace* b) const;
  };

protected:
  VisEdgeRefVec  myEdges;
  FaceElemRefVec myElementRefs;

  char myRefCount;
  bool iAmShellFace;
  bool IAmVisited;
  bool IAmAnExpandedFace;

  friend struct FFlVisFaceLess;

#ifdef FT_USE_MEMPOOL
  FFA_MAKE_MEMPOOL;
#endif
};

#endif
