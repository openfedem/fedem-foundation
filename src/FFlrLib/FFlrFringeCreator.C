// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFlrLib/FFlrFringeCreator.H"
#include "FFlrLib/FFlrFEResult.H"
#include "FFlrLib/FFlrFEResultBuilder.H"
#include "FFlrLib/FapFringeSetup.H"
#include "FFlLib/FFlLinkHandler.H"
#include "FFlLib/FFlVisualization/FFlGroupPartCreator.H"
#include "FFlLib/FFlVisualization/FFlVisFace.H"
#include "FFlLib/FFlVisualization/FFlVisEdge.H"
#include "FFlLib/FFlFEParts/FFlNode.H"

#ifdef FT_USE_PROFILER
#include "FFaLib/FFaProfiler/FFaMemoryProfiler.H"
#endif

#include <algorithm>


/*!
  Function to get colors from a group part,
  when the operation tree is built in the FFlr-classes.
  Called for each timestep.
*/

bool FFlrFringeCreator::getColorData(std::vector<double>& colors,
                                     const FFlGroupPartData& visRep,
                                     bool isPrFace)
{
  bool hasColorData = false;
  if (!visRep.isLineShape || !isPrFace)
    for (size_t i = 0; !hasColorData && i < visRep.colorOps.size(); i++)
      if (visRep.colorOps[i])
        hasColorData = visRep.colorOps[i]->hasData();

  if (!hasColorData)
    return false;

  colors.clear();
  colors.resize(visRep.colorOps.size(), HUGE_VAL);

  for (size_t i = 0; i < visRep.colorOps.size(); i++)
    if (visRep.colorOps[i])
    {
      visRep.colorOps[i]->invalidate();
      visRep.colorOps[i]->invoke(colors[i]);
    }

  return true;
}


void FFlrFringeCreator::deleteColorsXfs(FFlGroupPartData& visRep)
{
  for (size_t i = 0; i < visRep.colorOps.size(); i++)
  {
    if (visRep.colorOps[i])
      visRep.colorOps[i]->unref();
#ifdef FT_USE_PROFILER
    if (i == visRep.colorOps.size()/4)
      FFaMemoryProfiler::reportMemoryUsage("    Kvartveis");
    if (i == visRep.colorOps.size()/2)
      FFaMemoryProfiler::reportMemoryUsage("    Halveis");
    if (i == 3*visRep.colorOps.size()/4)
      FFaMemoryProfiler::reportMemoryUsage("    Trekvartveis");
#endif
  }

#ifdef FT_USE_PROFILER
  FFaMemoryProfiler::reportMemoryUsage("    Helveis");
#endif

  FFlrOperations empty;
  visRep.colorOps.swap(empty);
}


/*!
  Builds the value retrieval operations for each point
  on the supplied group part to have fringe color,
  based on the values in the setup object.

  The operations are stored in the FFlGroupPartData object provided.

  The returned value is the number of operations actually generated.
*/

int FFlrFringeCreator::buildColorXfs(FFlGroupPartData& visRep,
                                     FFlLinkHandler* lh,
                                     const FapFringeSetup& setup)
{
  visRep.colorOps.clear();

  size_t i;
  if (visRep.isLineShape)
  {
    visRep.colorOps.resize(2*visRep.edgePointers.size(),NULL);

    for (i = 0; i < visRep.edgePointers.size(); i++)
    {
      FFlVisEdge* edge = visRep.edgePointers[i].first;
      FFlr::getEdgeVxMergeOp(visRep.colorOps,2*i,edge,lh,setup);
      visRep.edgePointers[i].second = 2*i;
    }

    for (i = 0; i < visRep.hiddenEdges.size(); i++)
      visRep.hiddenEdges[i].second = -1;
  }
  else if (setup.isOneColorPrFace())
  {
    visRep.colorOps.resize(visRep.facePointers.size(),NULL);

    for (i = 0; i < visRep.facePointers.size(); i++)
    {
      FFlVisFace* face = visRep.facePointers[i].first;
      FFlr::getFaceMergeOp(visRep.colorOps[i],face,setup);
      visRep.facePointers[i].second = i;
    }

    for (i = 0; i < visRep.hiddenFaces.size(); i++)
      visRep.hiddenFaces[i].second = -1;
  }
  else
  {
    visRep.colorOps.resize(visRep.nVisiblePrimitiveVertexes,NULL);

    int previousFaceEndIdx = 0;
    for (i = 0; i < visRep.facePointers.size(); i++)
    {
      FFlVisFace* face = visRep.facePointers[i].first;
      FFlr::getFaceVxMergeOp(visRep.colorOps,previousFaceEndIdx,face,lh,setup);
      visRep.facePointers[i].second = previousFaceEndIdx;
      previousFaceEndIdx += face->getNumVertices();
    }

    for (i = 0; i < visRep.hiddenFaces.size(); i++)
      visRep.hiddenFaces[i].second = -1;
  }

  // Count the number of visual points that should have got data
  int nOps = visRep.colorOps.size();
  for (int j = 0; j < nOps; j++)
    if (visRep.colorOps[j])
      visRep.colorOps[j]->ref();

  return nOps;
}


int FFlrFringeCreator::buildColorXfs(FFlLinkHandler* lh,
                                     const FapFringeSetup& setup,
                                     const std::vector<int>& nodesFilter)
{
  FFlrFELinkResult* linkRes = lh->getResults();
  linkRes->elmStart.clear();
  linkRes->scalarOps.clear();

  if (setup.isOneColorPrVertex())
  {
    size_t iNod = 0;
    linkRes->scalarOps.resize(lh->getNodeCount(FFlLinkHandler::FFL_FEM));
    for (NodesCIter nit = lh->nodesBegin(); nit != lh->nodesEnd(); nit++)
      if (FFlrFringeCreator::filterNodes(*nit,nodesFilter))
        FFlr::getNodeMergeOp(linkRes->scalarOps[iNod++],*nit,lh,setup);
    linkRes->scalarOps.resize(iNod);
  }
  else if (setup.isOneColorPrFace())
  {
    size_t iel = 0;
    linkRes->scalarOps.resize(lh->buildFiniteElementVec(false));
    for (ElementsCIter eit = lh->fElementsBegin(); eit != lh->fElementsEnd(); eit++)
      FFlr::getElementMergeOp(linkRes->scalarOps[iel++],*eit,setup);
  }
  else
  {
    size_t iel = 0;
    linkRes->elmStart.resize(lh->buildFiniteElementVec(false)+1,0);
    for (ElementsCIter eit = lh->fElementsBegin(); eit != lh->fElementsEnd(); eit++)
    {
      FFlrOperations elmOps;
      FFlr::getElementMergeOps(elmOps,*eit,lh,setup);
      linkRes->scalarOps.insert(linkRes->scalarOps.end(),elmOps.begin(),elmOps.end());
      linkRes->elmStart[++iel] = linkRes->scalarOps.size();
    }
  }

  return linkRes->scalarOps.size();
}


bool FFlrFringeCreator::filterNodes(const FFlNode* node, const std::vector<int>& nodeFilter)
{
  if (!node->hasDOFs()) return false;
  if (nodeFilter.empty()) return true;

  return std::find(nodeFilter.begin(),nodeFilter.end(),node->getID()) != nodeFilter.end();
}
