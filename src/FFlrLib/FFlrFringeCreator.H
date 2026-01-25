// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFLR_FRINGE_CREATOR_H
#define FFLR_FRINGE_CREATOR_H

#include <vector>

class FFlLinkHandler;
class FFlNode;
struct FFlGroupPartData;
struct FapFringeSetup;


namespace FFlrFringeCreator
{
  bool getColorData(std::vector<double>& colors,
                    const FFlGroupPartData& visRep, bool isPrFace);

  void deleteColorsXfs(FFlGroupPartData& visRep, bool memPoll = false);

  int buildColorXfs(FFlGroupPartData& visRep,
                    FFlLinkHandler* lh, const FapFringeSetup& setup);
  int buildColorXfs(FFlLinkHandler* lh, const FapFringeSetup& setup,
                    const std::vector<int>& nodesFilter);

  bool filterNodes(const FFlNode*, const std::vector<int>& nodeFilter);
}

#endif
