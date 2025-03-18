// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

/*!
  \file FFlLinkHandler_F.C
  \brief Fortran wrapper for the FFlLinkHandler methods.

  \details This file contains some global-scope functions callable from Fortran.
  It wraps the functionality needed by the FE Part Reducer and Recovery modules,
  accessing the FE data of the Parts.

  \author Knut Morten Okstad
  \date 20 Sep 2000
*/

#include <algorithm>
#include <cstring>

#include "FFlLib/FFlInit.H"
#include "FFlLib/FFlLinkHandler.H"
#include "FFlLib/FFlFEParts/FFlAllFEParts.H"
#include "FFlLib/FFlIOAdaptors/FFlReaders.H"
#include "FFlLib/FFlIOAdaptors/FFlFedemReader.H"
#include "FFlLib/FFlIOAdaptors/FFlFedemWriter.H"
#include "FFlLib/FFlIOAdaptors/FFlVTFWriter.H"
#include "FFlLib/FFlUtils.H"
#include "FFaLib/FFaAlgebra/FFaTensor3.H"
#include "FFaLib/FFaAlgebra/FFaMat34.H"
#include "FFaLib/FFaAlgebra/FFaCheckSum.H"
#include "FFaLib/FFaCmdLineArg/FFaCmdLineArg.H"
#include "FFaLib/FFaDefinitions/FFaMsg.H"
#include "FFaLib/FFaString/FFaTokenizer.H"
#include "FFaLib/FFaOS/FFaFilePath.H"
#include "FFaLib/FFaOS/FFaFortran.H"
#include "FFaLib/FFaOS/FFaExport.H"

#ifdef FF_NAMESPACE
using namespace FF_NAMESPACE;
#endif

//! \brief Pointers to all FE parts subjected to recovery during dynamics solve
static std::vector<FFlLinkHandler*> ourLinks;
//! \brief Pointer to FE part currently in calculation focus
static FFlLinkHandler* ourLink = NULL;
//! \brief Pointer to FFaCheckSum object for FE part in calculation focus
static FFaCheckSum* chkSum = NULL;

//! \brief Convenience macro for dynamic casting of an element attribute pointer
#define GET_ATTRIBUTE(el,att) dynamic_cast<FFl##att*>((el)->getAttribute(#att))
//! \brief Convenience macro for dynamic casting of a link attribute pointer
#define LINK_ATTRIBUTE(att,ID) \
  ourLink ? dynamic_cast<FFl##att*>(ourLink->getAttribute(#att,ID)) : NULL


////////////////////////////////////////////////////////////////////////////////
//! \brief Initializes the FE part object #ourLink by reading data from file.
//! \details Common function used both by the Reducer and the Recovery modules.
//!
//! \author Jens Lien
//! \date Apr 2003

static int ffl_basic_init (int maxNodes, int maxElms,
                           const std::string& partName = "")
{
  std::string linkFile;
  if (partName.empty())
    FFaCmdLineArg::instance()->getValue("linkfile",linkFile);
  else
    linkFile = partName;

  if (linkFile.empty())
  {
    ListUI <<" *** Error: FE data file must be specified through -linkfile\n";
    return -1;
  }

  if (ourLink && partName.empty())
  {
    std::cerr <<"ffl_init: Logic error, FE part already exists"<< std::endl;
    return -99;
  }

  ourLink = new FFlLinkHandler(maxNodes,maxElms);
  if (!ourLink)
  {
    std::cerr <<"ffl_init: Error allocating FE part object"<< std::endl;
    return -2;
  }

  FFl::initAllReaders();
  FFl::initAllElements();

  // Read the FE data file into the FFlLinkHandler object
  FFaFilePath::checkName(linkFile);
  FFlFedemReader::ignoreCheckSum = !partName.empty();
  if (FFlReaders::instance()->read(linkFile,ourLink) > 0)
  {
    ourLinks.push_back(ourLink);
    return partName.empty() ? 0 : ourLinks.size();
  }

  if (ourLink->isTooLarge())
    ListUI <<" *** Reduction and recovery of FE parts is only a demo feature\n"
	   <<"     with the current license, and therefore limited to small"
	   <<" models only.\n"
	   <<"     To continue with the current model, you may toggle this part"
	   <<"  into a Generic Part before solving.\n";

  delete ourLink;
  ourLink = NULL;
  return -3;
}


////////////////////////////////////////////////////////////////////////////////
// Allocate the FE part object and read FE data file. Then optionally set
// the calculation flag for elements belonging to the user-specified groups.
//
// Coded by: Knut Morten Okstad
// Date/ver: 20 Sep 2000 / 1.0
////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(ffl_limited_init,FFL_LIMITED_INIT) (const int& maxNodes,
					       const int& maxElms,
					       const int& calcflag, int& ierr)
{
  ierr = ffl_basic_init(maxNodes,maxElms);
  if (ierr < 0 || !calcflag) return;

  // Check whether element groups for computation is specified
  // syntax: -group 55                      -> only group 55
  //         -group <33,22,44>              -> groups 33, 22, 44
  //         -group <PMAT 33, PTHICK 55>    -> elements containing references
  //                                           to PMAT 33 and PTHICK 55
  //         -group <22, PMAT 33, 44>       -> group 22, 44 and elements
  //                                           containing references to PMAT 33

  std::string elmGroups;
  FFaCmdLineArg::instance()->getValue("group",elmGroups);
  if (!elmGroups.empty())
    FFl::activateElmGroups(ourLink,elmGroups);
}


////////////////////////////////////////////////////////////////////////////////
// Allocate the FE part object and read FE data file. Then set
// the calculation flag for elements belonging to the user-specified groups.
//
// Coded by: Knut Morten Okstad
// Date/ver: 26 Oct 2015 / 1.0
////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(ffl_full_init,FFL_FULL_INIT) (const char* linkFile,
					 const char* elmGroups,
#ifdef _NCHAR_AFTER_CHARARG
					 const int ncharF, const int ncharG,
					 int& ierr
#else
					 int& ierr,
					 const int ncharF, const int ncharG
#endif
){
  ierr = ffl_basic_init(0,0,std::string(linkFile,ncharF));
  if (ierr >= 0 && elmGroups && ncharG > 0)
    FFl::activateElmGroups(ourLink,std::string(elmGroups,ncharG));
}


////////////////////////////////////////////////////////////////////////////////
// Allocate the FE part object and read FE data file. Then optionally set
// the external nodes based on the command line options. Also export the
// new complete FTL file if specified.
//
// Coded by: Jens Lien
// Date/ver: Apr 2003 / 1.0
////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(ffl_reducer_init,FFL_REDUCER_INIT) (const int& maxNodes,
					       const int& maxElms,
					       int& ierr)
{
  ierr = ourLink ? 0 : ffl_basic_init(maxNodes,maxElms);
  if (ierr < 0) return;

  // add external node reference through the extNodes flag:
  // syntax: -extNodes 10                   -> only node 10
  //         -extNodes <10,20,30>           -> nodes 10,20,30

  std::string extNodes;
  FFaCmdLineArg::instance()->getValue("extNodes",extNodes);

  if (!extNodes.empty())
  {
    std::vector<int> nodeID;
    if (extNodes[0] == '<')
    {
      FFaTokenizer nodes(extNodes,'<','>',',');
      for (const std::string& node : nodes)
        if (isdigit(node[0]))
          nodeID.push_back(atoi(node.c_str()));
    }
    else
      nodeID.push_back(atoi(extNodes.c_str()));

    if (!nodeID.empty())
    {
      FFlNode* theNode;
      for (int nId : nodeID)
        if ((theNode = ourLink->getNode(nId)))
          theNode->setExternal();
        else
          ListUI <<"  ** Warning: Non-existing external node "<< nId
                 <<" (ignored)\n";
    }
  }

  std::string ftlOut;
  FFaCmdLineArg::instance()->getValue("ftlout",ftlOut);
  if (ftlOut.empty()) return;

  FFlFedemWriter writer(ourLink);
  writer.write(ftlOut);
}


////////////////////////////////////////////////////////////////////////////////
// Sets calculation focus on an already open FE part.
//
// Coded by: Knut Morten Okstad
// Date/ver: 26 Oct 2015 / 1.0
////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(ffl_set,FFL_SET) (const int& linkIdx)
{
  if (linkIdx > 0 && linkIdx <= static_cast<int>(ourLinks.size()))
    ourLink = ourLinks[linkIdx-1];
  else
    ourLink = NULL;

  delete chkSum;
  chkSum = NULL;
}


////////////////////////////////////////////////////////////////////////////////
// Release the FE part object(s).
//
// Coded by: Knut Morten Okstad
// Date/ver: 20 Sep 2000 / 1.0
////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(ffl_release,FFL_RELEASE) (const int& removeSingletons)
{
  if (ourLinks.empty())
    delete ourLink;
  else for (FFlLinkHandler* link : ourLinks)
    delete link;
  ourLinks.clear();
  ourLink = NULL;
  delete chkSum;
  chkSum = NULL;
  if (removeSingletons)
  {
    FFl::releaseAllReaders();
    FFl::releaseAllElements();
  }
}


////////////////////////////////////////////////////////////////////////////////
// Export the FE part geometry to the specified VTF file.
// The specified VTF file must already exist.
//
// Coded by: Knut Morten Okstad
// Date/ver: 17 January 2006 / 1.0
////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(ffl_export_vtf,FFL_EXPORT_VTF) (const char* VTFfile,
					   const char* linkName,
#ifdef _NCHAR_AFTER_CHARARG
					   const int ncharF, const int ncharN,
					   const int& linkID, int& ierr
#else
					   const int& linkID, int& ierr,
					   const int ncharF, const int ncharN
#endif
){
  FFlVTFWriter writer(ourLink);
  if (writer.write(std::string(VTFfile,ncharF),
                   std::string(linkName,ncharN),linkID,-1))
    ierr = 0;
  else
    ierr = 1;
}


////////////////////////////////////////////////////////////////////////////////
// Sort the elements in the alphabetic order of the element types.
// This is the order the elements are written out to the VTF file.
//
// Coded by: Knut Morten Okstad
// Date/ver: 17 March 2006 / 1.0
////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(ffl_elmorder_vtf,FFL_ELMORDER_VTF) (int* vtfOrder, int& stat)
{
  stat = 0;
  if (!ourLink) return;

  using ElmType = std::pair<std::string,size_t>;

  std::vector<ElmType> elmTypeVec;
  elmTypeVec.reserve(ourLink->getElementCount(FFlLinkHandler::FFL_FEM));

  // Establish a vector of element type element number pairs
  ElementsCIter e;
  size_t iel = 0;
  for (e = ourLink->fElementsBegin(); e != ourLink->fElementsEnd(); ++e)
    elmTypeVec.push_back(ElmType((*e)->getTypeName(),++iel));

  // Use stable_sort to avoid that elements of the same type change order
  std::stable_sort(elmTypeVec.begin(),elmTypeVec.end(),
                   [](const ElmType& lhs, const ElmType& rhs)
                   { return lhs.first < rhs.first; });

  // Check if any elements actually have changed order
  vtfOrder[0] = elmTypeVec.front().second;
  for (iel = 1; iel < elmTypeVec.size(); iel++)
    if ((vtfOrder[iel] = elmTypeVec[iel].second) != vtfOrder[iel-1]+1)
      stat = iel;
}


////////////////////////////////////////////////////////////////////////////////
// Compute global mass properties for the FE part.
//
// Coded by: Knut Morten Okstad
// Date/ver: 27 Jun 2005 / 1.0
////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(ffl_massprop,FFL_MASSPROP) (double& mass, double* cg, double* II)
{
  if (!ourLink) return;

  FaVec3 Xcg;
  FFaTensor3 Icg;
  ourLink->getMassProperties(mass,Xcg,Icg);
  memcpy(cg,Xcg.getPt(),3*sizeof(double));
  memcpy(II,Icg.getPt(),6*sizeof(double));
}


////////////////////////////////////////////////////////////////////////////////
// Return some model size parameters.
//
// Coded by: Knut Morten Okstad
// Date/ver: 20 Sep 2000 / 1.0
////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(ffl_getsize,FFL_GETSIZE) (int& nnod, int& nel, int& ndof, int& nmnpc,
                                     int& nmat, int& nxnod, int& npbeam,
                                     int& nrgd, int& nrbar, int& nwavgm,
                                     int& nprop, int& ncons, int& ierr)
{
  if (!ourLink)
  {
    std::cerr <<"ffl_getsize: FE part object not initialized"<< std::endl;
    ierr = -1;
    return;
  }

  nnod = ndof = nmnpc = nxnod = npbeam = nrgd = nrbar = nwavgm = 0;

  NodesCIter nit;
  for (nit = ourLink->nodesBegin(); nit != ourLink->nodesEnd(); ++nit)
    if ((*nit)->hasDOFs()) // Skip all loose nodes
    {
      nnod ++;
      ndof += (*nit)->getMaxDOFs();
    }

  ierr = nel = ourLink->buildFiniteElementVec();
  if (ierr < 0) return;

  ElementsCIter eit;
  std::string curTyp;
  for (eit = ourLink->fElementsBegin(); eit != ourLink->fElementsEnd(); ++eit)
  {
    nmnpc += (*eit)->getNodeCount();
    curTyp = (*eit)->getTypeName();
    if (curTyp == "BEAM2")
    {
      // Add extra nodes for beams with pin flags
      FFlPBEAMPIN* myPin = GET_ATTRIBUTE(*eit,PBEAMPIN);
      if (myPin)
      {
        npbeam++;
        if (myPin->PA.getValue() > 0) nxnod++;
        if (myPin->PB.getValue() > 0) nxnod++;
      }
    }
    else if (curTyp == "RGD")   nrgd++;
    else if (curTyp == "RBAR")  nrbar++;
    else if (curTyp == "WAVGM") nwavgm++;
  }

  nnod += nxnod;
  ndof += nxnod*6;
  ncons = nrgd + nrbar + nwavgm;
  nmat  = ourLink->getAttributeCount("PMAT");
  nprop = ourLink->getAttributeCount("PTHICK")
    +     ourLink->getAttributeCount("PBEAMSECTION")
    +     ourLink->getAttributeCount("PNSM");

  // Count the number of activated finite elements,
  // should be equal to nel if no element group specified
  ierr = ourLink->getElementCount(FFlLinkHandler::FFL_ALL,true);
}


////////////////////////////////////////////////////////////////////////////////
// Establish global nodal arrays needed by SAM.
//
// Coded by: Knut Morten Okstad
// Date/ver: 20 Sep 2000 / 1.0
////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(ffl_getnodes,FFL_GETNODES) (int& nnod, int& ndof, int* madof,
                                       int* minex, int* mnode, int* msc,
                                       double* X, double* Y, double* Z,
                                       int& ierr)
{
  if (!ourLink)
  {
    std::cerr <<"ffl_getnodes: FE part object not initialized"<< std::endl;
    ierr = -1;
    return;
  }

  NodesCIter nit;
  int        inod, maxDOFs;

  ierr = inod = ndof = 0;
  madof[0] = 1;

  for (nit = ourLink->nodesBegin(); nit != ourLink->nodesEnd(); ++nit)
  {
    maxDOFs = (*nit)->getMaxDOFs();
    if (maxDOFs == 3 || maxDOFs == 6)
    {
      const FaVec3& pos = (*nit)->getPos();
      minex[inod] = (*nit)->getID();
      X[inod] = pos.x();
      Y[inod] = pos.y();
      Z[inod] = pos.z();

      mnode[inod] = (*nit)->isExternal() ? 2 : 1;
      for (int i = 0; i < maxDOFs; i++)
        msc[ndof+i] = (*nit)->isFixed(i+1) ? 0 : mnode[inod];
      madof[inod+1] = madof[inod] + maxDOFs;
      ndof += maxDOFs;
      inod ++;
    }
    else if (maxDOFs > 0)
    {
      ierr--;
      ListUI <<" *** Error: Invalid DOFs for node "<< (*nit)->getID()
             <<" : "<< (*nit)->getMaxDOFs() <<"\n";
    }
#if FFL_DEBUG > 1
    else
      std::cout <<"ffl_getnodes: Ignoring loose node "<< (*nit)->getID()
                << std::endl;
#endif
  }

  if (inod >= nnod) return;

  // Lambda function converting a beam pin flag into DOF status codes.
  auto&& resolvePinFlag = [](int pinFlag, int* msc)
  {
    if (pinFlag <= 0) return 0;

    int nndof = 6;
    while (pinFlag > 0)
    {
      int ldof = pinFlag%10;
      pinFlag /= 10;
      while (nndof > ldof)
        msc[--nndof] = 0;
      msc[--nndof] = 1;
    }
    while (nndof > 0)
      msc[--nndof] = 0;
    return 6;
  };

  ElementsCIter eit;
  for (eit = ourLink->fElementsBegin(); eit != ourLink->fElementsEnd(); ++eit)
    if ((*eit)->getTypeName() == "BEAM2")
    {
      // Add extra nodes for beams with pin flags
      FFlPBEAMPIN* myPin = GET_ATTRIBUTE(*eit,PBEAMPIN);
      if (myPin)
      {
        NodeCIter nit = (*eit)->nodesBegin();
        maxDOFs = resolvePinFlag(myPin->PA.getValue(),msc+ndof);
        if (maxDOFs > 0)
        {
          const FaVec3& pos = (*nit)->getPos();
          minex[inod] = -inod-1;
          X[inod] = pos.x();
          Y[inod] = pos.y();
          Z[inod] = pos.z();
          mnode[inod] = 1;
          madof[inod+1] = madof[inod] + maxDOFs;
          ndof += maxDOFs;
          inod ++;
        }
        ++nit;
        maxDOFs = resolvePinFlag(myPin->PB.getValue(),msc+ndof);
        if (maxDOFs > 0)
        {
          const FaVec3& pos = (*nit)->getPos();
          minex[inod] = -inod-1;
          X[inod] = pos.x();
          Y[inod] = pos.y();
          Z[inod] = pos.z();
          mnode[inod] = 1;
          madof[inod+1] = madof[inod] + maxDOFs;
          ndof += maxDOFs;
          inod ++;
        }
      }
    }
}


////////////////////////////////////////////////////////////////////////////////
// Establish element type and global topology arrays needed by SAM.
//
// Coded by: Knut Morten Okstad
// Date/ver: 20 Sep 2000 / 1.0
////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(ffl_gettopol,FFL_GETTOPOL) (int& nel, int& nmnpc,
                                       int* mekn, int* mmnpc, int* mpmnpc,
                                       int* mpbeam, int* mprgd, int* mprbar,
                                       int* mpwavgm, int& ierr)
{
  if (!ourLink)
  {
    std::cerr <<"ffl_gettopol: FE part object not initialized"<< std::endl;
    ierr = -1;
    return;
  }

  // Mapping from FTL to SAM element types

  static std::map<std::string,int> typeMap;
  if (typeMap.empty())
  {
    typeMap["BEAM2"]   = 11;
    typeMap["TRI3"]    = 21;
    typeMap["QUAD4"]   = 22;
    typeMap["TRI6"]    = 31;
    typeMap["QUAD8"]   = 32;
    typeMap["TET10"]   = 41;
    typeMap["WEDG15"]  = 42;
    typeMap["HEX20"]   = 43;
    typeMap["HEX8"]    = 44;
    typeMap["TET4"]    = 45;
    typeMap["WEDG6"]   = 46;
    typeMap["CMASS"]   = 51;
    typeMap["RGD"]     = 61;
    typeMap["RBAR"]    = 62;
    typeMap["WAVGM"]   = 63;
    typeMap["SPRING"]  = 71;
    typeMap["RSPRING"] = 71;
    typeMap["BUSH"]    = 72;
  }

  bool useANDES = false;
  FFaCmdLineArg::instance()->getValue("useANDESformulation",useANDES);

  ElementsCIter eit;
  NodeCIter     nit;
  std::map<std::string,int>::const_iterator tit;
  int inod, npbeam, nrgd, nrbar, nwavgm;

  ierr = nel = nmnpc = npbeam = nrbar = nrgd = nwavgm = 0;
  mpmnpc[0] = 1;

  for (eit = ourLink->fElementsBegin(); eit != ourLink->fElementsEnd(); ++eit)
  {
    // Find the SAM element type number
    tit = typeMap.find((*eit)->getTypeName());
    mekn[nel] = tit == typeMap.end() ? 0 : tit->second;

    // Build pointer arrays for elements resulting in constraint equations.
    // Switch the element type for shells if the ANDES formulation is used.
    // Switch the element type number for property-less CMASS and BUSH elements.
    switch (mekn[nel])
      {
      case 11:
	if ((*eit)->getAttribute("PBEAMPIN")) mpbeam[npbeam++] = nel+1;
	break;

      case 21:
      case 22:
	if (useANDES) mekn[nel] += 2;
	break;

      case 61:
	mprgd[nrgd++] = nel+1;
	break;

      case 62:
	mprbar[nrbar++] = nel+1;
	break;

      case 63:
	mpwavgm[nwavgm++] = nel+1;
	break;

      case 51:
	if (!(*eit)->getAttribute("PMASS")) mekn[nel] = 50;
	break;

      case 72:
	if (!(*eit)->getAttribute("PBUSHCOEFF")) mekn[nel] = 70;
	break;
    }

    // Find the nodal point correspondance
    for (nit = (*eit)->nodesBegin(); nit != (*eit)->nodesEnd(); ++nit)
      if ((inod = ourLink->getIntNodeID((*nit)->getID())) > 0)
        mmnpc[nmnpc++] = inod;
      else if (inod < 0)
        ListUI <<"  ** Warning : DOF-less node "<< (*nit)->getID()
               <<" referenced by "<< (*eit)->getTypeName()
               <<" element "<< (*eit)->getID()
               <<" is removed from the topology definition of this element.\n";

    if (mekn[nel] == 31)
    {
      // Reorder the nodes such that the 3 mid-side nodes are ordered last
      // 1-2-3-4-5-6 --> 1-3-5-2-4-6
      std::swap(mmnpc[nmnpc-5],mmnpc[nmnpc-4]);
      std::swap(mmnpc[nmnpc-4],mmnpc[nmnpc-2]);
      std::swap(mmnpc[nmnpc-3],mmnpc[nmnpc-2]);
    }

    mpmnpc[++nel] = nmnpc+1;
  }
}


////////////////////////////////////////////////////////////////////////////////
//! \brief Auxiliary function returning a pointer to and indexed element object.
//!
//! \author Knut Morten Okstad
//! \date 18 Sep 2002

static FFlElementBase* ffl_getElement (int iel)
{
  FFlElementBase* curElm = NULL;
  if (!ourLink)
    std::cerr <<"ffl_getElement: Internal error, ourLink is NULL"<< std::endl;
  else if (!(curElm = ourLink->getFiniteElement(iel)))
    ListUI <<" *** Error: Invalid element index "<< iel <<", out of range [1,"
           << ourLink->getElementCount(FFlLinkHandler::FFL_FEM) <<"]\n";

  return curElm;
}


////////////////////////////////////////////////////////////////////////////////
// Get external ID for a given internal element number.
// Negative if post-processing calculations should be skipped for this element.
//
// Coded by: Knut Morten Okstad
// Date/ver: 20 Oct 2000 / 1.0
////////////////////////////////////////////////////////////////////////////////

INTEGER_FUNCTION(ffl_getelmid,FFL_GETELMID) (const int& iel)
{
  FFlElementBase* curElm = ffl_getElement(iel);
  if (!curElm) return 0;

  int Id = curElm->getID();
  return curElm->doCalculations() ? Id : -Id;
}


////////////////////////////////////////////////////////////////////////////////
// Return the internal node/element number for the given external ID.
//
// Coded by: Knut Morten Okstad
// Date/ver: 14 Nov 2005 / 2.0
////////////////////////////////////////////////////////////////////////////////

INTEGER_FUNCTION(ffl_ext2int,FFL_EXT2INT) (const int& nodeID, const int& ID)
{
  if (ID == 0) return 0; // silently ignore zero external ID

  int intID = -1;
  if (!ourLink)
    std::cerr <<"ffl_ext2int: Internal error, ourLink is NULL"<< std::endl;
  else if (ID < 0)
    intID = 0;
  else if (nodeID)
    intID = ourLink->getIntNodeID(ID);
  else
    intID = ourLink->getIntElementID(ID);

  if (intID <= 0)
    ListUI <<" *** Error: Non-existing "<< (nodeID ? "node" : "element")
           <<" ID: "<< ID <<"\n";

  return intID;
}


////////////////////////////////////////////////////////////////////////////////
// Get global nodal coordinates for an element.
//
// Coded by: Knut Morten Okstad
// Date/ver: 20 Sep 2000 / 1.0
////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(ffl_getcoor,FFL_GETCOOR) (double* X, double* Y, double* Z,
                                     const int& iel, int& ierr)
{
  FFlElementBase* curElm = ffl_getElement(iel);
  ierr = curElm ? curElm->getNodalCoor(X,Y,Z) : -1;
}


////////////////////////////////////////////////////////////////////////////////
// Get material data for an element.
//
// Coded by: Knut Morten Okstad
// Date/ver: 20 Sep 2000 / 1.0
////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(ffl_getmat,FFL_GETMAT) (double& E, double& nu, double& rho,
                                   const int& iel, int& ierr)
{
  ierr = -1;
  FFlElementBase* curElm = ffl_getElement(iel);
  if (!curElm) return;

  FFlPMAT* curMat = GET_ATTRIBUTE(curElm,PMAT);
  if (!curMat)
  {
    ListUI <<" *** Error: No material attached to element "
           << curElm->getID() <<"\n";
    ierr = -2;
    return;
  }

  E   = curMat->youngsModule.getValue();
  nu  = curMat->poissonsRatio.getValue();
  rho = curMat->materialDensity.getValue();
  if (nu >= 0.0 && nu < 0.5)
    ierr = 0;
  else
  {
    ierr = -3;
    ListUI <<" *** Error: Poisson's ratio = "<< nu <<" in Material "
           << curMat->getID() <<". This is outside the valid range [0,0.5>.\n";
  }
}


////////////////////////////////////////////////////////////////////////////////
// Check if an element has a certain attribute
////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(ffl_attribute,FFL_ATTRIBUTE) (const char* type,
#ifdef _NCHAR_AFTER_CHARARG
                                         const int nchar,
                                         const int& iel, int& status
#else
                                         const int& iel, int& status,
                                         const int nchar
#endif
){
  FFlElementBase* curElm = ffl_getElement(iel);
  if (!curElm) // non-existing element
    status = -1;
  else if (status) // get the ID of this attribute, if the element has it
    status = curElm->getAttributeID(std::string(type,nchar));
  else // check whether the element has this attribute
    status = curElm->getAttribute(std::string(type,nchar)) ? 1 : 0;
}


////////////////////////////////////////////////////////////////////////////////
// Get composite data
////////////////////////////////////////////////////////////////////////////////

INTEGER_FUNCTION(ffl_getmaxcompositeplys,FFL_GETMAXCOMPOSITEPLYS) ()
{
  int maxPlys = -1;
  if (ourLink)
    for (const AttributeMap::value_type& p : ourLink->getAttributes("PCOMP"))
    {
      int nPlys = static_cast<FFlPCOMP*>(p.second)->plySet.data().size();
      if (nPlys > maxPlys) maxPlys = nPlys;
    }
  return maxPlys;
}

INTEGER_FUNCTION(ffl_getpcompnumplys,FFL_GETPCOMPNUMPLYS) (const int& compID)
{
  FFlPCOMP* curComp = LINK_ATTRIBUTE(PCOMP,compID);
  if (!curComp)
  {
    ListUI <<" *** Error: No PCOMP with ID "<< compID <<"\n";
    return -1;
  }

  return curComp->plySet.data().size();
}

SUBROUTINE(ffl_getpcomp,FFL_GETPCOMP) (int& compID, int& nPlys, double& Z0,
                                       double* T, double* theta,
                                       double* E1, double* E2, double* NU12,
                                       double* G12, double* G1Z, double* G2Z,
                                       double* rho, int& ierr)
{
  ierr = -1;
  if (!ourLink) return;
  const AttributeMap& pComps = ourLink->getAttributes("PCOMP");
  if (pComps.empty()) return;

  AttributeMap::const_iterator pit = pComps.begin();
  if (compID < 0 && -compID <= static_cast<int>(pComps.size()))
    std::advance(pit,-compID-1);
  else if ((pit = pComps.find(compID)) == pComps.end())
  {
    ListUI <<" *** Error: No PCOMP with"
           << (compID > 0 ? " ID " : " index ") << abs(compID) <<"\n";
    return;
  }

  FFlPCOMP* curComp = static_cast<FFlPCOMP*>(pit->second);
  compID = curComp->getID();
  Z0     = curComp->Z0.getValue();
  nPlys  = curComp->plySet.data().size();

  size_t        i = 0;
  FFlPMATSHELL* pMatShell;
  FFlPMAT*      pMat;
  ierr = 0;

  for (const FFlPly& ply : curComp->plySet.getValue())
  {
    if ((pMatShell = LINK_ATTRIBUTE(PMATSHELL,ply.MID)))
    {
      E1[i]   = pMatShell->E1.getValue();
      E2[i]   = pMatShell->E2.getValue();
      NU12[i] = pMatShell->NU12.getValue();
      G12[i]  = pMatShell->G12.getValue();
      G1Z[i]  = pMatShell->G1Z.getValue();
      G2Z[i]  = pMatShell->G2Z.getValue();
      rho[i]  = pMatShell->materialDensity.getValue();
    }
    else if ((pMat = LINK_ATTRIBUTE(PMAT,ply.MID)))
    {
      E1[i]   = pMat->youngsModule.getValue();
      E2[i]   = E1[i];
      NU12[i] = pMat->poissonsRatio.getValue();
      G12[i]  = pMat->shearModule.getValue();
      G1Z[i]  = G2Z[i] = G12[i];
      rho[i]  = pMat->materialDensity.getValue();
    }
    else
    {
      ListUI <<" *** Error: No PMATSHELL or PMAT with ID "<< ply.MID <<"\n";
      ierr--;
    }

    theta[i] = ply.theta;
    T[i++]   = ply.T;
  }
}


////////////////////////////////////////////////////////////////////////////////
// Get PMATSHELL data
////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(ffl_getpmatshell,FFL_GETPMATSHELL) (const int& MID,
					       double& E1, double& E2,
					       double& NU12, double& G12,
					       double& G1Z, double& G2Z,
					       double& rho, int& ierr)
{
  FFlPMATSHELL* pMatShell = LINK_ATTRIBUTE(PMATSHELL,MID);
  if (!pMatShell)
  {
    ListUI <<" *** Error: No PMATSHELL with ID "<< MID <<"\n";
    ierr = -1;
    return;
  }

  E1   = pMatShell->E1.getValue();
  E2   = pMatShell->E1.getValue();
  NU12 = pMatShell->NU12.getValue();
  G12  = pMatShell->G12.getValue();
  G1Z  = pMatShell->G1Z.getValue();
  G2Z  = pMatShell->G2Z.getValue();
  rho  = pMatShell->materialDensity.getValue();
  ierr = 0;
}


////////////////////////////////////////////////////////////////////////////////
// Get shell thickness for an element.
//
// Coded by: Knut Morten Okstad
// Date/ver: 20 Sep 2000 / 1.0
////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(ffl_getthick,FFL_GETTHICK) (double* Th, const int& iel, int& ierr)
{
  ierr = -1;
  FFlElementBase* curElm = ffl_getElement(iel);
  if (!curElm) return;

  FFlPTHICK* curProp = GET_ATTRIBUTE(curElm,PTHICK);
  if (!curProp)
  {
    ListUI <<" *** Error: No thickness attached to element "
           << curElm->getID() <<"\n";
    ierr = -2;
    return;
  }

  Th[0] = curProp->thickness.getValue();
  const int nenod = curElm->getNodeCount();
  for (int i = 1; i < nenod; i++) Th[i] = Th[0]; // Only uniform thickness yet
  ierr = 0;
}


////////////////////////////////////////////////////////////////////////////////
// Get the pin flags for a beam element.
//
// Coded by: Knut Morten Okstad
// Date/ver: 11 Sep 2002 / 1.0
////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(ffl_getpinflags,FFL_GETPINFLAGS) (int& pA, int& pB,
                                             const int& iel, int& ierr)
{
  ierr = -1;
  pA = pB = 0;
  FFlElementBase* curElm = ffl_getElement(iel);
  if (!curElm) return;

  FFlPBEAMPIN* curProperty = GET_ATTRIBUTE(curElm,PBEAMPIN);
  if (curProperty)
  {
    pA = curProperty->PA.getValue();
    pB = curProperty->PB.getValue();
  }
  ierr = 0;
}


////////////////////////////////////////////////////////////////////////////////
// Get non-structural mass property for an element.
//
// Coded by: Tommy Stokmo Jorstad
// Date/ver: 16 Aug 2001 / 1.0
////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(ffl_getnsm,FFL_GETNSM) (double& Mass, const int& iel, int& ierr)
{
  ierr = -1;
  Mass = 0.0;
  FFlElementBase* curElm = ffl_getElement(iel);
  if (!curElm) return;

  FFlPNSM* curProperty = GET_ATTRIBUTE(curElm,PNSM);
  if (curProperty)
    Mass = curProperty->NSM.getValue();
  ierr = 0;
}


////////////////////////////////////////////////////////////////////////////////
// Get material and cross section properties for a beam element.
//
// Coded by: Knut Morten Okstad
// Date/ver: 20 Sep 2000 / 1.0
////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(ffl_getbeamsection,FFL_GETBEAMSECTION) (double* section,
						   const int& iel, int& ierr)
{
  ierr = -1;
  FFlElementBase* curElm = ffl_getElement(iel);
  if (!curElm) return;

  FFlPMAT* curMat = GET_ATTRIBUTE(curElm,PMAT);
  if (!curMat)
  {
    ListUI <<" *** Error: No material attached to element "
           << curElm->getID() <<"\n";
    ierr = -2;
    return;
  }

  FFlPBEAMSECTION* curSec = GET_ATTRIBUTE(curElm,PBEAMSECTION);
  if (!curSec)
  {
    ListUI <<" *** Error: No beam section attached to element "
           << curElm->getID() <<"\n";
    ierr = -3;
    return;
  }

  section[0] = curMat->materialDensity.getValue();
  section[1] = curMat->youngsModule.getValue();
  section[2] = curMat->shearModule.getValue();
  section[3] = curSec->crossSectionArea.getValue();

  section[4] = curSec->Iy.getValue(); // Integral of y^2 = Izz
  section[5] = curSec->Iz.getValue(); // Integral of z^2 = Iyy
  section[6] = curSec->It.getValue(); // Torsional constant
  double Ixx = section[4] + section[5]; // Polar area moment
  // Note that if Iy and Iz both are zero, Ix is assumed equal to the torsional
  // constant (It) instead. This is correct for circular cross sections only.
  section[7] = Ixx > 0.0 ? Ixx : section[6];

  // Shear reduction factors: kappa_y = A/As_y, As_y = A/kappa_y = A*Kxy.
  // Note that we invert the shear factors here, since in the FE part data files
  // the quantities As/A are assumed stored, whereas the beam element routine
  // is based on the inverted quantity kappa = A/As (corrected 16-12-2020 kmo).
  section[8] = curSec->Kxy.getValue() > 0.0 ? 1.0/curSec->Kxy.getValue() : 0.0;
  section[9] = curSec->Kxz.getValue() > 0.0 ? 1.0/curSec->Kxz.getValue() : 0.0;

  // Shear center offset (with respect to neutral axis)
  section[10] = curSec->Sy.getValue();
  section[11] = curSec->Sz.getValue();

  // Main axis direction for non-symmetric cross sections
  section[13] = curSec->phi.getValue();

  FFlPEFFLENGTH* curLen = GET_ATTRIBUTE(curElm,PEFFLENGTH);
  section[12] = curLen ? curLen->length.getValue() : 0.0;
  ierr = 0;
}


////////////////////////////////////////////////////////////////////////////////
// Get global coordinates for a node.
//
// Coded by: Knut Morten Okstad
// Date/ver: 12 Oct 2000 / 1.0
////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(ffl_getnodalcoor,FFL_GETNODALCOOR) (double& X, double& Y, double& Z,
                                               const int& inod, int& ierr)
{
  if (!ourLink)
  {
    std::cerr <<"ffl_getnodalcoor: Internal error, ourLink is NULL"<< std::endl;
    ierr = -1;
    return;
  }

  FFlNode* curNode = ourLink->getFENode(inod);
  if (!curNode)
  {
    ListUI <<" *** Error: Invalid node index "<< inod <<", out of range [1,"
           << ourLink->getNodeCount(FFlLinkHandler::FFL_FEM) <<"]\n";
    ierr = -2;
    return;
  }

  const FaVec3& pos = curNode->getPos();
  X = pos.x();
  Y = pos.y();
  Z = pos.z();
  ierr = 0;
}


////////////////////////////////////////////////////////////////////////////////
// Get symmetric 6x6 mass matrix for a concentrated mass element.
//
// Coded by: Knut Morten Okstad
// Date/ver: 13 Nov 2000 / 1.0
////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(ffl_getmass,FFL_GETMASS) (double* em, const int& iel, int& ndof,
				     int& ierr)
{
  ndof = 0;
  ierr = -1;
  FFlElementBase* curElm = ffl_getElement(iel);
  if (!curElm) return;

  FFlPMASS* curMass = GET_ATTRIBUTE(curElm,PMASS);
  if (!curMass)
  {
    ListUI <<" *** Error: No mass matrix attached to element "
           << curElm->getID() <<"\n";
    ierr = -2;
    return;
  }

  ierr = 0;
  memset(em,0,36*sizeof(double));

  const std::vector<double>& Mvec = curMass->M.getValue();
  std::vector<double>::const_iterator mit = Mvec.begin();
  for (int i = 0; i < 6; i++)
    for (int j = 0; j <= i; j++)
    {
      if (mit == Mvec.end()) return;
      em[i+6*j] = *(mit++);
      if (j < i) em[j+6*i] = em[i+6*j];
      ndof = i+1;
    }
}


////////////////////////////////////////////////////////////////////////////////
// Get symmetric 6x6 or 12x12 stiffness matrix for a linear spring element.
//
// Coded by: Knut Morten Okstad
// Date/ver: 8 Mar 2002 / 1.0
////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(ffl_getspring,FFL_GETSPRING) (double* ek, int& nedof,
                                         const int& iel, int& ierr)
{
  ierr = -1;
  FFlElementBase* curElm = ffl_getElement(iel);
  if (!curElm) return;

  FFlPSPRING* spr = GET_ATTRIBUTE(curElm,PSPRING);
  if (!spr)
  {
    ListUI <<" *** Error: No stiffness matrix attached to spring element "
           << curElm->getID() <<"\n";
    ierr = -2;
    return;
  }

  int nndof, nenod = curElm->getNodeCount();
  if (spr->type.getValue() == FFlPSPRING::TRANS_SPRING)
    nndof = 3; // Translational spring; return 6x6 matrix
  else
    nndof = 6; // Rotational spring; return a 12x12 matrix

  ierr = 0;
  nedof = nndof*nenod;
  memset(ek,0,nedof*nedof*sizeof(double));

  int i, j, k = 0;
  for (i = 0; i < nedof; i++)
    for (j = 0; j <= i; j++)
      if (nedof <= 6 || (i%6 > 2 && j%6 > 2))
      {
        ek[i+nedof*j] = spr->K[k++].getValue();
        if (j < i) ek[j+nedof*i] = ek[i+nedof*j];
      }
}


////////////////////////////////////////////////////////////////////////////////
// Get element coordinate system.
//
// Coded by: Knut Morten Okstad
// Date/ver: 1 Oct 2010 / 2.0
////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(ffl_getelcoorsys,FFL_GETELCOORSYS) (double* T, const int& iel,
					       int& ierr)
{
  FFlElementBase* curElm = ffl_getElement(iel);
  if (!curElm)
    ierr = -1;
  else if (!curElm->getLocalSystem(T))
    ierr = -3;
  else
    ierr = 0;
}


////////////////////////////////////////////////////////////////////////////////
// Get stiffness coefficients for a bushing element.
//
// Coded by: Knut Morten Okstad
// Date/ver: 28 Aug 2003 / 1.0
////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(ffl_getbush,FFL_GETBUSH) (double* k, const int& iel, int& ierr)
{
  ierr = -1;
  FFlElementBase* curElm = ffl_getElement(iel);
  if (!curElm) return;

  FFlPBUSHCOEFF* bush = GET_ATTRIBUTE(curElm,PBUSHCOEFF);
  if (!bush)
  {
    ListUI <<" *** Error: No coefficients attached to bushing element "
           << curElm->getID() <<"\n";
    ierr = -2;
    return;
  }

  for (int i = 0; i < 6; i++) k[i] = bush->K[i].getValue();

  ierr = 0;
}


////////////////////////////////////////////////////////////////////////////////
// Get DOF-component definitions for a rigid element.
//
// Coded by: Knut Morten Okstad
// Date/ver: 24 Jul 2002 / 1.0
////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(ffl_getrgddofcomp,FFL_GETRGDDOFCOMP) (int* comp, const int& iel,
						 int& ierr)
{
  ierr = -1;
  FFlElementBase* curElm = ffl_getElement(iel);
  if (!curElm) return;

  if (curElm->getTypeName() == "RGD")
  {
    FFlPRGD* pRGD = GET_ATTRIBUTE(curElm,PRGD);
    if (pRGD)
      comp[0] = pRGD->dependentDofs.getValue();
    else
      comp[0] = 123456; // Default: all DOFs are coupled
  }
  else if (curElm->getTypeName() == "RBAR")
  {
    FFlPRBAR* pRBAR = GET_ATTRIBUTE(curElm,PRBAR);
    if (pRBAR)
    {
      comp[0] = pRBAR->CNA.getValue();
      comp[1] = pRBAR->CNB.getValue();
      comp[2] = pRBAR->CMA.getValue();
      comp[3] = pRBAR->CMB.getValue();
    }
    else
    {
      ListUI <<" *** Error: Missing element property for RBAR element "
             << curElm->getID() <<"\n";
      ierr = -2;
      return;
    }
  }

  ierr = 0;
}


////////////////////////////////////////////////////////////////////////////////
// Get component definitions and weights for a weighted average motion element.
// If iel is -1, only return the size of the largest weight array through refC.
//
// Coded by: Knut Morten Okstad
// Date/ver: 12 Aug 2002 / 1.0
////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(ffl_getwavgm,FFL_GETWAVGM) (int& refC, int* indC, double* weights,
                                       const int& iel, int& ierr)
{
  if (iel < 0 && ourLink)
  {
    int N = refC = ierr = 0;
    for (const AttributeMap::value_type& p : ourLink->getAttributes("PWAVGM"))
      if ((N = static_cast<FFlPWAVGM*>(p.second)->weightMatrix.getValue().size()) > refC)
        refC = N;
    return;
  }

  ierr = -1;
  FFlElementBase* curElm = ffl_getElement(iel);
  if (!curElm) return;

  if (curElm->getTypeName() != std::string("WAVGM"))
  {
    std::cerr <<"ffl_getwavgm: Invalid element type for element "
              << curElm->getID() <<": "<< curElm->getTypeName() << std::endl;
    return;
  }

  FFlPWAVGM* pWAVGM = GET_ATTRIBUTE(curElm,PWAVGM);
  if (pWAVGM)
  {
    ierr = 1;
    refC = pWAVGM->refC.getValue();
    size_t i, N = pWAVGM->weightMatrix.getValue().size();
    for (i = 0; i < 6; i++) indC[i] = pWAVGM->indC[i].getValue();
    for (i = 0; i < N; i++) weights[i] = pWAVGM->weightMatrix.getValue()[i];
  }
  else
    ierr = 0; // No properties given - use default
}


////////////////////////////////////////////////////////////////////////////////
// Get load set identification numbers.
//
// Coded by: Knut Morten Okstad
// Date/ver: 22 Feb 2008 / 1.0
////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(ffl_getloadcases,FFL_GETLOADCASES) (int* loadCases, int& nlc)
{
  if (!ourLink)
  {
    nlc = 0;
    return;
  }

  std::set<int> IDs;
  ourLink->getLoadCases(IDs);
  if (nlc > static_cast<int>(IDs.size()))
    nlc = IDs.size();

  std::set<int>::const_iterator sit = IDs.begin();
  for (int ilc = 0; ilc < nlc; ilc++, ++sit)
    loadCases[ilc] = *sit;
}


////////////////////////////////////////////////////////////////////////////////
// Return number of loads with the given load set id.
//
// Coded by: Knut Morten Okstad
// Date/ver: 2 Apr 2008 / 1.0
////////////////////////////////////////////////////////////////////////////////

INTEGER_FUNCTION(ffl_getnoload,FFL_GETNOLOAD) (const int& SID)
{
  if (!ourLink) return 0;

  int nLoad = 0;
  LoadsVec loads;
  ourLink->getLoads(SID,loads);
  for (FFlLoadBase* load : loads)
    nLoad += load->getTargetCount();

  return nLoad;
}


////////////////////////////////////////////////////////////////////////////////
// Get data for the next load with the given load set id.
//
// Coded by: Knut Morten Okstad
// Date/ver: 22 Feb 2008 / 1.0
////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(ffl_getload,FFL_GETLOAD) (const int& SID, int& iel, int& face,
				     double* P)
{
  if (!ourLink)
  {
    std::cerr <<"ffl_getload: Internal error, ourLink is NULL"<< std::endl;
    iel = face = 0;
    return;
  }

  static LoadsVec loads;
  static LoadsCIter lit;

  if (loads.empty())
  {
    iel = face = 0;
    ourLink->getLoads(SID,loads);
    if (loads.empty()) return;
    lit = loads.begin();
  }

  std::vector<FaVec3> Pglb;
  while (lit != loads.end())
    if ((iel = (*lit)->getLoad(Pglb,face)))
    {
      for (const FaVec3& Pnod : Pglb)
	for (int j = 0; j < 3; j++)
	  *(P++) = Pnod[j];

      int eid = iel;
      if (face < 0)
	iel = ourLink->getIntNodeID(eid);
      else
	iel = ourLink->getIntElementID(eid);
      if (iel > 0) return;

      // This should never happen (implies logic error somewhere)
      std::cerr <<"ffl_getload: Non-existing "
		<< (face < 0 ? "node " : "element ")
		<< eid <<" referenced by "<< (*lit)->getTypeName() <<" "
		<< SID <<" (ignored)"<< std::endl;
    }
    else
    {
      face = 0;
      ++lit;
    }

  loads.clear();
}


#ifdef FT_USE_STRAINCOAT

////////////////////////////////////////////////////////////////////////////////
//! \brief Auxiliary function for retrieval of strain coat attributes.
//!
//! \author Knut Morten Okstad
//! \date 28 May 2001

static void getStrainCoatAttributes (FFlPSTRC* p, FFlPFATIGUE* pFat,
                                     int& resSet, int& id,
                                     double& E, double& nu, double& Z,
                                     int& SNstd, int& SNcurve, double& SCF)
{
  resSet = id = 0;
  E = nu = Z = SCF = 0.0;
  SNstd = SNcurve = -1;
  if (!p) return;

  const std::string& name = p->name.getValue();
  if (name == "Bottom")
    resSet = 1;
  else if (name == "Mid")
    resSet = 2;
  else if (name == "Top")
    resSet = 3;

  FFlPMAT* curMat = GET_ATTRIBUTE(p,PMAT);
  if (curMat)
  {
    id = curMat->getID();
    E  = curMat->youngsModule.getValue();
    nu = curMat->poissonsRatio.getValue();
  }

  FFlPTHICKREF* curTref = NULL;
  FFlPHEIGHT* curHeight = GET_ATTRIBUTE(p,PHEIGHT);
  if (curHeight)
    Z = curHeight->height.getValue();
  else if ((curTref = GET_ATTRIBUTE(p,PTHICKREF)))
  {
    FFlPTHICK* curThk = GET_ATTRIBUTE(curTref,PTHICK);
    if (curThk)
      Z = curThk->thickness.getValue() * curTref->factor.getValue();
  }

  if (pFat)
  {
    SNstd = pFat->snCurveStd.getValue();
    SNcurve = pFat->snCurveIndex.getValue();
    SCF = pFat->stressConcentrationFactor.getValue();
  }
}


////////////////////////////////////////////////////////////////////////////////
// Get data for the next strain coat element.
//
// Coded by: Knut Morten Okstad
// Date/ver: 28 May 2001 / 1.0
////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(ffl_getstraincoat,FFL_GETSTRAINCOAT) (int& Id, int& nnod, int& npts,
                                                 int* nodes, int* matId,
                                                 double* E, double* nu,
                                                 double* Z, int* resSet,
                                                 double* SCF, int* SNcurve,
                                                 int& eId, int& ierr)
{
  if (!ourLink)
  {
    std::cerr <<"ffl_getstraincoat: Internal error, ourLink is NULL"
	      << std::endl;
    ierr = -1;
    return;
  }

  ierr = 0;
  static ElementsCIter eit = ourLink->elementsBegin();
  while (eit != ourLink->elementsEnd())
  {
    FFlElementBase* curElm = *eit;
    ++eit;

    if (FFlLinkHandler::isStrainCoat(curElm) && curElm->doCalculations())
    {
      // Get strain coat data
      Id = curElm->getID();
      nnod = npts = 0;
      for (NodeCIter it = curElm->nodesBegin(); it != curElm->nodesEnd(); ++it)
      {
        if ((nodes[nnod++] = ourLink->getIntNodeID((*it)->getID())) < 0)
        {
          ierr --;
          ListUI <<" *** Error: Non-existing node "<< (*it)->getID()
                 <<" referenced by element "<< curElm->getID() <<"\n";
        }

        // Temporarily skip every second node for the 6- and 8-noded elements
        if (curElm->getNodeCount() > 4) ++it;
      }

      std::vector<FFlAttributeBase*> myAtts = curElm->getAttributes("PSTRC");
      myAtts.push_back(curElm->getAttribute("PFATIGUE"));

      for (size_t i = 0; i < myAtts.size()-1; i++, npts++)
        getStrainCoatAttributes (dynamic_cast<FFlPSTRC*>(myAtts[i]),
                                 dynamic_cast<FFlPFATIGUE*>(myAtts.back()),
                                 resSet[npts], matId[npts],
                                 E[npts], nu[npts], Z[npts],
                                 SNcurve[2*npts], SNcurve[2*npts+1], SCF[npts]);

      // Get reference to underlying finite element
      curElm = curElm->getFElement();
      eId = curElm ? curElm->getID() : 0;
      return;
    }
  }

  // We have reached the end of the element list
  Id = nnod = npts = 0;
  eit = ourLink->elementsBegin();
  ierr = 1;
}


////////////////////////////////////////////////////////////////////////////////
// Return total number of materials for strain coat elements.
//
// Coded by: Knut Morten Okstad
// Date/ver: 28 June 2002 / 1.0
////////////////////////////////////////////////////////////////////////////////

INTEGER_FUNCTION(ffl_getnostrcmat,FFL_GETNOSTRCMAT) ()
{
  if (!ourLink)
  {
    std::cerr <<"ffl_getnostrcmat: FE part object not initialized"<< std::endl;
    return -1;
  }

  FFlAttributeBase*                              curAtt;
  std::vector<FFlElementBase*>::const_iterator   ei;
  std::vector<AttribData>::const_iterator        ai;
  std::map<int,int>                              usedMat;

  for (ei = ourLink->elementsBegin(); ei != ourLink->elementsEnd(); ++ei)
    if (FFlLinkHandler::isStrainCoat(*ei) && (*ei)->doCalculations())
      for (FFlAttributeBase* pstrc : (*ei)->getAttributes("PSTRC"))
        for (ai = pstrc->attributesBegin(); ai != pstrc->attributesEnd(); ++ai)
        {
          curAtt = ai->second.getReference();
          if (curAtt->getTypeName() == "PMAT")
            usedMat[curAtt->getID()] ++;
        }

  int nMat = usedMat.size();
#ifdef FFL_DEBUG
  std::cout <<"ffl_getnostrcmat: Number of strain coat materials = "<< nMat;
  for (const std::pair<const int,int>& imat : usedMat)
    std::cout <<"\n                  Id = "<< imat.first
              <<" : # = "<< imat.second;
  std::cout << std::endl;
#endif
  return nMat;
}


////////////////////////////////////////////////////////////////////////////////
// Return number of strain coat elements for which the calculation flag is set.
//
// Coded by: Knut Morten Okstad
// Date/ver: 20 Sep 2000 / 1.0
////////////////////////////////////////////////////////////////////////////////

INTEGER_FUNCTION(ffl_getnostrc,FFL_GETNOSTRC) ()
{
  if (!ourLink)
  {
    std::cerr <<"ffl_getnostrc: FE part object not initialized"<< std::endl;
    return -1;
  }

  return ourLink->getElementCount(FFlLinkHandler::FFL_STRC,true);
}

#endif


////////////////////////////////////////////////////////////////////////////////
// Calculate the checksum of the loaded FE part.
//
// Coded by: Knut Morten Okstad
// Date/ver: 8 Jan 2001 / 1.0
////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(ffl_calcs,FFL_CALCS) (int& ierr)
{
  if (!ourLink)
  {
    std::cerr <<"ffl_calcs: FE part object not initialized"<< std::endl;
    ierr = -1;
    return;
  }

  if (!chkSum)
    chkSum = new FFaCheckSum();

  if (!chkSum)
  {
    std::cerr <<"ffl_calcs: Error allocating checksum object"<< std::endl;
    ierr = -2;
    return;
  }

  chkSum->reset();
  ourLink->calculateChecksum(chkSum);
  ierr = chkSum->getCurrent();
}


////////////////////////////////////////////////////////////////////////////////
// Add an integer option to the checksum.
//
// Coded by: Knut Morten Okstad
// Date/ver: 8 Jan 2001 / 1.0
////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(ffl_addcs_int,FFL_ADDCS_INT) (int& value)
{
  if (chkSum)
    chkSum->add(value);
}


////////////////////////////////////////////////////////////////////////////////
// Add a double option to the checksum.
//
// Coded by: Knut Morten Okstad
// Date/ver: 8 Jan 2001 / 1.0
////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(ffl_addcs_double,FFL_ADDCS_DOUBLE) (double& value)
{
  if (chkSum)
    chkSum->add(value);
}


////////////////////////////////////////////////////////////////////////////////
// Return the checksum as an integer.
//
// Coded by: Knut Morten Okstad
// Date/ver: 10 Jan 2001 / 1.0
////////////////////////////////////////////////////////////////////////////////

SUBROUTINE(ffl_getcs,FFL_GETCS) (int& cs, int& ierr)
{
  if (chkSum)
    cs = chkSum->getCurrent();
  else
    ierr = -2;
}


//! \cond DO_NOT_DOCUMENT
////////////////////////////////////////////////////////////////////////////////
//! \brief Sets the FE part to use (used by test programs only).
//! \author Knut Morten Okstad
//! \date 21 Feb 2018

void ffl_setLink (FFlLinkHandler* link)
{
  delete ourLink;
  ourLink = link;
}

////////////////////////////////////////////////////////////////////////////////
//! \brief Allocates the FE part object and reads FE data file.
//! \details For unit testing only.
//! \author Knut Morten Okstad
//! \date 28 Feb 2020

DLLexport(int) ffl_loadPart (const std::string& fileName)
{
  return ffl_basic_init(0,0,fileName);
}

//! \endcond
