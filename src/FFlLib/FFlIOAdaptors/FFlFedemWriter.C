// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include <fstream>

#include "FFlLib/FFlIOAdaptors/FFlFedemWriter.H"
#include "FFlLib/FFlLinkHandler.H"
#include "FFlLib/FFlFEParts/FFlNode.H"
#include "FFlLib/FFlElementBase.H"
#include "FFlLib/FFlLoadBase.H"
#include "FFlLib/FFlAttributeBase.H"
#ifdef FT_USE_VISUALS
#include "FFlLib/FFlVisualBase.H"
#endif
#include "FFlLib/FFlGroup.H"
#include "FFlLib/FFlFieldBase.H"

#include "FFaLib/FFaAlgebra/FFaVec3.H"
#include "FFaLib/FFaDefinitions/FFaMsg.H"


/*!
  \class FFlFedemWriter FFlFedemWriter.H
  \brief Fedem link writer. Writes "new" version of the Fedem Link Model files.

  Format suggestions:
  \code
  # - comment string
  ENTRYNAME{ID primary_data {additional_data - connections}}
  \endcode

  Data: either numerical (integer or float) or attributes with identifiers

  Example:
  \code
  FFT3 { 3 32 22 33 44
           { PTHICK 34 }
           { STRCOATREF 3 T }
           { STRCOATREF 5 B } }
  \endcode
*/

#ifdef FF_NAMESPACE
namespace FF_NAMESPACE {
#endif


bool FFlFedemWriter::write(const std::string& filename,
			   bool writeExtNodes, bool writeChecksum,
			   const std::vector<std::string>& metaData) const
{
  if (!myLink) return false;

  std::ofstream os(filename.c_str(),std::ios::out);
  if (!os)
  {
    ListUI <<" *** Error: Can not open output file "<< filename <<"\n";
    return false;
  }

  if (this->writeMetaData(os,writeExtNodes,writeChecksum,metaData))
    if (this->writeNodeData(os,writeExtNodes))
      if (this->writeElementData(os))
	if (this->writeLoadData(os))
	  if (this->writeGroupData(os))
	    if (this->writeAttributeData(os))
	      if (this->writeVisualData(os))
	      {
		os <<"#\n# End of file\n";
		return true;
	      }

  ListUI <<" *** Error: Failed to write FE data file "<< filename <<"\n";
  return false;
}


bool FFlFedemWriter::writeMetaData(std::ostream& os,
				   bool writeExtNodes, bool writeChecksum,
				   const std::vector<std::string>& metaData) const
{
  os <<"FTLVERSION{7 ASCII}\n";
  for (const std::string& meta : metaData)
    os <<"# "<< meta << std::endl;

  if (writeChecksum)
  {
    // Save current link checksum such that we might detect manual editing
    int csMask = writeExtNodes ? 0 : FFl::CS_NOEXTINFO;
    os <<"# File checksum: "<< myLink->calculateChecksum(csMask) << std::endl;
  }

  return os ? true : false;
}


bool FFlFedemWriter::writeNodeData(std::ostream& os, bool writeExtNodes) const
{
  int OldPrecision = os.precision(10);
  if (myLink->getNodeCount())
    os <<"#\n# Nodal coordinates\n#\n";

  for (NodesCIter nit = myLink->nodesBegin(); nit != myLink->nodesEnd(); ++nit)
  {
    os <<"NODE{"<< (*nit)->getID() <<" ";
    if (writeExtNodes && (*nit)->isExternal())
      os << 1;
    else if ((*nit)->isFixed())
      os << (*nit)->getStatus(-128);
    else
      os << 0;
    os <<" "<< (*nit)->getPos();

    if ((*nit)->hasLocalSystem())
      os <<" {PCOORDSYS "<< (*nit)->getLocalSystemID() <<"}";

    os <<"}"<< std::endl;
  }
  os.precision(OldPrecision);
  return os ? true : false;
}


bool FFlFedemWriter::writeElementData(std::ostream& os) const
{
  if (myLink->getElementCount())
    os <<"#\n# Element definitions\n#\n";

  for (ElementsCIter eit = myLink->elementsBegin(); eit != myLink->elementsEnd(); ++eit)
  {
    FFlElementBase* curElm = *eit;
    os << curElm->getTypeName() <<"{"<< curElm->getID();

    for (NodeCIter it = curElm->nodesBegin(); it != curElm->nodesEnd(); ++it)
      if (it->isResolved())
        os <<" "<< it->getID();
      else
        os <<" UND";

    for (AttribsCIter aIt = curElm->attributesBegin(); aIt != curElm->attributesEnd(); ++aIt)
      if (aIt->second.isResolved())
        os <<" {"<< aIt->second->getTypeName() <<" "<< aIt->second->getID() <<"}";

#ifdef FT_USE_VISUALS
    FFlVisualBase* vapp = curElm->getVisualAppearance();
    if (vapp) os <<" {"<< vapp->getTypeName() <<" "<< vapp->getID() <<"}";

    FFlVisualBase* vdet = curElm->getVisualDetail();
    if (vdet) os <<" {"<< vdet->getTypeName() <<" "<< vdet->getID() <<"}";
#endif

    FFlElementBase* refElm = curElm->getFElement();
    if (refElm) os <<" {FE "<< refElm->getID() <<"}";

    os <<"}"<< std::endl;
  }

  return os ? true : false;
}


bool FFlFedemWriter::writeLoadData(std::ostream& os) const
{
  if (myLink->loadsBegin() != myLink->loadsEnd())
    os <<"#\n# Loads\n#\n";

  for (LoadsCIter lit = myLink->loadsBegin(); lit != myLink->loadsEnd(); ++lit)
  {
    FFlLoadBase* curLoad = *lit;
    os << curLoad->getTypeName() <<"{"<< curLoad->getID();
    int OldPrecision = os.precision(10);
    for (FFlFieldBase* field : *curLoad) os <<" "<< *field;
    os.precision(OldPrecision);

    for (AttribsCIter aIt = curLoad->attributesBegin(); aIt != curLoad->attributesEnd(); ++aIt)
      if (aIt->second.isResolved())
        os <<" {"<< aIt->second->getTypeName() <<" "<< aIt->second->getID() <<"}";

    int tid, face = 0;
    bool firstElm = true;
    while (curLoad->getTarget(tid,face))
      if (face > 0)
        os <<" {TARGET "<< tid <<" "<< face <<"}";
      else if (firstElm)
      {
        firstElm = false;
        os <<" {TARGET "<< tid;
      }
      else
        os <<" "<< tid;

    if (!firstElm) os <<"}";
    os <<"}"<< std::endl;
  }

  return os ? true : false;
}


bool FFlFedemWriter::writeGroupData(std::ostream& os) const
{
  if (myLink->groupsBegin() != myLink->groupsEnd())
    os <<"#\n# Element groups\n#\n";

  for (GroupCIter git = myLink->groupsBegin(); git != myLink->groupsEnd(); ++git)
  {
    FFlGroup* curGroup = git->second;
    os <<"GROUP{"<< curGroup->getID();

    for (const GroupElemRef& elm : *curGroup)
      if (elm.isResolved())
        os <<" "<< elm.getID();
    if (!curGroup->getName().empty())
      os <<" {NAME \""<< curGroup->getName() <<"\"}";
    os <<"}"<< std::endl;
  }

  return os ? true : false;
}


bool FFlFedemWriter::writeAttributeData(std::ostream& os) const
{
  for (AttributeTypeCIter aTypItr = myLink->attributeTypesBegin();
       aTypItr != myLink->attributeTypesEnd();
       aTypItr++)
  {
    if (aTypItr->second.empty()) continue;

    os <<"#\n# "<< aTypItr->second.begin()->second->getDescription() <<"\n#\n";
    for (const AttributeMap::value_type& attp : aTypItr->second)
    {
      FFlAttributeBase* curAttr = attp.second;
      os << curAttr->getTypeName() <<"{"<< curAttr->getID();
      int OldPrecision = os.precision(10);
      for (FFlFieldBase* field : *curAttr) os <<" "<< *field;
      os.precision(OldPrecision);

      for (AttribsCIter aIt = curAttr->attributesBegin(); aIt != curAttr->attributesEnd(); ++aIt)
        if (aIt->second.isResolved())
          os <<" {"<< aIt->second->getTypeName() <<" "<< aIt->second->getID() <<"}";

      if (!curAttr->getName().empty())
        os <<" {NAME \""<< curAttr->getName() <<"\"}";
      os <<"}"<< std::endl;
    }
  }

  return os ? true : false;
}


#ifdef FT_USE_VISUALS
bool FFlFedemWriter::writeVisualData(std::ostream& os) const
{
  if (myLink->visualsBegin() != myLink->visualsEnd())
    os <<"#\n# Visualization properties\n#\n";

  for (VisualsCIter vit = myLink->visualsBegin(); vit != myLink->visualsEnd(); ++vit)
  {
    os << (*vit)->getTypeName() <<"{"<< (*vit)->getID();
    for (FFlFieldBase* field : **vit) os <<" "<< *field;
    os <<"}"<< std::endl;
  }

  return os ? true : false;
}
#else
bool FFlFedemWriter::writeVisualData(std::ostream&) const { return true; }
#endif

#ifdef FF_NAMESPACE
} // namespace
#endif
