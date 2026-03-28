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

#include <algorithm>
#include <functional>

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

bool FFlVisFace::setFaceVertices(const std::vector<FFlVertex*>& vertices,
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
      myEdges.emplace_back(edge->setVertices(*it,*jt));
      FFlEdgePtBoolPair edgePair = tester.insertEdge(edge);
      myEdges.back() = *edgePair.first;
      if (!edgePair.second)
        delete edge;
      else
        edgeContainer.push_back(edge);
    }
  }

  if (myEdges.size() < 3)
    return false; // degenerated face

  // Rotate the edge sequence to make the min element first

  VisEdgeRefVec::iterator iEdge = std::min_element(myEdges.begin(),myEdges.end());
  faceRef.elementFaceNodeOffset = std::distance(myEdges.begin(),iEdge);
  std::rotate(myEdges.begin(),iEdge,myEdges.end());

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

  return true;
}


/*!
  Creates a vector of vertex indices for the face.
  The face has canonical form.
*/

void FFlVisFace::getFaceVertices(std::vector<int>& vertexRef) const
{
  vertexRef.clear();
  vertexRef.reserve(myEdges.size());

  for (const FFlVisEdgeRef& edge : myEdges)
    if (FFlVertex* firstVertex = edge.getFirstVertex(); firstVertex)
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
  // Lambda function extracting the id of the first vertex of an edge.
  std::function<void(const FFlVisEdgeRef&)> getFirstVertex =
    [&vertexIdxArrayPtr](const FFlVisEdgeRef& ref) -> void
  {
    if (FFlVertex* vertx = ref.getFirstVertex(); vertx)
      *vertexIdxArrayPtr = vertx->getRunningID();
    ++vertexIdxArrayPtr;
  };

  if (myElementRefs.empty() || myElementRefs.front().elementAndFaceNormalParallel)
    for (const FFlVisEdgeRef& ref : myEdges)
      getFirstVertex(ref);
  else if (VisEdgeRefVecCIter it = myEdges.begin(); it != myEdges.end())
  {
    getFirstVertex(*it);
    it = myEdges.end();
    for (--it; it != myEdges.begin(); --it)
      getFirstVertex(*it);
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


bool FFlVisFace::getFaceNormal(FaVec3& normal)
{
  int nEdges = myEdges.size();
  if (nEdges < 3)
    return false; // degenereated face, normal is undefined

  if (nEdges > 4)
  {
    int i2 = nEdges/2;
    int i4 = nEdges/4;
    normal = ((*(myEdges[i2].getFirstVertex()) - *(myEdges.front().getFirstVertex())) ^
              (*(myEdges[i2+i4].getFirstVertex()) - *(myEdges[i4].getFirstVertex())));
  }
  else
  {
    // Get the first vector
    VisEdgeRefVecCIter it = myEdges.begin();
    FaVec3 vec1 = *it->getSecondVertex() - *it->getFirstVertex();

    // Find a usable second vector
    for (++it; it != myEdges.end(); ++it)
      if (FaVec3 vec2 = *it->getSecondVertex() - *it->getFirstVertex();
          !vec1.isParallell(vec2))
      {
        normal = vec1 ^ vec2;
        break;
      }

    if (it == myEdges.end())
    {
      std::cout <<"  ** Surface normal creation failed.\n     Element refs:";
      for (const FFlFaceElemRef& ref : myElementRefs)
        std::cout <<" "<< ref.myElement->getID() <<" ("
                  << static_cast<int>(ref.myElementFaceNumber) <<")";
      std::cout <<"\n     All face edges are parallel:";
      for (it = myEdges.begin(); it != myEdges.end(); ++it)
        std::cout <<"\n\t"<< *it->getFirstVertex() - *it->getSecondVertex();
      std::cout << std::endl;
      return false;
    }
  }

  normal.normalize();
  return true;
}


bool FFlVisFace::getElmFaceNormal(FaVec3& normal)
{
  if (!this->getFaceNormal(normal))
    return false;

  if (!myElementRefs.empty())
    if (!myElementRefs.front().elementAndFaceNormalParallel)
      normal = -normal;

  return true;
}


bool FFlVisFace::isVisible() const
{
#ifdef FT_USE_VISUALS
  for (const FFlFaceElemRef& ref : myElementRefs)
    if (!ref.myElement->getDetail())
      return true;
    else if (ref.myElement->getDetail()->detail.getValue())
      return true;

  return false;
#else
  return true;
#endif
}


bool FFlVisFace::nextEdge(VisEdgeRefVecCIter& eit,
                          const VisEdgeRefVecCIter& splitEdgeIt,
                          bool loopForward) const
{
  if (loopForward)
  {
    if (eit == myEdges.end())
      eit = myEdges.begin();
    else if (++eit == myEdges.end() && eit != splitEdgeIt)
      eit = myEdges.begin();
  }
  else
  {
    if (eit == myEdges.begin())
    {
      if (splitEdgeIt != myEdges.end())
        eit = myEdges.end();
      else
        return false;
    }
    --eit;
  }

  return eit != splitEdgeIt;
}


/*!
  Used to sort and compare faces.
  Sorting order: Smallest edge list first.
  If same size, check each edge for difference.
*/

bool FFlVisFace::FFlVisFaceLess::operator()(const FFlVisFace* a,
                                            const FFlVisFace* b) const
{
  int i = a->myEdges.size();
  int j = b->myEdges.size();
  if (i < j)
    return true;
  else if (i > j)
    return false;

  for (--i; i >= 0; --i)
    if (a->myEdges[i] < b->myEdges[i])
      return true;
    else if (a->myEdges[i] > b->myEdges[i])
      return false;

  return false;
}
