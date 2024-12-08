// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "FFlLib/FFlVisualRefs.H"
#include "FFlLib/FFlFEParts/FFlVAppearance.H"
#include "FFlLib/FFlFEParts/FFlVDetail.H"
#include "FFlLib/FFlLinkCSMask.H"
#include "FFaLib/FFaDefinitions/FFaMsg.H"

#ifdef FF_NAMESPACE
namespace FF_NAMESPACE {
#endif


void FFlVisualRefs::useVisualsFrom(const FFlVisualRefs* obj)
{
  myApp    = obj->myApp;
  myDetail = obj->myDetail;
}


bool FFlVisualRefs::setVisual(const FFlVisualBase* vis)
{
  if (dynamic_cast<const FFlVAppearance*>(vis))
    myApp = vis;
  else if (dynamic_cast<const FFlVDetail*>(vis))
    myDetail = vis;
  else
    return false;

  return true;
}


bool FFlVisualRefs::setVisual(const std::string& type, int ID)
{
  using AppearanceTypeSpec = FFaSingelton<FFlTypeInfoSpec,FFlVAppearance>;
  using DetailTypeSpec     = FFaSingelton<FFlTypeInfoSpec,FFlVDetail>;

  if (type == AppearanceTypeSpec::instance()->getTypeName())
    myApp = ID;
  else if (type == DetailTypeSpec::instance()->getTypeName())
    myDetail = ID;
  else
    return false;

  return true;
}


bool FFlVisualRefs::resolveVisuals(const std::vector<FFlVisualBase*>& possibleViss,
				   bool suppressErrmsg)
{
  if (myApp.resolve(possibleViss) & myDetail.resolve(possibleViss))
    return true;
  else if (suppressErrmsg)
    return false;

  if (!myApp.isResolved())
    ListUI <<"\n*** Error: Failed to resolve visual appearance id: "
	   << myApp.getID() <<"\n";

  if (!myDetail.isResolved())
    ListUI <<"\n*** Error: Failed to resolve visual detail id: "
	   << myDetail.getID() <<"\n";

  return false;
}


FFlVAppearance* FFlVisualRefs::getAppearance() const
{
  if (myApp.isResolved())
    return dynamic_cast<FFlVAppearance*>(myApp.getReference());
  else
    return NULL;
}


FFlVDetail* FFlVisualRefs::getDetail() const
{
  if (myDetail.isResolved())
    return dynamic_cast<FFlVDetail*>(myDetail.getReference());
  else
    return NULL;
}


FFlVisualBase* FFlVisualRefs::getVisualAppearance() const
{
  return myApp.isResolved() ? myApp.getReference() : NULL;
}


FFlVisualBase* FFlVisualRefs::getVisualDetail() const
{
  return myDetail.isResolved() ? myDetail.getReference() : NULL;
}


bool FFlVisualRefs::isVisible() const
{
  if (myDetail.isResolved())
    return this->getDetail()->detail.getValue() == FFlVDetail::ON;
  else
    return true;
}


void FFlVisualRefs::checksum(FFaCheckSum* cs, int cstype) const
{
  if ((cstype & FFl::CS_VISUALMASK) == FFl::CS_NOVISUALINFO) return;

  if (myApp.getID())    cs->add(myApp.getID());
  if (myDetail.getID()) cs->add(myDetail.getID());
}

#ifdef FF_NAMESPACE
} // namespace
#endif
