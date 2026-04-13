// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFlLib/FFlVisualization/FFlGroupPartCreator.H"
#include "FFlLib/FFlVisualization/FFlTesselator.H"
#include "FFlLib/FFlLinkHandler.H"
#include "FFlLib/FFlVertex.H"
#include "FFaLib/FFaAlgebra/FFaMath.H"

#include <functional>


/*!
  \class FFlGroupPartCreator

  Idea to completely new geometry generation "engine" that removes
  the need for edges and faces to be explicitly instantiated,
  and is leaner and faster perhaps...

  Nodes will then have pointers to all the elements they are a member of.
  We will generate geometry by traversing the nodes.
  Start at some node,
    Get all unique geometry that refers to that node.
      Unique in the context of all the elements that refer to that node,
        and that the geometry does not contain some other visited node.
      The geometry needs to be sorted in cathegories like internal, surface,
      outline, but all that information should be locally evaluable.
        The node that was "responsible" for the creation of a certain geometry,
        is used as the first node in the geometry (to keep track of that for
        deletion, regeneration, etc.)
    Mark node as visited
    Do the same for all surrounding nodes recursively
    (Needs to handle that case where not all nodes are connected)
*/

FFlGroupPartCreator::FFlGroupPartCreator(FFlLinkHandler* lh)
  : FFlFaceGenerator(lh), myVertices(lh->getVertexes())
{
  myOutlineEdgeMinAngle = M_PI/4;
  myEdgesParallelAngle = 0.002;  // ca 0.1 degs
  myFaceReductionAngle = 0.05;

  IAmIncludingInOpsDir = false;
  joinFacesFromEdgeMethodCount = 0;
}


FFlGroupPartCreator::~FFlGroupPartCreator()
{
  for (GroupPartMap::value_type& gp : myLinkParts)
    delete gp.second;

  for (GroupPartMap::value_type& gp : mySpecialLines)
    delete gp.second;
}


void FFlGroupPartCreator::deleteShapeIndexes()
{
  for (GroupPartMap::value_type& gp : myLinkParts)
  {
    std::vector<IntVec> emptyVec;
    gp.second->shapeIndexes.swap(emptyVec);
  }

  for (GroupPartMap::value_type& gp : mySpecialLines)
  {
    std::vector<IntVec> emptyVec;
    gp.second->shapeIndexes.swap(emptyVec);
  }
}


/*!
  Regenerates the special visualization lines in the model
  (that is, all lines which are not associated with FE results).
  If \a XZscale is positive, only the lines visualizing the
  local beam systems are updated, and with their absolute
  length set to the provided value.
*/

bool FFlGroupPartCreator::recreateSpecialLines(double XZscale)
{
  if (XZscale != 0.0 && myBeamSysEdges.empty())
    return false;

  for (GroupPartMap::value_type& gp : mySpecialLines)
    delete gp.second;
  mySpecialLines.clear();

  if (XZscale == 0.0) // regenerate all spider element lines
    this->recreateSpecialEdges();

  this->createSpecialLines(XZscale);
  return true;
}


void FFlGroupPartCreator::makeLinkParts()
{
  for (unsigned short int partType = 0; partType <= INTERNAL_FACES; partType++)
    myLinkParts[partType] = new FFlGroupPartData();

  this->setEdgeGeomStatus();

  this->createLinkFullFaces(*myLinkParts[INTERNAL_FACES],
                            *myLinkParts[SURFACE_FACES]);

  this->createLinkFullEdges(*myLinkParts[INTERNAL_LINES],
                            *myLinkParts[SURFACE_LINES],
                            *myLinkParts[OUTLINE_LINES]);

  if (mySpecialLines.empty())
    this->createSpecialLines();

  this->createLinkReducedFaces(*myLinkParts[RED_INTERNAL_FACES],
                               *myLinkParts[RED_SURFACE_FACES]);

  this->createLinkReducedEdges(*myLinkParts[RED_INTERNAL_LINES],
                               *myLinkParts[RED_SURFACE_LINES],
                               *myLinkParts[RED_OUTLINE_LINES]);

  for (FFlVisEdge* edge : myVisEdges)
    edge->deleteRenderData();
  for (FFlVisEdge* edge : mySpecialEdges)
    edge->deleteRenderData();
}


/*!
  Find and create the geometrical status of the edges of the face,
  to be used when looping over the edges :
*/

void FFlGroupPartCreator::setEdgeGeomStatus()
{
  VisEdgeRefVecCIter edgeIt;

  for (FFlVisFace* face : myVisFaces)
    if (face->isSurfaceFace())
    {
      // If face is surface face :

      FaVec3 surfNorm;
      bool degenerated = !face->getFaceNormal(surfNorm);

      // Loop Over the edges of the surface face :

      for (edgeIt = face->edgesBegin(); edgeIt != face->edgesEnd(); ++edgeIt)
      {
	FFlVisEdgeRenderData* edgeRenderData = (*edgeIt)->getRenderData();
	edgeRenderData->faceReferences.emplace_back(face,surfNorm);

	if (degenerated || (*edgeIt)->getRefs() == 1)
	  edgeRenderData->edgeStatus = FFlVisEdgeRenderData::OUTLINE;
	else if (edgeRenderData->edgeStatus == FFlVisEdgeRenderData::INTERNAL)
	  edgeRenderData->edgeStatus = FFlVisEdgeRenderData::SURFACE;
	else if (edgeRenderData->edgeStatus == FFlVisEdgeRenderData::SURFACE)
	  // compare surfNorm to the edges existing surface normals.
	  // If some angle > OutlineEdgeMinAngle, upgrade to OUTLINE,
	  // else add this surface normal to the vector:
          for (const FFlFaceRef& fn : edgeRenderData->faceReferences)
            if (surfNorm.angle(fn.second) >= myOutlineEdgeMinAngle &&
                surfNorm.angle(-fn.second) >= myOutlineEdgeMinAngle)
            {
              edgeRenderData->edgeStatus = FFlVisEdgeRenderData::OUTLINE;
              break;
            }
      }
    }
}


void FFlGroupPartCreator::createLinkFullFaces(FFlGroupPartData& internal,
                                              FFlGroupPartData& surface)
{
  internal.isLineShape  = surface.isLineShape  = false;
  internal.isIndexShape = surface.isIndexShape = false;

  // Loop over the faces, add them into the result link parts.

  for (FFlVisFace* face : myVisFaces)
    if (face->isSurfaceFace())
    {
      if (face->isVisible())
        surface.facePointers.emplace_back(face,-1);
      else
        surface.hiddenFaces.emplace_back(face,-1);
    }
    else // Internal face
    {
      if (face->isVisible())
        internal.facePointers.emplace_back(face,-1);
      else
        internal.hiddenFaces.emplace_back(face,-1);
    }
}


void FFlGroupPartCreator::updateElementVisibility()
{
#ifdef FFL_DEBUG
  size_t gpNr = 0;
#endif

  for (GroupPartMap::value_type& gp : myLinkParts)
    if (gp.first == SURFACE_FACES || gp.first == INTERNAL_FACES)
      {
        std::vector<FFlVisFaceIdx> toBeHidden, toBeShown;
        std::vector<FFlVisFaceIdx>& faces = gp.second->facePointers;
        std::vector<FFlVisFaceIdx>& hfaces = gp.second->hiddenFaces;

	// Loop over visible faces.

	size_t faceNr = 0;
	int emptyVisiblFacePlace = -1;
	for (faceNr = 0; faceNr < faces.size(); ++faceNr)
	  if (!faces[faceNr].first->isVisible())
	  {
	    toBeHidden.push_back(faces[faceNr]);
	    if (emptyVisiblFacePlace == -1)
	      emptyVisiblFacePlace = faceNr;
	  }
	  else
	  {
	    if (emptyVisiblFacePlace >= 0)
	      faces[emptyVisiblFacePlace++] = faces[faceNr];
	  }

	// Loop over hidden faces.

	int emptyHiddenFacePlace = -1;
	for (faceNr = 0; faceNr < hfaces.size(); ++faceNr)
	  if (hfaces[faceNr].first->isVisible())
	  {
	    toBeShown.push_back(hfaces[faceNr]);
	    if (emptyHiddenFacePlace == -1)
	      emptyHiddenFacePlace = faceNr;
	  }
	  else if (emptyHiddenFacePlace >= 0)
	    hfaces[emptyHiddenFacePlace++] = hfaces[faceNr];

	// Add newly shown faces to facePointers

	if (emptyVisiblFacePlace == -1)
	  emptyVisiblFacePlace = faces.size();

	faceNr = emptyVisiblFacePlace;
	faces.resize(emptyVisiblFacePlace + toBeShown.size());
	for (const FFlVisFaceIdx& face : toBeShown)
	  faces[faceNr++] = face;

	// Add newly hidden faces to hiddenFaces

	if (emptyHiddenFacePlace == -1)
	  emptyHiddenFacePlace = hfaces.size();

	faceNr = emptyHiddenFacePlace;
	hfaces.resize(emptyHiddenFacePlace + toBeHidden.size());
	for (const FFlVisFaceIdx& face : toBeHidden)
	  hfaces[faceNr++] = face;

#ifdef FFL_DEBUG
	if (gp.first == SURFACE_FACES)
	  std::cout <<"SURFACE_FACES "<< ++gpNr;
	else
	  std::cout <<"INTERNAL_FACES "<< ++gpNr;
	std::cout <<"\nVisible faces:";
	for (const FFlVisFaceIdx& face : faces)
	  std::cout <<" "<< face.second;
	std::cout <<"\nHidden faces:";
	for (const FFlVisFaceIdx& face : hfaces)
	  std::cout <<" "<< face.second;
	std::cout << std::endl;
#endif
      }

#if FFL_DEBUG > 1
  this->dump();
#endif
}


void FFlGroupPartCreator::createLinkReducedFaces(FFlGroupPartData& internal,
                                                 FFlGroupPartData& surface)
{
  internal.isLineShape  = surface.isLineShape  = false;
  internal.isIndexShape = surface.isIndexShape = true;

  for (FFlVisFace* face : myVisFaces)
    if (!face->isVisited())
      if (FaVec3 normal; face->getElmFaceNormal(normal))
      {
        IntList polygon;
        this->expandPolygon(polygon,*face,normal);

        // Tesselate the reduced polygon,
        // and add triangles to shape indices in group parts
        if (face->isSurfaceFace())
          FFlTesselator::tesselate(surface.shapeIndexes,
                                   polygon,myVertices,normal);
        else
          FFlTesselator::tesselate(internal.shapeIndexes,
                                   polygon,myVertices,normal);
      }
}


void FFlGroupPartCreator::expandPolygon(IntList& polygon, FFlVisFace& f,
                                        const FaVec3& normal)
{
  polygon.clear();
  f.setVisited();

  // Fill indexes from initial face into polygon vxidx list:
  // Making the last edge explisitly defined in polygon by
  // inserting the start index at the end

  FaVec3 fNormal;
  f.getFaceNormal(fNormal);
  bool faceIsPositive = normal*fNormal > 0.0;

  IntList facePolygon;
  if (faceIsPositive)
    this->getPolygonPosFace(facePolygon, f, f.edgesBegin());
  else
  {
    VisEdgeRefVecCIter splEdge = f.edgesEnd(); --splEdge;
    this->getPolygonNegFace(facePolygon, f, splEdge);
  }

  polygon.splice(polygon.begin(), facePolygon);
  polygon.push_front(polygon.back());

#if FFL_DEBUG > 2
  std::cout <<"New Polygon:";
  for (int idx : polygon) std::cout <<" "<< idx;
  std::cout <<"\nNormal: "<< normal <<" "
            << std::boolalpha << faceIsPositive << std::endl;
#endif

  // Loop over all the edges of the start polygon and
  // do a recursive breath first joining of faces

  VisEdgeRefVecCIter edgeIt = f.edgesEnd();
  IntList::iterator nextSplEdgEndPolyIt = polygon.begin();
  ++nextSplEdgEndPolyIt;
  IAmIncludingInOpsDir = false;

  while (f.nextEdge(edgeIt,f.edgesEnd(),faceIsPositive))
  {
    joinFacesFromEdgeMethodCount = 0;
    this->joinFacesFromEdge(polygon, nextSplEdgEndPolyIt, *edgeIt, f,
                            faceIsPositive, f.isSurfaceFace(), normal);
    ++nextSplEdgEndPolyIt;
  }
  polygon.pop_back();

#if FFL_DEBUG > 2
  std::cout <<"Before Removing Dead Ends:";
  for (int idx : polygon) std::cout <<" "<< idx;
  std::cout << std::endl;
#endif

  // Remove Dead ends :

  IntList::iterator it1, it2, tmpIt, tmpIt2;

  // Initialize It1 and It2 to be separated by one, It1 first:

  it2 = polygon.end();
  for (int i = 0; i < 2; ++i)
    if (it2 != polygon.begin()) --it2;
    else return; // Polygon To Small

  it1 = polygon.begin();

  bool goneAround = false;

  while ( !goneAround && it1 != polygon.end() )
  {
    if (it2 == polygon.end()) it2 = polygon.begin();

    // If the indexes pointed to by the two iterators(separated by one)
    // are equal, we have a dead end. Cut it off

    if (*it2 == *it1)
    {
      tmpIt = it2;
      tmpIt2 = it2;

      if (it2 == polygon.begin()) it2 = polygon.end();
      --it2;
      if (it2 == it1) goneAround = true;

      if (it2 == polygon.begin()) it2 = polygon.end();
      --it2;
      if (it2 == it1) goneAround = true;

      if (!goneAround)
      {
	++tmpIt2;
	if (tmpIt2 == polygon.end()) tmpIt2 = polygon.begin();
	polygon.erase(tmpIt);
	polygon.erase(tmpIt2);
      }
      else
	polygon.clear();
    }
    else
    {
      ++it1;
      ++it2;
    }
  }

#if FFL_DEBUG > 2
  std::cout <<"After Removing Dead Ends:";
  for (int idx : polygon) std::cout <<" "<< idx;
  std::cout << std::endl;
#endif

  tmpIt = polygon.begin();
  for (int i = 0; i < 3; i++)
    if (tmpIt == polygon.end())
      return; // polygon too small
    else if (i < 2)
      ++tmpIt;

  // Initialize it1, it2, i3 to point on the tree first polygon indices
  it1 = tmpIt;
  it2 = polygon.begin();
  IntList::iterator it3 = it2++;

  bool start = true;
  while ((it1 != tmpIt || start) && it1 != it2 && it1 != it3)
  {
    // If the indices pointed to by the three iterators
    // are on same line we'll remove the middle point
    FaVec3 first  = *myVertices[*it2] - *myVertices[*it3];
    FaVec3 second = *myVertices[*it1] - *myVertices[*it2];

    if (first.angle(second) < myEdgesParallelAngle)
    {
      if (it2 == tmpIt)
      {
        tmpIt = it1;
        start = true;
      }
      polygon.erase(it2);
      it2 = it3;
      if (it3 == polygon.begin())
        it3 = polygon.end();
      --it3;
    }
    else
    {
      start = false;
      if (++it1 == polygon.end()) it1 = polygon.begin();
      if (++it2 == polygon.end()) it2 = polygon.begin();
      if (++it3 == polygon.end()) it3 = polygon.begin();
    }
  }
#if FFL_DEBUG > 2
  std::cout <<"After Simplifying Straight Lines:";
  for (int idx : polygon) std::cout <<" "<< idx;
  std::cout << std::endl;
#endif
}


/*!
  This method contains the logic on what faces are allowed to be joined.
  It also marks the faces joined (or othervise not suitable for further joining)
  as visited.

  It loops through the faces shared by the splitting edge (edgeIt)
  checks facenormal, whether faces are on top of each other in same plane,
  and finds the biggest joinable face.

  It then includes that face in the polygon and calls this method recursively
  to include all the faces around the edges of the face.
*/

void FFlGroupPartCreator::joinFacesFromEdge(IntList                  & polygon,
					    const IntList::iterator  & splEdgEndPolyIt,
					    const FFlVisEdgeRef      & prevSplEdge,
					    const FFlVisFace         & previousFace,
					    const bool               & prevFaceIsPositive,
					    const bool               & onlySurfaceFaces,
					    const FaVec3             & normal )
{
  // Workaround to reduce recursive stack depth
  // (joinFacesFromEdge <-> recIncludeFace)
  if (joinFacesFromEdgeMethodCount >= 4500)
  {
#ifdef FFL_DEBUG
    std::cout <<"  ** Max recursive depth ("<< joinFacesFromEdgeMethodCount
              <<") reached during merge of faces, bailing..."<< std::endl;
#endif
    return;
  }

  IntList::iterator splEdgBeginPolyIt = splEdgEndPolyIt;
  --splEdgBeginPolyIt;

  // Find a neigbouring face that is joinable to previousFace

  bool faceToJoinIsPositive = true;
  FFlVisFace* faceToJoin = NULL;
  VisEdgeRefVecCIter splEdgeRIt = previousFace.edgesEnd();

  { // A Scope to remove these variables from the recursive call stack

    bool isOkToJoin               = true;
    bool isSomeInPlaneFaceOutside = false;
    std::vector<FFlVisFace*> inPlaneFaces;
    for (FFlFaceRef& neighbor : prevSplEdge->getRenderData()->faceReferences)
      if (neighbor.first != &previousFace)
      {
        // Find whether neighbour is inPlane with previous one
        bool neigbFaceIsPositive = true;
        if (neighbor.first->isSurfaceFace() != onlySurfaceFaces)
          continue;
        else if (normal.angle(neighbor.second) < myFaceReductionAngle)
          neigbFaceIsPositive = true;
        else if (normal.angle(-neighbor.second) < myFaceReductionAngle)
          neigbFaceIsPositive = false;
        else
          continue; // not inPlane with previous

        inPlaneFaces.push_back(neighbor.first);

        // Find the edge reference in the neighbor face:

        VisEdgeRefVecCIter nbEdgeRefIt;
        for (nbEdgeRefIt = neighbor.first->edgesBegin(); nbEdgeRefIt != neighbor.first->edgesEnd(); ++nbEdgeRefIt)
          if (nbEdgeRefIt->getEdge() == prevSplEdge.getEdge())
            break;

        // Find whether neighbor is inside, or contains the previous face

        bool facesHasSameNormDir    = neigbFaceIsPositive     == prevFaceIsPositive;
        bool splitEdgeReffedSameWay = nbEdgeRefIt->isPosDir() == prevSplEdge.isPosDir();
        if ( (facesHasSameNormDir && !splitEdgeReffedSameWay) || (!facesHasSameNormDir && splitEdgeReffedSameWay) )
        {
          // Neighbor is Not coincident with previous face
          isSomeInPlaneFaceOutside = true;
          if (neighbor.first->isVisited())
            isOkToJoin = false;
          else if (!faceToJoin || neighbor.first->getNumVertices() > faceToJoin->getNumVertices()) {
            faceToJoin = neighbor.first;
            faceToJoinIsPositive = neigbFaceIsPositive;
            splEdgeRIt = nbEdgeRefIt;
          }
        }
        else // Neighbor Is coincident with previous face
          neighbor.first->setVisited();
      }

    // Mark all inPlane polygons visited, to make sure that polygons
    // on top of each other will not create loops in polygon.

    for (FFlVisFace* face : inPlaneFaces) face->setVisited();

    // Flip propagation direction if we got no inPlane face not coincident with
    // the "previous" face. This is typically true for edges along the outer edges
    // of a flat surface

    if (!isSomeInPlaneFaceOutside)
      IAmIncludingInOpsDir = !IAmIncludingInOpsDir;

    // Return now if the faceToJoin is not joinable

    if (!faceToJoin || !isOkToJoin)
      return;

#if FFL_DEBUG > 3
    std::cout <<"Splitting edge: "<< *splEdgeRIt
              <<" PolygonIterator: "<< *splEdgEndPolyIt << std::endl;
#endif

    // Get vertex indices from face and insert them into polygon list

    IntList facePolygon;
    if (faceToJoinIsPositive)
      this->getPolygonPosFace(facePolygon, *faceToJoin, splEdgeRIt);
    else
      this->getPolygonNegFace(facePolygon, *faceToJoin, splEdgeRIt);
    facePolygon.pop_front();
    facePolygon.pop_back();
    polygon.splice(splEdgEndPolyIt, facePolygon);

#if FFL_DEBUG > 3
    std::cout <<"Bigger Polygon:";
    for (int idx : polygon) std::cout <<" "<< idx;
    std::cout << std::endl;
#endif
  } // End of call stack reducing scope


  // Loop over the edges of this face in polygon direction,
  // and join the joinable faces recursivly to the polygon.
  // Start with the edge after or before the splitting edge,
  // depending on whether including faces are in opposite direction.

  ++joinFacesFromEdgeMethodCount;

  VisEdgeRefVecCIter edgeIt = splEdgeRIt;
  IntList::iterator nextSplEdgEndPolyIt;

  if (!IAmIncludingInOpsDir) // Spiral propagation
  {
    nextSplEdgEndPolyIt = splEdgBeginPolyIt;
    ++nextSplEdgEndPolyIt;

    while (faceToJoin->nextEdge(edgeIt,splEdgeRIt,faceToJoinIsPositive))
    {
      this->joinFacesFromEdge(polygon, nextSplEdgEndPolyIt, *edgeIt, *faceToJoin,
                              faceToJoinIsPositive, onlySurfaceFaces, normal);
      ++nextSplEdgEndPolyIt;
    }
  }
  else
  {
    splEdgBeginPolyIt = nextSplEdgEndPolyIt = splEdgEndPolyIt;
    --splEdgBeginPolyIt;

    while (faceToJoin->nextEdge(edgeIt,splEdgeRIt,!faceToJoinIsPositive))
    {
      this->joinFacesFromEdge(polygon, nextSplEdgEndPolyIt, *edgeIt, *faceToJoin,
                              faceToJoinIsPositive, onlySurfaceFaces, normal);
      nextSplEdgEndPolyIt = splEdgBeginPolyIt;
      --splEdgBeginPolyIt;
    }
  }

  --joinFacesFromEdgeMethodCount;
}


/*!
  Get a list of vertex indices (polygon) from face
  that starts on the end of the splitting edge.

  Loops from (and including) splEdge to and excluding splEdge.
  Looping in positive direction for the face and jumping from end to begin.
*/

void FFlGroupPartCreator::getPolygonPosFace(IntList& polygon,
                                            const FFlVisFace& f,
                                            const VisEdgeRefVecCIter& splEdge)
{
  VisEdgeRefVecCIter edgeIt = splEdge;
  do
  {
    polygon.push_back(edgeIt->getSecondVertex()->getRunningID());
    ++edgeIt;
    if (edgeIt == f.edgesEnd()) edgeIt = f.edgesBegin();
  }
  while (edgeIt != splEdge);

#if FFL_DEBUG > 3
  std::cout <<"Creating face polygon:";
  for (int idx : polygon) std::cout <<" "<< idx;
  std::cout << std::endl;
#endif
}


/*!
  Get a list of vertex indices (polygon) from face
  that starts on the end of the splitting edge.

  Loops from (and including) splEdge to and excluding splEdge.
  Looping in negative direction for the face and jumping from begin to end.
*/

void FFlGroupPartCreator::getPolygonNegFace(IntList& polygon,
                                            const FFlVisFace& f,
                                            const VisEdgeRefVecCIter& splEdge)
{
  VisEdgeRefVecCIter edgeIt = splEdge;
  do
  {
    polygon.push_back(edgeIt->getFirstVertex()->getRunningID());
    if (edgeIt == f.edgesBegin()) edgeIt = f.edgesEnd();
    --edgeIt;
  }
  while (edgeIt != splEdge);

#if FFL_DEBUG > 3
  std::cout <<"Creating face polygon:";
  for (int idx : polygon) std::cout <<" "<< idx;
  std::cout << std::endl;
#endif
}


/*!
  Creates Full edge representations of the link
  The edges in the FFlFaceGenerator must be assigned
  status before this method is called.
*/

void FFlGroupPartCreator::createLinkFullEdges(FFlGroupPartData& internal,
                                              FFlGroupPartData& surface,
                                              FFlGroupPartData& outline)
{
  internal.isIndexShape = surface.isIndexShape = outline.isIndexShape = false;

  for (FFlVisEdge* edge : myVisEdges)

    // Decide what group part to add the edge into
    switch (edge->getRenderData()->edgeStatus)
      {
      case FFlVisEdgeRenderData::SURFACE:
        surface.edgePointers.emplace_back(edge,-1);
        break;
      case FFlVisEdgeRenderData::OUTLINE:
        outline.edgePointers.emplace_back(edge,-1);
        break;
      default:
        internal.edgePointers.emplace_back(edge,-1);
        break;
      }
}


void FFlGroupPartCreator::createLinkReducedEdges(FFlGroupPartData& internal,
                                                 FFlGroupPartData& surface,
                                                 FFlGroupPartData& outline)
{
  internal.isIndexShape = surface.isIndexShape = outline.isIndexShape = true;

  // Create vertex-to-edge reference array
  std::vector< std::vector<FFlVisEdge*> > vertexEdgeRefs(myVertices.size());
  // Loop over edges, push edge into buckets labeled by the vertices they use
  for (FFlVisEdge* edge : myVisEdges)
  {
    vertexEdgeRefs[edge->getFirstVxIdx()].push_back(edge);
    vertexEdgeRefs[edge->getSecondVxIdx()].push_back(edge);
  }

  // Recursive lambda function for concatenating nearly co-linear edges.
  std::function<void(FFlVisEdge*,IntVec&,int)> expand =
    [&expand, angleTol=myEdgesParallelAngle, vertexEdgeRefs]
    (FFlVisEdge* origEdge, IntVec& simplifiedEdge, int idx) -> void
  {
    int                   endID    = origEdge->getVertexIdx(idx);
    FaVec3                startVec = origEdge->getVector();
    FFlVisEdgeRenderData* orgRData = origEdge->getRenderData();

    orgRData->simplified = true;

    // Loop over edges referred by the vertex
    for (FFlVisEdge* edge : vertexEdgeRefs[endID])
    {
      FFlVisEdgeRenderData* rData = edge->getRenderData();
      if (edge != origEdge && !rData->simplified &&
          rData->edgeStatus == orgRData->edgeStatus &&
          rData->linePattern == orgRData->linePattern)
      {
        // Orient the vector
        FaVec3 endVec = edge->getVector();
        if (edge->getVertexIdx(1-idx) != endID)
          endVec = -endVec;

        if (startVec.angle(endVec) < angleTol)
        {
          // Extend the simplified edge
          if (edge->getVertexIdx(1-idx) == endID)
            simplifiedEdge[idx] = edge->getVertexIdx(idx);
          else
            simplifiedEdge[idx] = edge->getVertexIdx(1-idx);

          // Try to expand more on this edge
          expand(edge,simplifiedEdge,idx);
          break;
        }
      }
    }
  };

  for (FFlVisEdge* edge : myVisEdges)
    if (!edge->getRenderData()->simplified)
    {
      IntVec simplEdge;
      edge->getEdgeVertices(simplEdge);
      expand(edge,simplEdge,1);
      expand(edge,simplEdge,0);

      // Decide what group part to add the edge into.
      // The simplified edge will have the same status and line pattern
      // as the original edges it is composed from.
      switch (edge->getRenderData()->edgeStatus)
        {
        case FFlVisEdgeRenderData::SURFACE:
          surface.shapeIndexes.push_back(simplEdge);
          break;
        case FFlVisEdgeRenderData::OUTLINE:
          outline.shapeIndexes.push_back(simplEdge);
          break;
        default:
          internal.shapeIndexes.push_back(simplEdge);
          break;
        }
    }
}


void FFlGroupPartCreator::createSpecialLines(double XZlen)
{
  // Lambda function for creating a special edge line.
  std::function<void(FFlVisEdge*,double)> createLine =
    [&lines=mySpecialLines](FFlVisEdge* edge, double length) -> void
  {
    unsigned short int linePattern = edge->getRenderData()->linePattern;
    GroupPartMap::iterator lit = lines.find(linePattern);
    if (lit == lines.end()) // Create a new group part for this line pattern
      lit = lines.emplace(linePattern,new FFlGroupPartData()).first;

    // Add edge to group part associated with this line pattern
    lit->second->edgePointers.emplace_back(edge,-1);
    if (length > 0.0)
    {
      // Adjust the second vertex such that the edge have the given length
      FaVec3  dirVec = edge->getVector();
      FaVec3* fstVtx = edge->getFirstVertex();
      FaVec3* secVtx = edge->getSecondVertex();
      *secVtx = *fstVtx + dirVec.normalize()*length;
    }
  };

  for (FFlVisEdge* edge : mySpecialEdges) createLine(edge,0.0);
  for (FFlVisEdge* edge : myBeamEccEdges) createLine(edge,0.0);
  if (XZlen < 0.0) return; // no local beam system marker
  for (FFlVisEdge* edge : myBeamSysEdges) createLine(edge,XZlen);
}


void FFlGroupPartCreator::dump() const
{
  static const char* type_name[NUM_TYPES] = {
    "\n  RED_OUTLINE_LINES: ",
    "\n  OUTLINE_LINES:     ",
    "\n  RED_SURFACE_LINES: ",
    "\n  SURFACE_LINES:     ",
    "\n  RED_SURFACE_FACES: ",
    "\n  SURFACE_FACES:     ",
    "\n  RED_INTERNAL_LINES:",
    "\n  INTERNAL_LINES:    ",
    "\n  RED_INTERNAL_FACES:",
    "\n  INTERNAL_FACES:    ",
    "\n  SPECIAL_LINES:     "
  };

  std::cout <<"FFlGroupPartCreator::dump:";
  for (const GroupPartMap::value_type& gp : myLinkParts)
    std::cout << type_name[gp.first]
              <<" "<< std::boolalpha << gp.second->isLineShape
              <<" "<< std::boolalpha << gp.second->isIndexShape
              <<" "<< gp.second->facePointers.size()
              <<" "<< gp.second->hiddenFaces.size()
              <<" "<< gp.second->edgePointers.size()
              <<" "<< gp.second->hiddenEdges.size()
              <<" "<< gp.second->shapeIndexes.size()
              <<" "<< gp.second->getNoVisibleVertices();
  std::cout <<"\n  SPECIAL_LINES:     ";
  for (const GroupPartMap::value_type& gp : mySpecialLines)
    std::cout <<" "<< std::boolalpha << gp.second->isLineShape
              <<" "<< std::boolalpha << gp.second->isIndexShape
              <<" "<< gp.first <<" "<< gp.second->edgePointers.size()
              <<" "<< gp.second->getNoVisibleVertices();
  std::cout << std::endl;

  this->FFlFaceGenerator::dump();
}


size_t FFlGroupPartData::getNoItems() const
{
  if (isIndexShape)
    return shapeIndexes.size();
  else if (isLineShape)
    return edgePointers.size();
  else
    return facePointers.size();
}


size_t FFlGroupPartData::getNoVisibleVertices(bool addOnePrItem) const
{
  size_t nVert = 0;
  if (isIndexShape)
  {
    for (const IntVec& shape : shapeIndexes)
      nVert += shape.size();
    if (addOnePrItem)
      nVert += shapeIndexes.size();
  }
  else if (isLineShape)
    nVert = (addOnePrItem ? 3 : 2)*edgePointers.size();
  else
  {
    for (const FFlVisFaceIdx& face : facePointers)
      nVert += face.first->getNumVertices();
    if (addOnePrItem)
      nVert += facePointers.size();
  }

  return nVert;
}


void FFlGroupPartData::getShapeIndexes(int* idx) const
{
  if (isIndexShape)

    for (const IntVec& shape : shapeIndexes)
    {
      for (int shapeIndex : shape)
        *(idx++) = shapeIndex;
      *(idx++) = -1; // Inserting face/line separator
    }

  else if (isLineShape)

    for (const FFlVisEdgeIdx& edge : edgePointers)
    {
      edge.first->getEdgeVertices(idx);
      *(idx++) = -1; // Inserting face/line separator
    }

  else

    for (const FFlVisFaceIdx& face : facePointers)
    {
      face.first->getElmFaceVertices(idx);
      *(idx++) = -1; // Inserting face/line separator
    }
}
