// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#if defined(win32) || defined(win64)
#pragma warning(disable:4503)
#endif

#include "FFlLib/FFlLinkHandler.H"
#include "FFlLib/FFlElementBase.H"
#include "FFlLib/FFlGroup.H"
#include "FFlLib/FFlVisualization/FFlFaceGenerator.H"
#include "FFlLib/FFlVisualization/FFlVisFace.H"
#include "FFlLib/FFlFEParts/FFlPSTRC.H"
#include "FFlLib/FFlFEParts/FFlPTHICK.H"
#include "FFlLib/FFlFEParts/FFlPTHICKREF.H"
#include "FFlLib/FFlFEParts/FFlPFATIGUE.H"


bool FFlLinkHandler::makeStrainCoat(FFlFaceGenerator* geometry,
				    FFlNamedPartBase* group)
{
  // Find existing strain coat related properties in the current link:
  //       Name                  PMAT                        PTHICKREF                   PHEIGHT            PSTRC
  std::map<std::string, std::map<FFlAttributeBase*, std::map<FFlAttributeBase*, std::map<FFlAttributeBase*, FFlAttributeBase*> > > > strcProps;
  //       PTHICK                      Factor  PTHICKREF
  std::map<FFlAttributeBase*, std::map<double, FFlAttributeBase*> > thickRefProps;

  for (const AttributeTypeMap::value_type& propType : myAttributes)

    if (propType.first == "PSTRC")

      for (const AttributeMap::value_type& attr : propType.second)
      {
	FFlPSTRC* prop = (FFlPSTRC*)attr.second;
	strcProps[prop->name.getValue()]
	  [prop->getAttribute("PMAT")]
	  [prop->getAttribute("PTHICKREF")]
	  [prop->getAttribute("PHEIGHT")] = prop;
      }

    else if (propType.first == "PTHICKREF")

      for (const AttributeMap::value_type& attr : propType.second)
      {
	FFlPTHICKREF* prop = (FFlPTHICKREF*)attr.second;
	thickRefProps[prop->getAttribute("PTHICK")][prop->factor.data()] = prop;
      }

  // Apply the group information to the calculation flag on the elements.
  // If no group is defined, set calculation on for all elements.

  if (group)
  {
    this->initiateCalculationFlag(false);
    this->updateCalculationFlag(group);
  }
  else
    this->initiateCalculationFlag(true);

  FFlGroup* explGroup = dynamic_cast<FFlGroup*>(group);

  // Loop over all faces, create strain coat on surface face if needed

  FaceElemRefVecCIter elmRefIt;
  int startElmID = this->getNewElmID();
  int newElmID = startElmID;

  for (FFlVisFace* face : geometry->getFaces())
  {
    if (!face->isSurfaceFace()) continue; // Internal surface
    if (face->isExpandedFace()) continue; // Expanded outer surface

    // Check whether this surface already has a strain coat element
    // and whether this surface is in the right element group

    bool isInGroup = false;
    FFlElementBase* strCoat = NULL;
    for (elmRefIt = face->elementRefsBegin();
	 elmRefIt != face->elementRefsEnd() && !strCoat;
	 elmRefIt ++)

      if (elmRefIt->myElement->doCalculations())
      {
	isInGroup = true;
	if (isStrainCoat(elmRefIt->myElement))
	  strCoat = elmRefIt->myElement;
      }

    if (!isInGroup) continue; // Surface belongs to wrong element group

    if (strCoat) // A strain coat element already exists
    {
      // Add it to the explicit group (if any), if not already a member of it
      if (explGroup) explGroup->addElement(strCoat,true);
      // Check if it also contains a reference to the underlying finite element
      // (if not, it is probably an older (pre R4.1) model)
      if (strCoat->getFElement()) continue;
    }

    // Find which finite element referenced by this face
    // the strain coat element should be based on

    double maxThickness = 0.0;
    const FFlFaceElemRef* refToLastSolid = NULL;
    const FFlFaceElemRef* refToThickestShell = NULL;
    const FFlElementBase* baseElm = NULL;

    for (elmRefIt = face->elementRefsBegin();
	 elmRefIt != face->elementRefsEnd();
	 elmRefIt ++)
    {
      FFlElementBase* baseElm = elmRefIt->myElement;
      if (baseElm->getCathegory() == FFlTypeInfoSpec::SOLID_ELM)
	refToLastSolid = &(*elmRefIt);
      else if (baseElm->getCathegory() == FFlTypeInfoSpec::SHELL_ELM)
      {
	FFlPTHICK* th = dynamic_cast<FFlPTHICK*>(baseElm->getAttribute("PTHICK"));
	if (th)
        {
	  double thickness = th->thickness.getValue();
	  if (maxThickness < thickness)
          {
	    maxThickness = thickness;
	    refToThickestShell = &(*elmRefIt);
	  }
	}
      }
    }

    if (refToThickestShell)
      baseElm = refToThickestShell->myElement;
    else if (refToLastSolid)
      baseElm = refToLastSolid->myElement;
    else
      continue; // Failure: No proper underlying finite element

    if (strCoat)
    {
      // Add reference to underlying finite element for existing strain coat
      strCoat->setFElement((FFlElementBase*)baseElm);
      continue;
    }

    // Create a strain coat element, set it up, and add it to the link

    switch (face->getNumVertices())
      {
      case 3:
	strCoat = ElementFactory::instance()->create("STRCT3",newElmID);
	break;
      case 4:
	strCoat = ElementFactory::instance()->create("STRCQ4",newElmID);
	break;
      case 6:
	strCoat = ElementFactory::instance()->create("STRCT6",newElmID);
	break;
      case 8:
	strCoat = ElementFactory::instance()->create("STRCQ8",newElmID);
	break;
      }

    if (!strCoat) continue; // Failed to create strain coat element

    // Topology based on underlying finite element

    std::vector<FFlNode*> nodeRefs;
    const FFlFaceElemRef* refToTopBaseElm = refToThickestShell ? refToThickestShell : refToLastSolid;
    if (baseElm->getFaceNodes(nodeRefs,1+refToTopBaseElm->myElementFaceNumber))
      strCoat->setNodes(nodeRefs);
    strCoat->setFElement((FFlElementBase*)baseElm);

    // Make, setup and add properties

    FFlAttributeBase* pthick = refToThickestShell ? baseElm->getAttribute("PTHICK") : NULL;
    FFlAttributeBase* pmat   = baseElm->getAttribute("PMAT");
    FFlPSTRC*         pstrc  = NULL;

    if (pthick)
    {
      // For shell elements, find out if we got matching PTHICKREF properties already

      FFlPTHICKREF* pthickRefTop = NULL;
      FFlPTHICKREF* pthickRefBottom = NULL;
      if (thickRefProps.find(pthick) != thickRefProps.end())
      {
	if (thickRefProps[pthick].find(0.5) != thickRefProps[pthick].end())
	  pthickRefTop = (FFlPTHICKREF*)thickRefProps[pthick][0.5];
	if (thickRefProps[pthick].find(-0.5) != thickRefProps[pthick].end())
	  pthickRefBottom = (FFlPTHICKREF*)thickRefProps[pthick][-0.5];
      }

      if (pthickRefTop)
      {
	// We got a top surface thickref, try to find a matching PSTRC
	if (strcProps.find("Top") != strcProps.end())
	  if (strcProps["Top"].find(pmat) != strcProps["Top"].end())
	    if (strcProps["Top"][pmat].find(pthickRefTop) != strcProps["Top"][pmat].end())
	      if (strcProps["Top"][pmat][pthickRefTop].find(0) != strcProps["Top"][pmat][pthickRefTop].end())
		pstrc = (FFlPSTRC*)strcProps["Top"][pmat][pthickRefTop][0];
      }
      else
      {
	// No top PTHICKREF was found, create one
	pthickRefTop = new FFlPTHICKREF(this->getNewAttribID("PTHICKREF"));
	pthickRefTop->factor = 0.5;
	pthickRefTop->setAttribute(pthick);
	if (this->addAttribute(pthickRefTop))
	  thickRefProps[pthick][0.5] = pthickRefTop;
	else
	  pthickRefTop = NULL;
      }

      if (!pstrc && pthickRefTop)
      {
	// No top PSTRC was found, create one
	pstrc = new FFlPSTRC(this->getNewAttribID("PSTRC"));
	pstrc->name = "Top";
	pstrc->setAttribute(pmat);
	pstrc->setAttribute(pthickRefTop);
	if (this->addAttribute(pstrc))
	  strcProps["Top"][pmat][pthickRefTop][0] = pstrc;
	else
	  pstrc = NULL;
      }

      if (pstrc)
        strCoat->setAttribute(pstrc);
      pstrc = NULL;

      if (pthickRefBottom)
      {
	// We got a bottom surface thickref, try to find a matching PSTRC
	if (strcProps.find("Bottom") != strcProps.end())
	  if (strcProps["Bottom"].find(pmat) != strcProps["Bottom"].end())
	    if (strcProps["Bottom"][pmat].find(pthickRefBottom) != strcProps["Bottom"][pmat].end())
	      if (strcProps["Bottom"][pmat][pthickRefBottom].find(0) != strcProps["Bottom"][pmat][pthickRefBottom].end())
		pstrc = (FFlPSTRC*)strcProps["Bottom"][pmat][pthickRefBottom][0];
      }
      else
      {
	// No bottom PTHICKREF was found, create one
	pthickRefBottom = new FFlPTHICKREF(this->getNewAttribID("PTHICKREF"));
	pthickRefBottom->factor = -0.5;
	pthickRefBottom->setAttribute(pthick);
	if (this->addAttribute(pthickRefBottom))
	  thickRefProps[pthick][-0.5] = pthickRefBottom;
	else
	  pthickRefBottom = NULL;
      }

      if (!pstrc && pthickRefBottom)
      {
	// No bottom PSTRC was found, create one
	pstrc = new FFlPSTRC(this->getNewAttribID("PSTRC"));
	pstrc->name = "Bottom";
	pstrc->setAttribute(pmat);
	pstrc->setAttribute(pthickRefBottom);
	if (this->addAttribute(pstrc))
	  strcProps["Bottom"][pmat][pthickRefBottom][0] = pstrc;
	else
	  pstrc = NULL;
      }

      if (pstrc)
        strCoat->setAttribute(pstrc);
    }
    else
    {
      // For solid elements, find out if we got a matching basic PSTRC property already

      if (strcProps.find("Basic") != strcProps.end())
	if (strcProps["Basic"].find(pmat) != strcProps["Basic"].end())
	  if (strcProps["Basic"][pmat].find(0) != strcProps["Basic"][pmat].end())
	    if (strcProps["Basic"][pmat][0].find(0) != strcProps["Basic"][pmat][0].end())
	      pstrc = (FFlPSTRC*)strcProps["Basic"][pmat][0][0];

      if (!pstrc)
      {
	// No basic PSTRC was found, create one
	pstrc = new FFlPSTRC(this->getNewAttribID("PSTRC"));
	pstrc->name = "Basic";
	pstrc->setAttribute(pmat);
	if (this->addAttribute(pstrc))
	  strcProps["Basic"][pmat][0][0] = pstrc;
	else
	  pstrc = NULL;
      }

      if (pstrc)
        strCoat->setAttribute(pstrc);
    }

    // Add strain coat element to the face reference list
    FFlFaceElemRef ref;
    ref.myElement = strCoat;
    ref.myElementFaceNumber = 0;
    ref.elementFaceNodeOffset = refToTopBaseElm->elementFaceNodeOffset;
    ref.elementAndFaceNormalParallel = refToTopBaseElm->elementAndFaceNormalParallel;

    face->addFaceElemRef(ref);
    face->ref();

    // Add strain coat element to link
    this->addElement(strCoat,false);
    // Add strain coat element to the explicit group (if any)
    if (explGroup) explGroup->addElement(strCoat,true);

    newElmID++;
  }

  return newElmID > startElmID ? true : false;
}


bool FFlLinkHandler::assignFatigueProperty(int stdIndx, int curveIndx, double sCF,
					   FFlNamedPartBase* part)
{
  // Create a fatigue property and add it to the link
  FFlPFATIGUE* pFat = new FFlPFATIGUE(this->getNewAttribID("PFATIGUE"));
  pFat->snCurveStd = stdIndx;
  pFat->snCurveIndex = curveIndx;
  pFat->stressConcentrationFactor = sCF;
  FFlAttributeBase* pfat = this->getAttribute("PFATIGUE",this->addUniqueAttribute(pFat));

  FFlGroup* group = dynamic_cast<FFlGroup*>(part);
  FFlAttributeBase* attr = dynamic_cast<FFlAttributeBase*>(part);

  // Add the property to strain coat elements in the specified group, or to all
  for (FFlElementBase* elm : myElements)
    if (isStrainCoat(elm))
    {
      bool isInGroup = true;
      if (group)
      {
	isInGroup = group->hasElement(elm->getID());
	if (!isInGroup && elm->getFElement())
	  isInGroup = group->hasElement(elm->getFElement()->getID());
      }
      else if (attr)
      {
	isInGroup = elm->hasAttribute(attr);
	if (!isInGroup && elm->getFElement())
	  isInGroup = elm->getFElement()->hasAttribute(attr);
      }

      if (isInGroup)
	if (elm->clearAttribute("PFATIGUE"))
	  elm->setAttribute(pfat);
    }

  return true;
}
