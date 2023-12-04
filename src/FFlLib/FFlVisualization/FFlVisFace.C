// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFlLib/FFlVisualization/FFlVisFace.H"
#include "FFlLib/FFlVisualization/FFlVisEdge.H"
#include "FFlLib/FFlVertex.H"
#include "FFlLib/FFlElementBase.H"
#include "FFlLib/FFlFEElementTopSpec.H"
#include "FFlLib/FFlFEParts/FFlVDetail.H"
#include "FFlLib/FFlVisualization/FFlGeomUniqueTester.H"

#ifdef FT_USE_MEMPOOL
FFaMemPool FFlVisFace::ourMemPool(sizeof(FFlVisFace));
#endif


FFlVisFace::FFlVisFace()
{
  myRefCount = 0;
  iAmShellFace = IAmVisited = IAmAnExpandedFace = false;
}


/*!
  Creates edges and a face for the given vertices.
  The edgeContainer is used for storing new edges.
  The face indexes are stored in canonical form.
  The method fills reference data (offset and normal)
  into faceRef in order to restore the original face indices.
*/

void FFlVisFace::setFaceVertices(const std::vector<FFlVertex*>& vertices,
                                 std::vector<FFlVisEdge*>& edgeContainer,
                                 FFlFaceElemRef& faceRef,
                                 FFlGeomUniqueTester& tester)
{
  myEdges.clear();
  myEdges.reserve(vertices.size());

  // Create edges

  std::vector<FFlVertex*>::const_iterator it, jt;
  for (it = vertices.begin(); it != vertices.end(); ++it)
  {
    jt = it+1 == vertices.end() ? vertices.begin() : it+1;
    if (!(*it)->equals(**jt,1.0e-12)) // Avoid adding degenerated edges
    {
      FFlVisEdge* edge = new FFlVisEdge();
      myEdges.push_back(FFlVisEdgeRef());
      myEdges.back().setPosDir(edge->setVertices(*it,*jt));
      FFlEdgePtBoolPair edgePair = tester.insertEdge(edge);
      myEdges.back() = *edgePair.first;
      if (!edgePair.second)
        delete edge;
      else
        edgeContainer.push_back(edge);
    }
  }

  // Rotate the sequence to make the min element first

  std::vector<FFlVisEdgeRef>::iterator minEdge = std::min_element(myEdges.begin(),myEdges.end());

  faceRef.elementFaceNodeOffset = std::distance(myEdges.begin(),minEdge);

  std::rotate(myEdges.begin(),minEdge,myEdges.end());

  // Reverse direction such that the second idx is the lowest of the two candidates

  if (myEdges.size() > 2 && myEdges.back() < myEdges[1])
  {
    // Reverse edge list
    std::reverse(myEdges.begin()+1,myEdges.end());
    faceRef.elementAndFaceNormalParallel = false;

    // Reverse edge directions
    for (FFlVisEdgeRef& edge : myEdges)
      edge.setPosDir(!edge.isPosDir());
  }
  else
    faceRef.elementAndFaceNormalParallel = true;
}


/*!
  Creates a vector of vertex indices for the face.
  The face has canonical form.
*/

void FFlVisFace::getFaceVertices(std::vector<int>& vertexRef) const
{
  vertexRef.clear();
  vertexRef.reserve(myEdges.size());

  FFlVertex* firstVertex;
  for (const FFlVisEdgeRef& edge : myEdges)
    if ((firstVertex = edge.getFirstVertex()))
      vertexRef.push_back(firstVertex->getRunningID());
}


/*!
  Creates a vector of vertex indices for the face.
  The face is orientated with respect to the original definition of the face
  if it has only been referenced once.
  For multi-referenced face, the first reference direction is used.
  TODO: Prioritize normals from shell elements.
*/

void FFlVisFace::getElmFaceVertices(std::vector<int>& vertexRef) const
{
  this->getFaceVertices(vertexRef);

  if (!myElementRefs.empty())
    if (!myElementRefs.front().elementAndFaceNormalParallel)
      std::reverse(vertexRef.begin()+1,vertexRef.end());
}


void FFlVisFace::getElmFaceVertices(int*& vertexIdxArrayPtr) const
{
  if (!myElementRefs.empty() && !myElementRefs.front().elementAndFaceNormalParallel)
  {
    VisEdgeRefVecCIter it = myEdges.begin();
    if (it != myEdges.end())
    {
      FFlVertex* firstVertex = it->getFirstVertex();
      if (firstVertex)
        *vertexIdxArrayPtr = firstVertex->getRunningID();
      ++vertexIdxArrayPtr;
      it = myEdges.end();
      for (--it; it != myEdges.begin(); --it)
      {
        firstVertex = it->getFirstVertex();
        if (firstVertex)
          *vertexIdxArrayPtr = firstVertex->getRunningID();
        ++vertexIdxArrayPtr;
      }
    }
  }
  else
    for (const FFlVisEdgeRef& ref : myEdges)
    {
      FFlVertex* firstVertex = ref.getFirstVertex();
      if (firstVertex)
        *vertexIdxArrayPtr = firstVertex->getRunningID();
      ++vertexIdxArrayPtr;
    }
}


void FFlVisFace::getElmFaceTopology(std::vector<int>& topology,
                                    const FaceElemRefVecCIter& elmRefIt)
{
  elmRefIt->myElement->getFEElementTopSpec()->
    getFaceTopology(elmRefIt->myElementFaceNumber,
                     this->isExpandedFace(),
                     !elmRefIt->elementAndFaceNormalParallel,
                     elmRefIt->elementFaceNodeOffset,
                     topology);

  if (!myElementRefs.empty())
    if (!myElementRefs.front().elementAndFaceNormalParallel)
      std::reverse(topology.begin()+1,topology.end());
}


void FFlVisFace::getFaceNormal(FaVec3& normal)
{
  static FaVec3 vec2;
  static FaVec3 vec1;

  int nEdges = myEdges.size();
  if (nEdges < 3)
    return; // degenereated face, normal is undefined

  if (nEdges <= 4)
  {
    // Get the first vector
    VisEdgeRefVecCIter it = myEdges.begin();
    vec1 = *it->getSecondVertex() - *it->getFirstVertex();

    // Find a usable second vector
    for (++it; it != myEdges.end(); ++it)
    {
      vec2 = *it->getSecondVertex() - *it->getFirstVertex();
      if (!vec1.isParallell(vec2))
        break;
    }

    if (it == myEdges.end())
    {
      std::cerr <<"Error in surface normal creation\n    Element refs:";
      for (const FFlFaceElemRef& ref : myElementRefs)
        std::cout << ref.myElement->getID() <<" ("<< ref.myElementFaceNumber <<")";
      std::cout << std::endl;
      return;
    }
  }
  else
  {
    int nEdgesH = nEdges/2;
    int nEdgesQ = nEdges/4;
    vec1 = *(myEdges[nEdgesH].getFirstVertex()) - *(myEdges.front().getFirstVertex());
    vec2 = *(myEdges[nEdgesH + nEdgesQ].getFirstVertex()) - *(myEdges[nEdgesQ].getFirstVertex());
  }

  normal = vec1 ^ vec2;
  normal.normalize();
}


void FFlVisFace::getElmFaceNormal(FaVec3& normal)
{
  this->getFaceNormal(normal);
  if (!myElementRefs.empty())
    if (!myElementRefs.front().elementAndFaceNormalParallel)
      normal = -normal;
}


bool FFlVisFace::isVisible() const
{
  for (const FFlFaceElemRef& ref : myElementRefs)
    if (!ref.myElement->getDetail())
      return true;
    else if (ref.myElement->getDetail()->detail.getValue())
      return true;

  return false;
}


/*!
  Used to sort and compare faces.
  Sorting order: Smallest edge list first.
  If same size, check each edge for difference.
*/

bool FFlVisFace::FFlVisFaceLess::operator()(const FFlVisFace* a,
                                            const FFlVisFace* b) const
{
  register size_t sizeA = a->myEdges.size();
  register size_t sizeB = b->myEdges.size();
  if (sizeA < sizeB)
    return true;
  else if (sizeA > sizeB)
    return false;

  static std::vector<FFlVisEdgeRef>::const_reverse_iterator aIt;
  static std::vector<FFlVisEdgeRef>::const_reverse_iterator bIt;

  aIt = a->myEdges.rbegin();
  bIt = b->myEdges.rbegin();
  for (; aIt != a->myEdges.rend(); ++aIt, ++bIt)
    if (*aIt < *bIt)
      return true;
    else if (*aIt > *bIt)
      return false;

  return false;
}
