// SPDX-FileCopyrightText: 2023 SAP SE
//
// SPDX-License-Identifier: Apache-2.0
//
// This file is part of FEDEM - https://openfedem.org
////////////////////////////////////////////////////////////////////////////////

#include "Admin/FedemAdmin.H"
#include "FFlLib/FFlIOAdaptors/FFlVdmReader.H"
#include "FFlLib/FFlIOAdaptors/FFlReaders.H"
#include "FFlLib/FFlLinkHandler.H"
#include "FFlLib/FFlElementBase.H"
#include "FFlLib/FFlFEParts/FFlNode.H"
#include "FFlLib/FFlFEParts/FFlPMAT.H"
#include "FFlLib/FFlFEParts/FFlPTHICK.H"
#include "FFlLib/FFlFEParts/FFlPBEAMSECTION.H"
#include "FFlLib/FFlFEParts/FFlPBEAMECCENT.H"
#include "FFlLib/FFlFEParts/FFlPORIENT.H"
#include "FFlLib/FFlFEParts/FFlPWAVGM.H"
#include "FFlLib/FFlFEParts/FFlPRBAR.H"
#include "FFlLib/FFlFEParts/FFlPRGD.H"
#include "FFlLib/FFlFEParts/FFlPMASS.H"

#include "FFaLib/FFaAlgebra/FFaAlgebra.H"
#include "FFaLib/FFaAlgebra/FFaMat33.H"
#include "FFaLib/FFaAlgebra/FFaMath.H"
#include "FFaLib/FFaDefinitions/FFaMsg.H"

#ifdef FFL_TIMER
#include "FFaLib/FFaProfiler/FFaProfiler.H"
#endif

#ifdef FT_HAS_VKI
#include "base/base.h"
#include "vis/vis.h"
#include "vdm/vdm.h"
#endif

#ifdef FFL_TIMER
#define START_TIMER(function) myProfiler->startTimer(function)
#define STOPP_TIMER(function) myProfiler->stopTimer(function)
#else
#define START_TIMER(function)
#define STOPP_TIMER(function)
#endif

#ifdef FF_NAMESPACE
namespace FF_NAMESPACE {
#endif


FFlVdmReader::FFlVdmReader(FFlLinkHandler* aLink) : FFlReaderBase(aLink)
{
#ifdef FFL_TIMER
  myProfiler = new FFaProfiler("VdmReader profiler");
  myProfiler->startTimer("FFlVdmReader");
#endif
#ifdef FT_HAS_VKI
  // Create a data function object
  datafun = vdm_DataFunBegin();
#else
  datafun = NULL;
#endif
  model = NULL;
  nWarnings = 0;
}


FFlVdmReader::~FFlVdmReader()
{
#ifdef FT_HAS_VKI
  if (datafun)
  {
    // Close the library device
    vdm_DataFunClose(datafun);
    vdm_DataFunEnd(datafun);
  }
  if (model)
  {
    // Delete the finite element model
    vis_ModelDelete(model);
    vis_ModelEnd(model);
  }
#endif
#ifdef FFL_TIMER
  myProfiler->stopTimer("FFlVdmReader");
  myProfiler->report();
  delete myProfiler;
#endif
}


void FFlAnsysReader::init()
{
  FFlReaders::instance()->registerReader("ANSYS input file","cdb",
					 FFaDynCB2S(FFlAnsysReader::readerCB,const std::string&,FFlLinkHandler*),
					 FFaDynCB2S(FFlAnsysReader::identifierCB,const std::string&,int&),
					 "ANSYS input file reader v1.0",
					 FedemAdmin::getCopyrightString());

  FFlReaders::instance()->addExtension("ANSYS input file","anf");
}


void FFlAnsysReader::identifierCB(const std::string& fileName, int& isAnsysFile)
{
  const char* keyWords[] = { ":CDWRITE", "N,", "EN,", 0 };
  if (!fileName.empty())
    isAnsysFile = searchKeyword(fileName.c_str(),keyWords);
}


void FFlAnsysReader::readerCB(const std::string& filename, FFlLinkHandler* link)
{
  FFlAnsysReader reader(link);
  if (reader.read(filename.c_str()))
    if (!reader.convert())
      link->deleteGeometry();
}


bool FFlAnsysReader::read(const char* filename)
{
  START_TIMER("read");
#ifdef FT_HAS_VKI

  // Create an ANSYS input file device
  vdm_ANSFil* ansfil = vdm_ANSFilBegin();
  vdm_ANSFilDataFun(ansfil,datafun);

  // Open library device
  vdm_DataFunOpen(datafun,"ANSYS reader",(Vchar*)filename,VDM_ANSYS_INPUT);

  // Load the finite element model
  bool status = loadModel();

  // Close the ANASYS input file device
  vdm_ANSFilEnd(ansfil);
#else
  ListUI <<"\n *** Error: The ANSYS input file reader"
	 <<" is not available in this version."
	 <<"\n            The file "<< filename <<" is not read.\n";
  bool status = false;
#endif
  STOPP_TIMER("read");
  return status;
}


void FFlAbaqusReader::init()
{
  FFlReaders::instance()->registerReader("ABAQUS input file","inp",
					 FFaDynCB2S(FFlAbaqusReader::readerCB,const std::string&,FFlLinkHandler*),
					 FFaDynCB2S(FFlAbaqusReader::identifierCB,const std::string&,int&),
					 "ABAQUS input file reader v1.0",
					 FedemAdmin::getCopyrightString());
}


void FFlAbaqusReader::identifierCB(const std::string& fileName, int& isAbaqusFile)
{
  const char* keyWords[] = { "*NODE", "*ELEMENT", 0 };
  if (!fileName.empty())
    isAbaqusFile = searchKeyword(fileName.c_str(),keyWords);
}


void FFlAbaqusReader::readerCB(const std::string& filename, FFlLinkHandler* link)
{
  FFlAbaqusReader reader(link);
  if (reader.read(filename.c_str()))
    if (!reader.convert())
      link->deleteGeometry();
}


bool FFlAbaqusReader::read(const char* filename)
{
  START_TIMER("read");
#ifdef FT_HAS_VKI

  // Create an ABAQUS input file device
  vdm_ABAFil* abafil = vdm_ABAFilBegin();
  vdm_ABAFilDataFun(abafil,datafun);

  // Open library device
  vdm_DataFunOpen(datafun,"ABAQUS reader",(Vchar*)filename,VDM_ABAQUS_INPUT);

  // Load the finite element model
  bool status = loadModel();

  // Close the ABAQUS input file device
  vdm_ABAFilEnd(abafil);
#else
  ListUI <<"\n *** Error: The ABAQUS input file reader"
	 <<" is not available in this version."
	 <<"\n            The file "<< filename <<" is not read.\n";
  bool status = false;
#endif
  STOPP_TIMER("read");
  return status;
}


bool FFlVdmReader::loadModel()
{
  START_TIMER("loadModel");
#ifdef FT_HAS_VKI

  // Create a model object for the finite element model
  model = vis_ModelBegin();

  // Use a Library Manager object to load the model
  vdm_LMan* lman = vdm_LManBegin();
  vdm_LManSetObject(lman,VDM_DATAFUN,datafun);
#ifdef FFL_DEBUG
  vdm_LManTOC(lman,"*");
  vdm_LManList(lman,"EID.E");
  vdm_LManList(lman,"ELEM.*.E");
  vdm_LManList(lman,"MATL.*");
  vdm_LManList(lman,"PROP.*");
#endif
  vdm_LManLoadModel(lman,model);
  bool status = vdm_LManError(lman) == 0;
  if (!status)
    std::cerr <<" *** Internal error: Unable to load model information."
              << std::endl;

  // Close the model object and library device
  vdm_LManEnd(lman);
  vdm_DataFunClose(datafun);
  vdm_DataFunEnd(datafun);
  datafun = 0;
#else
  bool status = false;
#endif
  STOPP_TIMER("loadModel");
  return status;
}


/*!
  This method is the main engine for the conversion of the loaded FE model
  from the VKI data structure into an FFlLinkHandler object.
*/

bool FFlVdmReader::convert()
{
#ifdef FT_HAS_VKI
  if (!model) return false;
  START_TIMER("convert");

  int i, j;
  nWarnings = 0;
  bool okAdd = true;

  // Nodal permutation arrays for the second-order elements
  //                   1 2 3  4  5  6  7  8 9 10 11 12 13 14 15 16 17 18 19 20
  const int p6[10]  = {1,3,5, 2, 4, 6};
  const int p8[10]  = {1,3,5, 7, 2, 4, 6, 8};
  const int p10[10] = {1,3,5,10, 2, 4, 6, 7,8, 9};
  const int p15[15] = {1,3,5,10,12,14, 2, 4,6, 7, 8, 9,11,13,15};
  const int p20[20] = {1,3,5, 7,13,15,17,19,2, 4, 6, 8, 9,10,11,12,14,16,18,20};

  // Get connect object with nodes and elements
  vis_Connect* connect;
  vis_ModelGetObject(model,VIS_CONNECT,(Vobject**)&connect);
  if (!connect)
  {
    std::cerr <<" *** Internal error: Failed to get connect object."
              << std::endl;
    STOPP_TIMER("convert");
    return false;
  }

  // Get number of nodes and elements
  Vint numnp, numel;
  vis_ConnectNumber(connect,SYS_NODE,&numnp);
  vis_ConnectNumber(connect,SYS_ELEM,&numel);

  // Get nodes: coordinates, user id and displacement coordinate system id
  Vdouble x[3];
  Vint nid, cid;
  for (i = 1; i <= numnp && okAdd; i++)
  {
    vis_ConnectCoordsdv(connect,1,&i,(Vdouble(*)[3])x);
    vis_ConnectNodeAssoc(connect,VIS_USERID,1,&i,&nid);
    vis_ConnectNodeAssoc(connect,VIS_CSYSID,1,&i,&cid);

    okAdd = myLink->addNode(new FFlNode(nid,x[0],x[1],x[2]));
  }

  // Temporary container for unnumbered elements
  std::vector<FFlElementBase*> unNumberedElms;

  // Allocate arrays for internal node ids and user ids
  Vint maxelemnode;
  vis_ConnectMaxElemNode(connect,&maxelemnode);
  Vint* ix = new Vint[maxelemnode];
  Vint* ux = new Vint[maxelemnode];

  // Get property hash tables
  vsy_HashTable* ephash;
  vsy_HashTable* mphash;
  vsy_HashTable* edhash;
  vsy_HashTable* cshash;
  vis_ModelGetHashTable(model,VIS_EPROP,&ephash);
  vis_ModelGetHashTable(model,VIS_MPROP,&mphash);
  vis_ModelGetHashTable(model,VIS_ELEMDAT,&edhash);
  vis_ModelGetHashTable(model,VIS_COORDSYS,&cshash);

  // Property variables
  Vint ntypes, type[EPROP_MAX];
  Vint flag, nval, nloc, dtyp;
  Vint iparams[100];
  Vdouble dparams[100];

  coordSys.clear();
  set<Vint> usedPid, usedMid;

  // Get elements: connectivity, user id, material and property id, etc.
  Vint nix, eid, mid, pid, featype, feaspec, eidmax = 0;
  for (i = 1; i <= numel && okAdd; i++)
  {
    vis_ConnectElemNode(connect,i,&nix,ix);
    vis_ConnectElemAssoc(connect,VIS_USERID,1,&i,&eid);
    vis_ConnectElemAssoc(connect,VIS_PROPID,1,&i,&pid);
    vis_ConnectElemAssoc(connect,VIS_MATLID,1,&i,&mid);
    vis_ConnectElemAssoc(connect,VIS_CSYSID,1,&i,&cid);
    vis_ConnectElemAssoc(connect,VIS_FEATYPE,1,&i,&featype);
    vis_ConnectElemAssoc(connect,VIS_FEASPEC,1,&i,&feaspec);

    // Interpret featype/feaspec (the element type)
    FFlElementBase* newElm = 0;
    switch (featype)
      {
      case SYS_ELEM_SOLID: // Solid element
	switch (nix)
	  {
	  case  4: // 4-noded tetrahedron
	    newElm = ElementFactory::instance()->create("TET4",eid);
	    break;
	  case  6: // 6-noded wedge
	    newElm = ElementFactory::instance()->create("WEDG6",eid);
	    break;
	  case  8: // 8-noded hexahedron
	    newElm = ElementFactory::instance()->create("HEX8",eid);
	    break;
	  case 10: // 10-noded tetrahedron
	    newElm = ElementFactory::instance()->create("TET10",eid);
	    break;
	  case 15: // 15-noded wedge
	    newElm = ElementFactory::instance()->create("WEDG15",eid);
	    break;
	  case 20: // 20-noded hexahedron
	    newElm = ElementFactory::instance()->create("HEX20",eid);
	    break;
	  }
	break;

      case SYS_ELEM_SHELL: // Shell element
	switch (nix)
	  {
	  case 3: // 3-noded triangle
	    newElm = ElementFactory::instance()->create("TRI3",eid);
	    break;
	  case 4: // 4-noded shell
	    newElm = ElementFactory::instance()->create("QUAD4",eid);
	    break;
	  case 6: // 6-noded triangle
	    newElm = ElementFactory::instance()->create("TRI6",eid);
	    break;
	  case 8: // 8-noded shell
	    newElm = ElementFactory::instance()->create("QUAD8",eid);
	    break;
	  }
	break;

      case SYS_ELEM_BEAM: // Beam element
	switch (nix)
	  {
	  case 2: // 2-noded beam
	    newElm = ElementFactory::instance()->create("BEAM2",eid);
	    break;
	  }
	break;
/* not yet
      case SYS_ELEM_SPRINGDASHPOT: // Spring dashpot element
	switch (feaspec)
	  {
	  case SYS_SPRINGDASHPOT_SCALAR:
	  case SYS_SPRINGDASHPOT_LINK:
	    newElm = ElementFactory::instance()->create("SPRING",eid);
	    break;
	  case SYS_SPRINGDASHPOT_BUSH:
	    newElm = ElementFactory::instance()->create("BUSH",eid);
	    break;
	  }
	break;
*/
      case SYS_ELEM_RIGID: // Rigid element
	switch (feaspec)
	  {
	  case SYS_RIGID_KINE:
	    newElm = ElementFactory::instance()->create("RGD",eid);
	    break;
	  case SYS_RIGID_DIST:
	  case SYS_RIGID_RBE3:
	    newElm = ElementFactory::instance()->create("WAVGM",eid);
	    break;
	  }
	break;

      case SYS_ELEM_MASS: // Concentrated mass
	switch (feaspec)
	  {
	  case SYS_MASS_LUMP:
	  case SYS_MASS_MATRIX:
	    newElm = ElementFactory::instance()->create("CMASS",eid);
	    break;
	  }
	break;
      }

    if (!newElm)
    {
      if (++nWarnings == 1) ListUI <<"\n";
      ListUI <<"  ** Warning: Unsupported element: "<< i
	     <<" Id="<< eid <<" Type="<< featype <<" Spec="<< feaspec
	     <<" Nodes="<< nix <<" (ignored)\n";
      continue;
    }

    if (eid > eidmax) eidmax = eid; // find highest element number

    // Convert internal node indices to user id
    vis_ConnectNodeAssoc(connect,VIS_USERID,nix,ix,ux);

    // Permute to Fedem order for the second order elements
    std::vector<int> nodeRefs(nix,0);
    if (newElm->getTypeName() == "TRI6")
      for (j = 0; j < 6; j++)
	nodeRefs[p6[j]-1] = ux[j];
    else if (newElm->getTypeName() == "QUAD8")
      for (j = 0; j < 8; j++)
	nodeRefs[p8[j]-1] = ux[j];
    else if (newElm->getTypeName() == "TET10")
      for (j = 0; j < 10; j++)
	nodeRefs[p10[j]-1] = ux[j];
    else if (newElm->getTypeName() == "WEDG15")
      for (j = 0; j < 15; j++)
	nodeRefs[p15[j]-1] = ux[j];
    else if (newElm->getTypeName() == "HEX20")
      for (j = 0; j < 20; j++)
	nodeRefs[p20[j]-1] = ux[j];
    else
      for (j = 0; j < nix; j++)
	nodeRefs[j] = ux[j];

    // Assign node numbers to the element
    newElm->setNodes(nodeRefs);

    if (cid > 0 && coordSys.find(cid) == coordSys.end())
    {
      // Get element coordinate system
      vis_CoordSys* coordsys;
      vsy_HashTableLookup(cshash,cid,(Vobject**)&coordsys);
      if (!coordsys)
      {
	if (++nWarnings == 1) ListUI <<"\n";
	ListUI <<"  ** Warning: Element "<< i <<" Id="<< eid
	       <<" is referring to a non-existing coordinate system,"
	       <<" Id="<< cid <<".\n";
      }
      else
      {
	Vdouble tm[3][3];
	vis_CoordSysInq(coordsys,&dtyp);
	switch (dtyp)
	  {
	  case SYS_CARTESIAN:
	    vis_CoordSysOriginTriaddv(coordsys,x,tm);
	    coordSys[cid] = FaMat34(FaVec3(tm[0][0],tm[0][1],tm[0][2]),
				    FaVec3(tm[1][0],tm[1][1],tm[1][2]),
				    FaVec3(tm[2][0],tm[2][1],tm[2][2]),
				    FaVec3(x[0],x[1],x[2]));
	  default:
	    if (++nWarnings == 1) ListUI <<"\n";
	    ListUI <<"  ** Warning: Unsupported coordinate system type: Id="
		   << cid <<" Type="<< dtyp <<" (ignored).\n";
	  }
      }
    }

    if (pid > 0)
    {
      // Get element properties for this element
      Vint eptype = -1;
      vis_EProp* eprop;
      vsy_HashTableLookup(ephash,pid,(Vobject**)&eprop);
      if (!eprop)
      {
	if (++nWarnings == 1) ListUI <<"\n";
	ListUI <<"  ** Warning: Element "<< i <<" Id="<< eid
	       <<" is referring to a non-existing property, Id="<< pid <<".\n";
      }
      else
      {
	// Interpret the property type (should be the same as the element type)
	bool ignoreElm = false;
	FFlAttributeBase* newAtt = 0;
	vis_EPropInq(eprop,&eptype);
	if (eptype != featype)
	{
	  if (++nWarnings == 1) ListUI <<"\n";
	  ListUI <<"  ** Warning: Element "<< i <<" Id="<< eid
		 <<" Type="<< featype <<" is referring to property "<< pid
		 <<" of invalid type "<< eptype <<".\n";
	}
	else switch (eptype)
	  {
	  case SYS_ELEM_SHELL:
	    // Assign a shell thickness property to the element
	    usedPid.insert(pid);
	    newElm->setAttribute("PTHICK",pid);
	    break;

	  case SYS_ELEM_BEAM:
	    // Assign a beam section property to the element
	    usedPid.insert(pid);
	    newElm->setAttribute("PBEAMSECTION",pid);
	    break;

	  case SYS_ELEM_RIGID:
	    // Get the actual rigid property here since it also
	    // depends on the number of element nodes
	    switch (feaspec)
	      {
	      case SYS_RIGID_KINE:
		newAtt = getRGDAttribute(eprop,pid,nix,ignoreElm);
		break;

	      case SYS_RIGID_DIST:
	      case SYS_RIGID_RBE3:
		newAtt = getWAVGMAttribute(eprop,pid,nix,ignoreElm);
		break;
	    }

	    if (ignoreElm)
	    {
	      // Invalid property specification - ignore this element
	      delete newElm;
	      ListUI <<"element "<< i <<" Id="<< eid
		     <<".\n              This element is ignored.\n";
	    }
	    else if (newAtt)
	    {
	      if (newAtt->getTypeName() == "PRBAR")
	      {
		// Must switch element type to RBAR
		delete newElm;
		newElm = ElementFactory::instance()->create("RBAR",eid);
		newElm->setNodes(nodeRefs);
	      }

	      // Add property to link handler and assign it to current element
	      if (myLink->addAttribute(newAtt,true))
		newElm->setAttribute(newAtt);
	      else
	      {
		// This rigid attribute has been defined previously
		if (newElm->getTypeName() == "RBAR")
		  newElm->setAttribute("PRBAR",pid);
		else if (newElm->getTypeName() == "RGD")
		  newElm->setAttribute("PRGD",pid);
		else
		  newElm->setAttribute("PWAVGM",pid);
	      }
	    }
	    mid = -1;
	    break;

	  case SYS_ELEM_MASS:
	    // Get the actual mass property here since it also
	    // may depend on some other element properties
	    newAtt = getMassAttribute(eprop,pid,cid);
	    if (!newAtt)
	    {
	      delete newElm;
	      ListUI <<"  ** Warning: Invalid property, Id="<< pid
		     <<", for mass element "<< i <<" Id="<< eid
		     <<".\n              This element is ignored.\n";
	    }

	    // Add property to link handler and assign it to current element
	    else if (myLink->addAttribute(newAtt,true))
	      newElm->setAttribute(newAtt);
	    else // This mass attribute has been defined previously
	      newElm->setAttribute("PMASS",pid);
	    mid = -1;
	    break;
	  }

	if (!newElm) continue;

	if (mid == 0)
	{
	  // Get the material property ID referred to by this element property
	  vis_EPropValueType(eprop,&ntypes,type);
	  for (j = 0; j < ntypes; j++)
	  {
	    if (type[j] != EPROP_MID) continue;

	    vis_EPropValueFlag(eprop,type[j],&flag);
	    if (flag == EPROP_UNDEFINED) continue;

	    vis_EPropValueParams(eprop,type[j],&nval,&nloc,&dtyp);
	    if (nval*nloc > sizeof(iparams)) continue;
	    if (dtyp != SYS_INTEGER) continue;

	    vis_EPropValueInteger(eprop,type[j],iparams);
	    mid = iparams[0];
	  }
	}
      }
    }

    if (mid > 0 && mphash)
    {
      vis_MProp* mprop;
      vsy_HashTableLookup(mphash,mid,(Vobject**)&mprop);
      if (!mprop)
      {
	if (++nWarnings == 1) ListUI <<"\n";
	ListUI <<"  ** Warning: Element "<< i <<" Id="<< eid
	       <<" is referring to a non-existing material, Id="<< mid <<".\n";
      }
      else
      {
	// Assign material property to the element
	usedMid.insert(mid);
	newElm->setAttribute("PMAT",mid);
      }
    }

    // Add element to link handler
    if (eid > 0)
      okAdd = myLink->addElement(newElm,false);
    else
      unNumberedElms.push_back(newElm);
  }

  // Now add the unnumbered elements while assigning an auto-genereted EID
  for (FFlElementBase* elm : unNumberedElms)
  {
    elm->setID(++eidmax);
    if (!(okAdd = myLink->addElement(unNumberedElms[e],false)))
      break;
  }

  delete[] ix;
  delete[] ux;

  // Loop over all used element properties
  FFlAttributeBase* newAtt = 0;
  for (Vint pid : usedPid)
  {
    Vint eptype = -1;
    vis_EProp* eprop;
    vsy_HashTableLookup(ephash,pid,(Vobject**)&eprop);
    vis_EPropInq(eprop,&eptype);

    // Interpret eptype (the element property type)
    switch (eptype)
      {
      case SYS_ELEM_SHELL:
	newAtt = AttributeFactory::instance()->create("PTHICK",pid);

	vis_EPropValueType(eprop,&ntypes,type);
	for (i = 0; i < ntypes; i++)
	{
	  vis_EPropValueFlag(eprop,type[i],&flag);
	  if (flag == EPROP_UNDEFINED) continue;

	  vis_EPropValueParams(eprop,type[i],&nval,&nloc,&dtyp);
	  if (nval*nloc > sizeof(dparams)) continue;
	  if (dtyp == SYS_INTEGER) continue;

	  // Get shell thickness value
	  if (type[i] == EPROP_THICKNESS)
	  {
	    double T = 0.0;
	    vis_EPropValueDouble(eprop,type[i],dparams);
	    for (j = 0; j < nloc; j++)
	      T += dparams[j]/nloc;
	    static_cast<FFlPTHICK*>(newAtt)->thickness = T;
	  }
	}

	// Add thickness property to link handler
	if (!myLink->addAttribute(newAtt)) nWarnings++;
	break;

      case SYS_ELEM_BEAM:
	newAtt = AttributeFactory::instance()->create("PBEAMSECTION",pid);

	vis_EPropValueType(eprop,&ntypes,type);
	for (i = 0; i < ntypes; i++)
	{
	  vis_EPropValueFlag(eprop,type[i],&flag);
	  if (flag == EPROP_UNDEFINED) continue;

	  vis_EPropValueParams(eprop,type[i],&nval,&nloc,&dtyp);
	  if (nval*nloc > sizeof(dparams)) continue;
	  if (dtyp == SYS_INTEGER) continue;

	  // Get beam cross section parameters
	  double par = 0.0;
	  vis_EPropValueDouble(eprop,type[i],dparams);
	  for (j = 0; j < nloc; j++)
	    par += dparams[j]/nloc;
	  if (type[i] == EPROP_AREA)
	    static_cast<FFlPBEAMSECTION*>(newAtt)->crossSectionArea = par;
	  else if (type[i] == EPROP_IYY)
	    static_cast<FFlPBEAMSECTION*>(newAtt)->Iy = par;
	  else if (type[i] == EPROP_IZZ)
	    static_cast<FFlPBEAMSECTION*>(newAtt)->Iz = par;
	  else if (type[i] == EPROP_J)
	    static_cast<FFlPBEAMSECTION*>(newAtt)->It = par;
	  else if (type[i] == EPROP_KSY)
	    static_cast<FFlPBEAMSECTION*>(newAtt)->Kxy = par;
	  else if (type[i] == EPROP_KSZ)
	    static_cast<FFlPBEAMSECTION*>(newAtt)->Kxz = par;
	  else if (type[i] == EPROP_OFFSETY)
	    static_cast<FFlPBEAMSECTION*>(newAtt)->Sy = par;
	  else if (type[i] == EPROP_OFFSETZ)
	    static_cast<FFlPBEAMSECTION*>(newAtt)->Sz = par;
	}

	// Add beam section property to link handler
	if (!myLink->addAttribute(newAtt)) nWarnings++;
	break;

      default:
	if (++nWarnings == 1) ListUI <<"\n";
	ListUI <<"  ** Warning: Unsupported property type: Id="<< pid
	       <<" Type="<< eptype <<" (ignored)\n";
      }
  }

  // Loop over all used material properties
  for (Vint mid : usedMid)
  {
    Vint mptype = -1;
    vis_MProp* mprop;
    vsy_HashTableLookup(mphash,mid,(Vobject**)&mprop);
    vis_MPropInq(mprop,&mptype);

    // Interpret mptype (the material type)
    switch (mptype)
      {
      case SYS_MAT_ISOTROPIC:
	newAtt = AttributeFactory::instance()->create("PMAT",mid);

	vis_MPropValueType(mprop,&ntypes,type);
	for (i = 0; i < ntypes; i++)
	{
	  vis_MPropValueFlag(mprop,type[i],&flag);
	  if (flag == MPROP_UNDEFINED) continue;

	  vis_MPropValueParams(mprop,type[i],&nval,&dtyp);
	  if (nval > sizeof(dparams)) continue;
	  if (dtyp == SYS_INTEGER) continue;

	  // Get material property value
	  vis_MPropValueDouble(mprop,type[i],dparams);
	  if (type[i] == MPROP_E)
	    static_cast<FFlPMAT*>(newAtt)->youngsModule = dparams[0];
	  else if (type[i] == MPROP_NU)
	    static_cast<FFlPMAT*>(newAtt)->poissonsRatio = dparams[0];
	  else if (type[i] == MPROP_DENSITY)
	    static_cast<FFlPMAT*>(newAtt)->materialDensity = dparams[0];
	}

	// Add material to link handler
	if (!myLink->addAttribute(newAtt)) nWarnings++;
	break;

      default:
	if (++nWarnings == 1) ListUI <<"\n";
	ListUI <<"  ** Warning: Unsupported material type: Id="<< mid
	       <<" Type="<< mptype <<" (ignored)\n";
      }
  }

  vis_ElemDat* elemdat;
  Vdouble vec[9][3];
  Vint iprop, nument, enttype, subtype, datatype;

  // Get element data: eccentricity and orientation vectors
  vsy_HashTableInitIter(edhash);
  while (vsy_HashTableNextIter(edhash,&iprop,(Vobject**)&elemdat),elemdat)
    switch (iprop)
      {
      case SYS_PROP_OFFSETVEC:
	// Eccentricity vectors (for beam elements only)
	vis_ElemDatInq(elemdat,&nument,&enttype,&subtype,&datatype);
	for (i = 1; i <= numel; i++)
	{
	  vis_ElemDatFlag(elemdat,i,&flag);
	  if (flag == 0) continue;

	  vis_ConnectElemAssoc(connect,VIS_USERID,1,&i,&eid);
	  FFlElementBase* elm = myLink->getElement(eid);
	  if (!elm) continue;
	  if (elm->getTypeName() != "BEAM2") continue;

	  newAtt = AttributeFactory::instance()->create("PBEAMECCENT",eid);
	  FFlPBEAMECCENT* ecc = static_cast<FFlPBEAMECCENT*>(newAtt);

	  vis_ElemDatDatadv(elemdat,i,1,(Vdouble*)vec);
	  if (subtype == SYS_NONE)
	  {
	    ecc->node1Offset = FaVec3(vec[0][0],vec[0][1],vec[0][2]);
	    ecc->node2Offset = FaVec3(vec[0][0],vec[0][1],vec[0][2]);
	  }
	  else
	  {
	    ecc->node1Offset = FaVec3(vec[0][0],vec[0][1],vec[0][2]);
	    ecc->node2Offset = FaVec3(vec[1][0],vec[1][1],vec[1][2]);
	  }

	  myLink->addAttribute(newAtt);
	  elm->setAttribute(newAtt);
	}
	break;

      case SYS_PROP_ELEMVEC:
	// Z-orientation vectors for beam elements
	vis_ElemDatInq(elemdat,&nument,&enttype,&subtype,&datatype);
	for (i = 1; i <= numel; i++)
	{
	  vis_ElemDatFlag(elemdat,i,&flag);
	  if (flag == 0) continue;

	  vis_ConnectElemAssoc(connect,VIS_USERID,1,&i,&eid);
	  FFlElementBase* elm = myLink->getElement(eid);
	  if (!elm) continue;
	  if (elm->getTypeName() != "BEAM2") continue;

	  newAtt = AttributeFactory::instance()->create("PORIENT",eid);
	  FFlPORIENT* ori = static_cast<FFlPORIENT*>(newAtt);

	  vis_ElemDatDatadv(elemdat,i,1,(Vdouble*)vec);
	  ori->directionVector = FaVec3(vec[0][0],vec[0][1],vec[0][2]);

	  myLink->addAttribute(newAtt);
	  elm->setAttribute(newAtt);
	}
	break;
      }

  if (nWarnings > 0)
    ListUI <<"  ** A total of "<< nWarnings <<" warnings were detected.\n";

  STOPP_TIMER("convert");
  return okAdd;
#else
  return false;
#endif
}


FFlAttributeBase* FFlVdmReader::getRGDAttribute(
#ifdef FT_HAS_VKI
						vis_EProp* eprop,
						int pid, int nelnod,
						bool& ignoreElm
#else
						vis_EProp*, int, int, bool&
#endif
){
  FFlAttributeBase* newAtt = NULL;
#ifdef FT_HAS_VKI
  Vint ntypes, type[EPROP_MAX];
  Vint flag, nval, nloc, dtyp;
  Vint iparams[100];

  int cm[2] = { 0, 0 };
  int cn[2] = { 0, 0 };

  // We need to find out whether an RGD or an RBAR element can be
  // used to represent the rigid element by checking the dof flags
  vis_EPropValueType(eprop,&ntypes,type);
  for (int i = 0; i < ntypes; i++)
  {
    vis_EPropValueFlag(eprop,type[i],&flag);
    if (flag == EPROP_UNDEFINED) continue;

    vis_EPropValueParams(eprop,type[i],&nval,&nloc,&dtyp);
    if (nval*nloc > sizeof(iparams)) continue;
    if (dtyp != SYS_INTEGER) continue;

    if (type[i] == EPROP_DOFFLAG_DEP)
      if (nloc > 2)
      {
	ignoreElm = true;
	if (++nWarnings == 1) ListUI <<"\n";
	ListUI <<"  ** Warning: Dependent dofs are specified in "
	       << nloc <<" (> 2) locations, for rigid ";
	return newAtt;
      }
      else
      {
	vis_EPropValueInteger(eprop,type[i],iparams);
	for (int l = 0; l < nloc; l++)
	  cm[l] = getDofFlag(iparams[l]);
      }

    else if (type[i] == EPROP_DOFFLAG_IND)
      if (nloc > 2)
      {
	ignoreElm = true;
	if (++nWarnings == 1) ListUI <<"\n";
	ListUI <<"  ** Warning: Independent dofs are specified in "
	       << nloc <<" (> 2) locations, for rigid ";
	return newAtt;
      }
      else
      {
	vis_EPropValueInteger(eprop,type[i],iparams);
	for (int l = 0; l < nloc; l++)
	  cn[l] = getDofFlag(iparams[l]);
      }
  }

  if (nelnod == 2 && (cm[0] > 0 || cn[0] < 123456))
  {
    // We must use an RBAR element for this rigid element
    newAtt = AttributeFactory::instance()->create("PRBAR",pid);
    FFlPRBAR* myAtt = static_cast<FFlPRBAR*>(newAtt);
    myAtt->CNA = cn[0];
    myAtt->CNB = cn[1];
    myAtt->CMA = cm[0];
    myAtt->CMB = cm[1];
  }
  else if (cm[0] == 0 && cm[1] < 123456)
  {
    // We have an RGD element with "end-release"
    newAtt = AttributeFactory::instance()->create("PRGD",pid);
    static_cast<FFlPRGD*>(newAtt)->dependentDofs = cm[1];
  }
  else if (cm[0] > 0)
  {
    ignoreElm = true;
    if (++nWarnings == 1) ListUI <<"\n";
    ListUI <<"  ** Warning: Invalid dof flags, CN="
	   << cn[0] <<" "<< cn[1] <<", CM="<< cm[0] <<" "<< cm[1]
	   <<", for rigid ";
  }
#endif
  return newAtt;
}


FFlAttributeBase* FFlVdmReader::getWAVGMAttribute(
#ifdef FT_HAS_VKI
						  vis_EProp* eprop,
						  int pid, int nelnod,
						  bool& ignoreElm
#else
						  vis_EProp*, int, int, bool&
#endif
){
  FFlAttributeBase* newAtt = NULL;
#ifdef FT_HAS_VKI
  Vint ntypes, type[EPROP_MAX];
  Vint flag, nval, nloc, dtyp;
  Vint iparams[100];
  Vdouble dparams[100];

  int l, cm = 0;
  std::vector<int> cn(nelnod-1,0);
  std::vector<double> w(nelnod-1,0.0);

  vis_EPropValueType(eprop,&ntypes,type);
  for (int i = 0; i < ntypes; i++)
  {
    vis_EPropValueFlag(eprop,type[i],&flag);
    if (flag == EPROP_UNDEFINED) continue;

    vis_EPropValueParams(eprop,type[i],&nval,&nloc,&dtyp);
    if (nval*nloc > sizeof(iparams)) continue;

    if (type[i] == EPROP_DOFFLAG_DEP && dtyp == SYS_INTEGER)
    {
      vis_EPropValueInteger(eprop,type[i],iparams);
      ignoreElm = iparams[0] == 0;
      for (l = 1; l < nloc && !ignoreElm; l++)
	ignoreElm = iparams[l] > 0;

      if (ignoreElm)
      {
	if (++nWarnings == 1) ListUI <<"\n";
	ListUI <<"  ** Warning: Invalid dependent dof specification"
	       <<", for constraint ";
	return 0;
      }
      else
	cm = getDofFlag(iparams[0]);
    }

    else if (type[i] == EPROP_DOFFLAG_IND && dtyp == SYS_INTEGER)
    {
      vis_EPropValueInteger(eprop,type[i],iparams);
      if (nloc < 2 || iparams[0] > 0)
      {
	ignoreElm = true;
	if (++nWarnings == 1) ListUI <<"\n";
	ListUI <<"  ** Warning: Invalid independent dof specification"
	       <<", for constraint ";
	return 0;
      }
      else
      {
	for (l = 1; l < nloc && l < nelnod; l++)
	  cn[l-1] = iparams[l];
	while (l++ < nelnod)
	  cn[l-2] = cn[nloc-2];
      }
    }

    else if (type[i] == EPROP_DOFFLAG_WGTS && dtyp != SYS_INTEGER)
    {
      vis_EPropValueDouble(eprop,type[i],dparams);
      for (l = 1; l < nloc && l < nelnod; l++)
	w[l-1] = dparams[l];
      while (l++ < nelnod)
	w[l-2] = w[nloc-2];
    }
  }

  newAtt = AttributeFactory::instance()->create("PWAVGM",pid);
  FFlPWAVGM* myAtt = static_cast<FFlPWAVGM*>(newAtt);
  myAtt->refC = cm;

  size_t nRows = 0, nCols = w.size();
  std::vector<double> weights(nCols), wmat[6];
  for (int r = 0, l = 1; r < 6; r++, l *= 2)
  {
    bool haveWeights = false;
    for (size_t j = 0; j < nCols; j++)
    {
      weights[j] = cn[j] & l ? w[j] : 0.0;
      if (weights[j] > 0.0)
	haveWeights = true;
    }
    if (haveWeights)
    {
      int iRow = -1;
      for (size_t k = 0; k < nRows && iRow < 0; k++)
	if (wmat[k] == weights) iRow = k;

      if (iRow < 0)
      {
	iRow = nRows++;
	wmat[iRow] = weights;
      }

      myAtt->indC[r] = iRow*nCols+1;
    }
  }

  weights.resize(nRows*nCols,0.0);
  for (size_t iRow = 0; iRow < nRows; iRow++)
    for (size_t j = 0; j < nCols; j++)
      weights[iRow*nCols+j] = wmat[iRow][j];
  myAtt->weightMatrix.data().swap(weights);
#endif
  return newAtt;
}


FFlAttributeBase* FFlVdmReader::getMassAttribute(
#ifdef FT_HAS_VKI
						 vis_EProp* eprop,
						 int pid, int cid
#else
						 vis_EProp*, int, int
#endif
){
  FFlAttributeBase* newAtt = NULL;
#ifdef FT_HAS_VKI
  Vint ntypes, type[EPROP_MAX];
  Vint flag, nval, nloc, dtyp;
  Vdouble dparams[100];

  std::vector<double> Mvec;
  FaVec3 X;

  vis_EPropValueType(eprop,&ntypes,type);
  for (int i = 0; i < ntypes; i++)
  {
    vis_EPropValueFlag(eprop,type[i],&flag);
    if (flag == EPROP_UNDEFINED) continue;

    vis_EPropValueParams(eprop,type[i],&nval,&nloc,&dtyp);
    if (nval*nloc > sizeof(dparams)) continue;
    if (dtyp == SYS_INTEGER) continue;

    if (type[i] == EPROP_MASS && nval == 3)
    {
      // Store lumped mass values on the diagonal
      if (Mvec.empty()) Mvec.resize(6,0.0);
      Mvec[0] = dparams[0];
      Mvec[2] = dparams[1];
      Mvec[5] = dparams[2];
    }
    else if (type[i] == EPROP_INERTIA && nval == 6)
    {
      // Store the inertia values
      if (Mvec.size() < 21) Mvec.resize(21,0.0);
      Mvec[ 9] = dparams[0];
      Mvec[14] = dparams[1];
      Mvec[20] = dparams[2];
      Mvec[13] = dparams[3];
      Mvec[19] = dparams[4];
      Mvec[18] = dparams[5];
    }
    else if (type[i] == EPROP_MASSMATRIX)
    {
      // Store the mass matrix
      Mvec.clear();
      Mvec.reserve(21);
      while (nval > 0 && dparams[nval-1] == 0.0) nval--;
      for (int k = 0; k < nval && nval < 21; k++)
	Mvec.push_back(dparams[k]);
    }
    else if (type[i] == EPROP_XYZOFF && nval == 3)
      X = FaVec3(dparams[0],dparams[1],dparams[2]);
  }
  if (Mvec.empty()) return 0;

  // Transform offset vector to global coordinate system
  if (!X.isZero() && cid > 0)
    X = coordSys[cid].direction() * X;

  if (!X.isZero() || cid > 0)
  {
    // Expand the symmetric matrix to a square matrix
    double M[6][6];
    size_t i, j, k;
    for (i = k = 0; i < 6; i++)
      for (j = 0; j <= i; j++)
      {
	if (k >= Mvec.size())
	  M[i][j] = 0.0;
	else
	  M[i][j] = Mvec[k++];
	if (j < i) M[j][i] = M[i][j];
      }

    // Eccentricity transformation of the mass matrix
    if (!X.isZero()) FFaAlgebra::eccTransform6(M,X);

    // Transform mass matrix to global coordinate system
    if (cid > 0)
    {
      double* Mat[6] = { M[0], M[1], M[2], M[3], M[4], M[5] };
      FFaAlgebra::congruenceTransform(Mat,coordSys[cid].direction(),2);
    }

    // Store upper triangel of the symmetric matrix in Mvec again
    for (i = k = 0; i < 6; i++)
      for (j = 0; j <= i; j++)
	if (k < Mvec.size())
	  Mvec[k++] = round(M[i][j],10); // use 10 significant digits
	else if (M[i][j] != 0.0)
	{
	  Mvec.resize(k+1,0.0);
	  Mvec[k++] = round(M[i][j],10); // use 10 significant digits
	}
	else
	  k++;
  }

  newAtt = AttributeFactory::instance()->create("PMASS",pid);
  static_cast<FFlPMASS*>(newAtt)->M.data().swap(Mvec);
#endif
  return newAtt;
}


int FFlVdmReader::getDofFlag(int bitFlag)
{
  int dofFlag = 0;
  int factor = 100000;

  for (int i = 1; i < 7 && bitFlag > 0; i++, bitFlag /= 2, factor /= 10)
    if (bitFlag%2 == 1)
      dofFlag += i*factor;

  return dofFlag;
}

#ifdef FF_NAMESPACE
} // namespace
#endif
