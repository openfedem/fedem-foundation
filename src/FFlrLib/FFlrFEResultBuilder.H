// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFLR_FE_RESULT_BUILDER_H
#define FFLR_FE_RESULT_BUILDER_H

#include "FFaLib/FFaOperation/FFaOperation.H"

class FFlLinkHandler;
class FFlElementBase;
class FFlNode;
class FFlVisFace;
class FFlVisEdge;
struct FapFringeSetup;

typedef FFaOperation<double>*      FFlrOperation;
typedef std::vector<FFlrOperation> FFlrOperations;


namespace FFlr
{
  //////////////////////////////////////////////////
  //
  // Methods for reading results for FFlVisFace

  void getFaceMergeOp  (FFlrOperation&  mrgOp,
                        FFlVisFace*     face,
                        const FapFringeSetup& setup);

  void getFaceVxMergeOp(FFlrOperations& mrgOps,
                        int             faceStartIdx,
                        FFlVisFace*     face,
                        FFlLinkHandler* lh,
                        const FapFringeSetup& setup);


  //////////////////////////////////////////////////
  //
  // Methods for reading results for FFlVisEdge

  void getEdgeVxMergeOp(FFlrOperations& mrgOps,
                        int             edgeStartIdx,
                        FFlVisEdge*     edge,
                        FFlLinkHandler* lh,
                        const FapFringeSetup& setup);

  //////////////////////////////////////////////////
  //
  // Methods for reading results onto the FE quantities

  void getElementMergeOp (FFlrOperation&  mrgOp,
                          FFlElementBase* element,
                          const FapFringeSetup& setup,
                          bool expandedFace = false);

  void getElementMergeOps(FFlrOperations& mrgOps,
                          FFlElementBase* element,
                          FFlLinkHandler* lh,
                          const FapFringeSetup& setup);

  void getNodeMergeOp    (FFlrOperation&  mrgOp,
                          FFlNode*        node,
                          FFlLinkHandler* lh,
                          const FapFringeSetup& setup);
}

#endif
