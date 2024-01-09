// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFlLib/FFlVisualization/FFlFaceGenerator.H"
#include "FFlLib/FFlVisualization/FFlGeomUniqueTester.H"
#include "FFlLib/FFlVisualization/FFlVisEdge.H"
#include "FFlLib/FFlVisualization/FFlVisFace.H"
#include "FFlLib/FFlFEParts/FFlNode.H"
#include "FFlLib/FFlFEParts/FFlPBEAMECCENT.H"
#include "FFlLib/FFlFEParts/FFlPORIENT.H"
#include "FFlLib/FFlElementBase.H"
#include "FFlLib/FFlFEElementTopSpec.H"
#include "FFlLib/FFlLinkHandler.H"
#include "FFlLib/FFlMemPool.H"

#include <map>


FFlFaceGenerator::FFlFaceGenerator(FFlLinkHandler* link) : myWorkLink(link)
{
  if (!myWorkLink) return;

#ifdef FT_USE_MEMPOOL
  FFlVisEdge::usePartOfPool((void*)this);
  FFlVisFace::usePartOfPool((void*)this);
#endif

  FFlGeomUniqueTester aUniqeTester(myWorkLink->getVertexCount());

  for (ElementsCIter it = myWorkLink->elementsBegin();
       it != myWorkLink->elementsEnd(); ++it)
  {
    if ((*it)->getCathegory() == FFlTypeInfoSpec::BEAM_ELM)
      this->createBeamGeometry(*it,aUniqeTester);
    else
      this->createGeometry(*it,aUniqeTester);
    this->createSpecialEdges(*it);
  }

#ifdef FT_USE_MEMPOOL
  FFlVisEdge::useDefaultPartOfPool();
  FFlVisFace::useDefaultPartOfPool();
#endif
}


FFlFaceGenerator::~FFlFaceGenerator()
{
#ifdef FT_USE_MEMPOOL
  FFlVisEdge::usePartOfPool((void*)this);
  FFlVisFace::usePartOfPool((void*)this);
#endif

  for (FFlVisFace* face : myVisFaces) delete face;
  for (FFlVisEdge* edge : myVisEdges) delete edge;
  for (FFlVisEdge* edge : mySpecialEdges) delete edge;
  for (FFlVisEdge* edge : myBeamEccEdges) delete edge;
  for (FFlVisEdge* edge : myBeamSysEdges) delete edge;

  myVisFaces.clear();
  myVisEdges.clear();
  mySpecialEdges.clear();
  myBeamEccEdges.clear();
  myBeamSysEdges.clear();

#ifdef FT_USE_MEMPOOL
  FFlVisEdge::useDefaultPartOfPool();
  FFlVisFace::useDefaultPartOfPool();

  FFlVisEdge::freePartOfPool((void*)this);
  FFlVisFace::freePartOfPool((void*)this);
#endif
}


bool FFlFaceGenerator::recreateSpecialEdges()
{
  if (!myWorkLink) return false;

#ifdef FT_USE_MEMPOOL
  FFlVisEdge::usePartOfPool((void*)this);
#endif

  for (FFlVisEdge* edge : mySpecialEdges) delete edge;
  mySpecialEdges.clear();

  for (ElementsCIter it = myWorkLink->elementsBegin();
       it != myWorkLink->elementsEnd(); ++it)
    this->createSpecialEdges(*it);

#ifdef FT_USE_MEMPOOL
  FFlVisFace::useDefaultPartOfPool();
#endif
  return !mySpecialEdges.empty();
}


/*!
  Builds visualization geometry and put in the face- and edge containers.
*/

void FFlFaceGenerator::createGeometry(FFlElementBase* elm,
                                      FFlGeomUniqueTester& tester)
{
  FFlFEElementTopSpec* topSpec = elm->getFEElementTopSpec();

  // For all faces in topology description of the element :

  int faceID = 0;
  for (const FaceType& faceDef : topSpec->myFaces)
  {
    // Create array to collect all face vertices
    std::vector<FFlVertex*> faceVertices;
    faceVertices.reserve(faceDef.myEdges.size());

    // Loop over all edges in the current face,
    // add the vertices from the edges into the faceVertices array
    for (const EdgeType& edgeDef : faceDef.myEdges)
      faceVertices.push_back(elm->getNode(edgeDef.first)->getVertex());

    // Create a face, set up core information defining the edges,
    // and set up geometry info in the element face reference as we go

    FFlFaceElemRef elmRef;
    elmRef.myElementFaceNumber = faceID++;
    FFlVisFace* face = new FFlVisFace();
    face->setFaceVertices(faceVertices, myVisEdges, elmRef, tester);
    if (face->getNumVertices() < 3)
      delete face; // Degenerated face, skip
    else
    {
      // Try to insert the face in the face set,
      // if it was there already, use the one already in the set instead
      FFlFacePtBoolPair faceP = tester.insertFace(face);
      if (faceP.second)
        myVisFaces.push_back(face);
      else
      {
        delete face;
        face = *faceP.first;
      }

      // Set element and face info
      elmRef.myElement = elm;
      face->setIsExpandedFace(false);
      face->addFaceElemRef(elmRef);
      face->ref();
      if (topSpec->isShellFaces())
        face->setShellFace();
    }
  }
}


/*!
  Special for beam elements, to account for eccentricity, etc.
  This method needs to be reimplemented if we want to visualize
  the beam cross sections. Then additional vertices are needed.
*/

void FFlFaceGenerator::createBeamGeometry(FFlElementBase* elm,
                                          FFlGeomUniqueTester& tester)
{
  FFlFEElementTopSpec* topSpec = elm->getFEElementTopSpec();

  std::vector<FFlVertex*> vx(4,NULL);
  std::vector<FFlNode*> n(elm->getNodeCount());
  for (int i = 0; i < elm->getNodeCount(); i++)
  {
    n[i] = elm->getNode(i+1);
    vx[i] = n[i]->getVertex();
  }

  FFlPBEAMECCENT* pecc = dynamic_cast<FFlPBEAMECCENT*>
                         (elm->getAttribute("PBEAMECCENT"));
  if (pecc)
  {
    // Add the eccentric nodes as separate vertices
    for (size_t j = 0; j < n.size(); j++)
    {
      FaVec3 ecc;
      switch (j) {
      case 0: ecc = pecc->node1Offset.getValue(); break;
      case 1: ecc = pecc->node2Offset.getValue(); break;
      case 2: ecc = pecc->node3Offset.getValue(); break;
      }
      if (!ecc.isZero())
      {
        vx[j] = new FFlVertex(*vx[j]+ecc);
        myWorkLink->addVertex(vx[j]);
        // Add a special line from the FE node to the eccentric beam end/node
        myBeamEccEdges.push_back(new FFlVisEdge(n[j]->getVertex(),vx[j]));
        FFlVisEdgeRenderData* rData = myBeamEccEdges.back()->getRenderData();
        rData->linePattern = 0xf0f0;
        rData->edgeStatus = FFlVisEdgeRenderData::OUTLINE;
      }
    }

    // Add mesh lines between the eccentric beam nodes
    for (size_t j = 1; j < n.size(); j++)
    {
      myVisEdges.push_back(new FFlVisEdge(vx[j-1],vx[j]));
      FFlVisEdgeRenderData* rData = myVisEdges.back()->getRenderData();
      rData->linePattern = topSpec->myExplEdgePattern;
      rData->edgeStatus = FFlVisEdgeRenderData::OUTLINE;
    }
  }
  else // No eccentricity, just add a mesh line between the FE nodes
    for (size_t j = 1; j < n.size(); j++)
    {
      FFlVisEdge* edge = new FFlVisEdge(vx[j-1],vx[j]);
      FFlEdgePtBoolPair edgePair = tester.insertEdge(edge);
      if (!edgePair.second)
        delete edge;
      else
      {
        FFlVisEdgeRenderData* rData = edge->getRenderData();
        rData->linePattern = topSpec->myExplEdgePattern;
        rData->edgeStatus  = FFlVisEdgeRenderData::OUTLINE;
        myVisEdges.push_back(edge);
      }
    }

  // Get local Z-axis direction for this element
  FaVec3 Zaxis;
  FFlPORIENT* orien = NULL;
  FFlPORIENT3* ori3 = NULL;
  if (n.size() > 2)
    ori3 = dynamic_cast<FFlPORIENT3*>(elm->getAttribute("PORIENT3"));
  if (ori3)
    Zaxis = ori3->directionVector[1].getValue();
  else if ((orien = dynamic_cast<FFlPORIENT*>(elm->getAttribute("PORIENT"))))
    Zaxis = orien->directionVector.getValue();
  else
    return; // No orientation, can happen with circular sections only

  // Create vertices for the orientation marker
  int imid = n.size() > 2 ? 1 : 2;
  if (imid == 2)
  {
    vx[2] = new FFlVertex(0.5*(*vx[0] + *vx[1]));
    myWorkLink->addVertex(vx[2]);
  }
  double leno2 = 0.5*(*vx[n.size()-1] - *vx[0]).length(); // half beam length
  vx[3] = new FFlVertex(0.5*(*vx[0] + *vx[imid] + leno2*Zaxis.normalize()));
  myWorkLink->addVertex(vx[3]);

  // Add a special line marking the local X-Z plane
  myBeamSysEdges.push_back(new FFlVisEdge(vx[imid],vx[3]));
  FFlVisEdgeRenderData* rData = myBeamSysEdges.back()->getRenderData();
  rData->linePattern = 0xf0f0;
  rData->edgeStatus = FFlVisEdgeRenderData::OUTLINE;
}


void FFlFaceGenerator::createSpecialEdges(FFlElementBase* elm)
{
  FFlFEElementTopSpec* topSpec = elm->getFEElementTopSpec();

  for (const EdgeType& xEdge : topSpec->myExplicitEdges)
  {
    FFlNode* n1 = elm->getNode(xEdge.first);
    FFlNode* n2 = elm->getNode(xEdge.second);
    if (n1 && n2)
    {
      mySpecialEdges.push_back(new FFlVisEdge(n1->getVertex(),n2->getVertex()));
      FFlVisEdgeRenderData* rData = mySpecialEdges.back()->getRenderData();
      rData->linePattern = topSpec->myExplEdgePattern;
      rData->edgeStatus  = FFlVisEdgeRenderData::OUTLINE;
    }
  }
}


void FFlFaceGenerator::dump() const
{
  std::cout <<"Type sizes :"
	    <<"\n  FFlVisFace           : " << sizeof(FFlVisFace)
	    <<"\n  FFlFaceElemRef       : " << sizeof(FFlFaceElemRef)
	    <<"\n  FFlVisEdgeRef        : " << sizeof(FFlVisEdgeRef)
	    <<"\n  FFlVisEdge           : " << sizeof(FFlVisEdge)
	    <<"\n  FFlVisEdgeRenderData : " << sizeof(FFlVisEdgeRenderData)
	    <<"\n  VisEdgeRefVec        : " << sizeof(VisEdgeRefVec)
	    <<"\n  std::vector<void*>   : " << sizeof(std::vector<void*>)
	    <<"\n  std::vector<FFlVisFaceRef> : "<< sizeof(std::vector<FFlFaceRef>)
	    <<"\n"<< std::endl;

  std::map<int,int> refCountMap;
  std::map<int,int> edgeRefCountMap;
  std::map<int,int> faceEdgeRefMap;

  // Count free faces and how many elements per face

  int freeFace = 0;
  for (FFlVisFace* face : myVisFaces)
  {
    if (face->getRefs() == 1) freeFace++;

    refCountMap[face->getRefs()]++;
    faceEdgeRefMap[face->getNumVertices()]++;
  }

  std::cout <<"Statistics :";

  // Print number of faces with the different number of element references

  std::cout <<"\n\tFFlFaceElemRef\tNumber of faces w/that number of FFlFaceElemRefs";
  int numElmRefs = 0;
  for (const std::pair<const int,int>& count : refCountMap)
  {
    std::cout <<"\n\t"<< count.first <<"\t\t"<< count.second;
    numElmRefs += count.first * count.second;
  }

  std::cout <<"\n\tFFlVisEdgeRef\tNumber of faces w/that number of FFlVisEdgeRefs";
  int numVisEdgeRefs = 0;
  for (const std::pair<const int,int>& count : faceEdgeRefMap)
  {
    std::cout <<"\n\t"<< count.first <<"\t\t"<< count.second;
    numVisEdgeRefs += count.first * count.second;
  }

  // Print number of edges

  for (FFlVisEdge* edge : myVisEdges)
    edgeRefCountMap[edge->getRefs()]++;

  // Print number of faces referring to elements

  std::cout <<"\n\tNFaces\tNEdges w/that num Faces";
  for (const std::pair<const int,int>& count : edgeRefCountMap)
    std::cout <<"\n\t"<< count.first <<"\t"<< count.second;

  std::cout <<"\nNumber of objects :"
	    <<"\n  FFlVisEdge           : " << myVisEdges.size()
	    <<"\n  FFlVisFace           : " << myVisFaces.size()
	    <<" (surface: " << freeFace << ")"
	    <<"\n  FFlFaceElemRefs      : " << numElmRefs
	    <<"\n  FFlVisEdgeRefs       : " << numVisEdgeRefs
	    <<"\nTheoretical RAM usage (bytes) : "
	    <<"\n  FFlVisEdge           : " << myVisEdges.size()*sizeof(FFlVisEdge)
	    <<"\n  FFlVisFace           : " << myVisFaces.size()*sizeof(FFlVisFace)
	    <<"\n  FFlFaceElemRefs      : " << numElmRefs       *sizeof(FFlFaceElemRef)
	    <<"\n  FFlVisEdgeRefs       : " << numVisEdgeRefs   *sizeof(FFlVisEdgeRef)
	    <<"\n  Total                : " <<
    numVisEdgeRefs      * sizeof(FFlVisEdgeRef)
    + numElmRefs        * sizeof(FFlFaceElemRef)
    + myVisFaces.size() * sizeof(FFlVisEdge)
    + myVisEdges.size() * sizeof(FFlVisFace);

  std::cout <<"\n  FFlVisEdgeRenderData : " << numVisEdgeRefs   *sizeof(FFlVisEdgeRenderData)
	    <<"\n  Total w/render data  : " <<
    numVisEdgeRefs      * sizeof(FFlVisEdgeRef)
    + numElmRefs        * sizeof(FFlFaceElemRef)
    + myVisFaces.size() * sizeof(FFlVisEdge)
    + myVisEdges.size() * sizeof(FFlVisFace)
    + myVisEdges.size() * sizeof(FFlVisEdgeRenderData) << std::endl;
}


void FFlMemPool::deleteVisualsMemPools()
{
#ifdef FT_USE_MEMPOOL
  FFlVisFace::freePool();
  FFlVisEdge::freePool();
  FFlVisEdgeRenderData::freePool();
#endif
}
