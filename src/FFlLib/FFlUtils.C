// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFlLib/FFlUtils.H"
#include "FFlLib/FFlLinkHandler.H"
#include "FFlLib/FFlElementBase.H"
#include "FFlLib/FFlFEParts/FFlNode.H"
#include "FFaLib/FFaAlgebra/FFaBody.H"
#include "FFaLib/FFaString/FFaTokenizer.H"

typedef std::pair<std::string,int> Property;


bool FFl::activateElmGroups(FFlLinkHandler* link, const std::string& elmGroups)
{
  // Activate/deactivate all elements in the link
  link->initiateCalculationFlag(elmGroups.empty());
  if (elmGroups.empty()) return true;

  // Developer note: The current implementation of FFaTokenizer strips white-
  // spaces such that the string comming in is on the form <PMAT33,PTHICK55>.
  // This creates a need for some extra parsing...

  std::vector<int>      groupId;
  std::vector<Property> implicitGroups;
  if (elmGroups[0] == '<')
  {
    // A list of element group numbers were specified
    FFaTokenizer groups(elmGroups,'<','>',',');
    for (const std::string& token : groups)
      if (isdigit(token[0]))
        groupId.push_back(atoi(token.c_str()));
      else
      {
        size_t idStart = token.size()+1;
        while (--idStart > 0 && isdigit(token[idStart-1]));
        if (idStart > 1)
        {
          int ID = atoi(token.substr(idStart).c_str());
          implicitGroups.push_back(std::make_pair(token.substr(0,idStart),ID));
        }
      }
  }
  else
    // A single element group number was specified
    groupId.push_back(atoi(elmGroups.c_str()));

  if (groupId.empty() && implicitGroups.empty()) return false;

  // Switch on all elements in the specified groups
  for (int gID : groupId)
    link->updateCalculationFlag(gID,true);
  for (const Property& group : implicitGroups)
    link->updateCalculationFlag(group.first,group.second,true);

  return true;
}


bool FFl::extractBodyFromShell(FFlLinkHandler* link, const FaMat34& partCS,
                               const std::string& fname)
{
  ElementsVec shells;
  ElementsCIter eit;
  std::map<FFlNode*,size_t,FFlFEPartBaseLess> nodeMap;
  std::map<FFlNode*,size_t,FFlFEPartBaseLess>::const_iterator nit;
  for (eit = link->elementsBegin(); eit != link->elementsEnd(); ++eit)
    if ((*eit)->doCalculations() && (*eit)->getNodeCount() <= 4)
    {
      shells.push_back(*eit);
      for (NodeCIter n = (*eit)->nodesBegin(); n != (*eit)->nodesEnd(); ++n)
        if (nodeMap.find(n->getReference()) == nodeMap.end())
          nodeMap[n->getReference()] = nodeMap.size();
    }

  if (shells.empty()) return false;

  // Order the nodes w.r.t. vertex indices
  NodesVec nodes(nodeMap.size());
  for (nit = nodeMap.begin(); nit != nodeMap.end(); ++nit)
    nodes[nit->second] = nit->first;

  // Create the body object
  FFaBody body;
  for (FFlNode* node : nodes)
    body.addVertex(node->getPos());
  for (FFlElementBase* elm : shells)
  {
    int j = 0;
    int mnpc[4] = { -1, -1, -1, -1 };
    for (NodeCIter n = elm->nodesBegin(); n != elm->nodesEnd(); ++n)
      mnpc[j++] = nodeMap[n->getReference()];
    body.addFace(mnpc[0],mnpc[1],mnpc[2],mnpc[3]);
  }

  // Write CAD file
  return body.writeCAD(fname,partCS);
}
