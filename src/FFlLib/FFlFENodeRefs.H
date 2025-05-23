// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFL_FE_NODE_REFS_H
#define FFL_FE_NODE_REFS_H

#include "FFlLib/FFlReference.H"
#include "FFlLib/FFlFEParts/FFlNode.H"

class FFlFEElementTopSpec;
class FFaCheckSum;

#ifdef FF_NAMESPACE
namespace FF_NAMESPACE {
#endif

using NodeRef   = FFlReference<FFlNode>;
using NodeVec   = std::vector<NodeRef>;
using NodeCIter = NodeVec::const_iterator;


class FFlFENodeRefs
{
protected:
  FFlFENodeRefs() { mySize = 0.0; }
  FFlFENodeRefs(const FFlFENodeRefs& obj);
  FFlFENodeRefs& operator=(const FFlFENodeRefs&) = delete;

public:
  virtual ~FFlFENodeRefs() {}

  // Get and set nodes on the different element node locations

  FFlNode* getNode(const int topologicalPos) const;
  int getNodeID(const int topologicalPos) const;
  int getTopPos(const int nodeID) const;

  short int getFaceNum(int node1, int node2) const;

  int getFaceSize(short int face = 1) const;

  bool getFaceNodes(std::vector<FFlNode*>& nodes, short int face = 1,
		    bool switchNormal = false) const;

  virtual bool getFaceNormals(std::vector<FaVec3>&, short int = 1,
			      bool = false) const { return false; }

  virtual bool setNode(const int topologicalPos, FFlNode* nodeRef);
  virtual bool setNode(const int topologicalPos, int nodeRef);
  virtual bool setNodes(const std::vector<int>& nodeRefs,
                        size_t offset = 0, bool = false);
  virtual bool setNodes(const std::vector<FFlNode*>& nodeRefs,
                        size_t offset = 0, bool = false);

  //! \brief Resolves the node ID to FFlNode pointer after reading a file.
  bool resolveNodeRefs(const std::vector<FFlNode*>& possibleReferences,
		       bool suppressErrmsg = false);

  // Access the node container

  NodeCIter nodesBegin() const { return myNodes.begin(); }
  NodeCIter nodesEnd()   const { return myNodes.end(); }
  int getNodeCount()     const { return myNodes.size(); }

  //! \brief Returns the geometric center of the element nodes.
  FaVec3 getNodeCenter() const;
  //! \brief Returns the characteristic element size.
  double getSize() const;

  //! \brief Returns the topological specification of the element.
  virtual FFlFEElementTopSpec* getFEElementTopSpec() const = 0;

  //! \brief Returns the nodal coordinates for this element.
  virtual int getNodalCoor(double* X, double* Y, double* Z) const;

private:
  void initNodeVector() const;

protected:
  void checksum(FFaCheckSum* cs) const;

  mutable NodeVec myNodes;

private:
  mutable double mySize;
};

#ifdef FF_NAMESPACE
} // namespace
#endif
#endif
