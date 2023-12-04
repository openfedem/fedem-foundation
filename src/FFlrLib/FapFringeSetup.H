// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FAP_FRINGE_SETUP_H
#define FAP_FRINGE_SETUP_H

#include <string>

class FFlGroup;


struct FapFringeSetup
{
  enum FringeEnums { NONE=0, NODE, ELM_NODE, ELM, ELM_FACE };

  FringeEnums resultClass = NONE;

  std::string variableName;
  std::string variableType;
  std::string resultSetName;
  bool        getOnlyExactResSetMatches = false;

  // Result set merging
  std::string resSetMergeOpName;

  // Averaging options
  FringeEnums doAverage = NONE;
  std::string averagingOpName;

  double maxMembraneAngle   = 0.0;
  bool materialsMustBeEqual = false;
  bool propsMustBeEqual     = false;
  bool elmTypesMustBeEqual  = false;

  bool isLoadingLineFringes = false;
  bool isLoadingFaceFringes = false;

  // To-scalar transformation
  std::string toScalarOpName;

  // Selection between multiple results on same face, node or edge
  FFlGroup*   prefGrp = NULL;
  std::string geomAveragingOpName;

  bool isOneColorPrVertex() const
  { return resultClass == NODE || (resultClass == ELM_NODE && doAverage == NODE); }

  bool isElmAveraging() const
  { return doAverage == ELM || doAverage == ELM_FACE; }

  bool isOneColorPrFace() const
  { return (resultClass == ELM && doAverage == NONE) || this->isElmAveraging(); }
};

/*
  Input:
    Part of element models,

    Result variable definition
       string variableNames
       string variableType
       string resultSetName
       bool   getOnlyExactResSetMatches

    Result set concat operation
       string opName = { N To OneOperations }

    Averaging operation
       string opName = { N To OneOperations }
       string averageingItem = { Element, Node }
       Reservations:
         double maxMembraneAngle
         bool materialsEqual
         bool propertiesEqual,
         bool elementTypesEqual.

    Transformation:
       string CS independent Tensor to scalar transformation name
       CS dependent -"-
           CS Global, Local

    Multi vis rep concat operation
       string opName = { N To OneOperations }

    Legend transformation operation name
       string opName = { none, 1/x, -x, log(x), ...}

    Scalar to look std::mapping
       string opName = { alongAxis, linear, aroundCenter ...}
*/

#endif
