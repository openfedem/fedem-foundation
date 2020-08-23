// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#ifndef FFL_NODE_H
#define FFL_NODE_H

#include "FFlLib/FFlPartBase.H"
#include "FFlLib/FFlTypeInfoSpec.H"
#include "FFlLib/FFlReference.H"
#ifdef FT_USE_MEMPOOL
#include "FFaLib/FFaPatterns/FFaMemPool.H"
#endif
#include "FFaLib/FFaPatterns/FFaSingelton.H"

class FFlVertex;
class FFlPCOORDSYS;
class FFlAttributeBase;
class FFlFEResultBase;
class FFlFENodeResult;
class FFaUnitCalculator;
class FFaCheckSum;
class FaVec3;


class FFlNode : public FFlPartBase
{
public:
  FFlNode(int ID);
  FFlNode(int ID, double x, double y, double z, int s = 0);
  FFlNode(int ID, const FaVec3& pos, int s = 0);
  FFlNode(const FFlNode& node);
  virtual ~FFlNode();

  FFlNode& operator=(const FFlNode&) = delete;
  FFlNode* clone() const { return new FFlNode(*this); }

  static void init();

  FFL_TYPE_INFO(FFlNode);

  void calculateChecksum(FFaCheckSum* cs, int precision,
                         bool includeExtNodeInfo);
  void convertUnits(const FFaUnitCalculator* convCal);

  // Vertex management:

  int getVertexID() const;

  void setVertex(FFlVertex* aVertex);
  FFlVertex* getVertex() const { return myVertex; }
  const FaVec3& getPos() const;
  FaVec3& position() { return const_cast<FaVec3&>(this->getPos()); }

  // Node status management:

  enum { INTERNAL = 0, EXTERNAL = 1, SLAVENODE = 2, REFNODE = 3 };

  bool setStatus(int newStat);
  int  getStatus(int ignore = 0) const { return status > ignore ? status : 0; }

  bool setExternal(bool ext = true);
  bool isFixed(int dof = 0) const;
  bool isExternal() const   { return status == 1; }
  bool isSlaveNode() const  { return status >= 2; }
  bool isRefNode() const    { return status == 3; }
  bool isAttachable() const { return status >= 0 && status != 2; }

  // Local dofs management:

  enum { FFL_ALL_DOFS = -1, FFL_THREE_DOFS = 3, FFL_SIX_DOFS = 6 };

  void pushDOFs(int dofs)          { if (dofs > myDOFCount) myDOFCount = dofs; }
  bool hasDOFs(int dofs = 1) const { return myDOFCount >= dofs; }
  int  getMaxDOFs() const          { return myDOFCount; }

  // Local coordinate system management:

  void setLocalSystem(const FFlPCOORDSYS* coorSys);
  void setLocalSystem(int ID) { myLocalSystem = ID; }

  bool resolveLocalSystem(const std::map<int,FFlAttributeBase*>& possibleCSs,
                          bool suppressErrmsg = false);

  bool hasLocalSystem() const { return myLocalSystem.isResolved(); }
  int  getLocalSystemID() const;
  FFlPCOORDSYS* getLocalSystem() const;

  // Nodal result management:

  bool hasResults() const { return myResults == NULL ? false : true; }
  FFlFENodeResult* getResults();
  void deleteResults();

protected:
  char             status; // 0=internal, 1=external, 2=slavenode(internal)
  char             myDOFCount;
  FFlVertex*       myVertex;
  FFlFEResultBase* myResults;

  FFlReference<FFlAttributeBase> myLocalSystem; // solution coordinate system

#ifdef FT_USE_MEMPOOL
  FFA_MAKE_MEMPOOL;
#endif
};


typedef FFaSingelton<FFlTypeInfoSpec,FFlNode> FFlNodeTypeInfoSpec;

#endif
