// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFL_UTILS_H
#define FFL_UTILS_H

#include <string>

class FFlLinkHandler;
class FaMat34;


namespace FFl
{
  bool activateElmGroups(FFlLinkHandler* link,
                         const std::string& elmGroups = "");
  bool extractBodyFromShell(FFlLinkHandler* link,
                            const FaMat34& partCS, const std::string& fname);
}

#endif
