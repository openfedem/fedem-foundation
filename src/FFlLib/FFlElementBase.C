// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFlLib/FFlElementBase.H"
#include "FFlLib/FFlFEElementTopSpec.H"
#include "FFlLib/FFlFEAttributeSpec.H"
#include "FFlLib/FFlFEResultBase.H"
#include "FFlLib/FFlFEParts/FFlPMAT.H"
#include "FFaLib/FFaAlgebra/FFaCheckSum.H"
#include "FFaLib/FFaAlgebra/FFaTensor3.H"
#include "FFaLib/FFaAlgebra/FFaMat33.H"


FFlElementBase::FFlElementBase(int id) : FFlPartBase(id)
{
  calculateResults = true;
  myResults = NULL;
}


FFlElementBase::FFlElementBase(const FFlElementBase& obj)
  : FFlPartBase(obj), FFlFEAttributeRefs(obj), FFlFENodeRefs(obj),
    FFlVisualRefs(obj)
{
  calculateResults = true;
  myResults = NULL;
}


FFlElementBase::~FFlElementBase()
{
  this->deleteResults();
}


void FFlElementBase::calculateChecksum(FFaCheckSum* cs, int cstype) const
{
  FFlPartBase::checksum(cs);
  FFlFEAttributeRefs::checksum(cs);
  FFlFENodeRefs::checksum(cs);
  FFlVisualRefs::checksum(cs,cstype);
}


void FFlElementBase::deleteResults()
{
  delete myResults;
  myResults = NULL;
}


double FFlElementBase::getMassDensity() const
{
  FFlPMAT* curMat = dynamic_cast<FFlPMAT*>(this->getAttribute("PMAT"));
  return curMat ? curMat->materialDensity.getValue() : 0.0;
}


bool FFlElementBase::getVolumeAndInertia(double& volume, FaVec3& cog,
					 FFaTensor3& inertia) const
{
  volume  = 0.0;
  cog     = FaVec3(0.0,0.0,0.0);
  inertia = FFaTensor3(0.0);
  return false;
}


bool FFlElementBase::getMassProperties(double& mass, FaVec3& cog,
				       FFaTensor3& inertia) const
{
  if (!this->getVolumeAndInertia(mass,cog,inertia)) return false;

  double rho = this->getMassDensity();
  mass    *= rho;
  inertia *= rho;
  return true;
}


FaMat33 FFlElementBase::getGlobalizedElmCS() const
{
  return FaMat33();
}


FaVec3 FFlElementBase::interpolate(const double*,
                                   const std::vector<FaVec3>&) const
{
  return FaVec3();
}


FaVec3 FFlElementBase::mapping(double, double, double) const
{
  return FaVec3();
}


bool FFlElementBase::invertMapping(const FaVec3&, double*) const
{
  std::cerr <<" *** FFlElementBase::invertMapping: Not implemented for "
            << this->getTypeName() <<" elements."<< std::endl;
  return false;
}
