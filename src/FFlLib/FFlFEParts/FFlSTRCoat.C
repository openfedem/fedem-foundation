// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFlLib/FFlFEParts/FFlSTRCoat.H"
#include "FFlLib/FFlFEParts/FFlNode.H"
#include "FFaLib/FFaAlgebra/FFaMat33.H"
#include "FFaLib/FFaDefinitions/FFaMsg.H"

#ifdef FF_NAMESPACE
namespace FF_NAMESPACE {
#endif


void FFlSTRCT3::init()
{
  using TypeInfoSpec   = FFaSingelton<FFlTypeInfoSpec,FFlSTRCT3>;
  using AttributeSpec  = FFaSingelton<FFlFEAttributeSpec,FFlSTRCT3>;
  using ElementTopSpec = FFaSingelton<FFlFEElementTopSpec,FFlSTRCT3>;

  TypeInfoSpec::instance()->setTypeName("STRCT3");
  TypeInfoSpec::instance()->setCathegory(FFlTypeInfoSpec::STRC_ELM);

  ElementFactory::instance()->registerCreator(TypeInfoSpec::instance()->getTypeName(),
                                              FFaDynCB2S(FFlSTRCT3::create,int,FFlElementBase*&));

  AttributeSpec::instance()->addLegalAttribute("PSTRC",true,true);
  AttributeSpec::instance()->addLegalAttribute("PFATIGUE",false);

  ElementTopSpec::instance()->setNodeCount(3);
  ElementTopSpec::instance()->setNodeDOFs(0);
  ElementTopSpec::instance()->setShellFaces(true);

  int faces[] = { 1, 2, 3,-1 };
  ElementTopSpec::instance()->setTopology(1,faces);
}


FaMat33 FFlSTRCT3::getGlobalizedElmCS() const
{
  FaMat33 T;
  return T.makeGlobalizedCS(this->getNode(1)->getPos(),
			    this->getNode(2)->getPos(),
			    this->getNode(3)->getPos());
}


void FFlSTRCQ4::init()
{
  using TypeInfoSpec   = FFaSingelton<FFlTypeInfoSpec,FFlSTRCQ4>;
  using AttributeSpec  = FFaSingelton<FFlFEAttributeSpec,FFlSTRCQ4>;
  using ElementTopSpec = FFaSingelton<FFlFEElementTopSpec,FFlSTRCQ4>;

  TypeInfoSpec::instance()->setTypeName("STRCQ4");
  TypeInfoSpec::instance()->setCathegory(FFlTypeInfoSpec::STRC_ELM);

  ElementFactory::instance()->registerCreator(TypeInfoSpec::instance()->getTypeName(),
                                              FFaDynCB2S(FFlSTRCQ4::create,int,FFlElementBase*&));

  AttributeSpec::instance()->addLegalAttribute("PSTRC",true,true);
  AttributeSpec::instance()->addLegalAttribute("PFATIGUE",false);

  ElementTopSpec::instance()->setNodeCount(4);
  ElementTopSpec::instance()->setNodeDOFs(0);
  ElementTopSpec::instance()->setShellFaces(true);

  int faces[] = { 1, 2, 3, 4,-1 };
  ElementTopSpec::instance()->setTopology(1,faces);
}


FaMat33 FFlSTRCQ4::getGlobalizedElmCS() const
{
  FaMat33 T;
  return T.makeGlobalizedCS(this->getNode(1)->getPos(),
			    this->getNode(2)->getPos(),
			    this->getNode(3)->getPos(),
			    this->getNode(4)->getPos());
}


void FFlSTRCT6::init()
{
  using TypeInfoSpec   = FFaSingelton<FFlTypeInfoSpec,FFlSTRCT6>;
  using AttributeSpec  = FFaSingelton<FFlFEAttributeSpec,FFlSTRCT6>;
  using ElementTopSpec = FFaSingelton<FFlFEElementTopSpec,FFlSTRCT6>;

  TypeInfoSpec::instance()->setTypeName("STRCT6");
  TypeInfoSpec::instance()->setCathegory(FFlTypeInfoSpec::STRC_ELM);

  ElementFactory::instance()->registerCreator(TypeInfoSpec::instance()->getTypeName(),
                                              FFaDynCB2S(FFlSTRCT6::create,int,FFlElementBase*&));

  AttributeSpec::instance()->addLegalAttribute("PSTRC",true,true);
  AttributeSpec::instance()->addLegalAttribute("PFATIGUE",false);

  ElementTopSpec::instance()->setNodeCount(6);
  ElementTopSpec::instance()->setNodeDOFs(0);
  ElementTopSpec::instance()->setShellFaces(true);

  int faces[] = { 1, 2, 3, 4, 5, 6,-1 };
  ElementTopSpec::instance()->setTopology(1,faces);
}


FaMat33 FFlSTRCT6::getGlobalizedElmCS() const
{
  FaMat33 T;
  return T.makeGlobalizedCS(this->getNode(1)->getPos(),
			    this->getNode(3)->getPos(),
			    this->getNode(5)->getPos());
}


void FFlSTRCQ8::init()
{
  using TypeInfoSpec   = FFaSingelton<FFlTypeInfoSpec,FFlSTRCQ8>;
  using AttributeSpec  = FFaSingelton<FFlFEAttributeSpec,FFlSTRCQ8>;
  using ElementTopSpec = FFaSingelton<FFlFEElementTopSpec,FFlSTRCQ8>;

  TypeInfoSpec::instance()->setTypeName("STRCQ8");
  TypeInfoSpec::instance()->setCathegory(FFlTypeInfoSpec::STRC_ELM);

  ElementFactory::instance()->registerCreator(TypeInfoSpec::instance()->getTypeName(),
                                              FFaDynCB2S(FFlSTRCQ8::create,int,FFlElementBase*&));

  AttributeSpec::instance()->addLegalAttribute("PSTRC",true,true);
  AttributeSpec::instance()->addLegalAttribute("PFATIGUE",false);

  ElementTopSpec::instance()->setNodeCount(8);
  ElementTopSpec::instance()->setNodeDOFs(0);
  ElementTopSpec::instance()->setShellFaces(true);

  int faces[] = { 1, 2, 3, 4, 5, 6, 7, 8,-1 };
  ElementTopSpec::instance()->setTopology(1,faces);
}


FaMat33 FFlSTRCQ8::getGlobalizedElmCS() const
{
  FaMat33 T;
  return T.makeGlobalizedCS(this->getNode(1)->getPos(),
			    this->getNode(3)->getPos(),
			    this->getNode(5)->getPos(),
			    this->getNode(7)->getPos());
}


bool FFlStrainCoatBase::resolveElmRef(const std::vector<FFlElementBase*>& possibleElms,
				      bool suppressErrmsg)
{
  if (possibleElms.empty())
    return false;

  if (myFElm.resolve(possibleElms))
    return true;

  if (!suppressErrmsg)
    ListUI <<"\n *** Error: Failed to resolve reference to finite element "
	   << myFElm.getID() <<"\n";

  return false;
}


FFlElementBase* FFlStrainCoatBase::getFElement() const
{
  if (myFElm.isResolved())
    return myFElm.getReference();
  else
    return 0;
}

#ifdef FF_NAMESPACE
} // namespace
#endif
