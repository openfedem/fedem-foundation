// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFlLib/FFlLinkHandler.H"
#include "FFlLib/FFlElementBase.H"
#include "FFlLib/FFlFEElementTopSpec.H"
#include "FFlLib/FFlVisualization/FFlVisFace.H"
#include "FFlLib/FFlVisualization/FFlVisEdge.H"
#include "FFlLib/FFlFEParts/FFlNode.H"
#include "FFlLib/FFlVertex.H"
#include "FFlLib/FFlGroup.H"
#include "FFaLib/FFaOperation/FFaOpUtils.H"

#include "FFlrLib/FFlrFEResultBuilder.H"
#include "FFlrLib/FFlrFEResult.H"
#include "FFlrLib/FFlrResultResolver.H"
#include "FFlrLib/FapFringeSetup.H"

enum ResKat { ELM, ELMNODE, EVALP };


/////////////////////////////////////////////////////////////////////////
//
// Read operation, to-scalar operation and result set merging
//
/////////////////////////////////////////////////////////////////////////

/*!
  Will return a pointer to an operation that can evaluate either the
  element result, the element node result or the evaluation point result,
  depending on the "rKat" parameter.
  The underlying results will be tried converted to the return type of
  the operation, depending on the "toScalarOpName" of the "setup" struct.
  The lNodeIdx parameter must contain the element local node number.
*/

static FFlrOperation getElmResSetMergeOp(FFlFEElmResult* thisPtr,
                                         const FFlElementBase* elm,
                                         const FapFringeSetup& setup,
                                         ResKat rKat, int lNodeIdx)
{
  if (thisPtr)
    // Check if operation exists for this element,
    // element node or evaluation point.
    // Return it if it does exist.
    switch (rKat) {
    case ELM:
      if (thisPtr->myElmRSMrgIsMade)
        return dynamic_cast<FFlrOperation>(thisPtr->elmResSetMrg);
      break;
    case ELMNODE:
      if (thisPtr->isENRSMergerMade(lNodeIdx))
        return dynamic_cast<FFlrOperation>(thisPtr->getENRSMerger(lNodeIdx));
      break;
    case EVALP:
      if (thisPtr->isEPRSMergerMade(lNodeIdx))
        return dynamic_cast<FFlrOperation>(thisPtr->getEPRSMerger(lNodeIdx));
      break;
    }

  // Must build operation
  std::vector<FFaOperationBase*> baseOps;
  if (rKat == ELM)
    FFlrResultResolver::getElmReadOps(baseOps, elm,
                                      setup.variableType,
                                      setup.variableName,
                                      setup.resultSetName,
                                      setup.getOnlyExactResSetMatches);
  else if (rKat == ELMNODE)
    FFlrResultResolver::getElmNodeReadOps(baseOps, elm, lNodeIdx,
                                          setup.variableType,
                                          setup.variableName,
                                          setup.resultSetName,
                                          setup.getOnlyExactResSetMatches);
  else
    FFlrResultResolver::getEvalPReadOps(baseOps, elm, lNodeIdx,
                                        setup.variableType,
                                        setup.variableName,
                                        setup.resultSetName,
                                        setup.getOnlyExactResSetMatches);

  // For each result set that was let through by the resolving, check if
  // a read operation for element results exists and use it if it does
  FFlrOperations readOps;
  for (FFaOperationBase* baseOp : baseOps)
    if (baseOp)
    {
      FFlrOperation readOp = dynamic_cast<FFlrOperation>(baseOp);

      // If it was not of this type, try to convert it
      if (!readOp)
        FFaOpUtils::getUnaryConvertOp(readOp,baseOp,setup.toScalarOpName);

      if (readOp)
        readOps.push_back(readOp);
    }

  // Create new merge operation
  FFlrOperation mrgOp = NULL;
  if (readOps.size() > 1 ||
      (rKat >= ELMNODE && readOps.size() == 1 &&
       setup.resSetMergeOpName == "Max Difference"))
    mrgOp = new FFaNToOneOp<double>(readOps,setup.resSetMergeOpName);
  else if (readOps.size() == 1)
    mrgOp = readOps.front();

  // Cache the result. The FFlElmResult takes care of whether the operation
  // is calculated or not, independently of whether it is NULL or not.
  if (thisPtr)
    switch (rKat) {
    case ELM:
      thisPtr->setElmRSMerger(mrgOp);
      break;
    case ELMNODE:
      thisPtr->setENRSMerger(lNodeIdx,mrgOp);
      break;
    case EVALP:
      thisPtr->setEPRSMerger(lNodeIdx,mrgOp);
      break;
    }

  return mrgOp;
}


static FFlrOperation getNodeResSetMergeOp(FFlFENodeResult* thisPtr,
                                          FFlNode*         node,
                                          const FapFringeSetup& setup)
{
  if (thisPtr && thisPtr->myResSetMergerIsMade)
    return dynamic_cast<FFlrOperation>(thisPtr->resSetMerger);

  // Must build operation
  std::vector<FFaOperationBase*> baseOps;
  FFlrResultResolver::getNodeReadOps(baseOps, node,
                                     setup.variableType,
                                     setup.variableName,
                                     setup.resultSetName,
                                     setup.getOnlyExactResSetMatches);

  // For each result set that was let through by the resolving, check if a
  // read operation for node results exists and use it if it does
  FFlrOperations readOps;
  for (FFaOperationBase* baseOp : baseOps)
    if (baseOp)
    {
      FFlrOperation readOp = dynamic_cast<FFlrOperation>(baseOp);

      if (!readOp) // it was not of this type, try to convert it
        FFaOpUtils::getUnaryConvertOp(readOp,baseOp,setup.toScalarOpName);

      if (readOp)
        readOps.push_back(readOp);
    }

  // Create new merge operation
  FFlrOperation mrgOp = NULL;
  if (readOps.size() > 1)
    mrgOp = new FFaNToOneOp<double>(readOps,setup.resSetMergeOpName);
  else if (readOps.size() == 1)
    mrgOp = readOps.front();

  if (thisPtr) {
    thisPtr->resSetMerger = mrgOp;
    thisPtr->myResSetMergerIsMade = true;
  }

  return mrgOp;
}


/////////////////////////////////////////////////////////////////////////
//
//  Average operation creating
//
/////////////////////////////////////////////////////////////////////////

/*!
  Will make or return an operation that can evaluate the
  average of the element-node results over an element.
  NULL is returned if no legal averaging can be done.
*/

static FFlrOperation getENToEAverager(FFlFEElmResult* thisPtr,
                                      const FFlElementBase* elm,
                                      const FapFringeSetup& setup)
{
  if (thisPtr && thisPtr->myAveragerIsMade)
    return dynamic_cast<FFlrOperation>(thisPtr->myAverager);

  FFlrOperation  mrgOp = NULL;
  FFlrOperations elmNodResMergers;
  int lNode, nNodes = elm->getFEElementTopSpec()->getNodeCount();
  for (lNode = 1; lNode <= nNodes; lNode++)
    if ((mrgOp = getElmResSetMergeOp(thisPtr, elm, setup, ELMNODE, lNode)))
      elmNodResMergers.push_back(mrgOp);

  if (elmNodResMergers.size() > 1)
    mrgOp = new FFaNToOneOp<double>(elmNodResMergers,setup.averagingOpName);
  else if (elmNodResMergers.size() == 1)
    mrgOp = elmNodResMergers.front();
  else
    mrgOp = NULL;

  if (thisPtr) {
    thisPtr->myAverager = mrgOp;
    thisPtr->myAveragerIsMade = true;
  }

  return mrgOp;
}


/*!
  Will make or return an operation that can evaluate the
  average of the evaluation point results over an element.
  NULL is returned if no legal averaging can be done.
*/

static FFlrOperation getEPToEAverager(FFlFEElmResult* thisPtr,
                                      const FFlElementBase* elm,
                                      const FapFringeSetup& setup)
{
  if (thisPtr && thisPtr->myAveragerIsMade)
    return dynamic_cast<FFlrOperation>(thisPtr->myAverager);

  FFlrOperation  mrgOp = NULL;
  FFlrOperations elmNodResMergers;
  int lNode, nNodes = elm->getFEElementTopSpec()->getExpandedNodeCount();
  for (lNode = 1; lNode <= nNodes; lNode++)
    if ((mrgOp = getElmResSetMergeOp(thisPtr, elm, setup, EVALP, lNode)))
      elmNodResMergers.push_back(mrgOp);

  if (elmNodResMergers.size() > 1)
    mrgOp = new FFaNToOneOp<double>(elmNodResMergers,setup.averagingOpName);
  else if (elmNodResMergers.size() == 1)
    mrgOp = elmNodResMergers.front();
  else
    mrgOp = NULL;

  if (thisPtr) {
    thisPtr->myAverager = mrgOp;
    thisPtr->myAveragerIsMade = true;
  }

  return mrgOp;
}


static FFlrOperation getENToNAverager(FFlFENodeResult* thisPtr,
                                      FFlElementBase*  elm,
                                      const FFlNode*   node,
                                      const FFlrVxToElmMap& elementsOnVertex,
                                      const FapFringeSetup& setup)
{
  // If no element is specified, we are looking for line fringes
  // and will do averaging "nomatterwhat"
  bool noElmSpecified = !elm;

  // We can use the cache if we have a cache, and we are looking for line averaging (noElmSpecified)
  // We can also use the cache when all elements around this node can be validly averaged
  // The last chance to reuse the cache is if none of the elements around the node had results.
  // The cache will store the "best" cache found up to now => when it is valid both for lines and for elements.
  if (thisPtr->myAveragerIsMade && (noElmSpecified || !thisPtr->myAveragerIsLineOnly))
    return dynamic_cast<FFlrOperation>(thisPtr->myAverager);

  FFlrOperation mrgOp = NULL;
  int vxIdx = node->getVertexID();
  if (vxIdx < 0 || (size_t)vxIdx >= elementsOnVertex.size())
    return mrgOp;

  // If we have a specified element, check whether this element node
  // actually has results
  size_t elmNr;
  if (elm)
  {
    for (elmNr = 0; elmNr < elementsOnVertex[vxIdx].size() && !mrgOp; elmNr++)
      if (elementsOnVertex[vxIdx][elmNr].first == elm)
        mrgOp = getElmResSetMergeOp(elm->getResults(), elm, setup, ELMNODE,
                                    elementsOnVertex[vxIdx][elmNr].second);

    // If the provided element had no result, return invalid operation
    if (!mrgOp) return mrgOp;
  }

  // Find all the elment results that are averagable with this element
  // If no element is specified, use all that has results.

  std::vector<FFlElementBase*> elementsToAverage;
  FFlrOperations allMergedENRes;

  // Add the previously found data.
  // Neccesary for the tests on whether to cache to be valid.

  if (elm) {
    elementsToAverage.push_back(elm);
    allMergedENRes.push_back(mrgOp);
  }

  bool elmTypesMustBeEqual = setup.doAverage && setup.elmTypesMustBeEqual;
  bool elmCsIsImportant = (setup.toScalarOpName == "Xx" ||
                           setup.toScalarOpName == "Xy" ||
                           setup.toScalarOpName == "Yy");
  double maxMembraneAngle = setup.doAverage ? setup.maxMembraneAngle : 0.26; // ca 15 deg

  // Lambda function for determining whether two elements can be averaged
  auto&& canBeAveraged = [elmTypesMustBeEqual,elmCsIsImportant,maxMembraneAngle]
    (FFlElementBase* e1, FFlElementBase* e2) -> bool
  {
    if (!e1 || !e2) return false;

    // TODO : Check material and geometry properties

    if (elmTypesMustBeEqual)
      if (e1->getTypeName() != e2->getTypeName())
        return false;

    if (e1->getCathegory() != e2->getCathegory())
      return false;

    FaMat33 elementCS1 = e1->getGlobalizedElmCS();
    FaMat33 elementCS2 = e2->getGlobalizedElmCS();

    if (elmCsIsImportant)
      if (!elementCS1.isCoincident(elementCS2,1.0-cos(maxMembraneAngle)))
        return false;

    // Averaging can be done, but only if the elements have the same normal
    return (elementCS1[2].angle( elementCS2[2]) < maxMembraneAngle ||
            elementCS1[2].angle(-elementCS2[2]) < maxMembraneAngle);
  };

  FFlElementBase* anelm;
  for (elmNr = 0; elmNr < elementsOnVertex[vxIdx].size(); elmNr++)
    if ((anelm = elementsOnVertex[vxIdx][elmNr].first) != elm)
      if (noElmSpecified || canBeAveraged(elm,anelm))
      {
        FFlrOperation mergedENRes = getElmResSetMergeOp(anelm->getResults(),
                                                        anelm, setup, ELMNODE,
                                                        elementsOnVertex[vxIdx][elmNr].second);
        if (mergedENRes) {
          allMergedENRes.push_back(mergedENRes);
          elementsToAverage.push_back(anelm);
        }
      }

  // Try to average :

  // Create new merge operation
  FFlrOperation averagingOp = NULL;
  if (allMergedENRes.size() > 1)
    averagingOp = new FFaNToOneOp<double>(allMergedENRes,
                                          setup.doAverage ? setup.averagingOpName : std::string("Average"));
  else if (allMergedENRes.size() == 1 && setup.averagingOpName != "Max Difference")
    averagingOp = allMergedENRes.front();

  // Cache the averager if it was uniquely defined (either all could be averaged
  // or none). Also record whether the averaging conditions was for lines only.
  if (allMergedENRes.size() == elementsOnVertex[vxIdx].size() ||
      allMergedENRes.empty()) {
    thisPtr->myAverager = averagingOp;
    thisPtr->myAveragerIsMade = true;
    thisPtr->myAveragerIsLineOnly = noElmSpecified;
  }

  return averagingOp;
}


static FFlrOperation getElmNodeMergeOp(FFlElementBase* element, int lNode,
                                       FFlLinkHandler* lh,
                                       const FapFringeSetup& setup,
                                       bool expandedFace)
{
  if (!element) return NULL;

  switch (setup.resultClass)
    {
    case FapFringeSetup::ELM_NODE:
      if (setup.doAverage == FapFringeSetup::NODE)
      {
        FFlNode* node = element->getNode(lNode);
        return getENToNAverager(node->getResults(), element, node,
                                lh->getVxToElementMapping(), setup);
      }
      else if (!setup.doAverage)
      {
        // Do cache merge operations if line results are loaded
        FFlFEElmResult* elmRes = setup.isLoadingLineFringes ? element->getResults() : NULL;
        return getElmResSetMergeOp(elmRes, element, setup,
                                   expandedFace ? EVALP : ELMNODE, lNode);
      }
      break;

    case FapFringeSetup::NODE:
      if (setup.doAverage == FapFringeSetup::NODE || !setup.doAverage)
      {
        FFlNode* node = element->getNode(lNode);
        return getNodeResSetMergeOp(node->getResults(), node, setup);
      }
      break;

    default:
      break;
    }

  return NULL;
}

/////////////////////////////////////////////////////////////////////////
//
// Visual result operation creation
//
/////////////////////////////////////////////////////////////////////////

void FFlr::getFaceMergeOp(FFlrOperation& mrgOp,
                          FFlVisFace*    face,
                          const FapFringeSetup& setup)
{
  mrgOp = NULL;
  if (!face) return;

  FFlrOperations allFaceResOps, prefGrFaceResOps;

  if (setup.resultClass == FapFringeSetup::ELM || setup.isElmAveraging())
  {
    // For each element referring to this face:
    FaceElemRefVecCIter eit;
    for (eit = face->elementRefsBegin(); eit != face->elementRefsEnd(); ++eit)
    {
      FFlrOperation op = NULL;
      getElementMergeOp(op, eit->myElement, setup,face->isExpandedFace());
      if (!op) continue;

      // Check if in the preferred element group
      if (setup.prefGrp && setup.prefGrp->hasElement(eit->myElement->getID()))
        prefGrFaceResOps.push_back(op);
      else
        allFaceResOps.push_back(op);
    }
  }

  // If some preferred elements were found, use them
  if (!prefGrFaceResOps.empty())
    allFaceResOps.swap(prefGrFaceResOps);

  // Create new merge operation
  if (allFaceResOps.size() > 1)
    mrgOp = new FFaNToOneOp<double>(allFaceResOps,setup.geomAveragingOpName);
  else if (allFaceResOps.size() == 1)
    mrgOp = allFaceResOps.front();
}


void FFlr::getFaceVxMergeOp(FFlrOperations& mrgOps,
                            int faceStartIdx, FFlVisFace* face,
                            FFlLinkHandler* lh,
                            const FapFringeSetup& setup)
{
  if (!face) return;

  std::vector<FFlrOperations> allFaceResOps, prefGrFaceResOps;

  // For each element referring to this face:

  FaceElemRefVecCIter eit;
  for (eit = face->elementRefsBegin(); eit != face->elementRefsEnd(); ++eit)
    if (eit->myElement->isVisible())
    {
      // Get element topological node index for each face vertex
      std::vector<int> faceTop;
      face->getElmFaceTopology(faceTop,eit);

      if (allFaceResOps.empty()) {
        allFaceResOps.resize(faceTop.size());
        prefGrFaceResOps.resize(faceTop.size());
      }

      // Loop over face vertices
      for (size_t i = 0; i < faceTop.size(); i++)
      {
        // Get operation for the element node corresponding to this vertex :
        FFlrOperation op = getElmNodeMergeOp(eit->myElement, faceTop[i], lh,
                                             setup, face->isExpandedFace());
        if (!op) continue;

        // Check if in the preferred element group
        if (setup.prefGrp && setup.prefGrp->hasElement(eit->myElement->getID()))
          prefGrFaceResOps[i].push_back(op);
        else
          allFaceResOps[i].push_back(op);
      }
    }

  for (size_t i = 0; i < allFaceResOps.size(); i++)
  {
    // If some preferred elements were found, use them
    if (!prefGrFaceResOps[i].empty())
      allFaceResOps[i].swap(prefGrFaceResOps[i]);

    // Create new merge operation
    if (allFaceResOps[i].size() > 1)
      mrgOps[i + faceStartIdx] = new FFaNToOneOp<double>(allFaceResOps[i],setup.geomAveragingOpName);
    else if (allFaceResOps[i].size() == 1)
      mrgOps[i + faceStartIdx] = allFaceResOps[i].front();
    else
      mrgOps[i + faceStartIdx] = NULL;
  }
}


void FFlr::getEdgeVxMergeOp(FFlrOperations& mrgOps,
                            int edgeStartIdx, FFlVisEdge* edge,
                            FFlLinkHandler* lh,
                            const FapFringeSetup& setup)
{
  if (!edge) return;

  // Try node results directly :

  getNodeMergeOp(mrgOps[edgeStartIdx],
                 edge->getFirstVertex()->getNode(), lh, setup);

  getNodeMergeOp(mrgOps[edgeStartIdx+1],
                 edge->getSecondVertex()->getNode(), lh, setup);
}


void FFlr::getElementMergeOp(FFlrOperation& mrgOp,
                             FFlElementBase* element,
                             const FapFringeSetup& setup, bool expandedFace)
{
  mrgOp = NULL;
  if (!element) return;

  FFlFEElmResult* eres = NULL;
  switch (setup.resultClass)
    {
    case FapFringeSetup::ELM:
      // Todo, if EToN merging is implemented in getEdgeVxOp,
      // then we have to test whether line fringe reading is on.
      // If yes, we have to send the FFlFEElmResult pointer and not NULL as now.
      // eres = element->getResults();
      // End todo statement
      mrgOp = getElmResSetMergeOp(eres, element, setup, ELM, 1);
      break;

    case FapFringeSetup::ELM_NODE:
      switch (setup.doAverage)
        {
        case FapFringeSetup::ELM:
          if (setup.isLoadingLineFringes)
            eres = element->getResults();

          if (expandedFace)
            mrgOp = getEPToEAverager(eres, element, setup);
          else
            mrgOp = getENToEAverager(eres, element, setup);
          break;

        case FapFringeSetup::ELM_FACE:
          // Not yet implemented
        default:
          break;
        }
      break;

    case FapFringeSetup::NODE:
      // Not yet implemented
    default:
      break;
    }
}


void FFlr::getElementMergeOps(FFlrOperations& mrgOps,
                              FFlElementBase* element,
                              FFlLinkHandler* lh,
                              const FapFringeSetup& setup)
{
  if (!element) return;

  mrgOps.resize(element->getNodeCount());
  for (size_t i = 0; i < mrgOps.size(); i++)
    mrgOps[i] = getElmNodeMergeOp(element, 1+i, lh, setup, false);
}


void FFlr::getNodeMergeOp(FFlrOperation&  mrgOp,
                          FFlNode*        node,
                          FFlLinkHandler* lh,
                          const FapFringeSetup& setup)
{
  mrgOp = NULL;
  if (!node) return;

  switch (setup.resultClass)
    {
    case FapFringeSetup::NODE:
      mrgOp = getNodeResSetMergeOp(node->getResults(), node, setup);
      break;

    case FapFringeSetup::ELM_NODE:
      mrgOp = getENToNAverager(node->getResults(), NULL, node,
                               lh->getVxToElementMapping(), setup);
      break;

    default:
      break;
    }
}
