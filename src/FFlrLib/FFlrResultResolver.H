// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFLR_RESULT_RESOLVER_H
#define FFLR_RESULT_RESOLVER_H

#include <string>
#include <vector>
#include <map>

class FFrExtractor;
class FFrEntryBase;
class FFlElementBase;
class FFlNode;
class FFaOperationBase;

typedef std::vector<FFrEntryBase*>     FFrEntryVec;
typedef std::vector<FFaOperationBase*> FFaOperationVec;


namespace FFlrResultResolver
{
  extern std::map<std::string,int> errMsg;

  void addMsg(const std::string& msg);
  char setLinkInFocus(int baseId, FFrExtractor* rdb, const char kind='b');
  void clearLinkInFocus();

  FFaOperationBase* findPosition(const std::string& oType, int baseId,
                                 FFrExtractor* extr);

  const FFrEntryVec* findFEResults(int baseId, FFrExtractor* extr,
                                   const std::string& FEResultName);

  void getElmNodeReadOps(FFaOperationVec& readOps,
                         const FFlElementBase* elm, int lNode,
                         const std::string& type,
                         const std::string& variableName,
                         const std::string& resSetName,
                         bool onlyResSetMatch);

  void getEvalPReadOps(FFaOperationVec& readOps,
                       const FFlElementBase* elm, int lNode,
                       const std::string& type,
                       const std::string& variableName,
                       const std::string& resSetName,
                       bool onlyResSetMatch);

  void getElmReadOps(FFaOperationVec& readOps,
                     const FFlElementBase* elm,
                     const std::string& type,
                     const std::string& variableName,
                     const std::string& resSetName,
                     bool onlyResSetMatch);

  void getNodeReadOps(FFaOperationVec& readOps,
                      const FFlNode* node,
                      const std::string& type,
                      const std::string& variableName,
                      const std::string& resSetName,
                      bool onlyResSetMatch);

  FFaOperationBase* getNodeReadOp(const FFrEntryBase* resNode,
                                  const std::string& type,
                                  const std::string& variableName,
                                  const std::string& resSetName);
}

#endif
