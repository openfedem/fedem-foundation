// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFlLib/FFlUtils.H"
#include "FFlLib/FFlLinkHandler.H"
#include "FFlLib/FFlElementBase.H"
#include "FFlLib/FFlFEParts/FFlPWAVGM.H"
#include "FFlLib/FFlFEParts/FFlNode.H"
#include "FFaLib/FFaAlgebra/FFaBody.H"
#include "FFaLib/FFaString/FFaTokenizer.H"
#include "FFaLib/FFaDefinitions/FFaMsg.H"


bool FFl::convertMPCsToWAVGM (FFlLinkHandler* part, const FFl::MPCMap& mpcs)
{
  using Doubles       = std::vector<double>;
  using DoublesMap    = std::map<int,Doubles>;
  using DoublesMapMap = std::map<int,DoublesMap>;

  // Create WAVGM elements for multi-point constraints with common slave node
  for (const std::pair<const int,MPC>& mpcGroup : mpcs)
  {
    // Find the element nodes
    std::vector<int> nodes = { mpcGroup.first };
    for (const std::pair<const short int,DepDOFs>& mpc : mpcGroup.second)
      for (const DepDOF& dof : mpc.second)
        if (std::find(nodes.begin(),nodes.end(),dof.node) == nodes.end())
          nodes.push_back(dof.node);

#if FFL_DEBUG > 1
    std::cout <<"\nWAVGM element nodes:";
    for (int n : nodes) std::cout <<" "<< n;
    std::cout << std::endl;
#endif

    size_t nRow = 0;
    DoublesMapMap dofWeights;
    for (const std::pair<const short int,DepDOFs>& mpc : mpcGroup.second)
      if (mpc.first > 0 && mpc.first < 7)
      {
        // Find the weight matrix associated with this slave DOF
        DoublesMap& dofWeight = dofWeights[mpc.first];
        for (const DepDOF& dof : mpc.second)
        {
          Doubles& weights = dofWeight[dof.lDof];
          weights.resize(nodes.size()-1,0.0);
          for (size_t iNod = 1; iNod < nodes.size(); iNod++)
            if (dof.node == nodes[iNod])
            {
              weights[iNod-1] = dof.coeff;
              break;
            }
        }
        nRow += 6; // Assuming all DOFs in the master node is referred
#if FFL_DEBUG > 1
        std::cout <<"Weight matrix associated with slave dof "<< mpc.first;
        for (const std::pair<const int,Doubles>& weights : dofWeight)
        {
          std::cout <<"\n\t"<< weights.first <<":";
          for (double c : weights.second) std::cout <<" "<< c;
        }
        std::cout << std::endl;
#endif
      }

    int refC = 0;
    size_t indx = 1;
    size_t nMst = nodes.size() - 1;
    int indC[6] = { 0, 0, 0, 0, 0, 0 };
    Doubles weights(nRow*nMst,0.0);
    for (const std::pair<const int,DoublesMap>& dofw : dofWeights)
    {
      refC = 10*refC + dofw.first; // Compressed slave DOFs identifier
      indC[dofw.first-1] = indx;   // Index to first weight for this slave DOF
      for (const std::pair<const int,Doubles>& dof : dofw.second)
        for (size_t j = 0; j < dof.second.size(); j++)
          weights[indx+6*j+dof.first-2] = dof.second[j];
      indx += 6*nMst;
    }

    int id = part->getNewElmID();
    FFlElementBase* newElem = ElementFactory::instance()->create("WAVGM",id);
    if (!newElem)
    {
      ListUI <<"\n *** Error: Failure creating WAVGM element "<< id <<".\n";
      return false;
    }

    newElem->setNodes(nodes);
    if (!part->addElement(newElem))
      return false;

    FFlAttributeBase* myAtt = AttributeFactory::instance()->create("PWAVGM",id);
    FFlPWAVGM* newAtt = static_cast<FFlPWAVGM*>(myAtt);
    newAtt->refC = -refC; // Hack: Negative refC means explicit constraints
    newAtt->weightMatrix.data().swap(weights);
    for (int j = 0; j < 6; j++)
      newAtt->indC[j] = indC[j];

    if (part->addUniqueAttributeCS(myAtt))
      newElem->setAttribute(myAtt);
    else
      return false;
  }

  return true;
}


bool FFl::activateElmGroups (FFlLinkHandler* part, const std::string& elmGroups)
{
  using Property = std::pair<std::string,int>;

  // Activate/deactivate all elements in the FE part
  part->initiateCalculationFlag(elmGroups.empty());
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
    part->updateCalculationFlag(gID,true);
  for (const Property& group : implicitGroups)
    part->updateCalculationFlag(group.first,group.second,true);

  return true;
}


bool FFl::extractBodyFromShell (FFlLinkHandler* part, const FaMat34& partCS,
                                const std::string& fname)
{
  using NodeMap = std::map<FFlNode*,size_t,FFlFEPartBaseLess>;

  NodeMap nodeMap;
  ElementsVec shells;
  ElementsCIter eit;
  for (eit = part->elementsBegin(); eit != part->elementsEnd(); ++eit)
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
  for (const NodeMap::value_type& node : nodeMap)
    nodes[node.second] = node.first;

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
